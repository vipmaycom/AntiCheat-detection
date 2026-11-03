/*
 * dpc_telemetry.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) Multi-Core DPC Scheduler (§12, §13),
 * Multiarray Chunk Streaming Protocol Serializer (§18), Heartbeat Token Validator
 * (IOCTL_EAC_HEARTBEAT 0x804), and User-Mode Service IPC Handler.
 *
 * Telemetry Format Strings Reconstructed from .rdata:
 *   - "multiarray %s %d->%d : {\n"
 *   - "newl+ chunk type %d = %s [%d->%d]\n"
 *   - "newl? chunk type %d = %s [%d->%d]\n"
 *   - "newl! chunk type %d = %s [%d->%d]\n"
 */

#include "eac.h"

/* =========================================================================
 * 1. MULTIARRAY CHUNK STREAMING PROTOCOL SERIALIZER (§18)
 * ========================================================================= */

VOID
EacEmitTelemetryChunk(
    _In_ EAC_CHUNK_OP Operation,
    _In_ ULONG ChunkType,
    _In_z_ PCSTR ChunkTag,
    _In_ ULONG StartIndex,
    _In_ ULONG EndIndex
)
{
    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&g_Eac.TelemetryLock, &lockHandle);

    ULONG slotIdx = g_Eac.TelemetryWriteIndex % EAC_MAX_TELEMETRY_CHUNKS;
    g_Eac.TelemetryWriteIndex++;

    PEAC_TELEMETRY_CHUNK chunk = &g_Eac.TelemetryRing[slotIdx];
    chunk->SequenceId   = g_Eac.TelemetryWriteIndex;
    chunk->TimestampQpc = (ULONG64)KeQueryPerformanceCounter(NULL).QuadPart;
    chunk->Operation    = Operation;
    chunk->ChunkType    = ChunkType;
    chunk->StartIndex   = StartIndex;
    chunk->EndIndex     = EndIndex;

    RtlStringCbCopyA(chunk->ChunkTag, sizeof(chunk->ChunkTag), ChunkTag ? ChunkTag : "none");

    PCSTR fmtStr = "newl+ chunk type %d = %s [%d->%d]\n";
    if (Operation == EAC_CHUNK_OP_QUERY) {
        fmtStr = "newl? chunk type %d = %s [%d->%d]\n";
    } else if (Operation == EAC_CHUNK_OP_ALERT) {
        fmtStr = "newl! chunk type %d = %s [%d->%d]\n";
    }

    RtlStringCbPrintfA(
        chunk->FormattedLine,
        sizeof(chunk->FormattedLine),
        fmtStr,
        ChunkType,
        chunk->ChunkTag,
        StartIndex,
        EndIndex
    );

    KeReleaseInStackQueuedSpinLock(&lockHandle);
}

/* =========================================================================
 * 2. PASSIVE WORK ITEM ROUTINE (IoQueueWorkItem Deferral from DPC) (§12.1)
 * ========================================================================= */

static VOID
NTAPI
EacPassiveWorkItemRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    if (!g_Eac.Unloading) {
        ULONG pass = (ULONG)InterlockedIncrement(&g_Eac.DpcPassCount);

        /* 1. Snapshot control registers (FUN_1401e828c) */
        EacReadControlRegisters_1401e828c(&g_Eac.ControlRegs);

        /* 2. Acquire system module list & run FUN_1400d97f3 + FUN_1400d9e46 */
        PRTL_PROCESS_MODULES modules = EacAcquireModuleList();
        if (modules != NULL) {
            EacAuditKernelPageTablesAndStacks(modules);

            /* Every 4 passes (~4s): Code Cave & Dispatch Table Integrity Sweeps */
            if ((pass & 3) == 0) {
                EacAuditModuleTextIntegrityAndCodeCaves(modules);
                EacAuditDriverDispatchTables(modules);
                EacAuditUsbHidAndMouseStack(modules);
            }

            /* Every 8 passes (~8s): BYOVD Object Directory, Root Certs & Hypervisor Checks */
            if ((pass & 7) == 0) {
                EacScanByovdAndDeviceObjects(modules);
                EacVerifyRegistryRootCertificates();
                EacAuditHypervisorAndAntiDebug();
            }

            /* Every 16 passes (~16s): Protected Process VAD/MDL & PCI DMA Sweeps */
            if ((pass & 15) == 0) {
                if (g_Eac.ProtectedGameProcessId != NULL) {
                    EacAuditProtectedProcessMemory(g_Eac.ProtectedGameProcessId);
                }
                EacAuditPciBusDmaAndAcpi();
            }

            ExFreePoolWithTag(modules, EAC_POOL_TAG_SCAN);
        }
    }

    InterlockedExchange(&g_Eac.WorkItemActive, 0);
}

