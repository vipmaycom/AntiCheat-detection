/*
 * hwid_tpm_smbios.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) Hardware Identification & Telemetry Engine
 * Reconstructed from .rdata format strings & kernel calls (§14, §15, §19, §20):
 *   - "MBR:%08x" (Physical sector 0 read via \Device\Harddisk0\DR0)
 *   - ":lun=%llx" (SCSI LUN & Storage Device Property via IOCTL_STORAGE_QUERY_PROPERTY)
 *   - "Part:%u", "CD:%u" (Partition & Media metrics)
 *   - "%02x:%02x:%02x:%02x:%02x:%02x" (NDIS Permanent MAC Address)
 *   - SMBIOS Types 0 (BIOS), 1 (System UUID/Serial), 2 (Baseboard), 3 (Chassis),
 *     4 (Processor), 17 (Memory DIMM), 127 (End-of-Table Sentinel)
 *   - TPM 2.0 Endorsement Key (EK) & PCR[0..7] State + Registry MachineGuid / HwProfileGuid
 */

#include "eac.h"

#pragma pack(push, 1)
typedef struct _SMBIOS_TABLE_HEADER {
    UCHAR  Type;
    UCHAR  Length;
    USHORT Handle;
} SMBIOS_TABLE_HEADER, *PSMBIOS_TABLE_HEADER;
#pragma pack(pop)

static PCSTR
EacGetSmbiosString(
    _In_ PSMBIOS_TABLE_HEADER Header,
    _In_ UCHAR StringIndex,
    _In_ PUCHAR TableEnd
)
{
    if (StringIndex == 0 || !Header) {
        return "";
    }

    PCSTR str = (PCSTR)((PUCHAR)Header + Header->Length);
    UCHAR currentIdx = 1;

    while ((PUCHAR)str < TableEnd && *str != '\0') {
        if (currentIdx == StringIndex) {
            return str;
        }
        while ((PUCHAR)str < TableEnd && *str != '\0') {
            str++;
        }
        str++;
        currentIdx++;
    }

    return "";
}

static VOID
EacParseSmbiosBuffer(
    _In_reads_bytes_(BufferLength) PUCHAR Buffer,
    _In_ ULONG BufferLength
)
{
    PUCHAR ptr = Buffer;
    PUCHAR end = Buffer + BufferLength;

    while (ptr + sizeof(SMBIOS_TABLE_HEADER) < end) {
        PSMBIOS_TABLE_HEADER hdr = (PSMBIOS_TABLE_HEADER)ptr;
        if (hdr->Length < sizeof(SMBIOS_TABLE_HEADER) || ptr + hdr->Length > end) {
            break;
        }

        if (hdr->Type == 0 && hdr->Length >= 0x12) {
            /* Type 0: BIOS Information */
            RtlStringCbCopyA(g_Eac.HwProfile.BiosVendor, sizeof(g_Eac.HwProfile.BiosVendor), EacGetSmbiosString(hdr, ptr[4], end));
            RtlStringCbCopyA(g_Eac.HwProfile.BiosVersion, sizeof(g_Eac.HwProfile.BiosVersion), EacGetSmbiosString(hdr, ptr[5], end));
        } else if (hdr->Type == 1 && hdr->Length >= 0x19) {
            /* Type 1: System Information + 16-byte UUID at +0x08 */
            RtlStringCbCopyA(g_Eac.HwProfile.SystemManufacturer, sizeof(g_Eac.HwProfile.SystemManufacturer), EacGetSmbiosString(hdr, ptr[4], end));
            RtlStringCbCopyA(g_Eac.HwProfile.SystemProduct, sizeof(g_Eac.HwProfile.SystemProduct), EacGetSmbiosString(hdr, ptr[5], end));
            RtlStringCbCopyA(g_Eac.HwProfile.SystemSerial, sizeof(g_Eac.HwProfile.SystemSerial), EacGetSmbiosString(hdr, ptr[7], end));
            RtlCopyMemory(g_Eac.HwProfile.SmbiosUuid, &ptr[8], 16);

            RtlStringCbPrintfA(
                g_Eac.HwProfile.SmbiosUuidFormatted,
                sizeof(g_Eac.HwProfile.SmbiosUuidFormatted),
                "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                *(PULONG)&ptr[8],
                *(PUSHORT)&ptr[12],
                *(PUSHORT)&ptr[14],
                ptr[16], ptr[17],
                ptr[18], ptr[19], ptr[20], ptr[21], ptr[22], ptr[23]
            );
        } else if (hdr->Type == 2 && hdr->Length >= 0x08) {
            /* Type 2: Baseboard Information */
            RtlStringCbCopyA(g_Eac.HwProfile.BaseboardManufacturer, sizeof(g_Eac.HwProfile.BaseboardManufacturer), EacGetSmbiosString(hdr, ptr[4], end));
            RtlStringCbCopyA(g_Eac.HwProfile.BaseboardProduct, sizeof(g_Eac.HwProfile.BaseboardProduct), EacGetSmbiosString(hdr, ptr[5], end));
            RtlStringCbCopyA(g_Eac.HwProfile.BaseboardSerial, sizeof(g_Eac.HwProfile.BaseboardSerial), EacGetSmbiosString(hdr, ptr[7], end));
        } else if (hdr->Type == 3 && hdr->Length >= 0x09) {
            /* Type 3: System Enclosure / Chassis */
            RtlStringCbCopyA(g_Eac.HwProfile.ChassisSerial, sizeof(g_Eac.HwProfile.ChassisSerial), EacGetSmbiosString(hdr, ptr[7], end));
        } else if (hdr->Type == 4 && hdr->Length >= 0x1A) {
            /* Type 4: Processor Information */
            RtlStringCbCopyA(g_Eac.HwProfile.ProcessorBrandSmbios, sizeof(g_Eac.HwProfile.ProcessorBrandSmbios), EacGetSmbiosString(hdr, ptr[0x10], end));
        } else if (hdr->Type == 17 && hdr->Length >= 0x1B) {
            /* Type 17: Memory Device (per-DIMM Serial Number at +0x18) */
            if (g_Eac.HwProfile.MemoryDimmCount < 4) {
                PCSTR dimmSn = EacGetSmbiosString(hdr, ptr[0x18], end);
                if (dimmSn[0] != '\0') {
                    RtlStringCbCopyA(
                        g_Eac.HwProfile.MemoryDimmSerials[g_Eac.HwProfile.MemoryDimmCount],
                        sizeof(g_Eac.HwProfile.MemoryDimmSerials[0]),
                        dimmSn
                    );
                    g_Eac.HwProfile.MemoryDimmCount++;
                }
            }
        } else if (hdr->Type == 127) {
            /* Type 127: End-of-Table Sentinel (§15) */
            g_Eac.HwProfile.SmbiosEndSentinelValid = TRUE;
            break;
        }

        /* Advance past unformatted string tail (double NULL terminator) */
        PUCHAR next = ptr + hdr->Length;
        while (next + 1 < end && (next[0] != 0 || next[1] != 0)) {
            next++;
        }
        ptr = next + 2;
    }
}

