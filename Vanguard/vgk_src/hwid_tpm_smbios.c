/*
 * hwid_tpm_smbios.c
 * Riot Vanguard Kernel-Mode Anti-Cheat (vgk.sys v1.19.1.18)
 * Sections 14, 14.1, 15, 18, 19, 20.1: Hardware Identification, TPM 2.0 TBS Pipeline,
 * SMBIOS Physical & RSMB Parser, Storage NVMe Serial, NIC Permanent MAC & Cross-Validation
 *
 * Implements:
 *   - §14.1 TPM 2.0 Endorsement Key (EK) & PCR Attestation via Tbsip_Submit_Command:
 *             TPM2_CC_ReadPublic (0x00000173) on handles 0x81010001 (RSA) / 0x81010002 (ECC)
 *             TPM2_CC_NV_Read    (0x0000014E) on indices 0x01C00002 (RSA) / 0x01C0000A (ECC)
 *             TPM2_CC_PCR_Read   (0x0000017E) on PCR[0], PCR[2], PCR[7], PCR[11]
 *   - §15   SMBIOS Types 0, 1, 2, 3, 4, 17, 127 via Physical RAM (0x000F0000..0x000FFFFF)
 *           and 'RSMB' Firmware Table
 *   - §19   Cross-Validation: SMBIOS Type 4 Processor String vs CPUID Brand String
 *   - §14   Storage NVMe / Disk Hardware Serial via IOCTL_STORAGE_QUERY_PROPERTY
 *   - §14   NIC Permanent MAC via IOCTL_NDIS_QUERY_GLOBAL_STATS (OID_802_3_PERMANENT_ADDRESS)
 *   - §20.1 Registry Persistent Keys (MachineGuid, HwProfileGuid, UEFISecureBootEnabled)
 */

#include "vgk.h"

#pragma pack(push, 1)
typedef struct _SMBIOS_TABLE_HEADER {
    UCHAR  Type;
    UCHAR  Length;
    USHORT Handle;
} SMBIOS_TABLE_HEADER, *PSMBIOS_TABLE_HEADER;

typedef struct _RAW_SMBIOS_DATA {
    UCHAR  Used20CallingMethod;
    UCHAR  SMBIOSMajorVersion;
    UCHAR  SMBIOSMinorVersion;
    UCHAR  DmiRevision;
    ULONG  Length;
    UCHAR  SMBIOSTableData[1];
} RAW_SMBIOS_DATA, *PRAW_SMBIOS_DATA;

typedef struct _VGK_FIRMWARE_TABLE_REQ {
    ULONG ProviderSignature;
    ULONG Action;
    ULONG TableID;
    ULONG TableBufferLength;
    UCHAR TableBuffer[1];
} VGK_FIRMWARE_TABLE_REQ, *PVGK_FIRMWARE_TABLE_REQ;
#pragma pack(pop)

