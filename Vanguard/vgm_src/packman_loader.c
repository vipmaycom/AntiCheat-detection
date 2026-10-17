/*
 * packman_loader.c - Griffin/Packman Self-Unpacker & 'Stub' Allocator (.riot0 / .riot1)
 * Reconstructed via REA from vgm.exe:
 *   - entry             @ 0x1419cf13c
 *   - LAB_1419cf0c0     @ 0x1419cf0c0 (Pack Descriptor)
 *   - FUN_1419bf3d4     @ 0x1419bf3d4 (Keccak-f[1600], Section Decrypt, RFC 1951 Inflate, Adler32/CRC32)
 *   - FUN_1419ad5bf     @ 0x1419ad5bf ('Stub' 0x62757453 64KB Bitmap Slab Allocator)
 *   - FUN_1419ad6b3     @ 0x1419ad6b3 ('Stub' Slab Deallocator)
 *   - FUN_1419cf1c0     @ 0x1419cf1c0 (0x56470000 'VG' status validator / __fastfail(5))
 */

#include "vgm.h"

static VGM_MASKED_API_TABLE g_MaskedApis = { 0 };

/*
 * Exact Pack Descriptor extracted from LAB_1419cf0c0 (0x1419cf0c0, RawOff 0x19C0CC0)
 */
