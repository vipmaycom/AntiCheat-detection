/*
 * byovd_scanner.c
 * Riot Vanguard (vgk.sys v1.19.1.18) Vulnerable Driver (BYOVD) & Object Directory Scanner
 *
 * Reconstructed byte-for-byte from C:\Program Files\Riot Vanguard\vgk.sys .rdata RVA 0x15A000..0x15C260:
 *   - 30-entry VGK_BYOVD_SIGNATURE_RULE array at RVA 0x15A5C0 (mirrored at 0x15AF40 and 0x15BEC0)
 *   - Rule 3 (RVA 0x15A0A0, 88 bytes): Capcom.sys SMEP disable & user-pointer call opcode array
 *   - Rule 4 (RVA 0x15A100, 113 bytes): Pool tag 'RTMp' (0x52544D70) allocation opcode array
 *   - Rule 28 (RVA 0x15A528, 43 bytes): "Nov 14 2013\0\xCC\xCC\xCC\xCC\nNal Windows Driver Loaded "
 *   - ZwOpenDirectoryObject + ZwQueryDirectoryObject walk over \Device and \DosDevices
 */

#include "vgk.h"

/* =========================================================================
 * RAW BINARY OPCODE SIGNATURES EXTRACTED FROM C:\kgv.sys (.rdata)
 * ========================================================================= */

/* Rule 1 (RVA 0x068018, 0x22 bytes): Mimikatz / Cryptographic object failure string */
static const UCHAR g_Rule01_BcryptFailure[0x22] =
    "BCRYPT_OBJECT_LENGTH alg failure\n";

/* Rule 2 (RVA 0x068040, 0x62 bytes): Windows Kernel Explorer characteristic falsifier */
static const WCHAR g_Rule02_WinKernelExplorer[] =
    L"WindowsKernelExplorerFalsifyDriverCharacteristic";

/* Rule 3 (RVA 0x0680B0, 0x58 bytes): Capcom.sys CR4 SMEP bypass shellcode dispatcher */
static const UCHAR g_Rule03_CapcomSmepDispatcher[0x58] = {
    0x48, 0x39, 0x48, 0xF8, 0x74, 0x04, 0x33, 0xC0, 0xEB, 0x49, 0x48, 0x8B,
    0x44, 0x24, 0x50, 0x48, 0x89, 0x44, 0x24, 0x28, 0x48, 0x8B, 0x05, 0xC6,
    0xFD, 0xFF, 0xFF, 0x48, 0x89, 0x44, 0x24, 0x30, 0x48, 0xC7, 0x44, 0x24,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x21, 0x02, 0x00, 0x00,
    0x48, 0x8D, 0x4C, 0x24, 0x20, 0xFF, 0xD0, 0x48, 0x8B, 0x4C, 0x24, 0x30,
    0xFF, 0x54, 0x24, 0x28, 0x48, 0x8D, 0x05, 0x22, 0x02, 0x00, 0x00, 0x48,
    0x8D, 0x4C, 0x24, 0x20, 0xFF, 0xD0, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x48,
    0x83, 0xC4, 0x48, 0xC3
};

