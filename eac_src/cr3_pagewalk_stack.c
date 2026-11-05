/*
 * cr3_pagewalk_stack.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) Deobfuscated CR3 Page Table Walker
 * and Custom Kernel Stack Unwinder.
 *
 * Exact Reconstructions from .text (ImageBase 0x140000000):
 *   1. FUN_14004a85a (0x14004a85a..0x14004a921):
 *      Decrypts [0x140307e90] -> ntoskrnl!MmCopyMemory and reads 8 bytes with
 *      Flags = MM_COPY_MEMORY_PHYSICAL (1).
 *   2. FUN_14004a922 (0x14004a922..0x14004a990):
 *      Validates NT_SUCCESS(status) && BytesTransferred == 8.
 *   3. FUN_1400d9e46 (0x1400d9e46..0x1400da430, Solved MBA Key r14 = 0xA1BBB7A6F3A55BF7):
 *      4-Level CR3 Kernel Page Table Walker (PML4 -> PDPT -> PD -> PT), supporting
 *      2MB LargePages and 4KB PTEs, aggregating effective NX (OR) and RW (AND) bits.
 *   4. FUN_1400d97f3 (0x1400d97f3..0x1400d9e20, Solved MBA Key rsi = 0x7C25BF06AA521F89):
 *      Custom Kernel Stack Unwinder using 0x4D0-byte CONTEXT (0x0010000B),
 *      RtlLookupFunctionEntry + RtlVirtualUnwind, and raw [RSP] leaf/unbacked pop
 *      when FunctionEntry == NULL (catching KDMapper pool execution).
 *   5. FUN_1401e828c (0x1401e828c..0x1401e85bf) & 0x140147ddb:
 *      Control Register Reader (CR2, CR3, CR4, CR8) & CR3 PFN extraction.
 */

#include "eac.h"

typedef NTSTATUS (NTAPI *PFN_EAC_MM_COPY_MEMORY)(
    _In_ PVOID TargetAddress,
    _In_ MM_COPY_ADDRESS SourceAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Flags,
    _Out_ PSIZE_T NumberOfBytesTransferred
);

/* =========================================================================
 * 1. DEOBFUSCATED FUN_14004a85a & FUN_14004a922 (File Offset 0x49C5A)
 * Physical 8-byte reader via ntoskrnl!MmCopyMemory(MM_COPY_MEMORY_PHYSICAL)
 * ========================================================================= */