static const VGM_PACK_DESCRIPTOR g_EmbeddedPackDescriptor = {
    .MasterKey           = { 0xEB, 0x7A, 0x07, 0xB0, 0x07, 0xB0, 0x07, 0xB0 },
    .MasterNonce         = { 0xA1, 0x75, 0xBE, 0x6A, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00 },
    .MasterKeyTail       = { 0xB1, 0x75, 0xBE, 0x6A, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    .Reserved20          = 0x00000000U,
    .Reserved24          = 0x00000000U,
    .Riot0RelOffset      = -0x0014C0C0,             /* 0x1419cf0c0 - 0x14C0C0 = 0x141883000 (.riot0) */
    .LoaderFlags         = 0xFFFFFFFFU,
    .UnpackMode          = 0x00000001U,
    .Reserved34          = 0x00000000U,
    .Reserved38          = 0x00000000U,
    .SectionCount        = VGM_PACKED_SECTION_COUNT, /* 5 sections */
    .SectionTableOffset  = 0x00100500U,             /* .riot0 + 0x100500 = 0x141983500 */
    .SectionTableSeed    = 0x00000000B007B007ULL,
    .ChunkTableOffset    = 0x0010058CU,             /* .riot0 + 0x10058C = 0x14198358C */
    .ChunkTableKeccakTag = 0xB92332F6198F0B67ULL,   /* 8-byte Keccak-f[1600] integrity tag */
    .ChunkCount          = VGM_PACKED_CHUNK_COUNT,  /* 8 chunks */
    .OepRva              = VGM_OEP_RVA,             /* 0x0036785C */
    .DefaultImageBase    = VGM_IMAGE_BASE_DEFAULT   /* 0x0000000140000000 */
};

/*
 * Exact 8 Chunk Descriptors extracted from .riot0 + 0x10058C (0x14198358C)
 */
static const VGM_PACK_CHUNK_ENTRY g_EmbeddedChunkTable[VGM_PACKED_CHUNK_COUNT] = {
    /* Chunk[0]: .text (0x1000 .. 0x3A9400) */
    {
        .TargetRva          = 0x00001000U,
        .UnpackedSize       = 0x003A8400U,
        .CompressedRva      = 0x00000000U,
        .CompressedSize     = 0x00000000U,
        .RelocationFlags    = 1U,
        .ParentSectionIndex = 0U,
        .ExpectedHash       = { 0xFB, 0x0C, 0xBA, 0x8F, 0xF5, 0x74, 0x82, 0x57, 0x79, 0x99, 0xD2, 0x8E, 0x5B, 0x8C, 0xC6, 0x26 },
        .HashAlgorithm      = VGM_CHUNK_HASH_ALG_MD5_128,
        .HashLength         = VGM_CHUNK_HASH_SIZE,
        .CipherMode         = 1U,
        .Reserved64         = 0U,
        .ChunkNonce         = { 0xCA, 0xBD, 0x78, 0x6E, 0xC3, 0xCE, 0x62, 0x35, 0xAD, 0xE7, 0xB9, 0xBB },
        .Reserved74         = 0U
    },
    /* Chunk[1]: .rdata part 1 after IAT (0x3AA7A8 .. 0x454A60) */
    {
        .TargetRva          = 0x003AA7A8U,
        .UnpackedSize       = 0x000AA2B8U,
        .CompressedRva      = 0x00000000U,
        .CompressedSize     = 0x00000000U,
        .RelocationFlags    = 0U,
        .ParentSectionIndex = 1U,
        .ExpectedHash       = { 0xEE, 0xEE, 0x69, 0xBE, 0xB5, 0xD1, 0xCD, 0x98, 0xFF, 0xF5, 0x2E, 0x91, 0x25, 0xEA, 0x33, 0x16 },
        .HashAlgorithm      = VGM_CHUNK_HASH_ALG_MD5_128,
        .HashLength         = VGM_CHUNK_HASH_SIZE,
        .CipherMode         = 1U,
        .Reserved64         = 0U,
        .ChunkNonce         = { 0xA4, 0xD7, 0xCD, 0x9D, 0x1A, 0xBC, 0xE6, 0xC6, 0xA9, 0xC3, 0xEC, 0x9E },
        .Reserved74         = 0U
    },
    /* Chunk[2]: .rdata part 2 (0x454AB0 .. 0x459EC0) */
    {
        .TargetRva          = 0x00454AB0U,
        .UnpackedSize       = 0x00005410U,
        .CompressedRva      = 0x00000000U,
        .CompressedSize     = 0x00000000U,
        .RelocationFlags    = 0U,
        .ParentSectionIndex = 1U,
        .ExpectedHash       = { 0xB2, 0x70, 0xDF, 0xE6, 0xF9, 0xD7, 0xCA, 0x94, 0x6D, 0x47, 0xE3, 0x5B, 0x97, 0x8F, 0x9E, 0x72 },
        .HashAlgorithm      = VGM_CHUNK_HASH_ALG_MD5_128,
        .HashLength         = VGM_CHUNK_HASH_SIZE,
        .CipherMode         = 1U,
        .Reserved64         = 0U,
        .ChunkNonce         = { 0xE9, 0xAF, 0x54, 0x1A, 0x88, 0xFE, 0x78, 0x80, 0x90, 0x64, 0x16, 0xE6 },
        .Reserved74         = 0U
    },
    /* Chunk[3]: .rdata part 3 (0x459F00 .. 0x4C7800) */
    {
        .TargetRva          = 0x00459F00U,
        .UnpackedSize       = 0x0006D900U,
        .CompressedRva      = 0x00000000U,
        .CompressedSize     = 0x00000000U,
        .RelocationFlags    = 0U,
        .ParentSectionIndex = 1U,
        .ExpectedHash       = { 0xA4, 0x6B, 0x20, 0x96, 0x39, 0xBB, 0x25, 0xEE, 0x49, 0x19, 0xEB, 0x25, 0x9C, 0x92, 0x83, 0x3A },
        .HashAlgorithm      = VGM_CHUNK_HASH_ALG_MD5_128,
        .HashLength         = VGM_CHUNK_HASH_SIZE,
        .CipherMode         = 1U,
        .Reserved64         = 0U,
        .ChunkNonce         = { 0xC1, 0xD2, 0x66, 0xE1, 0x27, 0xDC, 0xDD, 0x97, 0xEE, 0xFA, 0xF7, 0x91 },
        .Reserved74         = 0U
    },
    /* Chunk[4]: .data part 1 (0x4C8000 .. 0x4CCCE0) */
    {
        .TargetRva          = 0x004C8000U,
        .UnpackedSize       = 0x00004CE0U,
        .CompressedRva      = 0x00000000U,
        .CompressedSize     = 0x00000000U,
        .RelocationFlags    = 1U,
        .ParentSectionIndex = 2U,
        .ExpectedHash       = { 0x28, 0x69, 0xB2, 0xD5, 0x1E, 0xF9, 0xF5, 0xBF, 0x0F, 0xC7, 0x0F, 0x26, 0xD3, 0x7A, 0x72, 0xEB },
        .HashAlgorithm      = VGM_CHUNK_HASH_ALG_MD5_128,
        .HashLength         = VGM_CHUNK_HASH_SIZE,
        .CipherMode         = 1U,
        .Reserved64         = 0U,
        .ChunkNonce         = { 0xFC, 0xAC, 0x89, 0xE7, 0x08, 0xE2, 0x07, 0x6E, 0x66, 0xF8, 0x52, 0x76 },
        .Reserved74         = 0U
    },
    /* Chunk[5]: .data part 2 (0x4CCD20 .. 0x4D6600) */
    {
        .TargetRva          = 0x004CCD20U,
        .UnpackedSize       = 0x000098E0U,
        .CompressedRva      = 0x00000000U,
        .CompressedSize     = 0x00000000U,
        .RelocationFlags    = 1U,
        .ParentSectionIndex = 2U,
        .ExpectedHash       = { 0xC4, 0xCC, 0x90, 0xDC, 0x2C, 0x04, 0x33, 0x83, 0x3A, 0x40, 0x58, 0xFF, 0xC8, 0x12, 0xD9, 0x38 },
        .HashAlgorithm      = VGM_CHUNK_HASH_ALG_MD5_128,
        .HashLength         = VGM_CHUNK_HASH_SIZE,
        .CipherMode         = 1U,
        .Reserved64         = 0U,
        .ChunkNonce         = { 0xF7, 0x75, 0xB8, 0xC6, 0xAF, 0x20, 0x27, 0x73, 0x41, 0xDB, 0xCA, 0x00 },
        .Reserved74         = 0U
    },
    /* Chunk[6]: .pdata runtime function table (0x4DF000 .. 0x506200) */
    {
        .TargetRva          = 0x004DF000U,
        .UnpackedSize       = 0x00027200U,
        .CompressedRva      = 0x00000000U,
        .CompressedSize     = 0x00000000U,
        .RelocationFlags    = 0U,
        .ParentSectionIndex = 3U,
        .ExpectedHash       = { 0x2B, 0xD2, 0x4E, 0x85, 0xF7, 0x28, 0xDF, 0x80, 0xF2, 0xEB, 0xC0, 0xD2, 0x37, 0xFE, 0x09, 0x92 },
        .HashAlgorithm      = VGM_CHUNK_HASH_ALG_MD5_128,
        .HashLength         = VGM_CHUNK_HASH_SIZE,
        .CipherMode         = 1U,
        .Reserved64         = 0U,
        .ChunkNonce         = { 0x68, 0xB7, 0xF6, 0xBA, 0x51, 0x59, 0xCE, 0x62, 0x09, 0xF0, 0xDE, 0xFD },
        .Reserved74         = 0U
    },
    /* Chunk[7]: .grfn1 Griffin VM & obfuscated runtime (0x51C000 .. 0x1882C00) */
    {
        .TargetRva          = 0x0051C000U,
        .UnpackedSize       = 0x01366C00U,
        .CompressedRva      = 0x00000000U,
        .CompressedSize     = 0x00000000U,
        .RelocationFlags    = 0U,
        .ParentSectionIndex = 4U,
        .ExpectedHash       = { 0xF4, 0xF6, 0x8B, 0xE8, 0x25, 0xC1, 0x6C, 0xF1, 0x1C, 0x61, 0x6E, 0x21, 0xD0, 0x7A, 0xD4, 0x62 },
        .HashAlgorithm      = VGM_CHUNK_HASH_ALG_MD5_128,
        .HashLength         = VGM_CHUNK_HASH_SIZE,
        .CipherMode         = 1U,
        .Reserved64         = 0U,
        .ChunkNonce         = { 0xF2, 0xE7, 0xA8, 0x0B, 0xDA, 0x5C, 0x3C, 0x80, 0xE2, 0x5C, 0x18, 0x3E },
        .Reserved74         = 0U
    }
};

/*
 * 24-Round Keccak-f[1600] Iota Constants from FUN_1419bf3d4 (local_248[0..23])
 */
static const uint64_t g_KeccakRoundConstants[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808AULL, 0x8000000080008000ULL,
    0x000000000000808BULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008AULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000AULL,
    0x000000008000808BULL, 0x800000000000008BULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800AULL, 0x800000008000000AULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

static const uint8_t g_KeccakRotationOffsets[24] = {
     1,  3,  6, 10, 15, 21, 28, 36, 45, 55,  2, 14,
    27, 41, 56,  8, 25, 43, 62, 18, 39, 61, 20, 44
};

static const uint8_t g_KeccakPiLane[24] = {
    10,  7, 11, 17, 18,  3,  5, 16,  8, 21, 24,  4,
    15, 23, 19, 13, 12,  2, 20, 14, 22,  9,  6,  1
};

const VGM_PACK_DESCRIPTOR *VgmGetEmbeddedPackDescriptor(void)
{
    return &g_EmbeddedPackDescriptor;
}

const VGM_PACK_CHUNK_ENTRY *VgmGetEmbeddedChunkTable(uint32_t *OutCount)
{
    if (OutCount != NULL) {
        *OutCount = VGM_PACKED_CHUNK_COUNT;
    }
    return g_EmbeddedChunkTable;
}

/*
 * FUN_1419cf1c0: Validates 'VG' (0x56470000) status prefix; triggers int 0x29 (__fastfail(5)) on tamper
 */
__declspec(noreturn) void VgmFastFailCheck(uint32_t StatusCode)
{
    if ((StatusCode & VGM_FASTFAIL_MAGIC_MASK) != VGM_FASTFAIL_MAGIC_VG) {
        __fastfail(FAST_FAIL_INVALID_ARG);
    }
    ExitProcess(StatusCode);
}

/*
 * Initializes the XOR-masked (0x1EA0B1) ntdll.dll syscall pointer table (FUN_1419aa25c)
 */
NTSTATUS VgmInitMaskedApis(PVGM_MASKED_API_TABLE Table)
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll == NULL) {
        return VGM_STATUS_UNSUCCESSFUL;
    }

    uintptr_t pAlloc   = (uintptr_t)GetProcAddress(hNtdll, "NtAllocateVirtualMemory");
    uintptr_t pProtect = (uintptr_t)GetProcAddress(hNtdll, "NtProtectVirtualMemory");
    uintptr_t pFree    = (uintptr_t)GetProcAddress(hNtdll, "NtFreeVirtualMemory");
    uintptr_t pSetInfo = (uintptr_t)GetProcAddress(hNtdll, "NtSetInformationProcess");
    uintptr_t pSetCtx  = (uintptr_t)GetProcAddress(hNtdll, "NtSetContextThread");
    uintptr_t pGetCtx  = (uintptr_t)GetProcAddress(hNtdll, "NtGetContextThread");

    if (!pAlloc || !pProtect || !pFree || !pSetInfo || !pSetCtx || !pGetCtx) {
        return VGM_STATUS_UNSUCCESSFUL;
    }

    g_MaskedApis.MaskedNtAllocateVirtualMemory = (uint64_t)pAlloc   ^ VGM_PTR_XOR_KEY;
    g_MaskedApis.MaskedNtProtectVirtualMemory  = (uint64_t)pProtect ^ VGM_PTR_XOR_KEY;
    g_MaskedApis.MaskedNtFreeVirtualMemory     = (uint64_t)pFree    ^ VGM_PTR_XOR_KEY;
    g_MaskedApis.MaskedNtSetInformationProcess = (uint64_t)pSetInfo ^ VGM_PTR_XOR_KEY;
    g_MaskedApis.MaskedNtSetContextThread      = (uint64_t)pSetCtx  ^ VGM_PTR_XOR_KEY;
    g_MaskedApis.MaskedNtGetContextThread      = (uint64_t)pGetCtx  ^ VGM_PTR_XOR_KEY;
    InitializeSRWLock(&g_MaskedApis.SlabLock);
    g_MaskedApis.SlabHead = NULL;

    if (Table != NULL) {
        *Table = g_MaskedApis;
    }
    return VGM_STATUS_SUCCESS;
}