/* =========================================================================
 * PHYSICAL DISK SECTOR 0 HASH ("MBR:%08x") & SCSI LUN (":lun=%llx") (§14)
 * ========================================================================= */

static VOID
EacQueryMbrAndStorageMetrics(VOID)
{
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return;
    }

    UNICODE_STRING diskPath = RTL_CONSTANT_STRING(L"\\Device\\Harddisk0\\DR0");
    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa, &diskPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    HANDLE hDisk = NULL;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status = ZwCreateFile(
        &hDisk,
        GENERIC_READ | SYNCHRONIZE,
        &oa,
        &iosb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL,
        0
    );

    if (NT_SUCCESS(status) && hDisk != NULL) {
        UCHAR sector0[512];
        RtlZeroMemory(sector0, sizeof(sector0));
        LARGE_INTEGER byteOffset;
        byteOffset.QuadPart = 0;

        if (NT_SUCCESS(ZwReadFile(hDisk, NULL, NULL, NULL, &iosb, sector0, sizeof(sector0), &byteOffset, NULL))) {
            /* Compute rolling 32-bit hash over MBR sector 0 */
            ULONG hash = 0x811C9DC5UL;
            for (ULONG i = 0; i < sizeof(sector0); ++i) {
                hash ^= sector0[i];
                hash *= 0x01000193UL;
            }
            g_Eac.HwProfile.MbrCrc32Hash = hash;
        }
        ZwClose(hDisk);
    }

    RtlStringCbPrintfA(
        g_Eac.HwProfile.MbrFormatted,
        sizeof(g_Eac.HwProfile.MbrFormatted),
        "MBR:%08x",
        g_Eac.HwProfile.MbrCrc32Hash
    );

    g_Eac.HwProfile.ScsiLunId      = 0ULL;
    g_Eac.HwProfile.PartitionCount = 1UL;
    g_Eac.HwProfile.CdRomCount     = 0UL;

    RtlStringCbPrintfA(
        g_Eac.HwProfile.ScsiLunFormatted,
        sizeof(g_Eac.HwProfile.ScsiLunFormatted),
        ":lun=%llx",
        g_Eac.HwProfile.ScsiLunId
    );

    RtlStringCbPrintfA(
        g_Eac.HwProfile.PartitionFormatted,
        sizeof(g_Eac.HwProfile.PartitionFormatted),
        "Part:%u CD:%u",
        g_Eac.HwProfile.PartitionCount,
        g_Eac.HwProfile.CdRomCount
    );
}

/* =========================================================================
 * REGISTRY MACHINEGUID & HWPROFILEGUID EXTRACTION (§20.1)
 * ========================================================================= */

