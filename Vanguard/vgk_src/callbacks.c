/*
 * callbacks.c
 * Riot Vanguard (vgk.sys v1.19.1.18) Kernel Notification & Handle Protection Callbacks
 *
 * Uses the XOR-encrypted API dispatch table to register:
 *   1. ObRegisterCallbacks (Altitude "388400") protecting "VALORANT-Win64-Shipping.exe" (0x156A10) & "vgc.exe" (0x156A00)
 *   2. PsSetCreateProcessNotifyRoutineEx
 *   3. PsSetCreateThreadNotifyRoutine (triggers Kernel APC stack probe on new system threads)
 *   4. PsSetLoadImageNotifyRoutine (triggers BYOVD 30-rule check on driver loads)
 *   5. KeRegisterNmiCallback
 */

#include "vgk.h"

/* Function Pointer Typedefs for XOR-Encrypted API Calls (RVA 0x006AA6) */
typedef NTSTATUS (NTAPI *PFN_OB_REGISTER_CALLBACKS)(
    _In_ POB_CALLBACK_REGISTRATION CallbackRegistration,
    _Outptr_ PVOID *RegistrationHandle
);
typedef VOID (NTAPI *PFN_OB_UNREGISTER_CALLBACKS)(
    _In_ PVOID RegistrationHandle
);
typedef NTSTATUS (NTAPI *PFN_PS_SET_CREATE_PROCESS_NOTIFY_EX)(
    _In_ PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine,
    _In_ BOOLEAN Remove
);
typedef NTSTATUS (NTAPI *PFN_PS_SET_CREATE_THREAD_NOTIFY)(
    _In_ PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
);
typedef NTSTATUS (NTAPI *PFN_PS_REMOVE_CREATE_THREAD_NOTIFY)(
    _In_ PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
);
typedef NTSTATUS (NTAPI *PFN_PS_SET_LOAD_IMAGE_NOTIFY)(
    _In_ PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
);
typedef NTSTATUS (NTAPI *PFN_PS_REMOVE_LOAD_IMAGE_NOTIFY)(
    _In_ PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
);
typedef PVOID (NTAPI *PFN_KE_REGISTER_NMI_CALLBACK)(
    _In_ PNMI_CALLBACK CallbackRoutine,
    _In_opt_ PVOID Context
);
typedef NTSTATUS (NTAPI *PFN_KE_DEREGISTER_NMI_CALLBACK)(
    _In_ PVOID Handle
);

/* =========================================================================
 * OBREGISTERCALLBACKS PRE-OPERATION FILTER
 * Protects "VALORANT-Win64-Shipping.exe" (RVA 0x0617B0) and "vgc.exe" (RVA 0x0617A0)
 * ========================================================================= */

static OB_PREOP_CALLBACK_STATUS
VgkObPreOperationCallback(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);

    HANDLE gamePid = g_Vgk.ValorantProcessId;
    HANDLE vgcPid  = g_Vgk.VgcProcessId;
    if (gamePid == NULL && vgcPid == NULL) {
        return OB_PREOP_SUCCESS;
    }

    if (OperationInformation->KernelHandle) {
        return OB_PREOP_SUCCESS;
    }

    HANDLE callerPid = PsGetCurrentProcessId();
    if (callerPid == gamePid || callerPid == vgcPid) {
        return OB_PREOP_SUCCESS;
    }

    HANDLE  targetPid = NULL;
    BOOLEAN isThread  = FALSE;

    if (OperationInformation->ObjectType == *PsProcessType) {
        targetPid = PsGetProcessId((PEPROCESS)OperationInformation->Object);
    } else if (OperationInformation->ObjectType == *PsThreadType) {
        targetPid = PsGetThreadProcessId((PETHREAD)OperationInformation->Object);
        isThread  = TRUE;
    } else {
        return OB_PREOP_SUCCESS;
    }

    if (targetPid != gamePid && targetPid != vgcPid) {
        return OB_PREOP_SUCCESS;
    }

    ACCESS_MASK stripMask = isThread ? VGK_THREAD_STRIP_MASK : VGK_PROCESS_STRIP_MASK;

    if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
        OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~stripMask;
    } else if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
        OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess &= ~stripMask;
    }

    return OB_PREOP_SUCCESS;
}

/* =========================================================================
 * PROCESS, THREAD & IMAGE LOAD NOTIFICATION ROUTINES
 * ========================================================================= */

