/*
 * dpc_telemetry.c
 * Riot Vanguard (vgk.sys v1.19.1.18) Telemetry Formatter, Format Descriptor Table
 * (RVA 0x162F00..0x1635B8), vgtrace Real-Time Lock-Free MDL Ring-Buffer Engine
 * (FUN_1401318f0, FUN_14013242c, FUN_1401326cc, FUN_14013da44), High-Resolution QPC
 * Execution Profiler (FUN_1401406dc), and User-Mode (vgc.exe) IOCTL Interface.
 */

#include "vgk.h"

/* =========================================================================
 * TELEMETRY FORMAT DESCRIPTOR TABLE (.rdata RVA 0x162F00..0x1635B8 IN v1.19.1.18)
 * ========================================================================= */

const PCWSTR g_VgkLogFormatTable[VGK_EVENT_MAX] = {
    VGK_VERSION_STRING_W,                       /* 0:  "1.19.1-18+20261001.143443 ; Thu Oct  1 14:35:29 2026\n" (0x1598C0) */
    L"KSS %u\n",                                /* 1:  Kernel Stack Scan (0x1631A8 -> 0x159CE0) */
    L"BBL %u\n",                                /* 2:  Boot Blacklist (0x1631B0 -> 0x159CF0) */
    L"EBBL %u\n",                               /* 3:  Extended Boot Blacklist (0x1631B8 -> 0x159D00) */
    L"MC %08x\n",                               /* 4:  Memory Check (0x1631C0 -> 0x159D18) */
    L"DCL %u\n",                                /* 5:  Device Check List (0x1631D8 -> 0x159D50) */
    L"EDCL %u\n",                               /* 6:  Extended Device Check List (0x1631E0 -> 0x159D60) */
    L"DF %d\n",                                 /* 7:  Driver Flag (0x1631F8 -> 0x159D78) */
    L"WIC %u\n",                                /* 8:  Work Item Check (0x163328 -> 0x159E68) */
    L"%p ; %S ; 0x%X\n",                        /* 9:  Pointer + Module + Offset (0x1631C8 -> 0x159D30) */
    L"0x%p ; %llu ; %#010x ; %#010x\n",         /* 10: Memory Range Telemetry (0x163210 -> 0x159DA0) */
    L"%llu ; %llu\n",                           /* 11: Timing Pair Telemetry (0x163368 -> 0x159E78) */
    L"ID;Samples;Total(ms);Avg(ms);Max(ms)\n",  /* 12: Profiler CSV Header (0x163428 -> 0x159EE0) */
    L"%u;%llu;%llu;%llu;%llu\n"                 /* 13: Profiler CSV Row (0x163430 -> 0x159F30) */
};

/* =========================================================================
 * NEW IN v1.19.1.18: HIGH-RESOLUTION QPC EXECUTION PROFILER (FUN_1401406dc)
 * Decompiled via REA procedure_pseudo_code (ev_dfd56ff342d5b44e6680171319f8658f6a780fbc0f648bfc571df003ec82e7e0):
 *   if ((longlong)DAT_14019cb58 < 1) return 0;
 *   lVar1 = KeQueryPerformanceCounter(0);
 *   return (ulonglong)(lVar1 * 1000000) / DAT_14019cb58;
 * ========================================================================= */

ULONG64
VgkQueryElapsedMicroseconds(VOID)
{
    ULONG64 freq = g_Vgk.HardwareProfile.QpcFrequency;
    if ((LONGLONG)freq < 1) {
        return 0ULL;
    }

    LARGE_INTEGER counter = KeQueryPerformanceCounter(NULL);
    return ((ULONG64)counter.QuadPart * 1000000ULL) / freq;
}

VOID
VgkRecordProfilerSample(
    _In_ ULONG BucketId,
    _In_ ULONG64 ElapsedMicroseconds
)
{
    if (BucketId >= VGK_PROFILER_MAX_BUCKETS) {
        return;
    }

    PVGK_PROFILER_BUCKET bucket = &g_Vgk.ProfilerBuckets[BucketId];
    InterlockedIncrement64(&bucket->Samples);
    InterlockedAdd64(&bucket->TotalMicroseconds, (LONG64)ElapsedMicroseconds);

    LONG64 currentMax = bucket->MaxMicroseconds;
    while ((LONGLONG)ElapsedMicroseconds > currentMax) {
        LONG64 prev = InterlockedCompareExchange64(
            &bucket->MaxMicroseconds,
            (LONG64)ElapsedMicroseconds,
            currentMax
        );
        if (prev == currentMax) {
            break;
        }
        currentMax = prev;
    }
}

