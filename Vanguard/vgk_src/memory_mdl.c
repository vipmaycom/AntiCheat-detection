/*
 * memory_mdl.c
 * Riot Vanguard (vgk.sys v1.19.1.18) Guarded Region MDL Engine, CR3 PTE Scanner,
 * Section #2 ("        " / $$PUKE @ RVA 0x157000) PTE Cloak & CR4.PGE Global TLB Flush,
 * 8 Atomic Ref-Counted Hook Trampolines (RVA 0x155AD0..0x155D9F), and Win32k Auditor.
 *
 * Reconstructed via REA (Ghidra 12.1.4) from C:\Program Files\Riot Vanguard\vgk.sys:
 *   - RVA 0x157000: VgkPukeCloakPages (ev_a75091e3e4d504b6ef69dce331014d2e0ddc1017be476a79167d733fac68288b)
 *   - RVA 0x15707E: VgkPukeSpinWait
 *   - RVA 0x0CEEE4: VgkHookPreHandler (ev_10663caca1596273c2f31ddafc04064cf5219af2dd65e4f018db8acba8f37a54)
 *   - RVA 0x155AD0..0x155D9F: 8 atomic ref-counted hook trampolines (refcount @ RVA 0x18FF08)
 */

#include "vgk.h"

/* =========================================================================
 * SECTION #2 ("        " / $$PUKE @ RVA 0x157000) PTE CLOAK & CR4.PGE FLUSH
 * Exact instruction sequence decompiled via REA analyze_function:
 *   1. Clear Present bit (& ~1ULL) across PukeCtx->PteArray[0..PteCount-1]
 *   2. Toggle CR4.PGE (Bit 7 = 0x80) off and on to flush global TLB entries
 *   3. Serialize instruction stream with CPUID (EAX=1)
 *   4. Set bit 15 on PukeCtx->StateFlags (lock bts dword ptr [rbp], 0Fh)
 *   5. Restore Present bit (| 1ULL) across PukeCtx->PteArray[0..PteCount-1]
 *   6. Toggle CR4.PGE (Bit 7 = 0x80) off and on again
 *   7. Clear bit 0 on PukeCtx->StateFlags (lock btr dword ptr [rbp], 0)
 * ========================================================================= */

#pragma code_seg(push, ".puke")

VOID
VgkPukeCloakPages(
    _Inout_ PVGK_PUKE_CONTEXT PukeCtx
)
{
    if (!PukeCtx) {
        return;
    }

    ULONG count = PukeCtx->PteCount;
    for (ULONG i = 0; i < count && i < 64; ++i) {
        PULONG64 ptePtr = PukeCtx->PteArray[i];
        if (ptePtr != NULL && MmIsAddressValid(ptePtr)) {
            *ptePtr &= ~1ULL; /* Clear PTE Present bit (0x140157011: and qword ptr [rdx], ~1) */
        }
    }

    /* Toggle CR4.PGE (Bit 7 = 0x80) to invalidate global TLB entries (0x14015701F..0x140157031) */
    ULONG64 cr4 = __readcr4();
    __writecr4(cr4 & ~0x80ULL);
    __writecr4(cr4 | 0x80ULL);

    /* Serialize execution via CPUID leaf 1 (0x140157034..0x14015703B) */
    int cpuidRegs[4] = { 0 };
    __cpuid(cpuidRegs, 1);

    /* Signal cloak barrier completion: lock bts dword ptr [rbp], 0xF (0x14015703D) */
    _interlockedbittestandset(&PukeCtx->StateFlags, 15);

    /* Restore Present bit on all target PTEs (0x140157043..0x14015705C) */
    count = PukeCtx->PteCount;
    for (ULONG i = 0; i < count && i < 64; ++i) {
        PULONG64 ptePtr = PukeCtx->PteArray[i];
        if (ptePtr != NULL && MmIsAddressValid(ptePtr)) {
            *ptePtr |= 1ULL; /* Set PTE Present bit (0x140157052: or qword ptr [rdx], 1) */
        }
    }

    /* Flush global TLB entries again (0x14015705E..0x140157070) */
    cr4 = __readcr4();
    __writecr4(cr4 & ~0x80ULL);
    __writecr4(cr4 | 0x80ULL);

    /* Release active lock bit 0: lock btr dword ptr [rbp], 0 (0x140157076) */
    _interlockedbittestandreset(&PukeCtx->StateFlags, 0);
}

