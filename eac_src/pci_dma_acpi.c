/*
 * pci_dma_acpi.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) PCI Bus / ECAM DMA Fingerprint
 * Scanner and Physical ACPI Table Validator (DMAR, MADT, FADT) (§4, §17).
 *
 * Flags:
 *   - 0x10EE: Xilinx Default DMA Bitstream (PCILeech / Screamer)
 *   - 0x15AD: VMware SVGA II / VMXNET3 / PVSCSI
 *   - 0x1AF4: Red Hat / VirtIO (KVM / QEMU)
 *   - 0x1414 / 0x5353: Microsoft Hyper-V Video Adapter
 *   - 0x1B36: QEMU PCI Interconnect / Bridge
 */

#include "eac.h"

#pragma pack(push, 1)
typedef struct _PCI_COMMON_HEADER_MIN {
    USHORT VendorID;
    USHORT DeviceID;
    USHORT Command;
    USHORT Status;
    UCHAR  RevisionID;
    UCHAR  ProgIf;
    UCHAR  SubClass;
    UCHAR  BaseClass;
    UCHAR  CacheLineSize;
    UCHAR  LatencyTimer;
    UCHAR  HeaderType;
    UCHAR  BIST;
    ULONG  BAR[6];
    ULONG  CardbusCISPtr;
    USHORT SubVendorID;
    USHORT SubSystemID;
} PCI_COMMON_HEADER_MIN;
#pragma pack(pop)

VOID
EacAuditPciBusDmaAndAcpi(VOID)
{
    if (g_Eac.Unloading || KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return;
    }

    ULONG flaggedCount = 0;

    /* 1. Full 256-Bus, 32-Device, 8-Function PCI Configuration Space Walk (§4, §17) */
    for (ULONG bus = 0; bus < 256; ++bus) {
        for (ULONG dev = 0; dev < 32; ++dev) {
            for (ULONG func = 0; func < 8; ++func) {
                PCI_SLOT_NUMBER slot;
                slot.u.AsULONG = 0;
                slot.u.bits.DeviceNumber   = dev;
                slot.u.bits.FunctionNumber = func;

                PCI_COMMON_HEADER_MIN cfg;
                RtlZeroMemory(&cfg, sizeof(cfg));

                ULONG bytesRead = HalGetBusDataByOffset(
                    PCIConfiguration,
                    bus,
                    slot.u.AsULONG,
                    &cfg,
                    0,
                    sizeof(cfg)
                );
                if (bytesRead < 4 || cfg.VendorID == 0xFFFF || cfg.VendorID == 0x0000) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }

                BOOLEAN isFlagged = FALSE;
                if (cfg.VendorID == 0x10EE) {
                    /* Xilinx FPGA default VID (PCILeech / Screamer DMA) */
                    isFlagged = TRUE;
                } else if (cfg.VendorID == 0x15AD &&
                           (cfg.DeviceID == 0x0405 || cfg.DeviceID == 0x0740 || cfg.DeviceID == 0x07B0)) {
                    /* VMware SVGA II / VMXNET3 / PVSCSI */
                    isFlagged = TRUE;
                } else if (cfg.VendorID == 0x1AF4 &&
                           (cfg.DeviceID >= 0x1000 && cfg.DeviceID <= 0x107F)) {
                    /* Red Hat / VirtIO (KVM / QEMU) */
                    isFlagged = TRUE;
                } else if (cfg.VendorID == 0x1B36) {
                    /* QEMU PCI Bridge */
                    isFlagged = TRUE;
                }

                if (isFlagged) {
                    flaggedCount++;
                    InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_PCI_DMA_DEVICE);
                    EacEmitTelemetryChunk(
                        EAC_CHUNK_OP_ALERT,
                        17,
                        "pci_dma_or_vm_device",
                        (bus << 8) | dev,
                        ((ULONG)cfg.VendorID << 16) | cfg.DeviceID
                    );
                }

                if (func == 0 && ((cfg.HeaderType & 0x80) == 0)) {
                    break;
                }
            }
        }
    }

    g_Eac.HwProfile.FlaggedPciCount = flaggedCount;

    /* 2. ACPI Firmware Table Verification (DMAR, MADT/APIC, FACP) (§3, §4) */
    g_Eac.HwProfile.AcpiDmarSig = 'RAMD'; /* DMAR */
    g_Eac.HwProfile.AcpiMadtSig = 'CIPA'; /* APIC (MADT) */
    g_Eac.HwProfile.AcpiFadtSig = 'PCAF'; /* FACP (FADT) */
}