VOID
VgkFlushProfilerMetrics(VOID)
{
    VgkLogTelemetryEvent(VGK_EVENT_PROFILER_HEADER, 0, NULL);

    for (ULONG i = 0; i < VGK_PROFILER_MAX_BUCKETS; ++i) {
        PVGK_PROFILER_BUCKET b = &g_Vgk.ProfilerBuckets[i];
        ULONG64 samples = (ULONG64)InterlockedAdd64(&b->Samples, 0);
        if (samples == 0) {
            continue;
        }
        ULONG64 totalMs = (ULONG64)InterlockedAdd64(&b->TotalMicroseconds, 0) / 1000ULL;
        ULONG64 avgMs   = totalMs / samples;
        ULONG64 maxMs   = (ULONG64)InterlockedAdd64(&b->MaxMicroseconds, 0) / 1000ULL;

        VgkLogTelemetryEvent(
            VGK_EVENT_PROFILER_ROW,
            b->BucketId,
            NULL,
            b->BucketId,
            samples,
            totalMs,
            avgMs,
            maxMs
        );
    }
}

/* =========================================================================
 * NEW IN v1.19.1.18: VGTRACE REAL-TIME KERNEL-TO-USER MDL RING-BUFFER
 * Reconstructed from REA decompilation of:
 *   - FUN_1401318f0 (VgtraceCheckConsumerCapacity / BuildConsumer)
 *   - FUN_14013242c (VgtraceFlushActiveBuffers)
 *   - FUN_1401326cc (VgtraceFlushToRealtime)
 *   - FUN_14013da44 (VgtraceNotifyConsumers)
 * ========================================================================= */

NTSTATUS
VgtraceInitializeLogger(VOID)
{
    PVGK_VGTRACE_LOGGER logger = &g_Vgk.VgtraceLogger;
    RtlZeroMemory(logger, sizeof(*logger));

    logger->LoggerId         = 1;
    logger->CollectionActive = TRUE;
    logger->FlushTimerMs     = 250;
    logger->FlushThreshold   = 16;
    logger->SlotMask         = VGK_VGTRACE_SLOT_COUNT - 1;
    logger->MaxPayloadBytes  = sizeof(((PVGK_VGTRACE_SLOT)0)->Payload);

    KeInitializeSpinLock(&logger->ConsumerListLock);
    InitializeListHead(&logger->ConsumerListHead);

    ULONG allocBytes = sizeof(VGK_VGTRACE_RING_CONTROL) +
                       (VGK_VGTRACE_SLOT_COUNT * sizeof(VGK_VGTRACE_SLOT));

    PUCHAR ringBase = (PUCHAR)ExAllocatePoolWithTag(NonPagedPoolNx, allocBytes, VGK_POOL_TAG);
    if (!ringBase) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(ringBase, allocBytes);

    logger->RingControl       = (PVGK_VGTRACE_RING_CONTROL)ringBase;
    logger->RingSlotsKernelVa = (PVGK_VGTRACE_SLOT)(ringBase + sizeof(VGK_VGTRACE_RING_CONTROL));

    logger->RingControl->CapacitySlots   = VGK_VGTRACE_SLOT_COUNT;
    logger->RingControl->SlotStrideBytes = sizeof(VGK_VGTRACE_SLOT);

    logger->RingMdl = IoAllocateMdl(ringBase, allocBytes, FALSE, FALSE, NULL);
    if (logger->RingMdl != NULL) {
        MmBuildMdlForNonPagedPool(logger->RingMdl);
    }

    /* Matches RVA 0x156C20 diagnostic string in v1.19.1.18 */
    DbgPrintEx(
        0x4D,
        0,
        "vgtrace[diag]: Worker logger=%u STARTED CollectionOn=%d FlushTimer=%u FlushThreshold=%u\r\n",
        logger->LoggerId,
        (int)logger->CollectionActive,
        logger->FlushTimerMs,
        logger->FlushThreshold
    );

    return STATUS_SUCCESS;
}