/*
 * Reconstructed RVA 0x15707E: VgkPukeSpinWait
 *   0x15707E: bt dword ptr [rcx], 0
 *   0x157082: jnb +0x04
 *   0x157084: pause
 *   0x157086: jmp -0x0A
 *   0x157088: ret
 */
VOID
VgkPukeSpinWait(
    _In_ volatile LONG *LockWord
)
{
    if (!LockWord) {
        return;
    }
    while ((_InterlockedCompareExchange(LockWord, 0, 0) & 1) != 0) {
        _mm_pause();
    }
}

#pragma code_seg(pop)

/* =========================================================================
 * NEW IN v1.19.1.18: ATOMIC REF-COUNTED HOOK PRE-HANDLER & TRAMPOLINES
 * Reconstructed via REA from FUN_1400ceee4 and RVA 0x155AD0..0x155D9F:
 *   lock inc qword ptr [rip + g_Vgk.ActiveHookRefCount] ; RVA 0x18FF08
 *   call VgkHookPreHandler                              ; RVA 0x0CEEE4
 *   lock dec qword ptr [rip + g_Vgk.ActiveHookRefCount] ; RVA 0x18FF08
 * ========================================================================= */

static BOOLEAN
VgkValidateCallerAccessForSyscall(
    _In_ ULONG64 TargetHandleOrObject,
    _In_ ULONG AccessMask,
    _In_ ULONG64 CallerContext1,
    _In_ ULONG64 CallerContext2
)
{
    UNREFERENCED_PARAMETER(TargetHandleOrObject);
    UNREFERENCED_PARAMETER(AccessMask);
    UNREFERENCED_PARAMETER(CallerContext1);
    UNREFERENCED_PARAMETER(CallerContext2);

    HANDLE callerPid = PsGetCurrentProcessId();
    if (g_Vgk.ValorantProcessId != NULL && callerPid != g_Vgk.ValorantProcessId && callerPid != g_Vgk.VgcProcessId) {
        return TRUE; /* Flag unauthorized cross-process probe */
    }
    return FALSE;
}

/*
 * Decompiled via REA procedure_pseudo_code on FUN_1400ceee4 (RVA 0x0CEEE4):
 * Filters hook tags 0x2B1, 0x2B2, 0x2B3, 0x2B7, and 0x2B8 (with ProbeForWrite on param_5).
 */
BOOLEAN
VgkHookPreHandler(
    _In_ ULONG64 Arg1,
    _In_ ULONG Arg2,
    _In_ ULONG64 Arg3,
    _In_ ULONG64 Arg4,
    _Out_opt_ PULONG64 OutResult,
    _In_ ULONG SyscallTag
)
{
    InterlockedIncrement64(&g_Vgk.ActiveHookRefCount);

    BOOLEAN allowCall = TRUE;

    if (SyscallTag == 0x2B1 || SyscallTag == 0x2B2 || SyscallTag == 0x2B3 || SyscallTag == 0x2B7) {
        BOOLEAN blocked = VgkValidateCallerAccessForSyscall(Arg1, Arg2, Arg3, Arg4);
        allowCall = !blocked;
    } else if (SyscallTag == 0x2B8) {
        if (VgkValidateCallerAccessForSyscall(Arg1, Arg2, Arg3, Arg4)) {
            if (OutResult != NULL) {
                __try {
                    if (ExGetPreviousMode() != KernelMode) {
                        ProbeForWrite(OutResult, 0x10, 4);
                    }
                    *OutResult = 0;
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    /* Swallow user-mode fault */
                }
            }
            allowCall = FALSE;
        }
    }

    InterlockedDecrement64(&g_Vgk.ActiveHookRefCount);
    return allowCall;
}