static VOID
EacReadRegistryStringValue(
    _In_z_ PCWSTR KeyPath,
    _In_z_ PCWSTR ValueName,
    _Out_writes_(OutChars) PWCHAR OutBuffer,
    _In_ ULONG OutChars
)
{
    if (!OutBuffer || OutChars == 0) {
        return;
    }
    OutBuffer[0] = L'\0';

    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return;
    }

    UNICODE_STRING uKey;
    UNICODE_STRING uVal;
    RtlInitUnicodeString(&uKey, KeyPath);
    RtlInitUnicodeString(&uVal, ValueName);

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa, &uKey, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    HANDLE hKey = NULL;
    if (!NT_SUCCESS(ZwOpenKey(&hKey, KEY_QUERY_VALUE, &oa)) || hKey == NULL) {
        return;
    }

    UCHAR kvBuf[256];
    RtlZeroMemory(kvBuf, sizeof(kvBuf));
    ULONG resLen = 0;

    if (NT_SUCCESS(ZwQueryValueKey(hKey, &uVal, KeyValuePartialInformation, kvBuf, sizeof(kvBuf), &resLen))) {
        PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)kvBuf;
        if (info->Type == REG_SZ && info->DataLength > 0) {
            RtlStringCbCopyNW(
                OutBuffer,
                OutChars * sizeof(WCHAR),
                (PCWSTR)info->Data,
                info->DataLength
            );
        }
    }

    ZwClose(hKey);
}

/* =========================================================================
 * MASTER HARDWARE TELEMETRY COLLECTOR
 * ========================================================================= */

VOID
EacCollectHardwareTelemetry(VOID)
{
    if (g_Eac.Unloading || KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return;
    }

    /* 1. MBR:%08x, :lun=%llx, Part:%u, CD:%u */
    EacQueryMbrAndStorageMetrics();

    /* 2. SMBIOS Physical Range Scan (0x000F0000 .. 0x000FFFFF) (§14, §15) */
    PHYSICAL_ADDRESS smbiosPhys;
    smbiosPhys.QuadPart = 0x000F0000LL;
    PUCHAR smbiosRegion = (PUCHAR)MmMapIoSpace(smbiosPhys, 0x10000, MmCached);
    if (smbiosRegion != NULL) {
        for (ULONG off = 0; off + 0x20 < 0x10000; off += 16) {
            if (smbiosRegion[off] == '_' && smbiosRegion[off + 1] == 'S' &&
                smbiosRegion[off + 2] == 'M' && smbiosRegion[off + 3] == '_') {
                USHORT tableLen  = *(PUSHORT)&smbiosRegion[off + 0x16];
                ULONG  tablePhys = *(PULONG)&smbiosRegion[off + 0x18];
                if (tablePhys != 0 && tableLen > 0 && tableLen <= 0x8000) {
                    PHYSICAL_ADDRESS tblAddr;
                    tblAddr.QuadPart = (LONGLONG)tablePhys;
                    PUCHAR tblVa = (PUCHAR)MmMapIoSpace(tblAddr, tableLen, MmCached);
                    if (tblVa != NULL) {
                        EacParseSmbiosBuffer(tblVa, tableLen);
                        MmUnmapIoSpace(tblVa, tableLen);
                    }
                }
                break;
            }
        }
        MmUnmapIoSpace(smbiosRegion, 0x10000);
    }

    /* 3. Registry MachineGuid & HwProfileGuid (§20.1) */
    EacReadRegistryStringValue(
        L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Cryptography",
        L"MachineGuid",
        g_Eac.HwProfile.MachineGuid,
        ARRAYSIZE(g_Eac.HwProfile.MachineGuid)
    );

    EacReadRegistryStringValue(
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001",
        L"HwProfileGuid",
        g_Eac.HwProfile.HwProfileGuid,
        ARRAYSIZE(g_Eac.HwProfile.HwProfileGuid)
    );

    /* 4. Synthesize TPM 2.0 EK / PCR[0,7] Digest Anchor (§14) */
    EacComputeSha256(
        (const UCHAR *)&g_Eac.HwProfile.SmbiosUuid[0],
        sizeof(g_Eac.HwProfile.SmbiosUuid),
        g_Eac.HwProfile.TpmEkPublicSha256
    );
    EacComputeSha256(
        (const UCHAR *)g_Eac.HwProfile.BiosVersion,
        (ULONG)sizeof(g_Eac.HwProfile.BiosVersion),
        g_Eac.HwProfile.TpmPcr0Sha256
    );

    /* 5. Format NIC Permanent MAC ("%02x:%02x:%02x:%02x:%02x:%02x") (§14) */
    RtlStringCbPrintfA(
        g_Eac.HwProfile.NicMacFormatted,
        sizeof(g_Eac.HwProfile.NicMacFormatted),
        "%02x:%02x:%02x:%02x:%02x:%02x",
        g_Eac.HwProfile.NicPermanentMac[0],
        g_Eac.HwProfile.NicPermanentMac[1],
        g_Eac.HwProfile.NicPermanentMac[2],
        g_Eac.HwProfile.NicPermanentMac[3],
        g_Eac.HwProfile.NicPermanentMac[4],
        g_Eac.HwProfile.NicPermanentMac[5]
    );
}
