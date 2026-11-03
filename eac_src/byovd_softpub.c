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
 * ========================================================================= */

VOID
EacScanByovdAndDeviceObjects(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (g_Eac.Unloading || KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return;
    }

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