/*
 * Exact 8 Atomic Ref-Counted Hook Trampolines from .text RVA 0x155AD0..0x155D9F:
 *   - Trampoline 0 (RVA 0x155AD0): 4 args -> PreHandler(0x2B1) -> OriginalHookTargets[0] (0x1985B8)
 *   - Trampoline 1 (RVA 0x155B20): 4 args -> PreHandler(0x2B2) -> OriginalHookTargets[1] (0x1985C0)
 *   - Trampoline 2 (RVA 0x155B70): 7 args -> PreHandler(0x2B3) -> OriginalHookTargets[2] (0x1985C8)
 *   - Trampoline 3 (RVA 0x155BE0): 6 args -> PreHandler(0x2B8) -> OriginalHookTargets[3] (0x1985E8)
 *   - Trampoline 4 (RVA 0x155C50): 4 args -> PreHandler(0x2B4) -> OriginalHookTargets[4] (0x1985E0)
 *   - Trampoline 5 (RVA 0x155CB0): 4 args -> PreHandler(0x2B5) -> OriginalHookTargets[5] (0x1985E8)
 *   - Trampoline 6 (RVA 0x155D00): 4 args -> PreHandler(0x2B6) -> OriginalHookTargets[6] (0x1985F0)
 *   - Trampoline 7 (RVA 0x155D50): 4 args -> PreHandler(0x2B7) -> OriginalHookTargets[7] (0x1985F8)
 */
typedef NTSTATUS (NTAPI *PFN_HOOK_4ARGS)(ULONG64, ULONG, ULONG64, ULONG64);
typedef NTSTATUS (NTAPI *PFN_HOOK_6ARGS)(ULONG64, ULONG, ULONG64, ULONG64, ULONG64, ULONG64);
typedef NTSTATUS (NTAPI *PFN_HOOK_7ARGS)(ULONG64, ULONG, ULONG64, ULONG64, ULONG64, ULONG64, ULONG64);

NTSTATUS NTAPI
VgkHookTrampoline0(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9)
{
    if (!VgkHookPreHandler(Rcx, Rdx, R8, R9, NULL, 0x2B1)) {
        return STATUS_ACCESS_DENIED;
    }
    PFN_HOOK_4ARGS orig = (PFN_HOOK_4ARGS)g_Vgk.OriginalHookTargets[0];
    return orig ? orig(Rcx, Rdx, R8, R9) : STATUS_SUCCESS;
}

NTSTATUS NTAPI
VgkHookTrampoline1(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9)
{
    if (!VgkHookPreHandler(Rcx, Rdx, R8, R9, NULL, 0x2B2)) {
        return STATUS_ACCESS_DENIED;
    }
    PFN_HOOK_4ARGS orig = (PFN_HOOK_4ARGS)g_Vgk.OriginalHookTargets[1];
    return orig ? orig(Rcx, Rdx, R8, R9) : STATUS_SUCCESS;
}

NTSTATUS NTAPI
VgkHookTrampoline2(
    _In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9,
    _In_ ULONG64 Arg5, _In_ ULONG64 Arg6, _In_ ULONG64 Arg7
)
{
    if (!VgkHookPreHandler(Rcx, Rdx, R8, R9, (PULONG64)Arg5, 0x2B3)) {
        return STATUS_ACCESS_DENIED;
    }
    PFN_HOOK_7ARGS orig = (PFN_HOOK_7ARGS)g_Vgk.OriginalHookTargets[2];
    return orig ? orig(Rcx, Rdx, R8, R9, Arg5, Arg6, Arg7) : STATUS_SUCCESS;
}

NTSTATUS NTAPI
VgkHookTrampoline3(
    _In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9,
    _In_ ULONG64 Arg5, _In_ ULONG64 Arg6
)
{
    if (!VgkHookPreHandler(Rcx, Rdx, R8, R9, (PULONG64)Arg5, 0x2B8)) {
        return STATUS_ACCESS_DENIED;
    }
    PFN_HOOK_6ARGS orig = (PFN_HOOK_6ARGS)g_Vgk.OriginalHookTargets[3];
    return orig ? orig(Rcx, Rdx, R8, R9, Arg5, Arg6) : STATUS_SUCCESS;
}