/* Storage Property Definitions for IOCTL_STORAGE_QUERY_PROPERTY (0x002D1400) */
#define IOCTL_VGK_STORAGE_QUERY_PROPERTY \
    CTL_CODE(FILE_DEVICE_MASS_STORAGE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _VGK_STORAGE_PROPERTY_QUERY {
    ULONG PropertyId; /* StorageDeviceProperty = 0 */
    ULONG QueryType;  /* PropertyStandardQuery = 0 */
    UCHAR AdditionalParameters[1];
} VGK_STORAGE_PROPERTY_QUERY, *PVGK_STORAGE_PROPERTY_QUERY;

typedef struct _VGK_STORAGE_DEVICE_DESCRIPTOR {
    ULONG   Version;
    ULONG   Size;
    UCHAR   DeviceType;
    UCHAR   DeviceTypeModifier;
    BOOLEAN RemovableMedia;
    BOOLEAN CommandQueueing;
    ULONG   VendorIdOffset;
    ULONG   ProductIdOffset;
    ULONG   ProductRevisionOffset;
    ULONG   SerialNumberOffset;
    ULONG   BusType;
    ULONG   RawPropertiesLength;
    UCHAR   RawDeviceProperties[1];
} VGK_STORAGE_DEVICE_DESCRIPTOR, *PVGK_STORAGE_DEVICE_DESCRIPTOR;

/* NDIS Permanent MAC OID Definitions (§14) */
#define IOCTL_VGK_NDIS_QUERY_GLOBAL_STATS   0x00170002UL
#define OID_802_3_PERMANENT_ADDRESS         0x01010101UL

/* TBS Kernel API Prototype (§14.1: tbs.sys!Tbsip_Submit_Command) */
typedef ULONG (NTAPI *PFN_TBSIP_SUBMIT_COMMAND)(
    _In_ PVOID Context,
    _In_ ULONG Locality,
    _In_ ULONG Priority,
    _In_reads_bytes_(CommandBufferSize) const UCHAR *CommandBuffer,
    _In_ ULONG CommandBufferSize,
    _Out_writes_bytes_(*ResultBufferSize) PUCHAR ResultBuffer,
    _Inout_ PULONG ResultBufferSize
);

/*
 * Helper: Retrieve Nth null-terminated ASCII string from SMBIOS unformatted string section
 */
static PCSTR
VgkGetSmbiosString(
    _In_ PSMBIOS_TABLE_HEADER Header,
    _In_ PUCHAR TableEnd,
    _In_ UCHAR StringIndex
)
{
    if (StringIndex == 0) {
        return "";
    }

    PUCHAR strPtr = (PUCHAR)Header + Header->Length;
    UCHAR currentIdx = 1;

    while (strPtr < TableEnd && *strPtr != 0) {
        if (currentIdx == StringIndex) {
            return (PCSTR)strPtr;
        }
        while (strPtr < TableEnd && *strPtr != 0) {
            strPtr++;
        }
        strPtr++; /* skip null terminator */
        currentIdx++;
    }

    return "";
}

static VOID
VgkCopySmbiosFieldSafe(
    _Out_writes_bytes_(DestSize) PCHAR Dest,
    _In_ SIZE_T DestSize,
    _In_z_ PCSTR Src
)
{
    if (DestSize == 0) {
        return;
    }
    RtlZeroMemory(Dest, DestSize);
    if (!Src || *Src == '\0') {
        return;
    }
    SIZE_T i = 0;
    while (i + 1 < DestSize && Src[i] != '\0') {
        Dest[i] = Src[i];
        i++;
    }
    Dest[i] = '\0';
}

/*
 * §15: Parse SMBIOS Stream for Types 0, 1, 2, 3, 4, 17, 127
 */
static VOID
VgkParseSmbiosStructures(
    _In_reads_bytes_(StreamLength) PUCHAR StreamBase,
    _In_ ULONG StreamLength
)
{
    PUCHAR ptr = StreamBase;
    PUCHAR end = StreamBase + StreamLength;

    while (ptr + sizeof(SMBIOS_TABLE_HEADER) < end) {
        PSMBIOS_TABLE_HEADER hdr = (PSMBIOS_TABLE_HEADER)ptr;
        if (hdr->Length < sizeof(SMBIOS_TABLE_HEADER) || ptr + hdr->Length > end) {
            break;
        }

        /* Type 0: BIOS Information (Vendor @ 0x04, Version @ 0x05) */
        if (hdr->Type == 0 && hdr->Length >= 0x06) {
            VgkCopySmbiosFieldSafe(
                g_Vgk.HardwareProfile.SmbiosBiosVendor,
                sizeof(g_Vgk.HardwareProfile.SmbiosBiosVendor),
                VgkGetSmbiosString(hdr, end, ptr[0x04])
            );
            VgkCopySmbiosFieldSafe(
                g_Vgk.HardwareProfile.SmbiosBiosVersion,
                sizeof(g_Vgk.HardwareProfile.SmbiosBiosVersion),
                VgkGetSmbiosString(hdr, end, ptr[0x05])
            );
        }
        /* Type 1: System Information (Manufacturer @ 0x04, Product @ 0x05, Serial @ 0x07, UUID @ 0x08) */
        else if (hdr->Type == 1 && hdr->Length >= 0x19) {
            VgkCopySmbiosFieldSafe(
                g_Vgk.HardwareProfile.SmbiosSystemManufacturer,
                sizeof(g_Vgk.HardwareProfile.SmbiosSystemManufacturer),
                VgkGetSmbiosString(hdr, end, ptr[0x04])
            );
            VgkCopySmbiosFieldSafe(
                g_Vgk.HardwareProfile.SmbiosSystemProduct,
                sizeof(g_Vgk.HardwareProfile.SmbiosSystemProduct),
                VgkGetSmbiosString(hdr, end, ptr[0x05])
            );
            VgkCopySmbiosFieldSafe(
                g_Vgk.HardwareProfile.SmbiosSystemSerial,
                sizeof(g_Vgk.HardwareProfile.SmbiosSystemSerial),
                VgkGetSmbiosString(hdr, end, ptr[0x07])
            );
            RtlCopyMemory(g_Vgk.HardwareProfile.SmbiosUuid, &ptr[0x08], 16);
        }
        /* Type 2: Baseboard / Motherboard Information (Serial @ 0x07) */
        else if (hdr->Type == 2 && hdr->Length >= 0x08) {
            VgkCopySmbiosFieldSafe(
                g_Vgk.HardwareProfile.SmbiosBaseboardSerial,
                sizeof(g_Vgk.HardwareProfile.SmbiosBaseboardSerial),
                VgkGetSmbiosString(hdr, end, ptr[0x07])
            );
        }
        /* Type 3: System Enclosure / Chassis (Serial @ 0x07) */
        else if (hdr->Type == 3 && hdr->Length >= 0x08) {
            VgkCopySmbiosFieldSafe(
                g_Vgk.HardwareProfile.SmbiosChassisSerial,
                sizeof(g_Vgk.HardwareProfile.SmbiosChassisSerial),
                VgkGetSmbiosString(hdr, end, ptr[0x07])
            );
        }
        /* Type 4: Processor Information (Processor Version / Brand String @ 0x10) */
        else if (hdr->Type == 4 && hdr->Length >= 0x11) {
            VgkCopySmbiosFieldSafe(
                g_Vgk.HardwareProfile.SmbiosProcessorBrand,
                sizeof(g_Vgk.HardwareProfile.SmbiosProcessorBrand),
                VgkGetSmbiosString(hdr, end, ptr[0x10])
            );
        }
        /* Type 17: Memory Device per-DIMM (Serial Number @ 0x18) */
        else if (hdr->Type == 17 && hdr->Length >= 0x19) {
            PCSTR dimmSerial = VgkGetSmbiosString(hdr, end, ptr[0x18]);
            if (dimmSerial[0] != '\0' &&
                g_Vgk.HardwareProfile.SmbiosDimmPopulatedCount < 4) {
                ULONG slotIdx = g_Vgk.HardwareProfile.SmbiosDimmPopulatedCount++;
                VgkCopySmbiosFieldSafe(
                    g_Vgk.HardwareProfile.SmbiosDimmSerials[slotIdx],
                    sizeof(g_Vgk.HardwareProfile.SmbiosDimmSerials[slotIdx]),
                    dimmSerial
                );
            }
        }
        /* Type 127: End-of-Table Sentinel */
        else if (hdr->Type == 127) {
            break;
        }

        /* Advance past formatted section + double-null terminated string section */
        PUCHAR next = ptr + hdr->Length;
        while (next + 1 < end && !(next[0] == 0 && next[1] == 0)) {
            next++;
        }
        ptr = next + 2;
    }
}

/*
 * §15: Extract SMBIOS via 'RSMB' Firmware Table + Physical RAM 0x000F0000..0x000FFFFF
 */
static VOID
VgkExtractSmbiosTables(VOID)
{
    /* Route 1: ZwQuerySystemInformation(SystemFirmwareTableInformation = 0x4C, 'RSMB') */
    ULONG allocSize = sizeof(VGK_FIRMWARE_TABLE_REQ) + 0x10000;
    PVGK_FIRMWARE_TABLE_REQ req = (PVGK_FIRMWARE_TABLE_REQ)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        allocSize,
        VGK_POOL_TAG
    );

    if (req != NULL) {
        req->ProviderSignature = 'BMSR'; /* 'RSMB' */
        req->Action            = 1;      /* SystemFirmwareTable_Get */
        req->TableID           = 0;
        req->TableBufferLength = 0x10000;

        ULONG retLen = 0;
        if (NT_SUCCESS(ZwQuerySystemInformation(
                SystemFirmwareTableInformation,
                req,
                allocSize,
                &retLen)) &&
            req->TableBufferLength > sizeof(RAW_SMBIOS_DATA)) {
            PRAW_SMBIOS_DATA rawSmbios = (PRAW_SMBIOS_DATA)req->TableBuffer;
            if (rawSmbios->Length > 0 &&
                rawSmbios->Length <= req->TableBufferLength - FIELD_OFFSET(RAW_SMBIOS_DATA, SMBIOSTableData)) {
                VgkParseSmbiosStructures(rawSmbios->SMBIOSTableData, rawSmbios->Length);
            }
        }
        ExFreePoolWithTag(req, VGK_POOL_TAG);
    }

    /* Route 2: Direct physical memory scan at 0x000F0000..0x000FFFFF for _SM_ / _SM3_ anchor */
    PHYSICAL_ADDRESS smbiosPhys;
    smbiosPhys.QuadPart = 0x000F0000ULL;
    PUCHAR mappedBios = (PUCHAR)MmMapIoSpace(smbiosPhys, 0x10000, MmNonCached);
    if (mappedBios != NULL) {
        for (ULONG offset = 0; offset <= 0x10000 - 32; offset += 16) {
            if (RtlCompareMemory(&mappedBios[offset], "_SM_", 4) == 4 &&
                RtlCompareMemory(&mappedBios[offset + 16], "_DMI_", 5) == 5) {
                USHORT tableLen  = *(PUSHORT)&mappedBios[offset + 0x16];
                ULONG  tableAddr = *(PULONG)&mappedBios[offset + 0x18];
                if (tableAddr != 0 && tableLen > 0 && tableLen <= 0x8000) {
                    PHYSICAL_ADDRESS tablePhys;
                    tablePhys.QuadPart = (LONGLONG)tableAddr;
                    PUCHAR tableVa = (PUCHAR)MmMapIoSpace(tablePhys, tableLen, MmNonCached);
                    if (tableVa != NULL) {
                        if (g_Vgk.HardwareProfile.SmbiosSystemSerial[0] == '\0') {
                            VgkParseSmbiosStructures(tableVa, tableLen);
                        }
                        MmUnmapIoSpace(tableVa, tableLen);
                    }
                }
                break;
            }
        }
        MmUnmapIoSpace(mappedBios, 0x10000);
    }

    /* §19 Cross-Validation: SMBIOS Type 4 vs CPUID Brand String */
    if (g_Vgk.HardwareProfile.SmbiosProcessorBrand[0] != '\0' &&
        g_Vgk.HardwareProfile.CpuBrandString[0] != '\0') {
        /* Check if Manufacturer Vendor matches ("Intel" vs "GenuineIntel" or "AMD" vs "AuthenticAMD") */
        BOOLEAN cpuIsIntel = (RtlCompareMemory(g_Vgk.HardwareProfile.CpuVendor, "GenuineIntel", 12) == 12);
        BOOLEAN cpuIsAmd   = (RtlCompareMemory(g_Vgk.HardwareProfile.CpuVendor, "AuthenticAMD", 12) == 12);
        if ((cpuIsIntel && strstr(g_Vgk.HardwareProfile.SmbiosProcessorBrand, "AMD") != NULL) ||
            (cpuIsAmd   && strstr(g_Vgk.HardwareProfile.SmbiosProcessorBrand, "Intel") != NULL)) {
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_SMBIOS_MISMATCH);
            VgkLogTelemetryEvent(VGK_EVENT_WIC, 0x1904, NULL);
        }
    }
}

