/*
 * ipi_apc_stackwalk.c
 * Riot Vanguard (vgk.sys v1.19.1.18) Execution Profiling & Stack Unwind Engine
 *
 * Implements the exact execution-profiling primitives imported by vgk.sys v1.19.1.18:
 *   1. KeIpiGenericCall: Broadcasts synchronous Inter-Processor Interrupts across all cores
 *   2. KeRegisterNmiCallback: High-IRQL (HIGH_LEVEL) Non-Maskable Interrupt stack sampling
 *   3. KeInitializeApc + KeInsertQueueApc: Kernel APC thread stack walking
 *   4. RtlLookupFunctionEntry (.pdata RVA 0x1A1000) validation: Emits "KSS %u\n" (RVA 0x159CE0)
 */

#include "vgk.h"

/* =========================================================================
 * ALL-CORE INTER-PROCESSOR INTERRUPT (KeIpiGenericCall) CALLBACK
 * ========================================================================= */

static ULONG_PTR
NTAPI
VgkIpiBroadcastWorker(
    _In_ ULONG_PTR Argument
)
{
    UNREFERENCED_PARAMETER(Argument);

    /*
     * Runs at IPI_LEVEL across every logical processor simultaneously.
     * Captures active thread context, CR3, DR7, and stack words into pre-allocated NonPagedPool.
     */
    ULONG cpuIdx = KeGetCurrentProcessorNumberEx(NULL);
    if (cpuIdx >= VGK_MAX_CPUS) {
        return 0;
    }

    PVGK_CPU_STACK_SAMPLE sample = &g_Vgk.CpuSamples[cpuIdx];
    if (InterlockedCompareExchange(&sample->SampleValid, 0, 0) != 0) {
        return 0;
    }

    sample->ProcessorIndex     = cpuIdx;
    sample->ProcessId          = PsGetCurrentProcessId();
    sample->ThreadId           = PsGetCurrentThreadId();
    sample->InstructionPointer = (ULONG64)_ReturnAddress();
    sample->StackPointer       = (ULONG64)_AddressOfReturnAddress();
    sample->Cr3Register        = __readcr3();
    sample->Dr7Register        = __readdr(7);

    /* Flag active hardware debug registers (DR0-DR3 enabled in DR7) */
    if ((sample->Dr7Register & 0xFF) != 0) {
        InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_KSS_DR7_ACTIVE);
    }

    ULONG_PTR stackLow  = 0;
    ULONG_PTR stackHigh = 0;
    IoGetStackLimits(&stackLow, &stackHigh);

    ULONG64 rsp = sample->StackPointer;
    if (rsp >= stackLow && rsp < stackHigh) {
        ULONG maxWords = (ULONG)((stackHigh - (ULONG_PTR)rsp) / sizeof(ULONG64));
        if (maxWords > VGK_STACK_CAPTURE_DEPTH) {
            maxWords = VGK_STACK_CAPTURE_DEPTH;
        }
        RtlCopyMemory(sample->StackWords, (PVOID)rsp, maxWords * sizeof(ULONG64));
        sample->CapturedFramesCount = maxWords;
    } else {
        sample->CapturedFramesCount = 0;
    }

    InterlockedIncrement(&g_Vgk.IpiSampleCount);
    InterlockedExchange(&sample->SampleValid, 1);
    return 0;
}

VOID
VgkBroadcastIpiStackWalk(VOID)
{
    if (g_Vgk.Unloading) {
        return;
    }
    KeIpiGenericCall(VgkIpiBroadcastWorker, 0);
}

/* =========================================================================
 * HIGH_LEVEL NMI INTERRUPT CALLBACK (KeRegisterNmiCallback)
 * ========================================================================= */

BOOLEAN
VgkNmiInterruptHandler(
    _In_opt_ PVOID Context,
    _In_ BOOLEAN Handled
)
{
    UNREFERENCED_PARAMETER(Context);

    ULONG cpuIdx = KeGetCurrentProcessorNumberEx(NULL);
    if (cpuIdx < VGK_MAX_CPUS) {
        VgkIpiBroadcastWorker(0);
    }

    return Handled ? TRUE : TRUE;
}