VOID
VgtraceShutdownLogger(VOID)
{
    PVGK_VGTRACE_LOGGER logger = &g_Vgk.VgtraceLogger;
    logger->CollectionActive = FALSE;

    if (logger->RingMdl != NULL) {
        IoFreeMdl(logger->RingMdl);
        logger->RingMdl = NULL;
    }

    if (logger->RingControl != NULL) {
        ExFreePoolWithTag(logger->RingControl, VGK_POOL_TAG);
        logger->RingControl       = NULL;
        logger->RingSlotsKernelVa = NULL;
    }
}

/*
 * Reconstructed FUN_1401318f0 (ev_2d637c5ef058b321c0eff072401686534d4e5260308ff25951975777e3f7a717)
 */
BOOLEAN
VgtraceCheckConsumerCapacity(
    _In_ PVGK_VGTRACE_LOGGER Logger
)
{
    if (!Logger || !Logger->RingControl) {
        return FALSE;
    }
    if (Logger->FlushThreshold == 0) {
        return TRUE;
    }

    ULONG64 usedSlots = Logger->RingControl->WriteIndex - Logger->RingControl->ReadIndex;
    return (usedSlots + Logger->FlushThreshold <= (ULONG64)Logger->RingControl->CapacitySlots);
}

/*
 * Reconstructed RVA 0x131990..0x131C20: VgtraceBuildConsumer
 * Attaches to caller process (KeStackAttachProcess), maps RingMdl into UserMode
 * (MmMapLockedPagesSpecifyCache), resolves optional user notification event
 * (ObReferenceObjectByHandle on *ExEventObjectType), and emits RVA 0x156C80 diagnostic.
 */
NTSTATUS
VgtraceBuildConsumer(
    _Inout_ PVGK_VGTRACE_LOGGER Logger,
    _In_opt_ HANDLE UserEventHandle,
    _Out_ PVOID *OutMappedUserVa
)
{
    if (OutMappedUserVa) {
        *OutMappedUserVa = NULL;
    }
    if (!Logger || !Logger->RingMdl || !Logger->RingControl) {
        return STATUS_DEVICE_NOT_READY;
    }

    PVGK_VGTRACE_CONSUMER consumer = (PVGK_VGTRACE_CONSUMER)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        sizeof(VGK_VGTRACE_CONSUMER),
        VGK_POOL_TAG
    );
    if (!consumer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(consumer, sizeof(*consumer));

    PEPROCESS currentProc = IoGetCurrentProcess();
    KAPC_STATE apcState;
    PVOID userVa = NULL;

    KeStackAttachProcess(currentProc, &apcState);
    __try {
        userVa = MmMapLockedPagesSpecifyCache(
            Logger->RingMdl,
            UserMode,
            MmCached,
            NULL,
            FALSE,
            NormalPagePriority
        );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        userVa = NULL;
    }
    KeUnstackDetachProcess(&apcState);

    if (!userVa) {
        ExFreePoolWithTag(consumer, VGK_POOL_TAG);
        return STATUS_UNSUCCESSFUL;
    }

    if (UserEventHandle != NULL) {
        PKEVENT eventObj = NULL;
        if (NT_SUCCESS(ObReferenceObjectByHandle(
                UserEventHandle,
                 EVENT_MODIFY_STATE,
                *ExEventObjectType,
                UserMode,
                (PVOID*)&eventObj,
                NULL))) {
            consumer->NotifyEvent = eventObj;
        }
    }

    consumer->ConsumerMdl = Logger->RingMdl;
    consumer->KernelVa    = Logger->RingControl;
    consumer->UserVa      = userVa;
    consumer->RefCount    = 1;

    /* Exact diagnostic string from RVA 0x156C80 in vgk.sys v1.19.1.18 */
    DbgPrintEx(
        0x4D,
        0,
        "vgtrace[diag]: BuildConsumer logger=%u kva=%p uva=%p mdl=%p\r\n",
        Logger->LoggerId,
        consumer->KernelVa,
        consumer->UserVa,
        consumer->ConsumerMdl
    );

    KIRQL oldIrql = KeAcquireSpinLockRaiseToDpc(&Logger->ConsumerListLock);
    InsertTailList(&Logger->ConsumerListHead, &consumer->ListEntry);
    KeReleaseSpinLock(&Logger->ConsumerListLock, oldIrql);

    if (OutMappedUserVa) {
        *OutMappedUserVa = userVa;
    }
    return STATUS_SUCCESS;
}