NTSTATUS NTAPI
VgkHookTrampoline4(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9)
{
    if (!VgkHookPreHandler(Rcx, Rdx, R8, R9, NULL, 0x2B4)) {
        return STATUS_ACCESS_DENIED;
    }
    PFN_HOOK_4ARGS orig = (PFN_HOOK_4ARGS)g_Vgk.OriginalHookTargets[4];
    return orig ? orig(Rcx, Rdx, R8, R9) : STATUS_SUCCESS;
}

NTSTATUS NTAPI
VgkHookTrampoline5(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9)
{
    if (!VgkHookPreHandler(Rcx, Rdx, R8, R9, NULL, 0x2B5)) {
        return STATUS_ACCESS_DENIED;
    }
    PFN_HOOK_4ARGS orig = (PFN_HOOK_4ARGS)g_Vgk.OriginalHookTargets[5];
    return orig ? orig(Rcx, Rdx, R8, R9) : STATUS_SUCCESS;
}

NTSTATUS NTAPI
VgkHookTrampoline6(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9)
{
    if (!VgkHookPreHandler(Rcx, Rdx, R8, R9, NULL, 0x2B6)) {
        return STATUS_ACCESS_DENIED;
    }
    PFN_HOOK_4ARGS orig = (PFN_HOOK_4ARGS)g_Vgk.OriginalHookTargets[6];
    return orig ? orig(Rcx, Rdx, R8, R9) : STATUS_SUCCESS;
}

NTSTATUS NTAPI
VgkHookTrampoline7(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9)
{
    if (!VgkHookPreHandler(Rcx, Rdx, R8, R9, NULL, 0x2B7)) {
        return STATUS_ACCESS_DENIED;
    }
    PFN_HOOK_4ARGS orig = (PFN_HOOK_4ARGS)g_Vgk.OriginalHookTargets[7];
    return orig ? orig(Rcx, Rdx, R8, R9) : STATUS_SUCCESS;
}

/* =========================================================================
 * GUARDED REGION PHYSICAL MDL ALLOCATION & MAPPING
 * ========================================================================= */