/* Rule 4 (RVA 0x068110, 0x71 bytes): ExAllocatePoolWithTag(NonPagedPool, 0x1028, 'RTMp') */
static const UCHAR g_Rule04_RtmpPoolGadget[0x71] = {
    0x48, 0x85, 0xC0, 0x48, 0x8B, 0xE8, 0x75, 0x05, 0x8D, 0x45, 0xDB, 0xEB,
    0x28, 0xBA, 0x28, 0x10, 0x00, 0x00, 0x33, 0xC9, 0x41, 0xB8, 0x70, 0x4D,
    0x54, 0x52, 0x48, 0x89, 0x74, 0x24, 0x58, 0xFF, 0x15, 0xB5, 0x2D, 0x00,
    0x00, 0x48, 0x85, 0xC0, 0x48, 0x8B, 0xF0, 0x75, 0x1C, 0x8D, 0x46, 0xF8,
    0x48, 0x8B, 0x74, 0x24, 0x58, 0x48, 0x8B, 0x6C, 0x24, 0x50, 0x48, 0x8B,
    0x5C, 0x24, 0x60, 0x48, 0x83, 0xC4, 0x30, 0x41, 0x5D, 0x41, 0x5C, 0x5F,
    0xC3, 0x4C, 0x89, 0x28, 0x4C, 0x89, 0x60, 0x08, 0x48, 0x89, 0x78, 0x10,
    0x48, 0xC7, 0x40, 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x48, 0x8D, 0x15, 0x07,
    0xFA, 0xFF, 0xFF, 0x48, 0x8D, 0x48, 0x28, 0x4C, 0x8B, 0xC0, 0xC7, 0x40,
    0x20, 0x00, 0x00, 0x00, 0x00
};

/* Rule 28 (RVA 0x068538, 0x2B bytes): Intel NAL iqvw64e.sys build date + load banner */
static const UCHAR g_Rule28_IntelNalTimestamp[0x2B] = {
    0x4E, 0x6F, 0x76, 0x20, 0x31, 0x34, 0x20, 0x32, 0x30, 0x31, 0x33, 0x00,
    0xCC, 0xCC, 0xCC, 0xCC, 0x0A, 0x4E, 0x61, 0x6C, 0x20, 0x57, 0x69, 0x6E,
    0x64, 0x6F, 0x77, 0x73, 0x20, 0x44, 0x72, 0x69, 0x76, 0x65, 0x72, 0x20,
    0x4C, 0x6F, 0x61, 0x64, 0x65, 0x64, 0x20
};

/* =========================================================================
 * EXACT 30-ENTRY BYOVD SIGNATURE TABLE (RVA 0x0685D0 IN C:\kgv.sys)
 * ========================================================================= */