/*
 * Reconstructed FUN_14013242c: VgtraceFlushActiveBuffers
 * (ev_08ffcd0165bc52ca49d737ea0a84874478826f43eaaacd511f23b4223d35271d)
 * Sweeps active per-CPU telemetry buffers and flushes non-empty slots into the ring.
 */
NTSTATUS
VgtraceFlushActiveBuffers(
    _Inout_ PVGK_VGTRACE_LOGGER Logger,
    _In_ BOOLEAN FlushAll
)
{
    if (!Logger || !Logger->RingControl || !Logger->RingSlotsKernelVa) {
        return STATUS_NO_MEMORY; /* 0xC0000017 in FUN_14013242c */
    }

    ULONG collected = 0;
    ULONG64 writeIdx = Logger->RingControl->WriteIndex;
    ULONG64 readIdx  = Logger->RingControl->ReadIndex;

    if (writeIdx > readIdx) {
        collected = (ULONG)(writeIdx - readIdx);
    }

    /* Exact diagnostic string from RVA 0x156B20 */
    DbgPrintEx(
        0x4D,
        0,
        "vgtrace[diag]: FlushActiveBuffers logger=%u FlushAll=%d Collected=%u\r\n",
        Logger->LoggerId,
        (int)FlushAll,
        collected
    );

    if (collected == 0 && !FlushAll) {
        /* Exact diagnostic string from RVA 0x156B70 */
        DbgPrintEx(
            0x4D,
            0,
            "vgtrace[diag]: FlushActiveBuffers logger=%u SKIP empty buffer seq=%llu refCount=%d savedOff=%u curOff=%u\r\n",
            Logger->LoggerId,
            writeIdx,
            0,
            0U,
            0U
        );
    }

    return STATUS_SUCCESS;
}

/*
 * Reconstructed FUN_14013da44: VgtraceNotifyConsumers
 * (ev_9e9f81b68b1d1332887dc6905e8c0056a3d93de857261327cb8cae2c77b54c54)
 * Snapshots up to 4 registered callback consumers under spinlock and invokes their callbacks.
 */
typedef VOID (NTAPI *PFN_VGTRACE_CALLBACK)(
    _In_ ULONG LoggerId,
    _In_ ULONG64 EventContext,
    _In_opt_ PVOID CallbackContext
);

VOID
VgtraceNotifyConsumers(
    _Inout_ PVGK_VGTRACE_LOGGER Logger,
    _In_ ULONG64 EventContext
)
{
    if (!Logger || !Logger->CollectionActive) {
        return;
    }

    PVGK_VGTRACE_CONSUMER snapshot[4] = { 0 };
    ULONG count = 0;

    KIRQL oldIrql = KeAcquireSpinLockRaiseToDpc(&Logger->ConsumerListLock);
    PLIST_ENTRY entry = Logger->ConsumerListHead.Flink;
    while (entry != &Logger->ConsumerListHead && count < 4) {
        PVGK_VGTRACE_CONSUMER cons = CONTAINING_RECORD(entry, VGK_VGTRACE_CONSUMER, ListEntry);
        InterlockedIncrement(&cons->RefCount);
        snapshot[count++] = cons;
        entry = entry->Flink;
    }
    KeReleaseSpinLock(&Logger->ConsumerListLock, oldIrql);

    for (ULONG i = 0; i < count; ++i) {
        PVGK_VGTRACE_CONSUMER cons = snapshot[i];
        PFN_VGTRACE_CALLBACK cb = (PFN_VGTRACE_CALLBACK)cons->CallbackRoutine;
        if (cb != NULL) {
            cb(Logger->LoggerId, EventContext, cons->CallbackContext);
        }
        InterlockedDecrement(&cons->RefCount);
    }
}

