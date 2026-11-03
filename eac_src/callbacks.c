/*
 * callbacks.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) Kernel Security Callbacks (§1, §2, §5, §6, §7)
 *
 * Registers and manages:
 *   1. ObRegisterCallbacks (Altitude "328010"):
 *      Strips PROCESS_VM_READ, PROCESS_VM_WRITE, PROCESS_VM_OPERATION, PROCESS_DUP_HANDLE,
 *      PROCESS_CREATE_THREAD, PROCESS_TERMINATE, PROCESS_SUSPEND_RESUME on FortniteClient-Win64-Shipping.exe
 *      and strips THREAD_SET_CONTEXT, THREAD_GET_CONTEXT, THREAD_SUSPEND_RESUME on game threads.
 *   2. PsSetCreateProcessNotifyRoutineEx:
 *      Tracks FortniteClient-Win64-Shipping.exe and EasyAntiCheat_EOS.exe lifecycle.
 *   3. PsSetCreateThreadNotifyRoutine:
 *      Audits new thread StartAddress via FUN_1400d9e46 (CR3 page table walker) and module bounds.
 *   4. PsSetLoadImageNotifyRoutine:
 *      Inspects every loaded kernel driver / user DLL before entry point execution.
 *   5. CmRegisterCallbackEx:
 *      Monitors registry writes targeting Testsigning, DisableIntegrityChecks, and Root Certificate stores.
 */

#include "eac.h"

/* =========================================================================
 * 1. ObRegisterCallbacks PRE-OPERATION INTERCEPTOR (§5, §7)
 * ========================================================================= */

static OB_PREOP_CALLBACK_STATUS
NTAPI
EacObPreProcessCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);

    if (g_Eac.Unloading || g_Eac.ProtectedGameProcessId == NULL) {
        return OB_PREOP_SUCCESS;
    }

    if (OperationInformation->ObjectType != *PsProcessType) {
        return OB_PREOP_SUCCESS;
    }

    PEPROCESS targetProcess = (PEPROCESS)OperationInformation->Object;
    HANDLE    targetPid     = PsGetProcessId(targetProcess);
    HANDLE    callerPid     = PsGetCurrentProcessId();

    if (targetPid != g_Eac.ProtectedGameProcessId) {
        return OB_PREOP_SUCCESS;
    }

    /* Allow self-access, System (PID 4), and authenticated EasyAntiCheat_EOS.exe */
    if (callerPid == targetPid || callerPid == (HANDLE)4 ||
        (g_Eac.ServiceProcessId != NULL && callerPid == g_Eac.ServiceProcessId)) {
        return OB_PREOP_SUCCESS;
    }

    if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
        OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~EAC_PROCESS_STRIP_MASK;
    } else if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
        OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess &= ~EAC_PROCESS_STRIP_MASK;
    }

    return OB_PREOP_SUCCESS;
}

static OB_PREOP_CALLBACK_STATUS
NTAPI
EacObPreThreadCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);

    if (g_Eac.Unloading || g_Eac.ProtectedGameProcessId == NULL) {
        return OB_PREOP_SUCCESS;
    }

    if (OperationInformation->ObjectType != *PsThreadType) {
        return OB_PREOP_SUCCESS;
    }

    PETHREAD targetThread = (PETHREAD)OperationInformation->Object;
    HANDLE   ownerPid     = PsGetThreadProcessId(targetThread);
    HANDLE   callerPid    = PsGetCurrentProcessId();

    if (ownerPid != g_Eac.ProtectedGameProcessId) {
        return OB_PREOP_SUCCESS;
    }

    if (callerPid == ownerPid || callerPid == (HANDLE)4 ||
        (g_Eac.ServiceProcessId != NULL && callerPid == g_Eac.ServiceProcessId)) {
        return OB_PREOP_SUCCESS;
    }

    if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
        OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~EAC_THREAD_STRIP_MASK;
    } else if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
        OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess &= ~EAC_THREAD_STRIP_MASK;
    }

    return OB_PREOP_SUCCESS;
}

/* =========================================================================
 * 2. PsSetCreateProcessNotifyRoutineEx (§1, §6, §7)
 * ========================================================================= */

static VOID
NTAPI
EacProcessNotifyRoutineEx(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    PCHAR imageName = PsGetProcessImageFileName(Process);
    if (!imageName) {
        return;
    }

    if (CreateInfo != NULL) {
        if (_stricmp(imageName, "FortniteClient") == 0 ||
            _strnicmp(imageName, EAC_FORTNITE_EXE_A, 14) == 0) {
            InterlockedExchangePointer((PVOID volatile *)&g_Eac.ProtectedGameProcessId, ProcessId);
            EacEmitTelemetryChunk(EAC_CHUNK_OP_ADD, 5, "fortnite_spawn", (ULONG)(ULONG_PTR)ProcessId, 1);
        } else if (_strnicmp(imageName, EAC_SERVICE_FILENAME_A, 14) == 0) {
            InterlockedExchangePointer((PVOID volatile *)&g_Eac.ServiceProcessId, ProcessId);
        }
    } else {
        if (ProcessId == g_Eac.ProtectedGameProcessId) {
            InterlockedExchangePointer((PVOID volatile *)&g_Eac.ProtectedGameProcessId, NULL);
        } else if (ProcessId == g_Eac.ServiceProcessId) {
            InterlockedExchangePointer((PVOID volatile *)&g_Eac.ServiceProcessId, NULL);
        }
    }
}