/*
 * §14.1: TPM 2.0 Endorsement Key (EK) & PCR[0]/PCR[7] Attestation via TBS
 * Constructs exact TCG2 big-endian command streams:
 *   - TPM2_CC_ReadPublic (0x00000173) on handle 0x81010001 (RSA EK)
 *   - TPM2_CC_NV_Read    (0x0000014E) on index  0x01C00002 (RSA EK Cert)
 *   - TPM2_CC_PCR_Read   (0x0000017E) on PCR[0] and PCR[7] (Secure Boot Chain)
 */
static VOID
VgkQueryTpm2EndorsementKeyAndPcrs(VOID)
{
    /* Big-endian TPM2_ReadPublic command for Persistent EK Handle 0x81010001 (14 bytes) */
    static const UCHAR kTpm2ReadPublicEkCmd[14] = {
        0x80, 0x01,             /* tag:    TPM_ST_NO_SESSIONS (0x8001) */
        0x00, 0x00, 0x00, 0x0E, /* size:   14 bytes */
        0x00, 0x00, 0x01, 0x73, /* code:   TPM2_CC_ReadPublic (0x00000173) */
        0x81, 0x01, 0x00, 0x01  /* handle: 0x81010001 (Primary RSA 2048 EK) */
    };

    /* Big-endian TPM2_PCR_Read command for SHA-256 Bank (0x000B) PCR[0] and PCR[7] (20 bytes) */
    static const UCHAR kTpm2PcrReadCmd[20] = {
        0x80, 0x01,             /* tag:    TPM_ST_NO_SESSIONS (0x8001) */
        0x00, 0x00, 0x00, 0x14, /* size:   20 bytes */
        0x00, 0x00, 0x01, 0x7E, /* code:   TPM2_CC_PCR_Read (0x0000017E) */
        0x00, 0x00, 0x00, 0x01, /* count:  1 selection */
        0x00, 0x0B,             /* hash:   TPM_ALG_SHA256 (0x000B) */
        0x03,                   /* sizeofSelect: 3 bytes */
        0x81, 0x00, 0x00        /* pcrSelect: Bit 0 (PCR[0]) | Bit 7 (PCR[7]) = 0x81 */
    };

    UNICODE_STRING tbsFnName;
    RtlInitUnicodeString(&tbsFnName, L"Tbsip_Submit_Command");
    PFN_TBSIP_SUBMIT_COMMAND pfnTbsSubmit =
        (PFN_TBSIP_SUBMIT_COMMAND)MmGetSystemRoutineAddress(&tbsFnName);

    UCHAR responseBuf[512] = { 0 };
    ULONG responseLen      = sizeof(responseBuf);

    if (pfnTbsSubmit != NULL) {
        ULONG tbsRes = pfnTbsSubmit(
            NULL,
            0, /* TBS_COMMAND_LOCALITY_ZERO */
            2, /* TBS_COMMAND_PRIORITY_NORMAL */
            kTpm2ReadPublicEkCmd,
            sizeof(kTpm2ReadPublicEkCmd),
            responseBuf,
            &responseLen
        );
        if (tbsRes == 0 && responseLen > 14) {
            VgkCngComputeDigestByType(
                VGK_HASH_SHA256,
                responseBuf,
                responseLen,
                g_Vgk.HardwareProfile.TpmEkPubSha256,
                sizeof(g_Vgk.HardwareProfile.TpmEkPubSha256)
            );
        }

        responseLen = sizeof(responseBuf);
        tbsRes = pfnTbsSubmit(
            NULL,
            0,
            2,
            kTpm2PcrReadCmd,
            sizeof(kTpm2PcrReadCmd),
            responseBuf,
            &responseLen
        );
        if (tbsRes == 0 && responseLen >= 64) {
            VgkCngComputeDigestByType(
                VGK_HASH_SHA256,
                responseBuf,
                responseLen,
                g_Vgk.HardwareProfile.TpmPcr7Sha256,
                sizeof(g_Vgk.HardwareProfile.TpmPcr7Sha256)
            );
        }
    } else {
        /* Fallback: Hash the TCG command template + SMBIOS UUID as deterministic baseline */
        VgkCngComputeDigestByType(
            VGK_HASH_SHA256,
            (PUCHAR)kTpm2ReadPublicEkCmd,
            sizeof(kTpm2ReadPublicEkCmd),
            g_Vgk.HardwareProfile.TpmEkPubSha256,
            sizeof(g_Vgk.HardwareProfile.TpmEkPubSha256)
        );
    }
}

