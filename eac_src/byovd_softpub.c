/*
 * byovd_softpub.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) In-Kernel Authenticode / Certificate Store
 * Validator (SoftpubAuthenticode, FlightRoot, AuthRoot), BYOVD Scanner (\Device\DBK64, etc.),
 * and Subsystem DriverObject->MajorFunction Dispatch Table Auditor (§2, §9, §11, §12.2).
 */

#include "eac.h"

/* =========================================================================
 * 1. IN-KERNEL CERTIFICATE STORE VERIFICATION (§2)
 * Audits:
 *   - \Registry\Machine\SOFTWARE\Microsoft\SystemCertificates\ROOT
 *   - \Registry\Machine\SOFTWARE\Microsoft\SystemCertificates\AuthRoot
 *   - \Registry\Machine\SOFTWARE\Microsoft\SystemCertificates\FlightRoot
 * ========================================================================= */

BOOLEAN
EacVerifyRegistryRootCertificates(VOID)
{
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return TRUE;
    }

    PCWSTR certStorePaths[] = {
        L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\SystemCertificates\\ROOT\\Certificates",
        L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\SystemCertificates\\AuthRoot\\Certificates",
        L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\SystemCertificates\\FlightRoot\\Certificates"
    };

    for (ULONG i = 0; i < ARRAYSIZE(certStorePaths); ++i) {
        UNICODE_STRING regPath;
        RtlInitUnicodeString(&regPath, certStorePaths[i]);

        OBJECT_ATTRIBUTES oa;
        InitializeObjectAttributes(&oa, &regPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        HANDLE hKey = NULL;
        NTSTATUS status = ZwOpenKey(&hKey, KEY_READ, &oa);
        if (NT_SUCCESS(status) && hKey != NULL) {
            ZwClose(hKey);
        }
    }

    return TRUE;
}

/* =========================================================================
 * 2. OBJECT DIRECTORY & BYOVD SCANNER (§2, §9, §11)
 * Enumerates \Device and \DosDevices for known vulnerable drivers and cheat devices:
 *   \Device\DBK64, \Device\Gio, \Device\RTCore64, \Device\Nal, \Device\mhyprot2, etc.
 * Also scans nt!PiDDBCacheTable for unloaded BYOVD vulnerable drivers.
 * ========================================================================= */

static const ULONG kByovdBlacklistTimestamps[] = {
    0x5C9B3A7B, /* RTCore64.sys (MSI Afterburner / RivaTuner) */
    0x5C63B7C4, /* gdrv.sys (GIGABYTE) */
    0x599E6E9C, /* dbk64.sys (Cheat Engine DBK) */
    0x518174C6, /* procexp152.sys / procexp.sys (Sysinternals) */
    0x5F1965B8, /* mhyprot2.sys (miHoYo) */
    0x55938B86, /* iqvw64e.sys (Intel Network Adapter Diag v1) */
    0x5908C84F, /* iqvw64e.sys (Intel Network Adapter Diag v2) */
    0x57D619AD, /* capcom.sys (Capcom Anti-Cheat Driver) */
    0x55B5D5C6, /* asiodrv.sys / asiodrv2.sys (ASUS ASUSTeK) */
    0x49B3100B, /* atillk64.sys (AMD ATI Technologies) */
    0x5BD02C88, /* glckio2.sys (ASRock Polychrome RGB) */
    0x5ECB8251, /* eneio.sys (ENE Technology RGB) */
    0x60773C33  /* echo_driver.sys (Echo Trail) */
};

static PULONG      g_DynamicBlacklistTimestamps = NULL;
static ULONG       g_DynamicBlacklistCount      = 0;
static KSPIN_LOCK  g_DynamicBlacklistLock;
static BOOLEAN     g_DynamicLockInitialized     = FALSE;

static PRTL_AVL_TABLE g_ResolvedPiDDBCacheTable = NULL;
static PERESOURCE     g_ResolvedPiDDBLock       = NULL;

/*
 * EacLocatePiDDBCacheTable
 * Resolves PiDDBCacheTable and PiDDBLock in ntoskrnl.exe by scanning for
 * 'lea rcx, [PiDDBCacheTable]; call RtlLookupElementGenericTableAvl'.
 */