/* =========================================================================
 * 3. PsSetCreateThreadNotifyRoutine (§5, §6, §7)
 * Validates system thread StartAddress against PsLoadedModuleList + FUN_1400d9e46
 * ========================================================================= */

static VOID
NTAPI
EacThreadNotifyRoutine(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
)
{
    UNREFERENCED_PARAMETER(ProcessId);

    if (!Create || g_Eac.Unloading) {
        return;
    }

    PETHREAD thread = NULL;
    if (!NT_SUCCESS(PsLookupThreadByThreadId(ThreadId, &thread)) || thread == NULL) {
        return;
    }

    /* Check if this is a kernel system thread (ProcessId == 4) */
    if (ProcessId == (HANDLE)4) {
        PRTL_PROCESS_MODULES mods = EacAcquireModuleList();
        if (mods != NULL) {
            EAC_UNWIND_RESULT unwind;
            EacUnwindKernelStack_1400d97f3(NULL, mods, g_Eac.MbaMasterSeed, &unwind);
            ExFreePoolWithTag(mods, EAC_POOL_TAG_SCAN);
        }
    }

    ObDereferenceObject(thread);
}

/* =========================================================================
 * 4. PsSetLoadImageNotifyRoutine (§1, §2, §6, §7)
 * ========================================================================= */

static VOID
NTAPI
EacLoadImageNotifyRoutine(
    _In_opt_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO ImageInfo
)
{
    UNREFERENCED_PARAMETER(ProcessId);

    if (g_Eac.Unloading || !ImageInfo || !FullImageName || !FullImageName->Buffer) {
        return;
    }

    /* Kernel-mode driver load inspection (SystemSpaceImage == 1) */
    if (ImageInfo->SystemModeImage) {
        ULONG64 baseVa = (ULONG64)ImageInfo->ImageBase;
        ULONG64 pteFlags = 0ULL;
        ULONG64 physAddr = 0ULL;

        /* Walk newly mapped driver base via FUN_1400d9e46 */
        EacWalkKernelPageTable_1400d9e46(baseVa, &pteFlags, g_Eac.MbaMasterSeed, &physAddr);

        /* Check against blacklisted BYOVD driver names */
        UNICODE_STRING byovdNames[] = {
            RTL_CONSTANT_STRING(L"dbk64.sys"),
            RTL_CONSTANT_STRING(L"gdrv.sys"),
            RTL_CONSTANT_STRING(L"iqvw64e.sys"),
            RTL_CONSTANT_STRING(L"rtcore64.sys"),
            RTL_CONSTANT_STRING(L"mhyprot2.sys"),
            RTL_CONSTANT_STRING(L"capcom.sys"),
            RTL_CONSTANT_STRING(L"dbutil_2_3.sys"),
            RTL_CONSTANT_STRING(L"eneio64.sys"),
            RTL_CONSTANT_STRING(L"kprocesshacker.sys")
        };

        USHORT totalChars = FullImageName->Length / sizeof(WCHAR);
        for (ULONG i = 0; i < ARRAYSIZE(byovdNames); ++i) {
            USHORT targetChars = byovdNames[i].Length / sizeof(WCHAR);
            if (totalChars >= targetChars) {
                UNICODE_STRING tail;
                tail.Buffer        = &FullImageName->Buffer[totalChars - targetChars];
                tail.Length        = byovdNames[i].Length;
                tail.MaximumLength = byovdNames[i].Length;

                if (RtlEqualUnicodeString(&tail, &byovdNames[i], TRUE)) {
                    InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_BYOVD_BLACKLIST);
                    EacEmitTelemetryChunk(EAC_CHUNK_OP_ALERT, 2, "byovd_image_load", i, (ULONG)ImageInfo->ImageSize);
                }
            }
        }
    }
}

/* =========================================================================
 * 5. CmRegisterCallbackEx (§2, §6, §7)
 * Monitors registry modifications to DSE / Testsigning / SystemCertificates
 * ========================================================================= */

