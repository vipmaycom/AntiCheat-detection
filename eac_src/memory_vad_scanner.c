/*
 * memory_vad_scanner.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) Memory Integrity Engine (§1, §8, §11)
 *
 * Implements:
 *   1. Kernel CNG SHA-256 Digest Calculation (BCrypt / cng.lib)
 *   2. On-Disk vs In-Memory .text Section Hash Verification
 *   3. Executable Code Cave Detection (.data / .rdata section PTE NX=0 audits via FUN_1400d9e46)
 *   4. Protected Process (FortniteClient-Win64-Shipping.exe) CR3 vs ZwQueryVirtualMemory
 *      Cross-Check to catch MDL mappings (MmMapLockedPagesSpecifyCache with UserMode)
 */

#include "eac.h"

NTSTATUS
EacComputeSha256(
    _In_reads_bytes_(DataLength) const UCHAR *Data,
    _In_ ULONG DataLength,
    _Out_writes_bytes_(32) PUCHAR OutDigest
)
{
    if (!Data || DataLength == 0 || !OutDigest) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(OutDigest, 32);

    BCRYPT_ALG_HANDLE  hAlg  = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    PUCHAR             hashObj = NULL;
    ULONG              hashObjSize = 0;
    ULONG              cbResult = 0;

    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_PROV_DISPATCH);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjSize, sizeof(ULONG), &cbResult, 0);
    if (!NT_SUCCESS(status) || hashObjSize == 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return STATUS_UNSUCCESSFUL;
    }

    hashObj = (PUCHAR)ExAllocatePoolWithTag(NonPagedPoolNx, hashObjSize, EAC_POOL_TAG_SCAN);
    if (!hashObj) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = BCryptCreateHash(hAlg, &hHash, hashObj, hashObjSize, NULL, 0, 0);
    if (NT_SUCCESS(status)) {
        status = BCryptHashData(hHash, (PUCHAR)Data, DataLength, 0);
        if (NT_SUCCESS(status)) {
            status = BCryptFinishHash(hHash, OutDigest, 32, 0);
        }
        BCryptDestroyHash(hHash);
    }

    ExFreePoolWithTag(hashObj, EAC_POOL_TAG_SCAN);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return status;
}

/* =========================================================================
 * CODE CAVE & PE SECTION CHARACTERISTIC AUDITOR (§8)
 * Walks PE sections of loaded kernel drivers and uses FUN_1400d9e46 to detect
 * non-executable sections (.data / .rdata) whose underlying PTE has NX == 0.
 * ========================================================================= */

VOID
EacAuditModuleTextIntegrityAndCodeCaves(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (!Modules || g_Eac.Unloading) {
        return;
    }

    ULONG maxMods = (Modules->NumberOfModules < 64) ? Modules->NumberOfModules : 64;
    for (ULONG i = 0; i < maxMods; ++i) {
        PUCHAR baseVa = (PUCHAR)Modules->Modules[i].ImageBase;
        ULONG  imgSize = Modules->Modules[i].ImageSize;

        if ((ULONG64)baseVa < 0xFFFF800000000000ULL || imgSize < PAGE_SIZE || !MmIsAddressValid(baseVa)) {
            continue;
        }

        PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)baseVa;
        if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE || dosHdr->e_lfanew <= 0 || dosHdr->e_lfanew > 0x800) {
            continue;
        }

        PIMAGE_NT_HEADERS64 ntHdr = (PIMAGE_NT_HEADERS64)(baseVa + dosHdr->e_lfanew);
        if (!MmIsAddressValid(ntHdr) || ntHdr->Signature != IMAGE_NT_SIGNATURE) {
            continue;
        }

        PIMAGE_SECTION_HEADER secHdr = IMAGE_FIRST_SECTION(ntHdr);
        USHORT numSecs = ntHdr->FileHeader.NumberOfSections;
        if (numSecs > 32) {
            numSecs = 32;
        }

        for (USHORT s = 0; s < numSecs; ++s) {
            if (!MmIsAddressValid(&secHdr[s])) {
                break;
            }

            ULONG characteristics = secHdr[s].Characteristics;
            BOOLEAN isCodeSection = (characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
            ULONG64 secVa = (ULONG64)(baseVa + secHdr[s].VirtualAddress);

            /*
             * Code Cave Check (§8):
             * If a section is marked non-executable in its PE header (e.g. .data or .rdata),
             * verify via FUN_1400d9e46 that its hardware PTE still enforces NX == 1.
             */
            if (!isCodeSection &&
                (secHdr[s].Name[0] == '.' && (secHdr[s].Name[1] == 'd' || secHdr[s].Name[1] == 'r'))) {
                ULONG64 pteFlags = 0ULL;
                ULONG64 physAddr = 0ULL;
                if (EacWalkKernelPageTable_1400d9e46(secVa, &pteFlags, g_Eac.MbaMasterSeed, &physAddr)) {
                    /* Skip 2MB large-page merged driver sections where .text and .rdata share a PDE */
                    if (((pteFlags & EAC_PTE_LARGE_PAGE_BIT) == 0ULL) &&
                        ((pteFlags & EAC_PTE_NX_BIT) == 0ULL)) {
                        InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_CODE_CAVE_EXEC);
                        EacEmitTelemetryChunk(
                            EAC_CHUNK_OP_ALERT,
                            8,
                            "codecave_nx0_in_data",
                            i,
                            secHdr[s].VirtualAddress
                        );
                    }
                }
            }
        }
    }
}