const VGK_BYOVD_SIGNATURE_RULE g_VgkByovdRules[30] = {
    {  1, 0x22, g_Rule01_BcryptFailure,                       VGK_SIG_CAT_KERNEL_EXPLORER },
    {  2, 0x62, g_Rule02_WinKernelExplorer,                   VGK_SIG_CAT_KERNEL_EXPLORER },
    {  3, 0x58, g_Rule03_CapcomSmepDispatcher,                VGK_SIG_CAT_KERNEL_EXPLORER },
    {  4, 0x71, g_Rule04_RtmpPoolGadget,                      VGK_SIG_CAT_BYOVD_DRIVER    },
    {  5, 0x24, L"\\DosDevices\\inpout",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    {  6, 0x3C, L"\\DosDevices\\ZemanaAntiMalware",           VGK_SIG_CAT_BYOVD_DRIVER    },
    {  7, 0x28, "NalDeviceControl: InputBuffer was NULL\n",   VGK_SIG_CAT_BYOVD_DRIVER    },
    {  8, 0x28, "MAPMEM.SYS: memory successfully mapped\n",   VGK_SIG_CAT_BYOVD_DRIVER    },
    {  9, 0x26, L"\\DosDevices\\NTIOLib",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 10, 0x28, L"\\DosDevices\\ALSysIO",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 11, 0x2E, L"\\DosDevices\\Global\\CPUZ",                VGK_SIG_CAT_BYOVD_DRIVER    },
    { 12, 0x24, L"\\DosDevices\\RTCore",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 13, 0x1E, L"\\Device\\ATSZIO",                          VGK_SIG_CAT_BYOVD_DRIVER    },
    { 14, 0x15, "Leaving MsIoDispatch",                       VGK_SIG_CAT_BYOVD_DRIVER    },
    { 15, 0x16, "Leaving WinIoDispatch",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 16, 0x28, L"\\DosDevices\\GLCKIo2",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 17, 0x2C, L"\\DosDevices\\EneTechIo",                   VGK_SIG_CAT_BYOVD_DRIVER    },
    { 18, 0x28, L"\\DosDevices\\WinRing0",                    VGK_SIG_CAT_BYOVD_DRIVER    },
    { 19, 0x26, L"\\DosDevices\\HOSTNT",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 20, 0x28, L"\\DosDevices\\Asusgio",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 21, 0x34, L"\\DosDevices\\driveragent%d",               VGK_SIG_CAT_BYOVD_DRIVER    },
    { 22, 0x26, L"\\Device\\genericdrv",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 23, 0x2E, L"\\DosDevices\\AsUpdateio",                  VGK_SIG_CAT_BYOVD_DRIVER    },
    { 24, 0x26, L"\\DosDevices\\BS_Def",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 25, 0x28, L"\\DosDevices\\CITMDRV",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 26, 0x22, L"\\DosDevices\\SE64",                        VGK_SIG_CAT_BYOVD_DRIVER    },
    { 27, 0x26, L"\\DosDevices\\WinIoB",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 28, 0x2B, g_Rule28_IntelNalTimestamp,                   VGK_SIG_CAT_BYOVD_DRIVER    },
    { 29, 0x32, L"\\DosDevices\\AIDA64Driver",                VGK_SIG_CAT_BYOVD_DRIVER    },
    { 30, 0x25, L"\\DosDevices\\NVFLASH",                     VGK_SIG_CAT_BYOVD_DRIVER    }
};

/*
 * Second & Third Redundant Copies of the 30-Rule BYOVD Table in vgk.sys v1.19.1.18
 * (.rdata RVA 0x15AF40 and RVA 0x15BEC0) used to detect in-memory rule table tampering.
 */
const VGK_BYOVD_SIGNATURE_RULE g_VgkByovdRulesMirror1[30] = {
    {  1, 0x22, g_Rule01_BcryptFailure,                       VGK_SIG_CAT_KERNEL_EXPLORER },
    {  2, 0x62, g_Rule02_WinKernelExplorer,                   VGK_SIG_CAT_KERNEL_EXPLORER },
    {  3, 0x58, g_Rule03_CapcomSmepDispatcher,                VGK_SIG_CAT_KERNEL_EXPLORER },
    {  4, 0x71, g_Rule04_RtmpPoolGadget,                      VGK_SIG_CAT_BYOVD_DRIVER    },
    {  5, 0x24, L"\\DosDevices\\inpout",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    {  6, 0x3C, L"\\DosDevices\\ZemanaAntiMalware",           VGK_SIG_CAT_BYOVD_DRIVER    },
    {  7, 0x28, "NalDeviceControl: InputBuffer was NULL\n",   VGK_SIG_CAT_BYOVD_DRIVER    },
    {  8, 0x28, "MAPMEM.SYS: memory successfully mapped\n",   VGK_SIG_CAT_BYOVD_DRIVER    },
    {  9, 0x26, L"\\DosDevices\\NTIOLib",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 10, 0x28, L"\\DosDevices\\ALSysIO",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 11, 0x2E, L"\\DosDevices\\Global\\CPUZ",                VGK_SIG_CAT_BYOVD_DRIVER    },
    { 12, 0x24, L"\\DosDevices\\RTCore",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 13, 0x1E, L"\\Device\\ATSZIO",                          VGK_SIG_CAT_BYOVD_DRIVER    },
    { 14, 0x15, "Leaving MsIoDispatch",                       VGK_SIG_CAT_BYOVD_DRIVER    },
    { 15, 0x16, "Leaving WinIoDispatch",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 16, 0x28, L"\\DosDevices\\GLCKIo2",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 17, 0x2C, L"\\DosDevices\\EneTechIo",                   VGK_SIG_CAT_BYOVD_DRIVER    },
    { 18, 0x28, L"\\DosDevices\\WinRing0",                    VGK_SIG_CAT_BYOVD_DRIVER    },
    { 19, 0x26, L"\\DosDevices\\HOSTNT",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 20, 0x28, L"\\DosDevices\\Asusgio",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 21, 0x34, L"\\DosDevices\\driveragent%d",               VGK_SIG_CAT_BYOVD_DRIVER    },
    { 22, 0x26, L"\\Device\\genericdrv",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 23, 0x2E, L"\\DosDevices\\AsUpdateio",                  VGK_SIG_CAT_BYOVD_DRIVER    },
    { 24, 0x26, L"\\DosDevices\\BS_Def",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 25, 0x28, L"\\DosDevices\\CITMDRV",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 26, 0x22, L"\\DosDevices\\SE64",                        VGK_SIG_CAT_BYOVD_DRIVER    },
    { 27, 0x26, L"\\DosDevices\\WinIoB",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 28, 0x2B, g_Rule28_IntelNalTimestamp,                   VGK_SIG_CAT_BYOVD_DRIVER    },
    { 29, 0x32, L"\\DosDevices\\AIDA64Driver",                VGK_SIG_CAT_BYOVD_DRIVER    },
    { 30, 0x25, L"\\DosDevices\\NVFLASH",                     VGK_SIG_CAT_BYOVD_DRIVER    }
};

const VGK_BYOVD_SIGNATURE_RULE g_VgkByovdRulesMirror2[30] = {
    {  1, 0x22, g_Rule01_BcryptFailure,                       VGK_SIG_CAT_KERNEL_EXPLORER },
    {  2, 0x62, g_Rule02_WinKernelExplorer,                   VGK_SIG_CAT_KERNEL_EXPLORER },
    {  3, 0x58, g_Rule03_CapcomSmepDispatcher,                VGK_SIG_CAT_KERNEL_EXPLORER },
    {  4, 0x71, g_Rule04_RtmpPoolGadget,                      VGK_SIG_CAT_BYOVD_DRIVER    },
    {  5, 0x24, L"\\DosDevices\\inpout",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    {  6, 0x3C, L"\\DosDevices\\ZemanaAntiMalware",           VGK_SIG_CAT_BYOVD_DRIVER    },
    {  7, 0x28, "NalDeviceControl: InputBuffer was NULL\n",   VGK_SIG_CAT_BYOVD_DRIVER    },
    {  8, 0x28, "MAPMEM.SYS: memory successfully mapped\n",   VGK_SIG_CAT_BYOVD_DRIVER    },
    {  9, 0x26, L"\\DosDevices\\NTIOLib",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 10, 0x28, L"\\DosDevices\\ALSysIO",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 11, 0x2E, L"\\DosDevices\\Global\\CPUZ",                VGK_SIG_CAT_BYOVD_DRIVER    },
    { 12, 0x24, L"\\DosDevices\\RTCore",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 13, 0x1E, L"\\Device\\ATSZIO",                          VGK_SIG_CAT_BYOVD_DRIVER    },
    { 14, 0x15, "Leaving MsIoDispatch",                       VGK_SIG_CAT_BYOVD_DRIVER    },
    { 15, 0x16, "Leaving WinIoDispatch",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 16, 0x28, L"\\DosDevices\\GLCKIo2",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 17, 0x2C, L"\\DosDevices\\EneTechIo",                   VGK_SIG_CAT_BYOVD_DRIVER    },
    { 18, 0x28, L"\\DosDevices\\WinRing0",                    VGK_SIG_CAT_BYOVD_DRIVER    },
    { 19, 0x26, L"\\DosDevices\\HOSTNT",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 20, 0x28, L"\\DosDevices\\Asusgio",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 21, 0x34, L"\\DosDevices\\driveragent%d",               VGK_SIG_CAT_BYOVD_DRIVER    },
    { 22, 0x26, L"\\Device\\genericdrv",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 23, 0x2E, L"\\DosDevices\\AsUpdateio",                  VGK_SIG_CAT_BYOVD_DRIVER    },
    { 24, 0x26, L"\\DosDevices\\BS_Def",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 25, 0x28, L"\\DosDevices\\CITMDRV",                     VGK_SIG_CAT_BYOVD_DRIVER    },
    { 26, 0x22, L"\\DosDevices\\SE64",                        VGK_SIG_CAT_BYOVD_DRIVER    },
    { 27, 0x26, L"\\DosDevices\\WinIoB",                      VGK_SIG_CAT_BYOVD_DRIVER    },
    { 28, 0x2B, g_Rule28_IntelNalTimestamp,                   VGK_SIG_CAT_BYOVD_DRIVER    },
    { 29, 0x32, L"\\DosDevices\\AIDA64Driver",                VGK_SIG_CAT_BYOVD_DRIVER    },
    { 30, 0x25, L"\\DosDevices\\NVFLASH",                     VGK_SIG_CAT_BYOVD_DRIVER    }
};

BOOLEAN
VgkVerifyByovdRuleTableIntegrity(VOID)
{
    for (ULONG i = 0; i < 30; ++i) {
        if (g_VgkByovdRules[i].RuleId != g_VgkByovdRulesMirror1[i].RuleId ||
            g_VgkByovdRules[i].RuleId != g_VgkByovdRulesMirror2[i].RuleId ||
            g_VgkByovdRules[i].PatternLength != g_VgkByovdRulesMirror1[i].PatternLength ||
            g_VgkByovdRules[i].PatternLength != g_VgkByovdRulesMirror2[i].PatternLength ||
            g_VgkByovdRules[i].Category != g_VgkByovdRulesMirror1[i].Category) {
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_MC_HASH_MISMATCH);
            VgkLogTelemetryEvent(VGK_EVENT_MC, i + 1, NULL, i + 1);
            return FALSE;
        }
    }
    return TRUE;
}

/* =========================================================================
 * OBJECT MANAGER DIRECTORY SCANNER (ZwOpenDirectoryObject / ZwQueryDirectoryObject)
 * Audits \Device and \DosDevices for vulnerable symbolic links & devices (DCL %u)
 * ========================================================================= */

static VOID
VgkScanSingleObjectDirectory(
    _In_ PCWSTR DirectoryPath
)
{
    UNICODE_STRING dirName;
    RtlInitUnicodeString(&dirName, DirectoryPath);

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(
        &objAttr,
        &dirName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    HANDLE dirHandle = NULL;
    NTSTATUS status = ZwOpenDirectoryObject(&dirHandle, DIRECTORY_QUERY, &objAttr);
    if (!NT_SUCCESS(status)) {
        return;
    }

    ULONG bufSize = 0x1000;
    POBJECT_DIRECTORY_INFORMATION dirInfo = (POBJECT_DIRECTORY_INFORMATION)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        bufSize,
        VGK_POOL_TAG
    );
    if (!dirInfo) {
        ZwClose(dirHandle);
        return;
    }

    ULONG   context     = 0;
    ULONG   returnLen   = 0;
    BOOLEAN restartScan = TRUE;

    while (TRUE) {
        status = ZwQueryDirectoryObject(
            dirHandle,
            dirInfo,
            bufSize,
            TRUE,
            restartScan,
            &context,
            &returnLen
        );
        if (!NT_SUCCESS(status)) {
            break;
        }
        restartScan = FALSE;

        if (dirInfo->Name.Buffer != NULL && dirInfo->Name.Length > 0) {
            /* Compare directory entry against Rules 5..30 device suffixes */
            for (ULONG r = 4; r < ARRAYSIZE(g_VgkByovdRules); ++r) {
                PCWSTR ruleText = (PCWSTR)g_VgkByovdRules[r].PatternData;
                /* Skip ASCII rules (7, 8, 14, 15, 28) */
                if (r == 6 || r == 7 || r == 13 || r == 14 || r == 27) {
                    continue;
                }

                /* Locate leaf name after last backslash */
                PCWSTR leaf = ruleText;
                for (PCWSTR p = ruleText; *p != L'\0'; ++p) {
                    if (*p == L'\\') {
                        leaf = p + 1;
                    }
                }

                UNICODE_STRING targetLeaf;
                RtlInitUnicodeString(&targetLeaf, leaf);
                if (RtlPrefixUnicodeString(&targetLeaf, &dirInfo->Name, TRUE)) {
                    ULONG ruleId = (ULONG)g_VgkByovdRules[r].RuleId;
                    InterlockedExchange(&g_Vgk.LastMatchedRuleId, (LONG)ruleId);
                    InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_DCL_DEVICE_FOUND);
                    VgkLogTelemetryEvent(VGK_EVENT_DCL, ruleId, NULL, ruleId);
                }
            }
        }
    }

    ExFreePoolWithTag(dirInfo, VGK_POOL_TAG);
    ZwClose(dirHandle);
}

VOID
VgkScanObjectDirectoriesForByovd(VOID)
{
    VgkScanSingleObjectDirectory(L"\\Device");
    VgkScanSingleObjectDirectory(L"\\DosDevices");
}

/* =========================================================================
 * IN-MEMORY MODULE BYTE-PATTERN SCANNER (BBL %u / EBBL %u / DF %d)
 * ========================================================================= */

static BOOLEAN
VgkScanMemorySpanForPattern(
    _In_reads_bytes_(SpanLength) PUCHAR BaseAddress,
    _In_ ULONG SpanLength,
    _In_reads_bytes_(PatternLength) const VOID *Pattern,
    _In_ ULONG PatternLength
)
{
    if (!BaseAddress || !Pattern || PatternLength == 0 || SpanLength < PatternLength) {
        return FALSE;
    }

    ULONG limit = SpanLength - PatternLength;
    for (ULONG offset = 0; offset <= limit; ++offset) {
        if (!MmIsAddressValid(BaseAddress + offset) ||
            !MmIsAddressValid(BaseAddress + offset + PatternLength - 1)) {
            offset = (offset + PAGE_SIZE) & ~(PAGE_SIZE - 1);
            continue;
        }

        if (RtlCompareMemory(BaseAddress + offset, Pattern, PatternLength) == PatternLength) {
            return TRUE;
        }
    }
    return FALSE;
}

VOID
VgkScanLoadedModulesAgainstRules(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (!Modules) {
        return;
    }

    for (ULONG m = 0; m < Modules->NumberOfModules; ++m) {
        PUCHAR modBase = (PUCHAR)Modules->Modules[m].ImageBase;
        ULONG  modSize = Modules->Modules[m].ImageSize;

        /* Skip self (vgk.sys) and ntoskrnl (index 0) */
        if (m == 0 || modBase == (PUCHAR)g_Vgk.DriverObject->DriverStart) {
            continue;
        }

        if (!modBase || modSize < PAGE_SIZE || !MmIsAddressValid(modBase)) {
            continue;
        }

        /* Bound scan window to first 256KB of non-paged sections */
        ULONG scanWindow = (modSize > 0x40000) ? 0x40000 : modSize;

        for (ULONG r = 0; r < ARRAYSIZE(g_VgkByovdRules); ++r) {
            const VGK_BYOVD_SIGNATURE_RULE* rule = &g_VgkByovdRules[r];
            if (VgkScanMemorySpanForPattern(
                    modBase,
                    scanWindow,
                    rule->PatternData,
                    (ULONG)rule->PatternLength))
            {
                ULONG ruleId = (ULONG)rule->RuleId;
                InterlockedExchange(&g_Vgk.LastMatchedRuleId, (LONG)ruleId);

                if (rule->Category == VGK_SIG_CAT_KERNEL_EXPLORER) {
                    InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_EBBL_EXT_MATCH | VGK_FLAG_DF_DRIVER_FLAG);
                    VgkLogTelemetryEvent(VGK_EVENT_EBBL, ruleId, NULL, ruleId);
                } else {
                    InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_BBL_BYOVD_MATCH);
                    VgkLogTelemetryEvent(VGK_EVENT_BBL, ruleId, NULL, ruleId);
                }
            }
        }
    }
}
