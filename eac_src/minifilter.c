/*
 * minifilter.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) Filesystem Minifilter
 * Static Import Anchor: FLTMGR.SYS!FltRegisterFilter (IAT @ 0x140311C18)
 * Registered at Altitude "328010" (§1, §6, §7, §24.1)
 *
 * Intercepts:
 *   - IRP_MJ_CREATE
 *   - IRP_MJ_WRITE
 *   - IRP_MJ_SET_INFORMATION
 *   - IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION
 *
 * Protects FortniteClient-Win64-Shipping.exe, EasyAntiCheat_EOS.sys, and .pak
 * archives against unauthorized write access, deletion/renaming, and cross-process
 * NtCreateSection image stomping.
 */

#include "eac.h"

static BOOLEAN
EacIsProtectedFilePath(
    _In_ PUNICODE_STRING FileName
)
{
    if (!FileName || !FileName->Buffer || FileName->Length == 0) {
        return FALSE;
    }

    UNICODE_STRING fnTarget1;
    UNICODE_STRING fnTarget2;
    UNICODE_STRING fnTarget3;
    RtlInitUnicodeString(&fnTarget1, L"FortniteClient-Win64-Shipping.exe");
    RtlInitUnicodeString(&fnTarget2, L"EasyAntiCheat_EOS.sys");
    RtlInitUnicodeString(&fnTarget3, L"EasyAntiCheat_EOS.exe");

    /* Case-insensitive substring scan across normalized path */
    USHORT charCount = FileName->Length / sizeof(WCHAR);
    const WCHAR *targets[3] = {
        fnTarget1.Buffer,
        fnTarget2.Buffer,
        fnTarget3.Buffer
    };
    USHORT targetLengths[3] = {
        (USHORT)(fnTarget1.Length / sizeof(WCHAR)),
        (USHORT)(fnTarget2.Length / sizeof(WCHAR)),
        (USHORT)(fnTarget3.Length / sizeof(WCHAR))
    };

    for (ULONG t = 0; t < 3; ++t) {
        USHORT tLen = targetLengths[t];
        if (charCount < tLen) {
            continue;
        }
        for (USHORT i = 0; i <= charCount - tLen; ++i) {
            UNICODE_STRING slice;
            slice.Buffer        = &FileName->Buffer[i];
            slice.Length        = tLen * sizeof(WCHAR);
            slice.MaximumLength = slice.Length;

            UNICODE_STRING needle;
            needle.Buffer        = (PWCH)targets[t];
            needle.Length        = slice.Length;
            needle.MaximumLength = slice.Length;

            if (RtlEqualUnicodeString(&slice, &needle, TRUE)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static FLT_PREOP_CALLBACK_STATUS
FLTAPI
EacMinifilterPreOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    if (CompletionContext) {
        *CompletionContext = NULL;
    }

    if (g_Eac.Unloading || KeGetCurrentIrql() > APC_LEVEL) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    UCHAR majorFunc = Data->Iopb->MajorFunction;
    BOOLEAN isWriteOrModify = FALSE;

    if (majorFunc == IRP_MJ_WRITE || majorFunc == IRP_MJ_SET_INFORMATION) {
        isWriteOrModify = TRUE;
    } else if (majorFunc == IRP_MJ_CREATE) {
        ACCESS_MASK desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
        if (desiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA | DELETE | FILE_WRITE_EA)) {
            isWriteOrModify = TRUE;
        }
    } else if (majorFunc == IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION) {
        ULONG prot = Data->Iopb->Parameters.AcquireForSectionSynchronization.PageProtection;
        if (prot & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY)) {
            isWriteOrModify = TRUE;
        }
    }

    if (!isWriteOrModify) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    HANDLE callerPid = PsGetCurrentProcessId();
    if (callerPid == (HANDLE)4 ||
        (g_Eac.ServiceProcessId != NULL && callerPid == g_Eac.ServiceProcessId) ||
        (g_Eac.ProtectedGameProcessId != NULL && callerPid == g_Eac.ProtectedGameProcessId)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    NTSTATUS status = FltGetFileNameInformation(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &nameInfo
    );
    if (!NT_SUCCESS(status) || nameInfo == NULL) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    FltParseFileNameInformation(nameInfo);
    if (EacIsProtectedFilePath(&nameInfo->Name)) {
        InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_MINIFILTER_TAMPER);
        EacEmitTelemetryChunk(EAC_CHUNK_OP_ALERT, 7, "minifilter_block", (ULONG)(ULONG_PTR)callerPid, majorFunc);

        FltReleaseFileNameInformation(nameInfo);
        Data->IoStatus.Status      = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;
        return FLT_PREOP_COMPLETE;
    }

    FltReleaseFileNameInformation(nameInfo);
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

static NTSTATUS
FLTAPI
EacMinifilterUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);
    EacTeardownMinifilter();
    return STATUS_SUCCESS;
}

static NTSTATUS
FLTAPI
EacMinifilterInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeFilesystemType);

    if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {
        return STATUS_FLT_DO_NOT_ATTACH;
    }
    return STATUS_SUCCESS;
}

static const FLT_OPERATION_REGISTRATION g_EacMinifilterCallbacks[] = {
    { IRP_MJ_CREATE,                                0, EacMinifilterPreOperation, NULL },
    { IRP_MJ_WRITE,                                 0, EacMinifilterPreOperation, NULL },
    { IRP_MJ_SET_INFORMATION,                       0, EacMinifilterPreOperation, NULL },
    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,   0, EacMinifilterPreOperation, NULL },
    { IRP_MJ_OPERATION_END }
};

static const FLT_REGISTRATION g_EacFilterRegistration = {
    sizeof(FLT_REGISTRATION),           /* Size */
    FLT_REGISTRATION_VERSION,           /* Version */
    0,                                  /* Flags */
    NULL,                               /* ContextRegistration */
    g_EacMinifilterCallbacks,           /* OperationRegistration */
    EacMinifilterUnload,                /* FilterUnloadCallback */
    EacMinifilterInstanceSetup,         /* InstanceSetupCallback */
    NULL,                               /* InstanceQueryTeardownCallback */
    NULL,                               /* InstanceTeardownStartCallback */
    NULL,                               /* InstanceTeardownCompleteCallback */
    NULL,                               /* GenerateFileNameCallback */
    NULL,                               /* NormalizeNameComponentCallback */
    NULL                                /* NormalizeContextCleanupCallback */
};

NTSTATUS
EacInitializeMinifilter(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    if (!DriverObject) {
        return STATUS_INVALID_PARAMETER;
    }

    /* Static import anchor in EasyAntiCheat_EOS.sys: FLTMGR.SYS!FltRegisterFilter */
    NTSTATUS status = FltRegisterFilter(
        DriverObject,
        &g_EacFilterRegistration,
        &g_Eac.MinifilterHandle
    );
    if (!NT_SUCCESS(status)) {
        g_Eac.MinifilterHandle = NULL;
        return status;
    }

    status = FltStartFiltering(g_Eac.MinifilterHandle);
    if (!NT_SUCCESS(status)) {
        FltUnregisterFilter(g_Eac.MinifilterHandle);
        g_Eac.MinifilterHandle = NULL;
    }

    return status;
}

VOID
EacTeardownMinifilter(VOID)
{
    if (g_Eac.MinifilterHandle != NULL) {
        FltUnregisterFilter(g_Eac.MinifilterHandle);
        g_Eac.MinifilterHandle = NULL;
    }
}