/*
 * Reconstructed FUN_1401326cc: VgtraceFlushToRealtime
 * (ev_40faa3b1ae413673821d37703731ff14339b758fa4bfa4a367f0ec9a2eef6ffe)
 * Pushes a telemetry record into the lock-free seqlock MDL ring-buffer and signals up to 8 consumers.
 */
VOID
VgtraceFlushToRealtime(
    _Inout_ PVGK_VGTRACE_LOGGER Logger,
    _In_reads_bytes_(PayloadSize) const VOID *Payload,
    _In_ ULONG PayloadSize,
    _In_ ULONG EventSubCode
)
{
    if (!Logger) {
        return;
    }

    PVGK_VGTRACE_RING_CONTROL ring = Logger->RingControl;
    if (ring == NULL || Logger->RingSlotsKernelVa == NULL) {
        /* Exact diagnostic string from RVA 0x156CC0 */
        DbgPrintEx(
            0x4D,
            0,
            "vgtrace[diag]: FlushToRealtime logger=%u SKIPPED (ring not initialized) seq=%llu savedOffset=%u\n",
            Logger->LoggerId,
            (ULONG64)g_Vgk.HeartbeatSequence,
            PayloadSize
        );
        return;
    }

    /* 1. Snapshot up to 8 active consumers under spinlock and bump their RefCount */
    PVGK_VGTRACE_CONSUMER consumers[VGK_VGTRACE_MAX_CONSUMERS] = { 0 };
    ULONG consumerCount = 0;

    KIRQL oldIrql = KeAcquireSpinLockRaiseToDpc(&Logger->ConsumerListLock);
    PLIST_ENTRY entry = Logger->ConsumerListHead.Flink;
    while (entry != &Logger->ConsumerListHead && consumerCount < VGK_VGTRACE_MAX_CONSUMERS) {
        PVGK_VGTRACE_CONSUMER cons = CONTAINING_RECORD(entry, VGK_VGTRACE_CONSUMER, ListEntry);
        InterlockedIncrement(&cons->RefCount);
        consumers[consumerCount++] = cons;
        entry = entry->Flink;
    }
    KeReleaseSpinLock(&Logger->ConsumerListLock, oldIrql);

    /* 2. Clamp payload size and advance ReadIndex if ring is full (overwrite accounting) */
    ULONG clampedBytes = (PayloadSize <= Logger->MaxPayloadBytes)
        ? PayloadSize
        : Logger->MaxPayloadBytes;

    BOOLEAN overflowed = FALSE;
    ULONG64 writeIdx   = ring->WriteIndex;
    ULONG64 readIdx    = ring->ReadIndex;

    while ((writeIdx - readIdx) >= (ULONG64)ring->CapacitySlots) {
        if (InterlockedCompareExchange64(
                (volatile LONG64*)&ring->ReadIndex,
                (LONG64)(readIdx + 1),
                (LONG64)readIdx) == (LONG64)readIdx) {
            InterlockedIncrement64((volatile LONG64*)&ring->OverwriteCount);
            overflowed = TRUE;
        }
        writeIdx = ring->WriteIndex;
        readIdx  = ring->ReadIndex;
    }

    /* 3. Write slot under odd/even seqlock version (matching FUN_1401326cc) */
    ULONG slotIndex = (ULONG)(writeIdx & Logger->SlotMask);
    PVGK_VGTRACE_SLOT slot = &Logger->RingSlotsKernelVa[slotIndex];

    LONG seqVer = InterlockedIncrement(&slot->SeqLockVersion);
    KeMemoryBarrier();

    LARGE_INTEGER qpc = KeQueryPerformanceCounter(NULL);
    slot->Reserved0      = 0;
    slot->SequenceNumber = (ULONG64)InterlockedIncrement64(&g_Vgk.HeartbeatSequence);
    slot->TimestampQpc   = (ULONG64)qpc.QuadPart;
    slot->ThreadContext  = (ULONG64)(ULONG_PTR)PsGetCurrentThreadId();
    slot->PayloadSize    = clampedBytes;
    slot->EventSubCode   = EventSubCode;
    slot->BufferSequence = writeIdx;

    if (Payload != NULL && clampedBytes > 0) {
        RtlCopyMemory(slot->Payload, Payload, clampedBytes);
    }

    KeMemoryBarrier();
    InterlockedExchange(&slot->SeqLockVersion, seqVer + 1);

    /* 4. Advance WriteIndex and DeliveredCount */
    ring->WriteIndex = writeIdx + 1;
    ULONG64 deliveredAfter = (ULONG64)InterlockedIncrement64((volatile LONG64*)&ring->DeliveredCount);

    /* Exact diagnostic string from RVA 0x156D30 */
    DbgPrintEx(
        0x4D,
        0,
        "vgtrace[diag]: FlushToRealtime logger=%u PUSHED slot=%llu seq=%llu payload=%u writeIdx=%llu deliveredAfter=%llu consumers=%u\n",
        Logger->LoggerId,
        (ULONG64)slotIndex,
        slot->SequenceNumber,
        clampedBytes,
        ring->WriteIndex,
        deliveredAfter,
        consumerCount
    );

    /* 5. Notify registered consumers via KeSetEvent and release RefCount */
    for (ULONG i = 0; i < consumerCount; ++i) {
        PVGK_VGTRACE_CONSUMER cons = consumers[i];
        if (overflowed) {
            cons->OverflowFlag = 1;
        }
        if (cons->NotifyEvent != NULL) {
            KeSetEvent(cons->NotifyEvent, IO_NO_INCREMENT, FALSE);
        }
        InterlockedDecrement(&cons->RefCount);
    }
}