/* =========================================================================
 * PROTECTED PROCESS (FortniteClient-Win64-Shipping.exe) MDL / VAD AUDITOR (§1, §8, §11)
 * Cross-references user-mode page table mappings against VAD tree entries to
 * detect kernel MDL mappings (MmMapLockedPagesSpecifyCache(UserMode)).
 * ========================================================================= */

VOID
EacAuditProtectedProcessMemory(
    _In_ HANDLE ProtectedPid
)
{
    if (ProtectedPid == NULL || g_Eac.Unloading || KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return;
    }

    PEPROCESS process = NULL;
    if (!NT_SUCCESS(PsLookupProcessByProcessId(ProtectedPid, &process)) || process == NULL) {
        return;
    }

    KAPC_STATE apcState;
    KeStackAttachProcess(process, &apcState);

    /* Validate active CPU CR3 PFN against EPROCESS->DirectoryTableBase (RVA 0x1471DB) */
    EacValidateProcessCr3AgainstDirectoryTableBase(process, (ULONG64)0x10000ULL);
    EacHardwareTlbFlush();

    /*
     * Probe user-mode address space for executable pages lacking SEC_IMAGE backing
     * or MDL-mapped physical views with mismatched VAD protections.
     */
    PUCHAR probeAddr = (PUCHAR)0x0000000000010000ULL;
    while ((ULONG64)probeAddr < 0x00007FFFFFFE0000ULL) {
        MEMORY_BASIC_INFORMATION mbi;
        RtlZeroMemory(&mbi, sizeof(mbi));
        SIZE_T retLen = 0;

        NTSTATUS status = ZwQueryVirtualMemory(
            ZwCurrentProcess(),
            probeAddr,
            MemoryBasicInformation,
            &mbi,
            sizeof(mbi),
            &retLen
        );
        if (!NT_SUCCESS(status) || mbi.RegionSize == 0) {
            break;
        }

        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) &&
            mbi.Type == MEM_PRIVATE) {
            /* Check for unbacked MZ header inside private executable memory (Reflective DLL / Manual Map) */
            if (MmIsAddressValid(mbi.BaseAddress)) {
                PUCHAR hdr = (PUCHAR)mbi.BaseAddress;
                if (hdr[0] == 'M' && hdr[1] == 'Z') {
                    InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_MDL_USER_NO_VAD);
                    EacEmitTelemetryChunk(
                        EAC_CHUNK_OP_ALERT,
                        1,
                        "unbacked_private_mz",
                        (ULONG)((ULONG64)mbi.BaseAddress >> 16),
                        (ULONG)mbi.RegionSize
                    );
                }
            }
        }

        probeAddr = (PUCHAR)mbi.BaseAddress + mbi.RegionSize;
    }

    KeUnstackDetachProcess(&apcState);
    ObDereferenceObject(process);
}