NTSTATUS
__fastcall
EacMmCopyPhysical_14004a85a(
    _In_  ULONG64 MbaSeed,
    _Out_ PSIZE_T OutBytesTransferred,
    _In_  ULONG64 PhysicalAddress,
    _Out_writes_bytes_(8) PVOID TargetBuffer
)
{
    if (OutBytesTransferred) {
        *OutBytesTransferred = 0;
    }

    /* 0x14004a8ca: test r8, r8 / sete cl / test rdx, rdx / sete dl / or dl, cl / jne exit */
    if (PhysicalAddress == 0ULL || OutBytesTransferred == NULL || TargetBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * 0x14004a883: Decrypt [0x140307e90] -> ntoskrnl!MmCopyMemory
     * 0x14004a88b: Size = ((r10 * 0xe537d1a520eb3866) ^ 0x7775c7ad2c206e11) == 8
     * 0x14004a8a5: Flags = ((r10d * 0xbb90f955) ^ 0x125859ba) == 1 (MM_COPY_MEMORY_PHYSICAL)
     */
    PFN_EAC_MM_COPY_MEMORY fnMmCopyMemory = (PFN_EAC_MM_COPY_MEMORY)EacResolveMbaApi(
        EAC_API_MM_COPY_MEMORY,
        MbaSeed
    );
    if (fnMmCopyMemory == NULL) {
        fnMmCopyMemory = MmCopyMemory;
    }

    MM_COPY_ADDRESS srcPhys;
    srcPhys.PhysicalAddress.QuadPart = (LONGLONG)PhysicalAddress;

    return fnMmCopyMemory(
        TargetBuffer,
        srcPhys,
        sizeof(ULONG64),
        MM_COPY_MEMORY_PHYSICAL,
        OutBytesTransferred
    );
}

BOOLEAN
__fastcall
EacReadPhysicalQword_14004a922(
    _In_  ULONG64 PhysicalAddress,
    _Out_ PULONG64 OutValue,
    _In_  ULONG64 MbaSeed
)
{
    if (OutValue) {
        *OutValue = 0ULL;
    }
    if (!OutValue || PhysicalAddress == 0ULL) {
        return FALSE;
    }

    SIZE_T bytesCopied = 0;
    ULONG64 tempVal = 0ULL;
    NTSTATUS status = EacMmCopyPhysical_14004a85a(
        MbaSeed,
        &bytesCopied,
        PhysicalAddress,
        &tempVal
    );
    if (!NT_SUCCESS(status) || bytesCopied != sizeof(ULONG64)) {
        return FALSE;
    }

    *OutValue = tempVal;
    return TRUE;
}

/* =========================================================================
 * 2. DEOBFUSCATED FUN_1400d9e46 (0x1400d9e46 .. 0x1400da430)
 * Solved 64-bit MBA Key: r14 = 0xA1BBB7A6F3A55BF7ULL
 * 4-Level CR3 Kernel Page Table Walker (PML4 -> PDPT -> PD -> PT)
 * ========================================================================= */

BOOLEAN
__fastcall
EacWalkKernelPageTable_1400d9e46(
    _In_      ULONG64  TargetVa,             /* rcx */
    _Out_opt_ PULONG64 OutEffectivePteFlags, /* rdx */
    _In_      ULONG64  MbaSeed,              /* r8  */
    _Out_opt_ PULONG64 OutPhysicalAddress    /* r9  */
)
{
    if (OutEffectivePteFlags) {
        *OutEffectivePteFlags = 0ULL;
    }
    if (OutPhysicalAddress) {
        *OutPhysicalAddress = 0ULL;
    }

    /*
     * 0x1400d9ecb..0x1400d9ef6:
     * Verify x64 canonical address: sar rax, 48; add rax, -1; cmp rax, -2
     */
    LONGLONG canonicalSign = ((LONGLONG)TargetVa >> 48) - 1;
    if ((ULONG64)canonicalSign < 0xFFFFFFFFFFFFFFFEULL) {
        return FALSE;
    }

    /*
     * 0x1400d9fdd: cmp rbx, qword ptr [rcx]; jb 0x1400da41b
     * Rejects user-mode addresses below MmUserProbeAddress (0x00007FFFFFFF0000).
     */
    if (TargetVa < (ULONG64)MmUserProbeAddress) {
        return FALSE;
    }

    /*
     * 0x1400d9ff5: mov rax, cr3
     * Level 1: PML4E = PhysRead((CR3 & 0x0000FFFFFFFFF000) + ((TargetVa >> 36) & 0xFF8))
     */
    ULONG64 cr3 = __readcr3();
    ULONG64 pml4ePhys = (cr3 & EAC_PTE_PFN_MASK) + ((TargetVa >> 36) & EAC_PTE_INDEX_MASK);
    ULONG64 pml4e = 0ULL;
    if (!EacReadPhysicalQword_14004a922(pml4ePhys, &pml4e, MbaSeed) ||
        !(pml4e & EAC_PTE_PRESENT_BIT)) {
        return FALSE;
    }

    /*
     * 0x1400da0d8..0x1400da154:
     * Level 2: PDPTE = PhysRead((PML4E & 0x0000FFFFFFFFF000) + ((TargetVa >> 27) & 0xFF8))
     * Checks (PDPTE & 0x81) == 1 (Present == 1 && 1GB LargePage == 0).
     */
    ULONG64 pdptePhys = (pml4e & EAC_PTE_PFN_MASK) + ((TargetVa >> 27) & EAC_PTE_INDEX_MASK);
    ULONG64 pdpte = 0ULL;
    if (!EacReadPhysicalQword_14004a922(pdptePhys, &pdpte, MbaSeed)) {
        return FALSE;
    }
    if ((pdpte & EAC_PTE_PRESENT_OR_LARGE) != EAC_PTE_PRESENT_BIT) {
        return FALSE;
    }

    /*
     * 0x1400da15a..0x1400da1e1:
     * Level 3: PDE = PhysRead((PDPTE & 0x0000FFFFFFFFF000) + ((TargetVa >> 18) & 0xFF8))
     */
    ULONG64 pdePhys = (pdpte & EAC_PTE_PFN_MASK) + ((TargetVa >> 18) & EAC_PTE_INDEX_MASK);
    ULONG64 pde = 0ULL;
    if (!EacReadPhysicalQword_14004a922(pdePhys, &pde, MbaSeed) ||
        !(pde & EAC_PTE_PRESENT_BIT)) {
        return FALSE;
    }

    /*
     * 0x1400da1e1..0x1400da30b:
     * Branch A: 2MB LargePage (PDE & 0x80 != 0)
     */
    if (pde & EAC_PTE_LARGE_PAGE_BIT) {
        if (OutPhysicalAddress) {
            *OutPhysicalAddress = (pde & 0x0000FFFFFFE00000ULL) + (TargetVa & 0x1FFFFFULL);
        }
        if (OutEffectivePteFlags) {
            ULONG64 effectiveNx = (pml4e | pdpte | pde) & EAC_PTE_NX_BIT;
            ULONG64 effectiveRw = (pml4e & pdpte & pde) & EAC_PTE_RW_BIT;
            *OutEffectivePteFlags = effectiveNx | effectiveRw | ((pde & ~EAC_PTE_NX_BIT) & ~EAC_PTE_RW_BIT);
        }
        return TRUE;
    }

    /*
     * 0x1400da310..0x1400da416:
     * Branch B: Level 4 4KB PTE = PhysRead((PDE & 0x0000FFFFFFFFF000) + ((TargetVa >> 9) & 0xFF8))
     */
    ULONG64 ptePhys = (pde & EAC_PTE_PFN_MASK) + ((TargetVa >> 9) & EAC_PTE_INDEX_MASK);
    ULONG64 pte = 0ULL;
    if (!EacReadPhysicalQword_14004a922(ptePhys, &pte, MbaSeed) ||
        !(pte & EAC_PTE_PRESENT_BIT)) {
        return FALSE;
    }

    if (OutPhysicalAddress) {
        *OutPhysicalAddress = (pte & EAC_PTE_PFN_MASK) + (TargetVa & 0xFFFULL);
    }

    /*
     * Hierarchical Permission Aggregation (0x1400da3d2..0x1400da416):
     *   - Effective NX (bit 63) = (PML4E | PDPTE | PDE | PTE) & (1ULL << 63)
     *   - Effective RW (bit 1)  = (PML4E & PDPTE & PDE & PTE) & 2ULL
     */
    if (OutEffectivePteFlags) {
        ULONG64 effectiveNx = (pml4e | pdpte | pde | pte) & EAC_PTE_NX_BIT;
        ULONG64 effectiveRw = (pml4e & pdpte & pde & pte) & EAC_PTE_RW_BIT;
        *OutEffectivePteFlags = effectiveNx | effectiveRw | ((pte & ~EAC_PTE_NX_BIT) & ~EAC_PTE_RW_BIT);
    }

    return TRUE;
}

/* =========================================================================
 * 3. DEOBFUSCATED FUN_1400d97f3 (0x1400d97f3 .. 0x1400d9e20)
 * Solved 64-bit MBA Key: rsi = 0x7C25BF06AA521F89ULL
 * Custom Kernel Stack Unwinder with Leaf / Unbacked [RSP] Fallback
 * ========================================================================= */

NTSTATUS
__fastcall
EacUnwindKernelStack_1400d97f3(
    _In_opt_ PCONTEXT InitialContext,
    _In_     PRTL_PROCESS_MODULES Modules,
    _In_     ULONG64 MbaSeed,
    _Out_    PEAC_UNWIND_RESULT OutResult
)
{
    if (!OutResult || !Modules) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(OutResult, sizeof(*OutResult));

    /*
     * 0x1400d98ef..0x1400d998d:
     * Allocates 0x4D0-byte CONTEXT record and initializes ContextFlags = 0x0010000B.
     */
    PCONTEXT ctx = (PCONTEXT)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(CONTEXT), EAC_POOL_TAG_SCAN);
    if (!ctx) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(ctx, sizeof(CONTEXT));
    if (InitialContext != NULL) {
        RtlCopyMemory(ctx, InitialContext, sizeof(CONTEXT));
        ctx->ContextFlags = EAC_CONTEXT_AMD64_FULL;
    } else {
        RtlCaptureContext(ctx);
        ctx->ContextFlags = EAC_CONTEXT_AMD64_FULL;
    }

    ULONG_PTR stackLow  = 0;
    ULONG_PTR stackHigh = 0;
    IoGetStackLimits(&stackLow, &stackHigh);

    /*
     * Frame Unwind Loop (0x1400d9b35 .. 0x1400d9cab)
     */
    for (ULONG frameIdx = 0; frameIdx < EAC_MAX_UNWIND_FRAMES; ++frameIdx) {
        ULONG64 rip = ctx->Rip;
        ULONG64 rsp = ctx->Rsp;

        if (rip < 0xFFFF800000000000ULL || rsp < stackLow || (rsp + sizeof(ULONG64)) > stackHigh) {
            break;
        }

        PEAC_UNWIND_FRAME rec = &OutResult->Frames[OutResult->FrameCount];
        rec->Rip = rip;
        rec->Rsp = rsp;

        DWORD64 unwindImageBase = 0;
        PRUNTIME_FUNCTION fnEntry = RtlLookupFunctionEntry(rip, &unwindImageBase, NULL);
        rec->HasRuntimeFunction = (fnEntry != NULL);

        ULONG64 modBase = 0;
        rec->IsBackedByModule = EacIsAddressBackedByModule(Modules, rip, &modBase, NULL);
        rec->ImageBase        = (modBase != 0) ? modBase : unwindImageBase;

        /*
         * Query EAC's 4-level CR3 Page Table Walker (FUN_1400d9e46) on every kernel RIP.
         * If the RIP is NOT backed by any module in PsLoadedModuleList:
         *   - If FUN_1400d9e46 succeeds and effective NX == 0 -> Unbacked Executable Kernel Code (KDMapper)
         *   - If FUN_1400d9e46 fails on an actively executing RIP -> MmCopyMemory Physical / PFN Tamper!
         */
        BOOLEAN walkOk = EacWalkKernelPageTable_1400d9e46(
            rip,
            &rec->EffectivePteFlags,
            MbaSeed,
            &rec->PhysicalAddress
        );

        if (!rec->IsBackedByModule) {
            if (walkOk && ((rec->EffectivePteFlags & EAC_PTE_NX_BIT) == 0ULL)) {
                rec->IsExecutableUnbacked = TRUE;
                OutResult->UnbackedExecCount++;
                InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_UNBACKED_KERNEL_RIP);
                EacEmitTelemetryChunk(
                    EAC_CHUNK_OP_ALERT,
                    2,
                    "kdmapper_unbacked_rip",
                    frameIdx,
                    (ULONG)(rip & 0xFFFFFFFFUL)
                );
            } else if (!walkOk) {
                /* Active kernel RIP on stack failed physical MmCopyMemory walk (PFN CacheAttribute / PT cloak) */
                InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_MMCOPY_PHYS_CONFLICT);
                EacEmitTelemetryChunk(
                    EAC_CHUNK_OP_ALERT,
                    2,
                    "mmcopymemory_pfn_conflict",
                    frameIdx,
                    (ULONG)(rip & 0xFFFFFFFFUL)
                );
            }
        }

        OutResult->FrameCount++;

        /*
         * 0x1400d9b87..0x1400d9c6a:
         * If FunctionEntry != NULL, unwind via RtlVirtualUnwind.
         * If FunctionEntry == NULL (0x1400d9ba6: leaf function or unbacked manual-mapped driver),
         * pop return address from [RSP] and advance RSP += 8 ([rsp+0x78] = 8).
         */
        if (fnEntry != NULL) {
            PVOID   handlerData      = NULL;
            DWORD64 establisherFrame = 0;
            RtlVirtualUnwind(
                UNW_FLAG_NHANDLER,
                unwindImageBase,
                rip,
                fnEntry,
                ctx,
                &handlerData,
                &establisherFrame,
                NULL
            );
        } else {
            if (!MmIsAddressValid((PVOID)rsp)) {
                break;
            }
            ctx->Rip = *(PULONG64)rsp;
            ctx->Rsp = rsp + sizeof(ULONG64);
        }

        if (ctx->Rip == 0ULL || ctx->Rip == rip) {
            break;
        }
    }

    ExFreePoolWithTag(ctx, EAC_POOL_TAG_SCAN);
    return STATUS_SUCCESS;
}