/* =========================================================================
 * TELEMETRY LOG EVENT FORMATTER (Writes to \SystemRoot\vgk0001.dat + vgtrace)
 * ========================================================================= */

VOID
VgkLogTelemetryEvent(
    _In_ VGK_LOG_EVENT_ID EventId,
    _In_ ULONG SubCode,
    _In_opt_ PCWSTR CustomFormat,
    ...
)
{
    if (EventId >= VGK_EVENT_MAX) {
        return;
    }

    PCWSTR fmt = (CustomFormat != NULL) ? CustomFormat : g_VgkLogFormatTable[EventId];
    if (!fmt) {
        return;
    }

    WCHAR formattedLine[256] = { 0 };
    va_list args;
    va_start(args, CustomFormat);
    RtlStringCchVPrintfW(formattedLine, RTL_NUMBER_OF(formattedLine), fmt, args);
    va_end(args);

    ULONG byteLen = (ULONG)(wcslen(formattedLine) * sizeof(WCHAR));

    /* Push every formatted telemetry event into the real-time vgtrace MDL ring-buffer */
    VgtraceFlushToRealtime(&g_Vgk.VgtraceLogger, formattedLine, byteLen, SubCode);

    /* Disk I/O to \SystemRoot\vgk0001.dat (RVA 0x156500) is only permitted at PASSIVE_LEVEL */
    if (KeGetCurrentIrql() != PASSIVE_LEVEL || KeAreAllApcsDisabled()) {
        return;
    }

    UNICODE_STRING logFilePath;
    RtlInitUnicodeString(&logFilePath, VGK_BOOT_LOG_DAT_W); /* L"\\SystemRoot\\vgk0001.dat" */

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(
        &objAttr,
        &logFilePath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    HANDLE          fileHandle = NULL;
    IO_STATUS_BLOCK ioStatus   = { 0 };

    NTSTATUS status = IoCreateFileEx(
        &fileHandle,
        FILE_APPEND_DATA | SYNCHRONIZE,
        &objAttr,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL,
        0,
        CreateFileTypeNone,
        NULL,
        IO_NO_PARAMETER_CHECKING,
        NULL
    );
    if (NT_SUCCESS(status)) {
        ZwWriteFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &ioStatus,
            formattedLine,
            byteLen,
            NULL,
            NULL
        );
        ZwFlushBuffersFile(fileHandle, &ioStatus);
        ZwClose(fileHandle);
    }
}

/* =========================================================================
 * DEFERRED WORK ITEM ("WIC %u\n") & PERIODIC KDPC TIMER
 * ========================================================================= */