/* =========================================================================
 * DEFERRED STACK & .PDATA UNWIND VERIFIER ("KSS %u\n" at RVA 0x065658)
 * ========================================================================= */

VOID
VgkEvaluateCapturedStacks(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (!Modules) {
        return;
    }

    for (ULONG cpu = 0; cpu < VGK_MAX_CPUS; ++cpu) {
        PVGK_CPU_STACK_SAMPLE sample = &g_Vgk.CpuSamples[cpu];
        if (InterlockedCompareExchange(&sample->SampleValid, 0, 1) != 1) {
            continue;
        }

        for (ULONG idx = 0; idx < sample->CapturedFramesCount; ++idx) {
            ULONG64 candidateRip = sample->StackWords[idx];
            if (candidateRip < 0xFFFF800000000000ULL) {
                continue;
            }

            ULONG64 imageBase = 0;
            if (VgkIsAddressBackedByModule(Modules, candidateRip, &imageBase)) {
                DWORD64 unwindBase = 0;
                PRUNTIME_FUNCTION fnEntry = RtlLookupFunctionEntry(candidateRip, &unwindBase, NULL);
                if (fnEntry != NULL) {
                    ULONG rva = (ULONG)(candidateRip - unwindBase);
                    /*
                     * Detect synthetic return address spoofing (e.g. pushing +0x0 of an API):
                     * A real CALL instruction pushes the address *after* the 5/6-byte call,
                     * never the exact function entry point (BeginAddress).
                     */
                    if (rva == fnEntry->BeginAddress) {
                        InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_KSS_SPOOFED_FRAME);
                        VgkLogTelemetryEvent(VGK_EVENT_KSS, cpu, NULL, cpu);
                    }
                }
            }
        }
    }
}

/* =========================================================================
 * KERNEL APC THREAD STACK PROBE (KeInitializeApc / KeInsertQueueApc)
 * Imported directly in C:\kgv.sys Import Table
 * ========================================================================= */

static VOID
NTAPI
VgkApcRundownRoutine(
    _In_ PKAPC Apc
)
{
    ExFreePoolWithTag(Apc, VGK_POOL_TAG);
}

static VOID
NTAPI
VgkApcKernelRoutine(
    _In_ PKAPC Apc,
    _Inout_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_ PVOID *NormalContext,
    _Inout_ PVOID *SystemArgument1,
    _Inout_ PVOID *SystemArgument2
)
{
    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    /*
     * Executes in the target thread's context at APC_LEVEL.
     * Captures the thread's kernel stack frames and frees the APC object.
     */
    PVOID backtrace[16] = { 0 };
    USHORT captured = RtlCaptureStackBackTrace(1, 16, backtrace, NULL);

    for (USHORT i = 0; i < captured; ++i) {
        ULONG64 frameRip = (ULONG64)backtrace[i];
        if (frameRip >= 0xFFFF800000000000ULL) {
            DWORD64 modBase = 0;
            PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(frameRip, &modBase, NULL);
            if (fn == NULL && modBase == 0) {
                /* Unbacked executable kernel frame without .pdata entry */
                InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_KSS_UNBACKED_RIP);
            }
        }
    }

    ExFreePoolWithTag(Apc, VGK_POOL_TAG);
}

NTSTATUS
VgkProbeThreadViaKernelApc(
    _In_ PETHREAD TargetThread
)
{
    if (!TargetThread || g_Vgk.Unloading) {
        return STATUS_INVALID_PARAMETER;
    }

    PKAPC apc = (PKAPC)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(KAPC), VGK_POOL_TAG);
    if (!apc) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeApc(
        apc,
        (PRKTHREAD)TargetThread,
        OriginalApcEnvironment,
        VgkApcKernelRoutine,
        VgkApcRundownRoutine,
        NULL,
        KernelMode,
        NULL
    );

    if (!KeInsertQueueApc(apc, NULL, NULL, IO_NO_INCREMENT)) {
        ExFreePoolWithTag(apc, VGK_POOL_TAG);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