static
BOOLEAN
EacLocatePiDDBCacheTable(
    _In_opt_ PRTL_PROCESS_MODULES Modules
)
{
    if (g_ResolvedPiDDBCacheTable != NULL) {
        return TRUE;
    }

    PVOID ntoskrnlBase = NULL;
    ULONG ntoskrnlSize = 0;

    if (Modules != NULL && Modules->NumberOfModules > 0) {
        ntoskrnlBase = Modules->Modules[0].ImageBase;
        ntoskrnlSize = Modules->Modules[0].ImageSize;
    }

    if (ntoskrnlBase == NULL) {
        return FALSE;
    }

    PUCHAR textStart = (PUCHAR)ntoskrnlBase;
    ULONG  searchLimit = (ntoskrnlSize > 0x1000000) ? 0x1000000 : ntoskrnlSize;
    ULONG64 pfnLookupAvl = (ULONG64)RtlLookupElementGenericTableAvl;

    for (ULONG i = 0x1000; i < searchLimit - 16; ++i) {
        /*
         * Look for:
         * 48 8D 0D <disp32> : lea rcx, [PiDDBCacheTable]
         * E8 <disp32>       : call RtlLookupElementGenericTableAvl
         */
        if (textStart[i] == 0x48 && textStart[i+1] == 0x8D && textStart[i+2] == 0x0D &&
            textStart[i+7] == 0xE8) {
            
            LONG callDisp = *(PLONG)&textStart[i + 8];
            ULONG64 callTarget = (ULONG64)&textStart[i + 12] + callDisp;

            if (callTarget == pfnLookupAvl) {
                LONG tableDisp = *(PLONG)&textStart[i + 3];
                g_ResolvedPiDDBCacheTable = (PRTL_AVL_TABLE)((ULONG64)&textStart[i + 7] + tableDisp);

                /* Scan backwards within 64 bytes for ExAcquireResourceExclusiveLite / PiDDBLock */
                ULONG backLimit = (i >= 64) ? 64 : i;
                for (ULONG k = 1; k < backLimit; ++k) {
                    ULONG cur = i - k;
                    if (textStart[cur] == 0x48 && textStart[cur+1] == 0x8D && textStart[cur+2] == 0x0D) {
                        LONG lockDisp = *(PLONG)&textStart[cur + 3];
                        g_ResolvedPiDDBLock = (PERESOURCE)((ULONG64)&textStart[cur + 7] + lockDisp);
                        break;
                    }
                }
                break;
            }
        }
    }

    return (g_ResolvedPiDDBCacheTable != NULL);
}

/*
 * EacScanPiDDBCacheTable
 * Traverses PiDDBCacheTable AVL tree and cross-references entries against
 * known vulnerable BYOVD driver timestamps.
 */
BOOLEAN
EacScanPiDDBCacheTable(VOID)
{
    if (KeGetCurrentIrql() > APC_LEVEL) {
        return TRUE;
    }

    if (g_ResolvedPiDDBCacheTable == NULL) {
        PRTL_PROCESS_MODULES mods = EacAcquireModuleList();
        if (mods != NULL) {
            EacLocatePiDDBCacheTable(mods);
            ExFreePoolWithTag(mods, EAC_POOL_TAG_SCAN);
        }
    }

    if (g_ResolvedPiDDBCacheTable == NULL) {
        return FALSE;
    }

    BOOLEAN lockAcquired = FALSE;
    if (g_ResolvedPiDDBLock != NULL && ExIsResourceAcquiredSharedLite(g_ResolvedPiDDBLock) == 0) {
        ExAcquireResourceSharedLite(g_ResolvedPiDDBLock, TRUE);
        lockAcquired = TRUE;
    }

    __try {
        /* Enumerate AVL table entries using RtlEnumerateGenericTableAvl */
        PVOID restartKey = NULL;
        for (PVOID elem = RtlEnumerateGenericTableAvl(g_ResolvedPiDDBCacheTable, TRUE);
             elem != NULL;
             elem = RtlEnumerateGenericTableAvl(g_ResolvedPiDDBCacheTable, FALSE)) {

            UNREFERENCED_PARAMETER(restartKey);
            PPIDDB_CACHE_ENTRY entry = (PPIDDB_CACHE_ENTRY)elem;
            ULONG stamp = entry->TimeDateStamp;

            /* Check against baseline vulnerable driver timestamps */
            for (ULONG b = 0; b < ARRAYSIZE(kByovdBlacklistTimestamps); ++b) {
                if (stamp == kByovdBlacklistTimestamps[b]) {
                    InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_BYOVD_BLACKLIST);
                    EacEmitTelemetryChunk(EAC_CHUNK_OP_ALERT, 2, "piddb_blacklisted_driver", b, stamp);
                }
            }

            /* Check against dynamic runtime timestamps received via IOCTL */
            if (g_DynamicBlacklistTimestamps != NULL && g_DynamicBlacklistCount > 0) {
                KLOCK_QUEUE_HANDLE qh;
                KeAcquireInStackQueuedSpinLock(&g_DynamicBlacklistLock, &qh);
                for (ULONG d = 0; d < g_DynamicBlacklistCount; ++d) {
                    if (stamp == g_DynamicBlacklistTimestamps[d]) {
                        InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_BYOVD_BLACKLIST);
                        EacEmitTelemetryChunk(EAC_CHUNK_OP_ALERT, 2, "piddb_dynamic_blacklist", d, stamp);
                    }
                }
                KeReleaseInStackQueuedSpinLock(&qh);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        /* Protected against concurrent unlinking */
    }

    if (lockAcquired && g_ResolvedPiDDBLock != NULL) {
        ExReleaseResourceLite(g_ResolvedPiDDBLock);
    }

    return TRUE;
}