/* =========================================================================
 * 3. MULTI-CORE PERIODIC DPC ROUTINE (KeSetTimerEx 1000ms + KeSetTargetProcessorDpc)
 * ========================================================================= */

static VOID
NTAPI
EacPeriodicDpcRoutine(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (g_Eac.Unloading) {
        return;
    }

    /*
     * Multi-core rotation (§12.1):
     * Cycles the target logical processor on each 1000ms tick so CR3/MSR/DR7 checks
     * execute across all cores.
     */
    ULONG activeCount = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
    if (activeCount > 1) {
        g_Eac.CurrentDpcTargetCpu = (g_Eac.CurrentDpcTargetCpu + 1) % activeCount;
        KeSetTargetProcessorDpc(Dpc, (CCHAR)g_Eac.CurrentDpcTargetCpu);
    }

    /* Check hardware debug registers DR7 on current core at DISPATCH_LEVEL */
    if ((__readdr(7) & 0xFFULL) != 0ULL) {
        InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_KERNEL_DEBUGGER);
    }

    /* Delegate blocking / paging audits to PASSIVE_LEVEL worker thread */
    if (g_Eac.PeriodicWorkItem != NULL) {
        if (InterlockedCompareExchange(&g_Eac.WorkItemActive, 1, 0) == 0) {
            IoQueueWorkItem(
                g_Eac.PeriodicWorkItem,
                EacPassiveWorkItemRoutine,
                DelayedWorkQueue,
                NULL
            );
        }
    }
}

NTSTATUS
EacInitializeDpcAndTimer(VOID)
{
    if (!g_Eac.DeviceObject) {
        return STATUS_INVALID_PARAMETER;
    }

    g_Eac.PeriodicWorkItem = IoAllocateWorkItem(g_Eac.DeviceObject);
    if (!g_Eac.PeriodicWorkItem) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeTimerEx(&g_Eac.PeriodicTimer, NotificationTimer);
    KeInitializeDpc(&g_Eac.PeriodicDpc, EacPeriodicDpcRoutine, NULL);

    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -10000000LL; /* 1 second relative due time */
    KeSetTimerEx(&g_Eac.PeriodicTimer, dueTime, 1000, &g_Eac.PeriodicDpc);

    return STATUS_SUCCESS;
}

VOID
EacShutdownDpcAndTimer(VOID)
{
    KeCancelTimer(&g_Eac.PeriodicTimer);
    KeFlushQueuedDpcs();

    LARGE_INTEGER shortWait;
    shortWait.QuadPart = -100000LL; /* 10ms */
    while (InterlockedCompareExchange(&g_Eac.WorkItemActive, 0, 0) != 0) {
        KeDelayExecutionThread(KernelMode, FALSE, &shortWait);
    }

    if (g_Eac.PeriodicWorkItem != NULL) {
        IoFreeWorkItem(g_Eac.PeriodicWorkItem);
        g_Eac.PeriodicWorkItem = NULL;
    }
}

/* =========================================================================
 * 4. IRP_MJ_DEVICE_CONTROL HANDLER (EasyAntiCheat_EOS.exe <-> Driver IPC)
 * ========================================================================= */