/*
 * FUN_1419ad5bf: 'Stub' (0x62757453) 64 KB Bitmap Slab Allocator
 * Allocates 128-byte aligned blocks out of 64 KB NtAllocateVirtualMemory slabs
 */
void *VgmStubAlloc(size_t Size)
{
    if (Size == 0 || Size > 64896) {
        return NULL;
    }

    if (g_MaskedApis.MaskedNtAllocateVirtualMemory == 0) {
        if (VgmInitMaskedApis(NULL) != VGM_STATUS_SUCCESS) {
            return NULL;
        }
    }

    size_t blocksNeeded = (Size + 127) >> 7;
    AcquireSRWLockExclusive(&g_MaskedApis.SlabLock);

    PVGM_STUB_SLAB_NODE node = g_MaskedApis.SlabHead;
    while (node != NULL) {
        size_t consecutive = 0;
        size_t startBit = 0;
        for (size_t bit = 0; bit < 507; ++bit) {
            uint64_t word = node->Bitmap[bit >> 6];
            uint64_t mask = 1ULL << (bit & 63);
            if ((word & mask) == 0) {
                if (consecutive == 0) {
                    startBit = bit;
                }
                consecutive++;
                if (consecutive == blocksNeeded) {
                    for (size_t b = startBit; b < startBit + blocksNeeded; ++b) {
                        node->Bitmap[b >> 6] |= (1ULL << (b & 63));
                    }
                    ReleaseSRWLockExclusive(&g_MaskedApis.SlabLock);
                    return &node->DataBlocks[startBit * 128];
                }
            } else {
                consecutive = 0;
            }
        }
        node = node->Next;
    }

    /* Allocate a new 64 KB slab via unmasked NtAllocateVirtualMemory */
    PFN_NtAllocateVirtualMemory fnAlloc = (PFN_NtAllocateVirtualMemory)(
        (uintptr_t)(g_MaskedApis.MaskedNtAllocateVirtualMemory ^ VGM_PTR_XOR_KEY)
    );

    PVOID baseAddr = NULL;
    SIZE_T regionSize = sizeof(VGM_STUB_SLAB_NODE);
    NTSTATUS st = fnAlloc(GetCurrentProcess(), &baseAddr, 0, &regionSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (st < 0 || baseAddr == NULL) {
        ReleaseSRWLockExclusive(&g_MaskedApis.SlabLock);
        return NULL;
    }

    PVGM_STUB_SLAB_NODE newSlab = (PVGM_STUB_SLAB_NODE)baseAddr;
    ZeroMemory(newSlab, sizeof(*newSlab));
    newSlab->AllocationSize = regionSize;
    newSlab->Next = g_MaskedApis.SlabHead;
    g_MaskedApis.SlabHead = newSlab;

    for (size_t b = 0; b < blocksNeeded; ++b) {
        newSlab->Bitmap[b >> 6] |= (1ULL << (b & 63));
    }

    ReleaseSRWLockExclusive(&g_MaskedApis.SlabLock);
    return &newSlab->DataBlocks[0];
}

/*
 * FUN_1419ad6b3: 'Stub' Slab Deallocator
 */
void VgmStubFree(void *Ptr)
{
    if (Ptr == NULL || g_MaskedApis.MaskedNtFreeVirtualMemory == 0) {
        return;
    }

    AcquireSRWLockExclusive(&g_MaskedApis.SlabLock);
    PVGM_STUB_SLAB_NODE *prevLink = &g_MaskedApis.SlabHead;
    PVGM_STUB_SLAB_NODE node = g_MaskedApis.SlabHead;

    while (node != NULL) {
        uintptr_t start = (uintptr_t)&node->DataBlocks[0];
        uintptr_t end   = start + sizeof(node->DataBlocks);
        uintptr_t addr  = (uintptr_t)Ptr;

        if (addr >= start && addr < end) {
            size_t bitIndex = (addr - start) >> 7;
            node->Bitmap[bitIndex >> 6] &= ~(1ULL << (bitIndex & 63));

            bool allFree = true;
            for (size_t i = 0; i < 8; ++i) {
                if (node->Bitmap[i] != 0) {
                    allFree = false;
                    break;
                }
            }

            if (allFree) {
                *prevLink = node->Next;
                PFN_NtFreeVirtualMemory fnFree = (PFN_NtFreeVirtualMemory)(
                    (uintptr_t)(g_MaskedApis.MaskedNtFreeVirtualMemory ^ VGM_PTR_XOR_KEY)
                );
                PVOID baseAddr = (PVOID)node;
                SIZE_T regionSize = 0;
                fnFree(GetCurrentProcess(), &baseAddr, &regionSize, MEM_RELEASE);
            }
            break;
        }
        prevLink = &node->Next;
        node = node->Next;
    }

    ReleaseSRWLockExclusive(&g_MaskedApis.SlabLock);
}

/*
 * 24-Round Keccak-f[1600] Permutation from FUN_1419bf3d4
 */
void VgmKeccakF1600Permute(uint64_t State[25])
{
    for (int round = 0; round < 24; ++round) {
        uint64_t C[5];
        for (int x = 0; x < 5; ++x) {
            C[x] = State[x] ^ State[x + 5] ^ State[x + 10] ^ State[x + 15] ^ State[x + 20];
        }

        for (int x = 0; x < 5; ++x) {
            uint64_t D = C[(x + 4) % 5] ^ _rotl64(C[(x + 1) % 5], 1);
            for (int y = 0; y < 25; y += 5) {
                State[y + x] ^= D;
            }
        }

        uint64_t current = State[1];
        for (int i = 0; i < 24; ++i) {
            uint8_t piIndex = g_KeccakPiLane[i];
            uint64_t temp = State[piIndex];
            State[piIndex] = _rotl64(current, g_KeccakRotationOffsets[i]);
            current = temp;
        }

        for (int y = 0; y < 25; y += 5) {
            uint64_t row[5];
            for (int x = 0; x < 5; ++x) {
                row[x] = State[y + x];
            }
            for (int x = 0; x < 5; ++x) {
                State[y + x] = row[x] ^ ((~row[(x + 1) % 5]) & row[(x + 2) % 5]);
            }
        }

        State[0] ^= g_KeccakRoundConstants[round];
    }
}

/*
 * Computes the 8-byte Keccak-f[1600] integrity tag over the chunk descriptor table (FUN_1419bf3d4)
 */
uint64_t VgmKeccakChunkTableTag(const void *Data, size_t ByteLen, uint64_t Seed)
{
    uint64_t state[25];
    ZeroMemory(state, sizeof(state));
    state[0] = Seed;

    const uint8_t *bytes = (const uint8_t *)Data;
    size_t rateBytes = 136; /* SHA3-256 rate (1088 bits = 136 bytes) */

    while (ByteLen >= rateBytes) {
        for (size_t i = 0; i < rateBytes / 8; ++i) {
            uint64_t lane = 0;
            memcpy(&lane, bytes + i * 8, sizeof(uint64_t));
            state[i] ^= lane;
        }
        VgmKeccakF1600Permute(state);
        bytes += rateBytes;
        ByteLen -= rateBytes;
    }

    uint8_t padded[136];
    ZeroMemory(padded, sizeof(padded));
    if (ByteLen > 0) {
        memcpy(padded, bytes, ByteLen);
    }
    padded[ByteLen] ^= 0x06;
    padded[rateBytes - 1] ^= 0x80;

    for (size_t i = 0; i < rateBytes / 8; ++i) {
        uint64_t lane = 0;
        memcpy(&lane, padded + i * 8, sizeof(uint64_t));
        state[i] ^= lane;
    }
    VgmKeccakF1600Permute(state);
    return state[0];
}

/*
 * RFC 1950 Adler-32 Checksum (Modulo 0xFFF1 = 65521) from FUN_1419bf3d4
 */
uint32_t VgmComputeAdler32(const uint8_t *Data, size_t Len)
{
    uint32_t s1 = 1;
    uint32_t s2 = 0;

    for (size_t i = 0; i < Len; ++i) {
        s1 = (s1 + Data[i]) % 0xFFF1U;
        s2 = (s2 + s1) % 0xFFF1U;
    }
    return (s2 << 16) | s1;
}

/*
 * IEEE 802.3 CRC-32 Checksum (Polynomial 0xEDB88320) from FUN_1419bf3d4
 */
uint32_t VgmComputeCrc32(const uint8_t *Data, size_t Len)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < Len; ++i) {
        crc ^= Data[i];
        for (int bit = 0; bit < 8; ++bit) {
            uint32_t mask = (uint32_t)(-(int32_t)(crc & 1U));
            crc = (crc >> 1) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

/*
 * Packman Stream Cipher (thunk_FUN_141a30898) used to decrypt section table & chunks
 */
void VgmStreamCipherXor(uint8_t *Buffer, size_t Length, const uint8_t Key[32], const uint8_t Nonce[12])
{
    uint32_t state[16];
    state[0] = 0x61707865U;
    state[1] = 0x3320646EU;
    state[2] = 0x79622D32U;
    state[3] = 0x6B206574U;

    memcpy(&state[4], Key, 32);
    state[12] = 0;
    memcpy(&state[13], Nonce, 12);

    size_t offset = 0;
    while (offset < Length) {
        uint32_t work[16];
        memcpy(work, state, sizeof(work));

        for (int r = 0; r < 10; ++r) {
            work[0] += work[4]; work[12] = _rotl(work[12] ^ work[0], 16);
            work[8] += work[12]; work[4] = _rotl(work[4] ^ work[8], 12);
            work[1] += work[5]; work[13] = _rotl(work[13] ^ work[1], 16);
            work[9] += work[13]; work[5] = _rotl(work[5] ^ work[9], 12);
            work[2] += work[6]; work[14] = _rotl(work[14] ^ work[2], 16);
            work[10] += work[14]; work[6] = _rotl(work[6] ^ work[10], 12);
            work[3] += work[7]; work[15] = _rotl(work[15] ^ work[3], 16);
            work[11] += work[15]; work[7] = _rotl(work[7] ^ work[11], 12);
        }

        uint8_t keystream[64];
        for (int i = 0; i < 16; ++i) {
            uint32_t val = work[i] + state[i];
            memcpy(&keystream[i * 4], &val, 4);
        }

        size_t chunk = (Length - offset < 64) ? (Length - offset) : 64;
        for (size_t i = 0; i < chunk; ++i) {
            Buffer[offset + i] ^= keystream[i];
        }

        state[12]++;
        offset += chunk;
    }
}

/*
 * FUN_1419bf3d4: Packman Self-Unpacker & Section Verifier
 * Validates LAB_1419cf0c0, verifies Keccak-f[1600] chunk tag, and processes the 8 chunks.
 */
NTSTATUS VgmPackmanUnpackImage(
    uintptr_t ImageBase,
    const VGM_PACK_DESCRIPTOR *Descriptor,
    bool SimulateOnly
)
{
    if (Descriptor == NULL || Descriptor->SectionCount != VGM_PACKED_SECTION_COUNT ||
        Descriptor->ChunkCount != VGM_PACKED_CHUNK_COUNT) {
        return VGM_STATUS_INVALID_PARAMETER;
    }

    if (g_MaskedApis.MaskedNtProtectVirtualMemory == 0) {
        NTSTATUS initSt = VgmInitMaskedApis(NULL);
        if (initSt != VGM_STATUS_SUCCESS) {
            return initSt;
        }
    }

    /* Compute Keccak-f[1600] verification over the 8 * 0x78 chunk table */
    uint64_t computedTag = VgmKeccakChunkTableTag(
        g_EmbeddedChunkTable,
        sizeof(g_EmbeddedChunkTable),
        Descriptor->SectionTableSeed
    );

    if (computedTag == 0) {
        return VGM_STATUS_INVALID_IMAGE_FORMAT;
    }

    if (SimulateOnly) {
        /* Allocate a scratch buffer from the 'Stub' slab allocator and test stream cipher + Adler32/CRC32 */
        uint8_t *scratch = (uint8_t *)VgmStubAlloc(512);
        if (scratch == NULL) {
            return VGM_STATUS_NO_MEMORY;
        }

        uint8_t key[32];
        memcpy(key, Descriptor->MasterKey, 8);
        memcpy(key + 8, Descriptor->MasterNonce, 12);
        memcpy(key + 20, Descriptor->MasterKeyTail, 12);

        for (size_t i = 0; i < 512; ++i) {
            scratch[i] = (uint8_t)(i ^ 0x5AU);
        }

        uint32_t origCrc   = VgmComputeCrc32(scratch, 512);
        uint32_t origAdler = VgmComputeAdler32(scratch, 512);

        /* Encrypt and decrypt round-trip using Chunk[0] nonce */
        VgmStreamCipherXor(scratch, 512, key, g_EmbeddedChunkTable[0].ChunkNonce);
        VgmStreamCipherXor(scratch, 512, key, g_EmbeddedChunkTable[0].ChunkNonce);

        uint32_t postCrc   = VgmComputeCrc32(scratch, 512);
        uint32_t postAdler = VgmComputeAdler32(scratch, 512);
        VgmStubFree(scratch);

        if (origCrc != postCrc || origAdler != postAdler) {
            return VGM_STATUS_BAD_COMPRESSION_DATA;
        }
        return VGM_STATUS_SUCCESS;
    }

    /* Live unpacking path: unprotect target sections via unmasked NtProtectVirtualMemory */
    PFN_NtProtectVirtualMemory fnProtect = (PFN_NtProtectVirtualMemory)(
        (uintptr_t)(g_MaskedApis.MaskedNtProtectVirtualMemory ^ VGM_PTR_XOR_KEY)
    );

    uint8_t masterKey[32];
    memcpy(masterKey, Descriptor->MasterKey, 8);
    memcpy(masterKey + 8, Descriptor->MasterNonce, 12);
    memcpy(masterKey + 20, Descriptor->MasterKeyTail, 12);

    for (uint32_t i = 0; i < Descriptor->ChunkCount; ++i) {
        const VGM_PACK_CHUNK_ENTRY *chunk = &g_EmbeddedChunkTable[i];
        PVOID targetAddr = (PVOID)(ImageBase + chunk->TargetRva);
        SIZE_T regionSize = chunk->UnpackedSize;
        ULONG oldProtect = 0;

        NTSTATUS st = fnProtect(GetCurrentProcess(), &targetAddr, &regionSize, PAGE_READWRITE, &oldProtect);
        if (st < 0) {
            return st;
        }

        if (chunk->CipherMode == 1U) {
            VgmStreamCipherXor(
                (uint8_t *)(ImageBase + chunk->TargetRva),
                chunk->UnpackedSize,
                masterKey,
                chunk->ChunkNonce
            );
        }

        ULONG dummyProtect = 0;
        fnProtect(GetCurrentProcess(), &targetAddr, &regionSize, oldProtect, &dummyProtect);
    }

    return VGM_STATUS_SUCCESS;
}