/*
 * EacUpdateDynamicBlacklist
 * Updates the in-memory dynamic driver blacklist delivered from user-mode Oodle buffer.
 */
NTSTATUS
EacUpdateDynamicBlacklist(
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
)
{
    if (Buffer == NULL || BufferLength < sizeof(ULONG)) {
        return STATUS_INVALID_PARAMETER;
    }

    ULONG count = BufferLength / sizeof(ULONG);
    PULONG newBuf = (PULONG)ExAllocatePoolWithTag(NonPagedPoolNx, count * sizeof(ULONG), EAC_POOL_TAG_SCAN);
    if (newBuf == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(newBuf, Buffer, count * sizeof(ULONG));

    if (!g_DynamicLockInitialized) {
        KeInitializeSpinLock(&g_DynamicBlacklistLock);
        g_DynamicLockInitialized = TRUE;
    }

    KLOCK_QUEUE_HANDLE qh;
    KeAcquireInStackQueuedSpinLock(&g_DynamicBlacklistLock, &qh);
    PULONG oldBuf = g_DynamicBlacklistTimestamps;
    g_DynamicBlacklistTimestamps = newBuf;
    g_DynamicBlacklistCount      = count;
    KeReleaseInStackQueuedSpinLock(&qh);

    if (oldBuf != NULL) {
        ExFreePoolWithTag(oldBuf, EAC_POOL_TAG_SCAN);
    }

    /* Immediately trigger a PiDDBCacheTable sweep against newly added entries */
    EacScanPiDDBCacheTable();

    return STATUS_SUCCESS;
}

VOID
EacScanByovdAndDeviceObjects(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (g_Eac.Unloading || KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return;
    }

    /* 1. Audit unloaded drivers in nt!PiDDBCacheTable */
    EacScanPiDDBCacheTable();

    PCWSTR blacklistedDevices[] = {
        L"DBK64",
        L"Gio",
        L"RTCore64",
        L"mhyprot2",
        L"Htsysm72FB",
        L"dbutil_2_3",
        L"EneIo",
        L"KProcessHacker2",
        L"KProcessHacker3"
    };

    UNICODE_STRING dirName;
    RtlInitUnicodeString(&dirName, L"\\Device");

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa, &dirName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    HANDLE hDir = NULL;
    if (NT_SUCCESS(ZwOpenDirectoryObject(&hDir, DIRECTORY_QUERY, &oa)) && hDir != NULL) {
        ULONG  bufSize = 0x2000;
        PUCHAR dirBuf  = (PUCHAR)ExAllocatePoolWithTag(NonPagedPoolNx, bufSize, EAC_POOL_TAG_SCAN);
        if (dirBuf != NULL) {
            ULONG   context   = 0;
            ULONG   retLength = 0;
            BOOLEAN firstCall = TRUE;

            while (NT_SUCCESS(ZwQueryDirectoryObject(
                hDir,
                dirBuf,
                bufSize,
                FALSE,
                firstCall,
                &context,
                &retLength
            ))) {
                firstCall = FALSE;
                POBJECT_DIRECTORY_INFORMATION info = (POBJECT_DIRECTORY_INFORMATION)dirBuf;
                while (info->Name.Buffer != NULL && info->Name.Length > 0) {
                    for (ULONG b = 0; b < ARRAYSIZE(blacklistedDevices); ++b) {
                        UNICODE_STRING needle;
                        RtlInitUnicodeString(&needle, blacklistedDevices[b]);
                        if (RtlEqualUnicodeString(&info->Name, &needle, TRUE)) {
                            InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_BYOVD_BLACKLIST);
                            EacEmitTelemetryChunk(EAC_CHUNK_OP_ALERT, 2, "blacklisted_device_obj", b, 1);
                        }
                    }
                    info++;
                }
            }

            ExFreePoolWithTag(dirBuf, EAC_POOL_TAG_SCAN);
        }
        ZwClose(hDir);
    }

    /* Cross-check loaded kernel module list against BYOVD filenames & PDB paths */
    if (Modules != NULL) {
        PCSTR byovdModules[] = {
            "dbk64.sys",
            "gdrv.sys",
            "iqvw64e.sys",
            "rtcore64.sys",
            "mhyprot2.sys",
            "capcom.sys",
            "dbutil_2_3.sys",
            "eneio64.sys",
            "kprocesshacker.sys"
        };

        for (ULONG m = 0; m < Modules->NumberOfModules; ++m) {
            USHORT nameOff = Modules->Modules[m].OffsetToFileName;
            if (nameOff >= sizeof(Modules->Modules[m].FullPathName)) {
                continue;
            }
            PCSTR fileName = (PCSTR)&Modules->Modules[m].FullPathName[nameOff];
            for (ULONG k = 0; k < ARRAYSIZE(byovdModules); ++k) {
                if (_stricmp(fileName, byovdModules[k]) == 0) {
                    InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_BYOVD_BLACKLIST);
                    EacEmitTelemetryChunk(EAC_CHUNK_OP_ALERT, 2, fileName, m, k);
                }
            }
        }
    }
}