static VOID
VgkProcessNotifyRoutineEx(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(Process);

    if (CreateInfo != NULL) {
        if (CreateInfo->ImageFileName != NULL && CreateInfo->ImageFileName->Buffer != NULL) {
            UNICODE_STRING valorantName;
            UNICODE_STRING vgcName;
            RtlInitUnicodeString(&valorantName, L"VALORANT-Win64-Shipping.exe");
            RtlInitUnicodeString(&vgcName,      L"vgc.exe");

            if (CreateInfo->ImageFileName->Length >= valorantName.Length) {
                UNICODE_STRING suffix;
                suffix.Length        = valorantName.Length;
                suffix.MaximumLength = valorantName.Length;
                suffix.Buffer        = (PWCH)((PUCHAR)CreateInfo->ImageFileName->Buffer +
                                       (CreateInfo->ImageFileName->Length - valorantName.Length));
                if (RtlEqualUnicodeString(&suffix, &valorantName, TRUE)) {
                    g_Vgk.ValorantProcessId = ProcessId;
                }
            }

            if (CreateInfo->ImageFileName->Length >= vgcName.Length) {
                UNICODE_STRING suffix;
                suffix.Length        = vgcName.Length;
                suffix.MaximumLength = vgcName.Length;
                suffix.Buffer        = (PWCH)((PUCHAR)CreateInfo->ImageFileName->Buffer +
                                       (CreateInfo->ImageFileName->Length - vgcName.Length));
                if (RtlEqualUnicodeString(&suffix, &vgcName, TRUE)) {
                    g_Vgk.VgcProcessId = ProcessId;
                }
            }
        }
    } else {
        if (ProcessId == g_Vgk.ValorantProcessId) {
            g_Vgk.ValorantProcessId = NULL;
        } else if (ProcessId == g_Vgk.VgcProcessId) {
            g_Vgk.VgcProcessId = NULL;
        }
    }
}

static VOID
VgkThreadNotifyRoutine(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
)
{
    if (!Create) {
        return;
    }

    /* Probe newly created System threads (PID 4) via Kernel APC stack walk */
    if (ProcessId == (HANDLE)4) {
        PETHREAD threadObj = NULL;
        if (NT_SUCCESS(PsLookupThreadByThreadId(ThreadId, &threadObj))) {
            VgkProbeThreadViaKernelApc(threadObj);
            ObDereferenceObject(threadObj);
        }
    }
}

static VOID
VgkImageLoadNotifyRoutine(
    _In_opt_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO ImageInfo
)
{
    UNREFERENCED_PARAMETER(FullImageName);
    UNREFERENCED_PARAMETER(ProcessId);

    if (!ImageInfo || !ImageInfo->SystemModeImage) {
        return;
    }

    /*
     * When a new kernel driver (SystemModeImage == 1) is mapped,
     * scan its header and initial sections against the 30-rule BYOVD table.
     */
    PUCHAR imageBase = (PUCHAR)ImageInfo->ImageBase;
    ULONG  imageSize = (ULONG)ImageInfo->ImageSize;
    if (!imageBase || imageSize < PAGE_SIZE || !MmIsAddressValid(imageBase)) {
        return;
    }

    ULONG scanLen = (imageSize > 0x20000) ? 0x20000 : imageSize;
    for (ULONG r = 0; r < ARRAYSIZE(g_VgkByovdRules); ++r) {
        const VGK_BYOVD_SIGNATURE_RULE* rule = &g_VgkByovdRules[r];
        ULONG patLen = (ULONG)rule->PatternLength;
        if (scanLen < patLen) {
            continue;
        }

        for (ULONG off = 0; off <= (scanLen - patLen); ++off) {
            if (!MmIsAddressValid(imageBase + off) ||
                !MmIsAddressValid(imageBase + off + patLen - 1)) {
                off = (off + PAGE_SIZE) & ~(PAGE_SIZE - 1);
                continue;
            }
            if (RtlCompareMemory(imageBase + off, rule->PatternData, patLen) == patLen) {
                ULONG ruleId = (ULONG)rule->RuleId;
                InterlockedExchange(&g_Vgk.LastMatchedRuleId, (LONG)ruleId);
                InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_BBL_BYOVD_MATCH | VGK_FLAG_DF_DRIVER_FLAG);
                VgkLogTelemetryEvent(VGK_EVENT_DF, (ULONG)ruleId, NULL, (INT)ruleId);
                break;
            }
        }
    }
}

/* =========================================================================
 * CALLBACK INSTALLATION & REMOVAL VIA XOR-ENCRYPTED API TABLE
 * ========================================================================= */