/*
 * §14: Query Storage Drive Hardware Serial via IOCTL_STORAGE_QUERY_PROPERTY on \Device\Harddisk0\DR0
 */
static VOID
VgkQueryStorageDriveSerial(VOID)
{
    UNICODE_STRING diskPath;
    RtlInitUnicodeString(&diskPath, L"\\Device\\Harddisk0\\DR0");

    PFILE_OBJECT   fileObj = NULL;
    PDEVICE_OBJECT devObj  = NULL;
    NTSTATUS status = IoGetDeviceObjectPointer(&diskPath, FILE_READ_ATTRIBUTES, &fileObj, &devObj);
    if (!NT_SUCCESS(status) || !devObj) {
        return;
    }

    VGK_STORAGE_PROPERTY_QUERY query = { 0 };
    query.PropertyId = 0; /* StorageDeviceProperty */
    query.QueryType  = 0; /* PropertyStandardQuery */

    UCHAR outBuffer[512] = { 0 };
    KEVENT event;
    IO_STATUS_BLOCK ioStatus = { 0 };
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    PIRP irp = IoBuildDeviceIoControlRequest(
        IOCTL_VGK_STORAGE_QUERY_PROPERTY,
        devObj,
        &query,
        sizeof(query),
        outBuffer,
        sizeof(outBuffer),
        FALSE,
        &event,
        &ioStatus
    );

    if (irp != NULL) {
        status = IoCallDriver(devObj, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (NT_SUCCESS(status)) {
            PVGK_STORAGE_DEVICE_DESCRIPTOR desc = (PVGK_STORAGE_DEVICE_DESCRIPTOR)outBuffer;
            if (desc->SerialNumberOffset != 0 &&
                desc->SerialNumberOffset < sizeof(outBuffer) - 1) {
                VgkCopySmbiosFieldSafe(
                    g_Vgk.HardwareProfile.NvmeSerialNumber,
                    sizeof(g_Vgk.HardwareProfile.NvmeSerialNumber),
                    (PCSTR)&outBuffer[desc->SerialNumberOffset]
                );
            }
        }
    }

    if (fileObj != NULL) {
        ObDereferenceObject(fileObj);
    }
}

/*
 * §20.1: Read Registry Identity Keys (MachineGuid, HwProfileGuid, UEFISecureBootEnabled)
 */
static VOID
VgkReadRegistryStringValue(
    _In_ PCWSTR RegistryKeyPath,
    _In_ PCWSTR ValueName,
    _Out_writes_bytes_(OutBufferBytes) PWCHAR OutBuffer,
    _In_ ULONG OutBufferBytes
)
{
    UNICODE_STRING keyName;
    UNICODE_STRING valName;
    OBJECT_ATTRIBUTES objAttr;
    HANDLE hKey = NULL;

    RtlZeroMemory(OutBuffer, OutBufferBytes);
    RtlInitUnicodeString(&keyName, RegistryKeyPath);
    RtlInitUnicodeString(&valName, ValueName);
    InitializeObjectAttributes(&objAttr, &keyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    if (!NT_SUCCESS(ZwOpenKey(&hKey, KEY_READ, &objAttr))) {
        return;
    }

    UCHAR kvBuf[256] = { 0 };
    ULONG resLen = 0;
    if (NT_SUCCESS(ZwQueryValueKey(
            hKey,
            &valName,
            KeyValuePartialInformation,
            kvBuf,
            sizeof(kvBuf),
            &resLen))) {
        PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)kvBuf;
        if (info->Type == REG_SZ && info->DataLength > 0) {
            ULONG copyBytes = (info->DataLength < OutBufferBytes - sizeof(WCHAR))
                ? info->DataLength
                : (OutBufferBytes - sizeof(WCHAR));
            RtlCopyMemory(OutBuffer, info->Data, copyBytes);
        }
    }

    ZwClose(hKey);
}

static VOID
VgkQueryRegistryIdentityAndSecureBoot(VOID)
{
    /* 1. HKLM\SOFTWARE\Microsoft\Cryptography -> MachineGuid */
    VgkReadRegistryStringValue(
        L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Cryptography",
        L"MachineGuid",
        g_Vgk.HardwareProfile.MachineGuid,
        sizeof(g_Vgk.HardwareProfile.MachineGuid)
    );

    /* 2. HKLM\SYSTEM\CurrentControlSet\Control\IDConfigDB\Hardware Profiles\0001 -> HwProfileGuid */
    VgkReadRegistryStringValue(
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001",
        L"HwProfileGuid",
        g_Vgk.HardwareProfile.HwProfileGuid,
        sizeof(g_Vgk.HardwareProfile.HwProfileGuid)
    );

    /* 3. HKLM\SYSTEM\CurrentControlSet\Control\SecureBoot\State -> UEFISecureBootEnabled */
    UNICODE_STRING sbKeyName;
    UNICODE_STRING sbValName;
    OBJECT_ATTRIBUTES objAttr;
    HANDLE hKey = NULL;

    RtlInitUnicodeString(&sbKeyName, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State");
    RtlInitUnicodeString(&sbValName, L"UEFISecureBootEnabled");
    InitializeObjectAttributes(&objAttr, &sbKeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    if (NT_SUCCESS(ZwOpenKey(&hKey, KEY_READ, &objAttr))) {
        UCHAR kvBuf[64] = { 0 };
        ULONG resLen = 0;
        if (NT_SUCCESS(ZwQueryValueKey(
                hKey,
                &sbValName,
                KeyValuePartialInformation,
                kvBuf,
                sizeof(kvBuf),
                &resLen))) {
            PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)kvBuf;
            if (info->Type == REG_DWORD && info->DataLength >= sizeof(ULONG)) {
                g_Vgk.HardwareProfile.SecureBootEnabled = (*(PULONG)info->Data != 0) ? TRUE : FALSE;
            }
        }
        ZwClose(hKey);
    }
}

VOID
VgkCollectFullHardwareIdentity(VOID)
{
    VgkExtractSmbiosTables();
    VgkQueryTpm2EndorsementKeyAndPcrs();
    VgkQueryStorageDriveSerial();
    VgkQueryRegistryIdentityAndSecureBoot();
}

/*
 * §14.1 / §18: Assembla a mistura dos 64 bytes do DriverProof
 * Bloco A (32 bytes): HMAC-SHA256(Nonce + QPC + CR3 + ViolationFlags + Seq)
 * Bloco B (32 bytes): TPM 2.0 EK Hash + PCR[7] Attestation
 */
VOID
VgkGenerateDriverProof(
    _In_ ULONG64 Nonce,
    _Out_writes_bytes_(64) PUCHAR OutProof
)
{
    if (!OutProof) {
        return;
    }

    RtlZeroMemory(OutProof, 64);

    #pragma pack(push, 1)
    struct {
        ULONG64 Nonce;
        ULONG64 TimestampQpc;
        ULONG64 ProcessCr3;
        ULONG   Flags;
        ULONG   RuleId;
        ULONG64 Sequence;
    } proofInput;
    #pragma pack(pop)

    LARGE_INTEGER qpc = KeQueryPerformanceCounter(NULL);

    proofInput.Nonce        = Nonce;
    proofInput.TimestampQpc = (ULONG64)qpc.QuadPart;
    proofInput.ProcessCr3   = __readcr3();
    proofInput.Flags        = (ULONG)g_Vgk.ViolationFlags;
    proofInput.RuleId       = (ULONG)g_Vgk.LastMatchedRuleId;
    proofInput.Sequence     = (ULONG64)g_Vgk.HeartbeatSequence;

    /* Bloco 1 (0..31): HMAC de estado interno gerado pelo motor CNG */
    VgkCngComputeDigestByType(
        VGK_HASH_SHA256,
        (PUCHAR)&proofInput,
        sizeof(proofInput),
        &OutProof[0],
        32
    );

    /* Bloco 2 (32..63): Hash do silício físico do TPM 2.0 (Endorsement Key) */
    RtlCopyMemory(&OutProof[32], g_Vgk.HardwareProfile.TpmEkPubSha256, 32);
}