/* =========================================================================
 * 4. DEOBFUSCATED FUN_1401e828c & 0x140147ddb
 * Control Register Snapshot (CR2, CR3, CR4, CR8) & CR3 PFN Extractor
 * ========================================================================= */

VOID
__fastcall
EacReadControlRegisters_1401e828c(
    _Out_ PEAC_CONTROL_REGISTERS OutRegs
)
{
    if (!OutRegs) {
        return;
    }

    OutRegs->Cr2    = __readcr2();                                  /* 0x1401e85a6: mov rax, cr2 */
    OutRegs->Cr3    = __readcr3();                                  /* 0x1401e859e: mov rax, cr3 */
    OutRegs->Cr4    = __readcr4();                                  /* 0x1401e85ae: mov rax, cr4 */
    OutRegs->Cr8    = __readcr8();                                  /* 0x1401e85b6: mov rax, cr8 */
    OutRegs->Cr3Pfn = (OutRegs->Cr3 >> 12) & 0xFFFFFFFFFULL;        /* 0x140147ddb: shr rcx, 0Ch; and rcx, rdx */
}

/* =========================================================================
 * 4.1 HARDWARE TLB FLUSH & CR3 DIRECTORY TABLE BASE VERIFIER
 * Exact Deobfuscated Sequences:
 *   - 0x14032ACBE / 0x14032CF6F: mov rax, cr3; mov cr3, rax (Hardware TLB flush)
 *   - 0x1401471DB: mov rax, cr3; shr rax, 12; shr r14, 12; cmp rax, [rdi+0x28]
 * ========================================================================= */