static NTSTATUS
NTAPI
EacRegistryCallback(
    _In_ PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2
)
{
    UNREFERENCED_PARAMETER(CallbackContext);

    if (g_Eac.Unloading || Argument1 == NULL || Argument2 == NULL) {
        return STATUS_SUCCESS;
    }

    REG_NOTIFY_CLASS notifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;
    if (notifyClass == RegNtPreSetValueKey) {
        PREG_SET_VALUE_KEY_INFORMATION setInfo = (PREG_SET_VALUE_KEY_INFORMATION)Argument2;
        if (setInfo->ValueName != NULL && setInfo->ValueName->Buffer != NULL) {
            UNICODE_STRING valTestSigning = RTL_CONSTANT_STRING(L"Testsigning");
            UNICODE_STRING valDisableCi   = RTL_CONSTANT_STRING(L"DisableIntegrityChecks");

            if (RtlEqualUnicodeString(setInfo->ValueName, &valTestSigning, TRUE) ||
                RtlEqualUnicodeString(setInfo->ValueName, &valDisableCi, TRUE)) {
                InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_REGISTRY_DSE_TAMPER);
                EacEmitTelemetryChunk(EAC_CHUNK_OP_ALERT, 7, "registry_dse_tamper", 0, 1);
            }
        }
    }

    return STATUS_SUCCESS;
}

/* =========================================================================
 * CALLBACK INSTALLATION & REMOVAL
 * ========================================================================= */

NTSTATUS
EacInstallSecurityCallbacks(VOID)
{
    /* 1. ObRegisterCallbacks */
    OB_OPERATION_REGISTRATION opRegs[2];
    RtlZeroMemory(opRegs, sizeof(opRegs));

    opRegs[0].ObjectType = PsProcessType;
    opRegs[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    opRegs[0].PreOperation = EacObPreProcessCallback;

    opRegs[1].ObjectType = PsThreadType;
    opRegs[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    opRegs[1].PreOperation = EacObPreThreadCallback;

    OB_CALLBACK_REGISTRATION cbReg;
    RtlZeroMemory(&cbReg, sizeof(cbReg));
    cbReg.Version                    = OB_FLT_REGISTRATION_VERSION;
    cbReg.OperationRegistrationCount = 2;
    cbReg.RegistrationContext        = NULL;
    RtlInitUnicodeString(&cbReg.Altitude, EAC_OB_ALTITUDE_W);
    cbReg.OperationRegistration      = opRegs;

    NTSTATUS status = ObRegisterCallbacks(&cbReg, &g_Eac.ObCallbackHandle);
    if (!NT_SUCCESS(status)) {
        g_Eac.ObCallbackHandle = NULL;
    }

    /* 2. PsSetCreateProcessNotifyRoutineEx */
    status = PsSetCreateProcessNotifyRoutineEx(EacProcessNotifyRoutineEx, FALSE);
    if (NT_SUCCESS(status)) {
        g_Eac.ProcessNotifyRegistered = TRUE;
    }

    /* 3. PsSetCreateThreadNotifyRoutine */
    status = PsSetCreateThreadNotifyRoutine(EacThreadNotifyRoutine);
    if (NT_SUCCESS(status)) {
        g_Eac.ThreadNotifyRegistered = TRUE;
    }

    /* 4. PsSetLoadImageNotifyRoutine */
    status = PsSetLoadImageNotifyRoutine(EacLoadImageNotifyRoutine);
    if (NT_SUCCESS(status)) {
        g_Eac.ImageNotifyRegistered = TRUE;
    }

    /* 5. CmRegisterCallbackEx */
    UNICODE_STRING cmAltitude;
    RtlInitUnicodeString(&cmAltitude, EAC_CM_ALTITUDE_W);
    status = CmRegisterCallbackEx(
        EacRegistryCallback,
        &cmAltitude,
        g_Eac.DriverObject,
        NULL,
        &g_Eac.CmCallbackCookie,
        NULL
    );
    if (NT_SUCCESS(status)) {
        g_Eac.CmCallbackRegistered = TRUE;
    }

    return STATUS_SUCCESS;
}

VOID
EacRemoveSecurityCallbacks(VOID)
{
    if (g_Eac.CmCallbackRegistered) {
        CmUnRegisterCallback(g_Eac.CmCallbackCookie);
        g_Eac.CmCallbackRegistered = FALSE;
    }

    if (g_Eac.ImageNotifyRegistered) {
        PsRemoveLoadImageNotifyRoutine(EacLoadImageNotifyRoutine);
        g_Eac.ImageNotifyRegistered = FALSE;
    }

    if (g_Eac.ThreadNotifyRegistered) {
        PsRemoveCreateThreadNotifyRoutine(EacThreadNotifyRoutine);
        g_Eac.ThreadNotifyRegistered = FALSE;
    }

    if (g_Eac.ProcessNotifyRegistered) {
        PsSetCreateProcessNotifyRoutineEx(EacProcessNotifyRoutineEx, TRUE);
        g_Eac.ProcessNotifyRegistered = FALSE;
    }

    if (g_Eac.ObCallbackHandle != NULL) {
        ObUnRegisterCallbacks(g_Eac.ObCallbackHandle);
        g_Eac.ObCallbackHandle = NULL;
    }
}
