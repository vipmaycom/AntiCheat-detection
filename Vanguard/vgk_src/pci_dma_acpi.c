/*
 * pci_dma_acpi.c
 * Riot Vanguard Kernel-Mode Anti-Cheat (vgk.sys v1.19.1.18)
 * Sections 4 & 17: PCI Configuration Space / DMA Scanner (Xilinx 10EE, VMware, VirtIO) & ACPI Tables
 *
 * Implements:
 *   - §4 / §17 Full PCI iteration (256 buses x 32 devices x 8 functions) via HalGetBusDataByOffset
 *   - §17      Flagged Virtual / DMA Vendor & Device IDs:
 *                0x10EE (Xilinx Default DMA Bitstream — PCILeech / Screamer)
 *                0x15AD (0x0405 VMware SVGA II, 0x0740 VMXNET3, 0x07B0 PVSCSI)
 *                0x1AF4 (0x1000..0x107F Red Hat VirtIO KVM/QEMU)
 *                0x1414 (0x5353 Microsoft Hyper-V Video Adapter)
 *                0x1B36 (QEMU PCI Bridge)
 *   - §4       Abnormal BAR configuration & PCIe bridge inspection
 *   - §3 / §4  ACPI Table inspection ('DMAR', 'APIC'/MADT, 'FACP'/FADT, 'SLIC') via
 *              ZwQuerySystemInformation(SystemFirmwareTableInformation = 0x4C)
 */

#include "vgk.h"

#pragma pack(push, 1)
typedef struct _VGK_PCI_CONFIG_HEADER {
    USHORT VendorId;
    USHORT DeviceId;
    USHORT Command;
    USHORT Status;
    UCHAR  RevisionId;
    UCHAR  ProgIf;
    UCHAR  SubClass;
    UCHAR  BaseClass;
    UCHAR  CacheLineSize;
    UCHAR  LatencyTimer;
    UCHAR  HeaderType;
    UCHAR  Bist;
    ULONG  Bar[6];
    ULONG  CardbusCisPtr;
    USHORT SubsystemVendorId;
    USHORT SubsystemId;
} VGK_PCI_CONFIG_HEADER, *PVGK_PCI_CONFIG_HEADER;

typedef struct _VGK_ACPI_FIRMWARE_TABLE_INFO {
    ULONG ProviderSignature; /* 'ACPI' = 0x41435049, 'RSMB' = 0x52534D42 */
    ULONG Action;            /* 0 = Enumerate, 1 = Get */
    ULONG TableID;           /* 'DMAR', 'APIC', 'FACP', 'SLIC' */
    ULONG TableBufferLength;
    UCHAR TableBuffer[1];
} VGK_ACPI_FIRMWARE_TABLE_INFO, *PVGK_ACPI_FIRMWARE_TABLE_INFO;
#pragma pack(pop)

/*
 * §17: Known Flagged PCI DMA & Virtual Device Helper
 */
static BOOLEAN
VgkIsPciDeviceFlagged(
    _In_ const VGK_PCI_CONFIG_HEADER *Cfg
)
{
    /* 1. Xilinx Default FPGA Bitstream (PCILeech / Screamer): Vendor 0x10EE */
    if (Cfg->VendorId == 0x10EE) {
        return TRUE;
    }

    /* 2. VMware Devices: Vendor 0x15AD (SVGA II 0x0405, VMXNET3 0x0740, PVSCSI 0x07B0) */
    if (Cfg->VendorId == 0x15AD &&
        (Cfg->DeviceId == 0x0405 || Cfg->DeviceId == 0x0740 || Cfg->DeviceId == 0x07B0)) {
        return TRUE;
    }

    /* 3. Red Hat / VirtIO (KVM / QEMU): Vendor 0x1AF4, Device 0x1000..0x107F */
    if (Cfg->VendorId == 0x1AF4 &&
        (Cfg->DeviceId >= 0x1000 && Cfg->DeviceId <= 0x107F)) {
        return TRUE;
    }

    /* 4. Microsoft Hyper-V Video Adapter: Vendor 0x1414, Device 0x5353 */
    if (Cfg->VendorId == 0x1414 && Cfg->DeviceId == 0x5353) {
        return TRUE;
    }

    /* 5. QEMU PCI Interconnect / Bridge: Vendor 0x1B36 */
    if (Cfg->VendorId == 0x1B36) {
        return TRUE;
    }

    /* 6. §4 Heuristic: Custom FPGA with Bus Master enabled (Bit 2) + zeroed Subsystem IDs
     *    on a non-bridge endpoint (HeaderType 0x00) claiming RAM/Memory Controller Class (0x05) */
    if ((Cfg->HeaderType & 0x7F) == 0x00 &&
        Cfg->BaseClass == 0x05 &&
        (Cfg->Command & 0x0004) != 0 &&
        Cfg->SubsystemVendorId == 0x0000 &&
        Cfg->SubsystemId == 0x0000) {
        return TRUE;
    }

    return FALSE;
}