VOID
EacHardwareTlbFlush(VOID)
{
    /* Invalidate all non-global TLB entries across current CPU core */
    ULONG64 currentCr3 = __readcr3();
    __writecr3(currentCr3);
}

BOOLEAN
EacValidateProcessCr3AgainstDirectoryTableBase(
    _In_     PEPROCESS TargetProcess,
    _In_opt_ ULONG64   TargetVa
)
{
    if (!TargetProcess) {
        return FALSE;
    }

    /*
     * 0x1401471DB: Read current CPU hardware CR3, shift by PAGE_SHIFT (12)
     * and compare against EPROCESS.DirectoryTableBase (offset 0x28).
     */
    ULONG64 hwCr3 = __readcr3();
    ULONG64 hwCr3Pfn = hwCr3 >> 12;

    PULONG64 pDirectoryTableBase = (PULONG64)((PUCHAR)TargetProcess + 0x028);
    ULONG64 expectedCr3 = *pDirectoryTableBase;
    ULONG64 expectedPfn = expectedCr3 >> 12;

    if (TargetVa != 0ULL) {
        ULONG64 targetPfn = TargetVa >> 12;
        UNREFERENCED_PARAMETER(targetPfn);
    }

    if (hwCr3Pfn != expectedPfn) {
        InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_CR3_SHADOW_MISMATCH);
        EacEmitTelemetryChunk(
            EAC_CHUNK_OP_ALERT,
            4,
            "cr3_directory_table_mismatch",
            (ULONG)hwCr3Pfn,
            (ULONG)expectedPfn
        );
        return FALSE;
    }

    return TRUE;
}