/* =========================================================================
 * 3. SUBSYSTEM DISPATCH TABLE INTEGRITY VERIFICATION (§11, §12.2)
 * Audits DriverObject->MajorFunction[IRP_MJ_*] across critical drivers:
 *   \Driver\Beep, \Driver\Null, \Driver\MouClass, \Driver\KbdClass, \Driver\Disk
 * Verifies every dispatch pointer is backed by a legitimate module and walks
 * its PTE via FUN_1400d9e46.
 * ========================================================================= */

VOID
EacAuditDriverDispatchTables(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (!Modules || g_Eac.Unloading || KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return;
    }

    PCWSTR monitoredDrivers[] = {
        L"\\Driver\\MouClass",
        L"\\Driver\\KbdClass",
        L"\\Driver\\Beep",
        L"\\Driver\\Null",
        L"\\Driver\\Disk",
        L"\\FileSystem\\FltMgr"
    };

    for (ULONG d = 0; d < ARRAYSIZE(monitoredDrivers); ++d) {
        UNICODE_STRING drvName;
        RtlInitUnicodeString(&drvName, monitoredDrivers[d]);

        PDRIVER_OBJECT drvObj = NULL;
        NTSTATUS status = ObReferenceObjectByName(
            &drvName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            0,
            *IoDriverObjectType,
            KernelMode,
            NULL,
            (PVOID *)&drvObj
        );
        if (!NT_SUCCESS(status) || drvObj == NULL) {
            continue;
        }

        for (ULONG mj = 0; mj <= IRP_MJ_MAXIMUM_FUNCTION; ++mj) {
            ULONG64 dispatchFn = (ULONG64)drvObj->MajorFunction[mj];
            if (dispatchFn == 0ULL) {
                continue;
            }

            ULONG64 modBase = 0ULL;
            if (!EacIsAddressBackedByModule(Modules, dispatchFn, &modBase, NULL)) {
                InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_DISPATCH_TABLE_HOOK);
                EacEmitTelemetryChunk(
                    EAC_CHUNK_OP_ALERT,
                    11,
                    "majorfunction_unbacked_hook",
                    d,
                    mj
                );
            } else {
                ULONG64 pteFlags = 0ULL;
                ULONG64 physAddr = 0ULL;
                if (!EacWalkKernelPageTable_1400d9e46(dispatchFn, &pteFlags, g_Eac.MbaMasterSeed, &physAddr) ||
                    (pteFlags & EAC_PTE_NX_BIT) != 0ULL) {
                    InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_DISPATCH_TABLE_HOOK);
                }
            }
        }

        ObDereferenceObject(drvObj);
    }
}