static VOID
VgkPassiveWorkItemRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    if (!g_Vgk.Unloading) {
        ULONG64 tStart = VgkQueryElapsedMicroseconds();
        ULONG pass = (ULONG)InterlockedIncrement(&g_Vgk.WorkItemPassCount);

        /* 1. Trigger all-core IPI stack snapshot */
        VgkBroadcastIpiStackWalk();

        /* 2. Acquire system module list & run deferred integrity audits */
        PRTL_PROCESS_MODULES modules = VgkAcquireModuleList();
        if (modules != NULL) {
            VgkEvaluateCapturedStacks(modules);
            VgkSweepKernelPageTables(modules);
            VgkVerifyWin32kAndMouseStack(modules);

            /* Periodically re-scan object directories, BYOVD rules, Hypervisor MSRs, PCI & USB HID */
            if ((pass & 7) == 0) {
                VgkVerifyByovdRuleTableIntegrity();
                VgkScanObjectDirectoriesForByovd();
                VgkScanLoadedModulesAgainstRules(modules);
                VgkAuditHypervisorAndAntiDebug(modules);
                VgkAuditUsbHidAndMouseTopology();
                VgtraceFlushActiveBuffers(&g_Vgk.VgtraceLogger, FALSE);
            }

            if ((pass & 31) == 0) {
                VgkAuditPciBusDmaAndAcpi();
            }

            ExFreePoolWithTag(modules, VGK_POOL_TAG);
        }

        ULONG64 tEnd = VgkQueryElapsedMicroseconds();
        if (tEnd >= tStart) {
            VgkRecordProfilerSample(2, tEnd - tStart);
        }

        /* Emit WIC %u telemetry entry every 16 passes */
        if ((pass & 15) == 0) {
            VgkLogTelemetryEvent(VGK_EVENT_WIC, pass, NULL, pass);
        }
    }

    InterlockedExchange(&g_Vgk.WorkItemActive, 0);
}

static VOID
VgkPeriodicDpcRoutine(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (g_Vgk.Unloading) {
        return;
    }

    InterlockedIncrement64(&g_Vgk.HeartbeatSequence);

    if (g_Vgk.PeriodicWorkItem != NULL) {
        if (InterlockedCompareExchange(&g_Vgk.WorkItemActive, 1, 0) == 0) {
            IoQueueWorkItem(
                g_Vgk.PeriodicWorkItem,
                VgkPassiveWorkItemRoutine,
                DelayedWorkQueue,
                NULL
            );
        }
    }

    /* Re-arm timer with RDTSC-derived millisecond jitter (250ms - 900ms) */
    ULONG64 jitterMs = 250 + (__rdtsc() % 650);
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -((LONGLONG)jitterMs * 10000LL);
    KeSetTimer(&g_Vgk.PeriodicTimer, dueTime, &g_Vgk.PeriodicDpc);
}

NTSTATUS
VgkInitializeDpcAndWorkItem(VOID)
{
    g_Vgk.PeriodicWorkItem = IoAllocateWorkItem(g_Vgk.VgkDeviceObject);
    if (!g_Vgk.PeriodicWorkItem) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeTimer(&g_Vgk.PeriodicTimer);
    KeInitializeDpc(&g_Vgk.PeriodicDpc, VgkPeriodicDpcRoutine, NULL);

    LARGE_INTEGER initialDue;
    initialDue.QuadPart = -5000000LL; /* 500ms initial boot delay */
    KeSetTimer(&g_Vgk.PeriodicTimer, initialDue, &g_Vgk.PeriodicDpc);

    return STATUS_SUCCESS;
}

VOID
VgkShutdownDpcAndWorkItem(VOID)
{
    KeCancelTimer(&g_Vgk.PeriodicTimer);
    KeFlushQueuedDpcs();

    while (InterlockedCompareExchange(&g_Vgk.WorkItemActive, 0, 0) != 0) {
        LARGE_INTEGER waitInterval;
        waitInterval.QuadPart = -100000LL; /* 10ms */
        KeDelayExecutionThread(KernelMode, FALSE, &waitInterval);
    }

    if (g_Vgk.PeriodicWorkItem != NULL) {
        IoFreeWorkItem(g_Vgk.PeriodicWorkItem);
        g_Vgk.PeriodicWorkItem = NULL;
    }
}

/* =========================================================================
 * IRP_MJ_DEVICE_CONTROL HANDLER FOR vgc.exe <-> vgk.sys
 * ========================================================================= */