/* =========================================================================
 * 5. BIGPOOL & KERNEL STACK / PAGE TABLE INTEGRITY AUDITOR
 * Combines FUN_1400d97f3 + FUN_1400d9e46 + SystemBigPoolInformation
 * ========================================================================= */

VOID
EacAuditKernelPageTablesAndStacks(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (!Modules || g_Eac.Unloading) {
        return;
    }

    /* 1. Unwind current worker/system thread stack via FUN_1400d97f3 */
    EAC_UNWIND_RESULT unwindRes;
    EacUnwindKernelStack_1400d97f3(NULL, Modules, g_Eac.MbaMasterSeed, &unwindRes);

    /* 2. Query SystemBigPoolInformation (0x42) and run FUN_1400d9e46 on NonPaged allocations */
    ULONG poolBufSize = 0x100000; /* 1 MB initial buffer */
    PSYSTEM_BIGPOOL_INFORMATION bigPool = (PSYSTEM_BIGPOOL_INFORMATION)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        poolBufSize,
        EAC_POOL_TAG_SCAN
    );
    if (!bigPool) {
        return;
    }

    ULONG returnLen = 0;
    NTSTATUS status = ZwQuerySystemInformation(
        SystemBigPoolInformation,
        bigPool,
        poolBufSize,
        &returnLen
    );
    if (NT_SUCCESS(status)) {
        ULONG maxEntries = ( bigPool->Count < 2048 ) ? bigPool->Count : 2048;
        for (ULONG i = 0; i < maxEntries; ++i) {
            /* Mask out low bit flag in VirtualAddress */
            ULONG64 poolVa = ((ULONG64)bigPool->AllocatedInfo[i].VirtualAddress) & ~1ULL;
            if (poolVa < 0xFFFF800000000000ULL) {
                continue;
            }

            ULONG64 effectiveFlags = 0ULL;
            ULONG64 physAddr       = 0ULL;
            if (EacWalkKernelPageTable_1400d9e46(poolVa, &effectiveFlags, g_Eac.MbaMasterSeed, &physAddr)) {
                /*
                 * If a BigPool allocation is marked executable (NX == 0) across all 4 paging
                 * levels (PML4E | PDPTE | PDE | PTE) and is not backed by a signed module:
                 * flag manual-mapped kernel driver (KDMapper / pool shellcode).
                 */
                if ((effectiveFlags & EAC_PTE_NX_BIT) == 0ULL &&
                    !EacIsAddressBackedByModule(Modules, poolVa, NULL, NULL)) {
                    InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_UNBACKED_KERNEL_RIP);
                    EacEmitTelemetryChunk(
                        EAC_CHUNK_OP_ALERT,
                        8,
                        "bigpool_executable_nx0",
                        i,
                        bigPool->AllocatedInfo[i].TagUlong
                    );
                }
            }
        }
    }

    ExFreePoolWithTag(bigPool, EAC_POOL_TAG_SCAN);
}