NTSTATUS
EacHandleDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioctl = irpSp->Parameters.DeviceIoControl.IoControlCode;
    PVOID sysBuf = Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen  = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR bytesReturned = 0;

    switch (ioctl) {
    case IOCTL_EAC_BIND_SERVICE:
        InterlockedExchangePointer((PVOID volatile *)&g_Eac.ServiceProcessId, PsGetCurrentProcessId());
        EacEmitTelemetryChunk(
            EAC_CHUNK_OP_ADD,
            1,
            "service_bind",
            (ULONG)(ULONG_PTR)g_Eac.ServiceProcessId,
            1
        );
        break;

    case IOCTL_EAC_PROTECT_PROCESS:
        if (sysBuf != NULL && inLen >= sizeof(ULONG)) {
            ULONG targetPid = *(PULONG)sysBuf;
            InterlockedExchangePointer(
                (PVOID volatile *)&g_Eac.ProtectedGameProcessId,
                (HANDLE)(ULONG_PTR)targetPid
            );
            EacEmitTelemetryChunk(EAC_CHUNK_OP_ADD, 5, "protect_pid", targetPid, 1);
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_EAC_HEARTBEAT: /* 0x804 (§12.2, §18) */
        if (sysBuf != NULL && inLen >= sizeof(EAC_HEARTBEAT_PACKET) && outLen >= sizeof(EAC_HEARTBEAT_PACKET)) {
            PEAC_HEARTBEAT_PACKET hb = (PEAC_HEARTBEAT_PACKET)sysBuf;
            ULONG seq = (ULONG)InterlockedIncrement(&g_Eac.HeartbeatSequence);
            InterlockedExchange64(&g_Eac.LastHeartbeatTimestamp, KeQueryPerformanceCounter(NULL).QuadPart);

            /* Compute MBA response token using SplitMix64 constant 0xC6BC279692B5C323 */
            hb->DriverResponseToken  = EacDeriveMbaKey(hb->ClientChallenge ^ (ULONG64)seq);
            hb->ActiveViolationFlags = (ULONG)g_Eac.ViolationFlags;
            hb->HeartbeatSequence    = seq;
            bytesReturned            = sizeof(EAC_HEARTBEAT_PACKET);
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_EAC_RUN_PAGEWALK:
        if (sysBuf != NULL && inLen >= sizeof(EAC_PAGEWALK_REQUEST) && outLen >= sizeof(EAC_PAGEWALK_REQUEST)) {
            PEAC_PAGEWALK_REQUEST req = (PEAC_PAGEWALK_REQUEST)sysBuf;
            req->WalkSucceeded = EacWalkKernelPageTable_1400d9e46(
                req->TargetKernelVa,
                &req->OutEffectivePteFlags,
                g_Eac.MbaMasterSeed,
                &req->OutPhysicalAddress
            );

            PRTL_PROCESS_MODULES mods = EacAcquireModuleList();
            if (mods != NULL) {
                req->IsBackedBySignedModule = EacIsAddressBackedByModule(mods, req->TargetKernelVa, NULL, NULL);
                ExFreePoolWithTag(mods, EAC_POOL_TAG_SCAN);
            } else {
                req->IsBackedBySignedModule = FALSE;
            }
            bytesReturned = sizeof(EAC_PAGEWALK_REQUEST);
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_EAC_QUERY_HWID:
        if (sysBuf != NULL && outLen >= sizeof(EAC_HARDWARE_PROFILE)) {
            EacCollectHardwareTelemetry();
            RtlCopyMemory(sysBuf, &g_Eac.HwProfile, sizeof(EAC_HARDWARE_PROFILE));
            bytesReturned = sizeof(EAC_HARDWARE_PROFILE);
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_EAC_POLL_TELEMETRY:
        if (sysBuf != NULL && outLen >= sizeof(g_Eac.TelemetryRing)) {
            KLOCK_QUEUE_HANDLE lh;
            KeAcquireInStackQueuedSpinLock(&g_Eac.TelemetryLock, &lh);
            RtlCopyMemory(sysBuf, g_Eac.TelemetryRing, sizeof(g_Eac.TelemetryRing));
            KeReleaseInStackQueuedSpinLock(&lh);
            bytesReturned = sizeof(g_Eac.TelemetryRing);
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_EAC_EXEC_VM_CHUNK:
        if (sysBuf != NULL && inLen >= sizeof(ULONG)) {
            ULONG opcode = *(PULONG)sysBuf;
            status = EacExecuteSection7VmDispatcher(opcode, NULL);
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