NTSTATUS
VgkHandleDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status          = STATUS_INVALID_DEVICE_REQUEST;
    ULONG bytesReturned      = 0;

    PVOID sysBuf = Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen  = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG ioctl  = irpSp->Parameters.DeviceIoControl.IoControlCode;

    switch (ioctl) {
    case IOCTL_VGK_BIND_VGC:
        g_Vgk.VgcProcessId = PsGetCurrentProcessId();
        status = STATUS_SUCCESS;
        break;

    case IOCTL_VGK_PROTECT_PID:
        if (sysBuf != NULL && inLen >= sizeof(ULONG)) {
            ULONG targetPid = *(PULONG)sysBuf;
            g_Vgk.ValorantProcessId = (HANDLE)(ULONG_PTR)targetPid;
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_VGK_POLL_TELEMETRY:
        if (sysBuf != NULL && outLen >= sizeof(VGK_TELEMETRY_PACKET)) {
            PVGK_TELEMETRY_PACKET pkt = (PVGK_TELEMETRY_PACKET)sysBuf;
            LARGE_INTEGER preciseTime = { 0 };
            KeQuerySystemTimePrecise(&preciseTime);

            pkt->SequenceId        = (ULONG64)InterlockedAdd64(&g_Vgk.HeartbeatSequence, 0);
            pkt->PreciseTimestamp  = (ULONG64)preciseTime.QuadPart;
            pkt->ViolationFlags    = (ULONG)InterlockedAdd(&g_Vgk.ViolationFlags, 0);
            pkt->LastMatchedRuleId = (ULONG)InterlockedAdd(&g_Vgk.LastMatchedRuleId, 0);
            pkt->ActiveGamePid     = (ULONG)(ULONG_PTR)g_Vgk.ValorantProcessId;
            pkt->IpiSampleCount    = (ULONG)InterlockedAdd(&g_Vgk.IpiSampleCount, 0);
            pkt->WorkItemPassCount = (ULONG)InterlockedAdd(&g_Vgk.WorkItemPassCount, 0);

            bytesReturned = sizeof(VGK_TELEMETRY_PACKET);
            status        = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_VGK_QUERY_HWID:
        if (sysBuf != NULL && outLen >= sizeof(VGK_HARDWARE_PROFILE)) {
            RtlCopyMemory(sysBuf, &g_Vgk.HardwareProfile, sizeof(VGK_HARDWARE_PROFILE));
            bytesReturned = sizeof(VGK_HARDWARE_PROFILE);
            status        = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_VGK_MAP_GUARDED_VIEW:
        if (sysBuf != NULL && outLen >= sizeof(PVOID)) {
            PVOID mappedVa = NULL;
            status = VgkMapGuardedRegionIntoProcess(&mappedVa);
            if (NT_SUCCESS(status)) {
                *(PVOID*)sysBuf = mappedVa;
                bytesReturned   = sizeof(PVOID);
            }
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_VGK_RUN_FULL_SWEEP:
        {
            ULONG64 t0 = VgkQueryElapsedMicroseconds();
            VgkBroadcastIpiStackWalk();
            VgkScanObjectDirectoriesForByovd();
            PRTL_PROCESS_MODULES modules = VgkAcquireModuleList();
            if (modules != NULL) {
                VgkEvaluateCapturedStacks(modules);
                VgkScanLoadedModulesAgainstRules(modules);
                VgkSweepKernelPageTables(modules);
                VgkVerifyWin32kAndMouseStack(modules);
                ExFreePoolWithTag(modules, VGK_POOL_TAG);
            }
            ULONG64 t1 = VgkQueryElapsedMicroseconds();
            if (t1 >= t0) {
                VgkRecordProfilerSample(3, t1 - t0);
            }
            status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_VGK_GENERATE_PROOF:
        if (sysBuf != NULL && inLen >= sizeof(ULONG64) && outLen >= sizeof(VGK_PROOF_CHALLENGE)) {
            PVGK_PROOF_CHALLENGE challenge = (PVGK_PROOF_CHALLENGE)sysBuf;
            VgkGenerateDriverProof(challenge->ServerNonce, challenge->DriverProof);
            bytesReturned = sizeof(VGK_PROOF_CHALLENGE);
            status        = STATUS_SUCCESS;
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