NTSTATUS
VgkInitializeGuardedRegionMdl(VOID)
{
    PHYSICAL_ADDRESS lowAddress  = { 0 };
    PHYSICAL_ADDRESS highAddress = { 0 };
    PHYSICAL_ADDRESS skipBytes   = { 0 };

    highAddress.QuadPart = ~0ULL;

    PMDL mdl = MmAllocatePagesForMdl(
        lowAddress,
        highAddress,
        skipBytes,
        VGK_GUARDED_REGION_SIZE
    );
    if (!mdl) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PVOID kernelVa = MmMapLockedPagesSpecifyCache(
        mdl,
        KernelMode,
        MmCached,
        NULL,
        FALSE,
        NormalPagePriority
    );
    if (!kernelVa) {
        MmFreePagesFromMdl(mdl);
        IoFreeMdl(mdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(kernelVa, VGK_GUARDED_REGION_SIZE);

    /* Stamp Vanguard Guarded Region header magic at +0x0 */
    PULONG64 headerWords = (PULONG64)kernelVa;
    headerWords[0] = 0x56474B4755415244ULL; /* "VGKGUARD" */
    headerWords[1] = g_Vgk.ApiXorKey;

    g_Vgk.GuardedRegionMdl      = mdl;
    g_Vgk.GuardedRegionKernelVa = kernelVa;
    return STATUS_SUCCESS;
}

NTSTATUS
VgkMapGuardedRegionIntoProcess(
    _Out_ PVOID *OutUserAddress
)
{
    if (OutUserAddress) {
        *OutUserAddress = NULL;
    }
    if (!g_Vgk.GuardedRegionMdl) {
        return STATUS_DEVICE_NOT_READY;
    }

    PVOID userVa = NULL;
    __try {
        userVa = MmMapLockedPagesSpecifyCache(
            g_Vgk.GuardedRegionMdl,
            UserMode,
            MmCached,
            NULL,
            FALSE,
            NormalPagePriority
        );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        userVa = NULL;
    }

    if (!userVa) {
        return STATUS_UNSUCCESSFUL;
    }

    g_Vgk.GuardedRegionUserVa = userVa;
    if (OutUserAddress) {
        *OutUserAddress = userVa;
    }
    return STATUS_SUCCESS;
}

VOID
VgkFreeGuardedRegionMdl(VOID)
{
    if (g_Vgk.GuardedRegionKernelVa && g_Vgk.GuardedRegionMdl) {
        MmUnmapLockedPages(g_Vgk.GuardedRegionKernelVa, g_Vgk.GuardedRegionMdl);
        g_Vgk.GuardedRegionKernelVa = NULL;
    }

    if (g_Vgk.GuardedRegionMdl) {
        MmFreePagesFromMdl(g_Vgk.GuardedRegionMdl);
        IoFreeMdl(g_Vgk.GuardedRegionMdl);
        g_Vgk.GuardedRegionMdl = NULL;
    }
}

/* =========================================================================
 * CR3 / PAGE TABLE ENTRY (PTE) NX=0 UNBACKED EXECUTABLE MEMORY SCANNER
 * Emits "MC %08x\n" (RVA 0x159D18) when unbacked executable pool is found
 * ========================================================================= */

VOID
VgkSweepKernelPageTables(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (!Modules) {
        return;
    }

    ULONG64 cr3Phys = __readcr3() & 0x000FFFFFFFFFF000ULL;
    PHYSICAL_ADDRESS pml4Phys;
    pml4Phys.QuadPart = (LONGLONG)cr3Phys;

    PHARDWARE_PTE_X64 pml4Va = (PHARDWARE_PTE_X64)MmGetVirtualForPhysical(pml4Phys);
    if (!pml4Va || !MmIsAddressValid(pml4Va)) {
        return;
    }

    /* Sweep 4 kernel-half PML4 entries per pass (indices 256..511) */
    for (ULONG step = 0; step < 4; ++step) {
        ULONG pml4Idx = 256 + ((g_Vgk.Pml4SweepCursor + step) % 256);
        HARDWARE_PTE_X64 pml4e = pml4Va[pml4Idx];

        if (!pml4e.Bits.Present || pml4e.Bits.NoExecute) {
            continue;
        }

        PHYSICAL_ADDRESS pdptPhys;
        pdptPhys.QuadPart = (LONGLONG)(pml4e.Bits.PageFrameNumber << 12);
        PHARDWARE_PTE_X64 pdptVa = (PHARDWARE_PTE_X64)MmGetVirtualForPhysical(pdptPhys);
        if (!pdptVa || !MmIsAddressValid(pdptVa)) {
            continue;
        }

        for (ULONG pdptIdx = 0; pdptIdx < 512; ++pdptIdx) {
            HARDWARE_PTE_X64 pdpte = pdptVa[pdptIdx];
            if (!pdpte.Bits.Present || pdpte.Bits.NoExecute || pdpte.Bits.LargePage) {
                continue;
            }

            PHYSICAL_ADDRESS pdPhys;
            pdPhys.QuadPart = (LONGLONG)(pdpte.Bits.PageFrameNumber << 12);
            PHARDWARE_PTE_X64 pdVa = (PHARDWARE_PTE_X64)MmGetVirtualForPhysical(pdPhys);
            if (!pdVa || !MmIsAddressValid(pdVa)) {
                continue;
            }

            for (ULONG pdIdx = 0; pdIdx < 32; ++pdIdx) {
                HARDWARE_PTE_X64 pde = pdVa[pdIdx];
                if (!pde.Bits.Present || pde.Bits.NoExecute) {
                    continue;
                }

                if (pde.Bits.LargePage) {
                    ULONG64 va = 0xFFFF000000000000ULL
                               | ((ULONG64)pml4Idx << 39)
                               | ((ULONG64)pdptIdx << 30)
                               | ((ULONG64)pdIdx   << 21);

                    if (MmIsAddressValid((PVOID)va)) {
                        PUCHAR pageBytes = (PUCHAR)va;
                        if (pageBytes[0] == 'M' && pageBytes[1] == 'Z') {
                            if (!VgkIsAddressBackedByModule(Modules, va, NULL)) {
                                InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_MC_PTE_UNBACKED);
                                VgkLogTelemetryEvent(VGK_EVENT_MC, (ULONG)pml4Idx, NULL, (ULONG)pml4Idx);
                            }
                        }
                    }
                }
            }
        }
    }

    g_Vgk.Pml4SweepCursor = (g_Vgk.Pml4SweepCursor + 4) % 256;
}

/* =========================================================================
 * WIN32K ("win32kbase.sys" / "win32kfull.sys") & MOUSE STACK AUDITOR
 * Reconstructed from RVA 0x156DB0..0x156DC0 & "EDCL %u\n" (RVA 0x159D60)
 * ========================================================================= */

static VOID
VgkInspectDriverDispatchAndFilters(
    _In_ PCWSTR DriverObjectPath,
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    UNICODE_STRING drvName;
    RtlInitUnicodeString(&drvName, DriverObjectPath);

    PDRIVER_OBJECT drvObj = NULL;
    NTSTATUS status = ObReferenceObjectByName(
        &drvName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        0,
        *IoDriverObjectType,
        KernelMode,
        NULL,
        (PVOID*)&drvObj
    );
    if (!NT_SUCCESS(status) || !drvObj) {
        return;
    }

    /* 1. Verify IRP_MJ_* dispatch pointers reside inside legitimate loaded modules */
    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i) {
        ULONG64 fn = (ULONG64)drvObj->MajorFunction[i];
        if (fn != 0 && !VgkIsAddressBackedByModule(Modules, fn, NULL)) {
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_EDCL_FILTER_FOUND);
            VgkLogTelemetryEvent(VGK_EVENT_EDCL, i, NULL, i);
        }
    }

    /* 2. Walk AttachedDevice chain to flag unbacked filter drivers on input stack */
    PDEVICE_OBJECT devObj = drvObj->DeviceObject;
    while (devObj != NULL) {
        PDEVICE_OBJECT attached = devObj->AttachedDevice;
        while (attached != NULL) {
            if (attached->DriverObject != NULL) {
                ULONG64 startVa = (ULONG64)attached->DriverObject->DriverStart;
                if (startVa == 0 || !VgkIsAddressBackedByModule(Modules, startVa, NULL)) {
                    InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_EDCL_FILTER_FOUND);
                    VgkLogTelemetryEvent(VGK_EVENT_EDCL, 0xFF, NULL, 0xFF);
                }
            }
            attached = attached->AttachedDevice;
        }
        devObj = devObj->NextDevice;
    }

    ObDereferenceObject(drvObj);
}

VOID
VgkVerifyWin32kAndMouseStack(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (!Modules) {
        return;
    }

    /* Verify win32kbase.sys (RVA 0x156DB0) and win32kfull.sys (RVA 0x156DC0) presence in module list */
    for (ULONG i = 0; i < Modules->NumberOfModules; ++i) {
        const CHAR* fileName = (const CHAR*)&Modules->Modules[i].FullPathName[Modules->Modules[i].OffsetToFileName];
        if (_stricmp(fileName, VGK_WIN32KBASE_SYS_A) == 0 ||
            _stricmp(fileName, VGK_WIN32KFULL_SYS_A) == 0) {
            ULONG64 base = (ULONG64)Modules->Modules[i].ImageBase;
            if (base == 0 || Modules->Modules[i].ImageSize == 0) {
                InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_MC_HASH_MISMATCH);
            }
        }
    }

    /* Audit Mouse Class and HID stacks */
    VgkInspectDriverDispatchAndFilters(L"\\Driver\\MouClass", Modules);
    VgkInspectDriverDispatchAndFilters(L"\\Driver\\MouHid",   Modules);
}