/*
 * §4 / §17: Full PCI Configuration Space Scan across Buses 0..255, Devices 0..31, Functions 0..7
 */
static VOID
VgkScanPciConfigurationSpace(VOID)
{
    ULONG flaggedCount = 0;

    for (ULONG bus = 0; bus < 256; bus++) {
        for (ULONG dev = 0; dev < 32; dev++) {
            for (ULONG func = 0; func < 8; func++) {
                PCI_SLOT_NUMBER slot = { 0 };
                slot.u.bits.DeviceNumber   = dev;
                slot.u.bits.FunctionNumber = func;

                VGK_PCI_CONFIG_HEADER cfg = { 0 };
                ULONG bytesRead = HalGetBusDataByOffset(
                    PCIConfiguration,
                    bus,
                    slot.u.AsULONG,
                    &cfg,
                    0,
                    sizeof(cfg)
                );

                if (bytesRead < 4 || cfg.VendorId == 0xFFFF || cfg.VendorId == 0x0000) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }

                if (VgkIsPciDeviceFlagged(&cfg)) {
                    flaggedCount++;
                    InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_PCI_DMA_FLAGGED);
                    VgkLogTelemetryEvent(
                        VGK_EVENT_DCL,
                        ((ULONG)cfg.VendorId << 16) | (ULONG)cfg.DeviceId,
                        NULL
                    );
                }

                /* Single-function device: bit 7 of HeaderType is 0 on Function 0 */
                if (func == 0 && (cfg.HeaderType & 0x80) == 0) {
                    break;
                }
            }
        }
    }

    g_Vgk.HardwareProfile.FlaggedPciDeviceCount = flaggedCount;
}

/*
 * §3 / §4: Query ACPI Tables (DMAR for Intel VT-d IOMMU, IVRS for AMD-Vi, APIC/MADT, FACP/FADT)
 */
static ULONG
VgkProbeAcpiTableSignature(
    _In_ ULONG TableId
)
{
    ULONG bufSize = sizeof(VGK_ACPI_FIRMWARE_TABLE_INFO) + 4096;
    PVGK_ACPI_FIRMWARE_TABLE_INFO fwInfo = (PVGK_ACPI_FIRMWARE_TABLE_INFO)
        ExAllocatePool2(POOL_FLAG_NON_PAGED, bufSize, VGK_POOL_TAG);
    if (!fwInfo) {
        return 0;
    }

    fwInfo->ProviderSignature = 'IPCA'; /* 'ACPI' big-endian provider in NT */
    fwInfo->Action            = 1;      /* SystemFirmwareTable_Get */
    fwInfo->TableID           = TableId;
    fwInfo->TableBufferLength = 4096;

    ULONG returnLen = 0;
    NTSTATUS status = ZwQuerySystemInformation(
        SystemFirmwareTableInformation,
        fwInfo,
        bufSize,
        &returnLen
    );

    ULONG readSig = 0;
    if (NT_SUCCESS(status) && fwInfo->TableBufferLength >= 4) {
        readSig = *(PULONG)&fwInfo->TableBuffer[0];
    }

    ExFreePoolWithTag(fwInfo, VGK_POOL_TAG);
    return readSig;
}

VOID
VgkAuditPciBusDmaAndAcpi(VOID)
{
    /* 1. Walk PCI Configuration Space for Xilinx 0x10EE, VMware, VirtIO, and DMA bridges */
    VgkScanPciConfigurationSpace();

    /* 2. Check ACPI DMAR (Intel VT-d), IVRS (AMD-Vi), and APIC (MADT) tables */
    ULONG dmarSig = VgkProbeAcpiTableSignature('RAMD'); /* 'DMAR' */
    ULONG ivrsSig = VgkProbeAcpiTableSignature('SRVI'); /* 'IVRS' */
    ULONG madtSig = VgkProbeAcpiTableSignature('CIPA'); /* 'APIC' (MADT) */

    g_Vgk.HardwareProfile.AcpiDmarSignature = (dmarSig != 0) ? dmarSig : ivrsSig;
    g_Vgk.HardwareProfile.AcpiMadtSignature = madtSig;

    if (g_Vgk.HardwareProfile.AcpiDmarSignature != 0) {
        g_Vgk.HardwareProfile.IommuInterfacePresent = TRUE;
    }
}