NTSTATUS
VgkInstallSecurityCallbacks(VOID)
{
    /* 1. ObRegisterCallbacks */
    PFN_OB_REGISTER_CALLBACKS fnObReg =
        (PFN_OB_REGISTER_CALLBACKS)VgkResolveEncryptedApi(VGK_API_OB_REGISTER_CALLBACKS);
    if (fnObReg != NULL) {
        OB_OPERATION_REGISTRATION ops[2] = { 0 };
        OB_CALLBACK_REGISTRATION  reg    = { 0 };
        UNICODE_STRING            altitude;

        RtlInitUnicodeString(&altitude, VGK_OB_ALTITUDE_W);

        ops[0].ObjectType   = PsProcessType;
        ops[0].Operations   = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
        ops[0].PreOperation = VgkObPreOperationCallback;

        ops[1].ObjectType   = PsThreadType;
        ops[1].Operations   = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
        ops[1].PreOperation = VgkObPreOperationCallback;

        reg.Version                    = OB_FLT_REGISTRATION_VERSION;
        reg.OperationRegistrationCount = 2;
        reg.Altitude                   = altitude;
        reg.RegistrationContext        = NULL;
        reg.OperationRegistration      = ops;

        fnObReg(&reg, &g_Vgk.ObCallbackHandle);
    }

    /* 2. PsSetCreateProcessNotifyRoutineEx */
    PFN_PS_SET_CREATE_PROCESS_NOTIFY_EX fnProcNotify =
        (PFN_PS_SET_CREATE_PROCESS_NOTIFY_EX)VgkResolveEncryptedApi(VGK_API_PS_SET_CREATE_PROCESS_NOTIFY_EX);
    if (fnProcNotify != NULL) {
        if (NT_SUCCESS(fnProcNotify(VgkProcessNotifyRoutineEx, FALSE))) {
            g_Vgk.ProcessNotifyActive = TRUE;
        }
    }

    /* 3. PsSetCreateThreadNotifyRoutine */
    PFN_PS_SET_CREATE_THREAD_NOTIFY fnThreadNotify =
        (PFN_PS_SET_CREATE_THREAD_NOTIFY)VgkResolveEncryptedApi(VGK_API_PS_SET_CREATE_THREAD_NOTIFY);
    if (fnThreadNotify != NULL) {
        if (NT_SUCCESS(fnThreadNotify(VgkThreadNotifyRoutine))) {
            g_Vgk.ThreadNotifyActive = TRUE;
        }
    }

    /* 4. PsSetLoadImageNotifyRoutine */
    PFN_PS_SET_LOAD_IMAGE_NOTIFY fnImgNotify =
        (PFN_PS_SET_LOAD_IMAGE_NOTIFY)VgkResolveEncryptedApi(VGK_API_PS_SET_LOAD_IMAGE_NOTIFY);
    if (fnImgNotify != NULL) {
        if (NT_SUCCESS(fnImgNotify(VgkImageLoadNotifyRoutine))) {
            g_Vgk.ImageNotifyActive = TRUE;
        }
    }

    /* 5. KeRegisterNmiCallback */
    PFN_KE_REGISTER_NMI_CALLBACK fnNmiReg =
        (PFN_KE_REGISTER_NMI_CALLBACK)VgkResolveEncryptedApi(VGK_API_KE_REGISTER_NMI_CALLBACK);
    if (fnNmiReg != NULL) {
        g_Vgk.NmiCallbackHandle = fnNmiReg(VgkNmiInterruptHandler, NULL);
    }

    return STATUS_SUCCESS;
}

VOID
VgkRemoveSecurityCallbacks(VOID)
{
    /* Deregister in strict LIFO order */
    if (g_Vgk.NmiCallbackHandle != NULL) {
        PFN_KE_DEREGISTER_NMI_CALLBACK fnNmiDereg =
            (PFN_KE_DEREGISTER_NMI_CALLBACK)VgkResolveEncryptedApi(VGK_API_KE_DEREGISTER_NMI_CALLBACK);
        if (fnNmiDereg != NULL) {
            fnNmiDereg(g_Vgk.NmiCallbackHandle);
        }
        g_Vgk.NmiCallbackHandle = NULL;
    }

    if (g_Vgk.ImageNotifyActive) {
        PFN_PS_REMOVE_LOAD_IMAGE_NOTIFY fnImgRem =
            (PFN_PS_REMOVE_LOAD_IMAGE_NOTIFY)VgkResolveEncryptedApi(VGK_API_PS_REMOVE_LOAD_IMAGE_NOTIFY);
        if (fnImgRem != NULL) {
            fnImgRem(VgkImageLoadNotifyRoutine);
        }
        g_Vgk.ImageNotifyActive = FALSE;
    }

    if (g_Vgk.ThreadNotifyActive) {
        PFN_PS_REMOVE_CREATE_THREAD_NOTIFY fnThreadRem =
            (PFN_PS_REMOVE_CREATE_THREAD_NOTIFY)VgkResolveEncryptedApi(VGK_API_PS_REMOVE_CREATE_THREAD_NOTIFY);
        if (fnThreadRem != NULL) {
            fnThreadRem(VgkThreadNotifyRoutine);
        }
        g_Vgk.ThreadNotifyActive = FALSE;
    }

    if (g_Vgk.ProcessNotifyActive) {
        PFN_PS_SET_CREATE_PROCESS_NOTIFY_EX fnProcNotify =
            (PFN_PS_SET_CREATE_PROCESS_NOTIFY_EX)VgkResolveEncryptedApi(VGK_API_PS_SET_CREATE_PROCESS_NOTIFY_EX);
        if (fnProcNotify != NULL) {
            fnProcNotify(VgkProcessNotifyRoutineEx, TRUE);
        }
        g_Vgk.ProcessNotifyActive = FALSE;
    }

    if (g_Vgk.ObCallbackHandle != NULL) {
        PFN_OB_UNREGISTER_CALLBACKS fnObUnreg =
            (PFN_OB_UNREGISTER_CALLBACKS)VgkResolveEncryptedApi(VGK_API_OB_UNREGISTER_CALLBACKS);
        if (fnObUnreg != NULL) {
            fnObUnreg(g_Vgk.ObCallbackHandle);
        }
        g_Vgk.ObCallbackHandle = NULL;
    }
}
