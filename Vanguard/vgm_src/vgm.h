/*
 * vgm.h - Master Architecture Header for Riot Vanguard User-mode Module (vgm.exe)
 * Reconstructed via REA (Ghidra 12.1.4 + Binary Analysis) from:
 *   C:\Program Files\Riot Vanguard\vgm.exe
 *   Version: 1.19.1-18+20261001.143443
 *   SHA-256: 55E25CCE83C8EC9088B99D1193A7AA750609E69ED8CFC7C758076EC3591DE3D9
 *
 * Key Binary Layout (ImageBase = 0x140000000):
 *   .text     : 0x140001000 (VirtSize 0x3A82CC) - Packed main user-mode module code (OEP @ +0x36785C)
 *   .rdata    : 0x1403AA000 (VirtSize 0x11CFBC) - Packed read-only data & IAT (0x3AA000..0x3AA7A8)
 *   .data     : 0x1404C7000 (VirtSize 0x017300) - Packed global state
 *   .pdata    : 0x1404DF000 (VirtSize 0x03B3AC) - Exception unwind directory (9,491 RUNTIME_FUNCTIONs)
 *   .fptable  : 0x14051B000 (VirtSize 0x000028) - Griffin function pointer dispatch table
 *   .rsrc     : 0x14051C000 (VirtSize 0x000808) - Resource directory
 *   .riot2    : 0x14051D000 (VirtSize 0x02CC98) - Auxiliary integrity table
 *   .toir     : 0x14054A000 (VirtSize 0x02AFBC) - Reverse-tag metadata ("riot" reversed)
 *   .grfn1    : 0x140575000 (VirtSize 0x130DC00)- Griffin VM / obfuscation runtime (~20.0 MB)
 *   .riot0    : 0x141883000 (VirtSize 0x100A00) - Packman container header (0x5647020B) + chunk table
 *   .riot1    : 0x141984000 (VirtSize 0x655A22) - Packman unpacker, Nirvana hook, crypto, crash handler
 *   .reloc    : 0x141FDA000 (VirtSize 0x002488) - Base relocations
 */

#ifndef VGM_H
#define VGM_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winternl.h>
#include <bcrypt.h>
#include <intrin.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. PACKMAN / GRIFFIN CONSTANTS & SIGNATURES (.riot0 / .riot1)
 * ============================================================================ */

#define VGM_IMAGE_BASE_DEFAULT          0x0000000140000000ULL
#define VGM_OEP_RVA                     0x0036785CU             /* 0x14036785C in .text */
#define VGM_ENTRYPOINT_RVA              0x019CF13CU             /* 0x1419CF13C in .riot1 */
#define VGM_PACK_DESC_RVA               0x019CF0C0U             /* LAB_1419cf0c0 in .riot1 */
#define VGM_RIOT0_RVA                   0x01883000U             /* 0x141883000 (.riot0 base) */

#define VGM_RIOT0_MAGIC                 0x5647020BU             /* 'VG' (0x5647) + PE32+ (0x020B) */
#define VGM_FASTFAIL_MAGIC_MASK         0xFFFF0000U
#define VGM_FASTFAIL_MAGIC_VG           0x56470000U             /* Checked in FUN_1419cf1c0 */
#define VGM_STATUS_ALREADY_INIT         0xD4494E49U             /* Checked in entry @ 0x1419cf13c */

#define VGM_PTR_XOR_KEY                 0x00000000001EA0B1ULL   /* API pointer obfuscation mask */
#define VGM_STUB_POOL_TAG               0x62757453U             /* 'Stub' in FUN_1419ad5bf */
#define VGM_USER_MODE_LIMIT             0x00007FFFFFFEFFFFULL

/* NTSTATUS Error Codes returned by FUN_1419bf3d4 */
#define VGM_STATUS_SUCCESS              ((NTSTATUS)0x00000000L)
#define VGM_STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)
#define VGM_STATUS_INVALID_PARAMETER    ((NTSTATUS)0xC000000DL)
#define VGM_STATUS_NO_MEMORY            ((NTSTATUS)0xC0000017L)
#define VGM_STATUS_ACCESS_VIOLATION     ((NTSTATUS)0xC0000005L)
#define VGM_STATUS_INVALID_IMAGE_FORMAT ((NTSTATUS)0xE000007BL)
#define VGM_STATUS_BAD_COMPRESSION_DATA ((NTSTATUS)0xE0000242L)
#define VGM_STATUS_INVALID_IMAGE_HASH   ((NTSTATUS)0xE0000428L)

#define VGM_PACKED_SECTION_COUNT        5U
#define VGM_PACKED_CHUNK_COUNT          8U
#define VGM_CHUNK_HASH_ALG_MD5_128      0x2005U
#define VGM_CHUNK_HASH_SIZE             16U

/* ============================================================================
 * 2. PACKMAN STRUCTURES (LAB_1419cf0c0 & .riot0 @ 0x141883000)
 * ============================================================================ */

#pragma pack(push, 1)

/*
 * Packman Descriptor at LAB_1419cf0c0 (0x1419cf0c0, RawOff 0x19C0CC0, 0x68 bytes)
 * Passed as R8 to thunk_FUN_141a54471(rcx, rdx, &LAB_1419cf0c0)
 */
typedef struct _VGM_PACK_DESCRIPTOR {
    uint8_t  MasterKey[8];              /* +0x00: EB 7A 07 B0 07 B0 07 B0 */
    uint8_t  MasterNonce[12];           /* +0x08: A1 75 BE 6A 00 00 00 00 14 00 00 00 */
    uint8_t  MasterKeyTail[12];         /* +0x14: B1 75 BE 6A 14 00 00 00 00 00 00 00 */
    uint32_t Reserved20;                /* +0x20: 0x00000000 */
    uint32_t Reserved24;                /* +0x24: 0x00000000 */
    int32_t  Riot0RelOffset;            /* +0x28: -0x0014C0C0 (points to .riot0 @ 0x141883000) */
    uint32_t LoaderFlags;               /* +0x2C: 0xFFFFFFFF */
    uint32_t UnpackMode;                /* +0x30: 0x00000001 */
    uint32_t Reserved34;                /* +0x34: 0x00000000 */
    uint32_t Reserved38;                /* +0x38: 0x00000000 */
    uint32_t SectionCount;              /* +0x3C: 5 (.text, .rdata, .data, .pdata, .grfn1) */
    uint32_t SectionTableOffset;        /* +0x40: 0x00100500 (.riot0 + 0x100500 = 0x141983500) */
    uint64_t SectionTableSeed;          /* +0x44: 0x00000000B007B007 */
    uint32_t ChunkTableOffset;          /* +0x4C: 0x0010058C (.riot0 + 0x10058C = 0x14198358C) */
    uint64_t ChunkTableKeccakTag;       /* +0x50: 0xB92332F6198F0B67 (Keccak-f[1600] 8-byte tag) */
    uint32_t ChunkCount;                /* +0x58: 8 */
    uint32_t OepRva;                    /* +0x5C: 0x0036785C */
    uint64_t DefaultImageBase;          /* +0x60: 0x0000000140000000 */
} VGM_PACK_DESCRIPTOR, *PVGM_PACK_DESCRIPTOR;

/*
 * .riot0 Header at 0x141883000 (RawOff 0x1877800)
 */
typedef struct _VGM_RIOT0_HEADER {
    uint32_t Magic;                     /* +0x00: 0x5647020B ('VG' + 0x020B) */
    uint32_t HeaderVersion;             /* +0x04: 0x003BA014 */
    uint64_t PackTimestamp;             /* +0x08: 0x00000000019B0A00 */
    uint64_t RuntimeImageBase;          /* +0x10: 0x0000000140000000 (updated during relocation) */
    uint64_t OepRva;                    /* +0x18: 0x000010000036785C */
    uint32_t BaseOfDataRva;             /* +0x20: 0x003AA000 (.rdata) */
    uint32_t SizeOfHeaders;             /* +0x24: 0x00000400 */
    uint32_t SizeOfImage;               /* +0x28: 0x01FDD000 */
    uint32_t SubsystemAndFlags;         /* +0x2C: 0x00020022 (WINDOWS_GUI) */
    uint32_t LoaderFlags;               /* +0x30: 0x81600006 */
    uint32_t NumberOfRvaAndSizes;       /* +0x34: 0x00100000 */
} VGM_RIOT0_HEADER, *PVGM_RIOT0_HEADER;

/*
 * Encrypted Section Entry (0x1C bytes each, 5 entries at .riot0 + 0x100500)
 * Decrypted in FUN_1419bf3d4 via stream cipher thunk_FUN_141a30898
 */
typedef struct _VGM_PACK_SECTION_ENTRY {
    uint32_t VirtualAddress;            /* +0x00: Section RVA */
    uint32_t VirtualSize;               /* +0x04: Section VirtualSize */
    uint32_t RawDataOffset;             /* +0x08: Raw offset or key derivation word 0 */
    uint32_t RawDataSize;               /* +0x0C: Raw size or key derivation word 1 */
    uint32_t Characteristics;           /* +0x10: Original page protection flags */
    uint32_t KeySeedLow;                /* +0x14: Per-section key derivation word 2 */
    uint32_t KeySeedHigh;               /* +0x18: Per-section key derivation word 3 */
} VGM_PACK_SECTION_ENTRY, *PVGM_PACK_SECTION_ENTRY;

/*
 * Chunk Descriptor Entry (0x78 bytes each, 8 entries at .riot0 + 0x10058C)
 * Verified by Keccak-f[1600] in FUN_1419bf3d4 before unpacking
 */
typedef struct _VGM_PACK_CHUNK_ENTRY {
    uint32_t TargetRva;                 /* +0x00: Destination RVA to unpack */
    uint32_t UnpackedSize;              /* +0x04: Uncompressed size in bytes */
    uint32_t CompressedRva;             /* +0x08: Source RVA of compressed stream (0 if in-place) */
    uint32_t CompressedSize;            /* +0x0C: Compressed size (0 if stored raw encrypted) */
    uint32_t RelocationFlags;           /* +0x10: 1 if chunk requires relocation fixups */
    uint32_t ParentSectionIndex;        /* +0x14: Index into VGM_PACK_SECTION_ENTRY[0..4] */
    uint8_t  ExpectedHash[64];          /* +0x18..+0x57: Expected chunk digest (first 16B used) */
    uint32_t HashAlgorithm;             /* +0x58: 0x00002005 */
    uint32_t HashLength;                /* +0x5C: 0x00000010 (16 bytes) */
    uint32_t CipherMode;                /* +0x60: 1 = encrypted, 0 = plaintext */
    uint32_t Reserved64;                /* +0x64: 0 */
    uint8_t  ChunkNonce[12];            /* +0x68..+0x73: 12-byte per-chunk stream cipher nonce */
    uint32_t Reserved74;                /* +0x74: 0 */
} VGM_PACK_CHUNK_ENTRY, *PVGM_PACK_CHUNK_ENTRY;

#pragma pack(pop)

/* ============================================================================
 * 3. 'Stub' SLAB ALLOCATOR & MASKED API DISPATCHER (FUN_1419ad5bf / 1419aa25c)
 * ============================================================================ */

typedef struct _VGM_STUB_SLAB_NODE {
    struct _VGM_STUB_SLAB_NODE *Next;   /* +0x00: Linked list of 64 KB slabs */
    uint64_t AllocationSize;            /* +0x08: Region size (0x10000 = 64 KB) */
    uint64_t Bitmap[8];                 /* +0x10: 512-bit allocation bitmap (1 bit = 128-byte block) */
    uint8_t  DataBlocks[64896];         /* +0x50: 507 * 128-byte blocks */
} VGM_STUB_SLAB_NODE, *PVGM_STUB_SLAB_NODE;

typedef NTSTATUS (NTAPI *PFN_NtAllocateVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    ULONG_PTR ZeroBits,
    PSIZE_T RegionSize,
    ULONG AllocationType,
    ULONG Protect
);

typedef NTSTATUS (NTAPI *PFN_NtProtectVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    PSIZE_T RegionSize,
    ULONG NewProtect,
    PULONG OldProtect
);

typedef NTSTATUS (NTAPI *PFN_NtFreeVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    PSIZE_T RegionSize,
    ULONG FreeType
);

typedef NTSTATUS (NTAPI *PFN_NtSetInformationProcess)(
    HANDLE ProcessHandle,
    ULONG ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength
);

typedef NTSTATUS (NTAPI *PFN_NtSetContextThread)(
    HANDLE ThreadHandle,
    PCONTEXT ThreadContext
);

typedef NTSTATUS (NTAPI *PFN_NtGetContextThread)(
    HANDLE ThreadHandle,
    PCONTEXT ThreadContext
);

typedef struct _VGM_MASKED_API_TABLE {
    uint64_t MaskedNtAllocateVirtualMemory;
    uint64_t MaskedNtProtectVirtualMemory;
    uint64_t MaskedNtFreeVirtualMemory;
    uint64_t MaskedNtSetInformationProcess;
    uint64_t MaskedNtSetContextThread;
    uint64_t MaskedNtGetContextThread;
    SRWLOCK  SlabLock;
    PVGM_STUB_SLAB_NODE SlabHead;
} VGM_MASKED_API_TABLE, *PVGM_MASKED_API_TABLE;

/* ============================================================================
 * 4. PROCESS INSTRUMENTATION CALLBACK / NIRVANA HOOK (FUN_1419cf400 / 1419c7170)
 * ============================================================================ */

#define ProcessInstrumentationCallback  (( ULONG )40)

typedef struct _PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION {
    ULONG Version;
    ULONG Reserved;
    PVOID Callback;
} PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION, *PPROCESS_INSTRUMENTATION_CALLBACK_INFORMATION;

typedef void (*PFN_VGM_SYSCALL_OBSERVER)(
    uint64_t ReturnValue,
    uintptr_t PostSyscallRip,
    uint8_t OpcodeType,                 /* 0 = syscall (0F 05), 1 = int 2Eh (CD 2E) */
    void *UserContext
);

typedef struct _VGM_NIRVANA_OBSERVER_NODE {
    struct _VGM_NIRVANA_OBSERVER_NODE *Next;
    struct _VGM_NIRVANA_OBSERVER_NODE *Prev;
    PFN_VGM_SYSCALL_OBSERVER Callback;
    void *UserContext;
} VGM_NIRVANA_OBSERVER_NODE, *PVGM_NIRVANA_OBSERVER_NODE;

typedef struct _VGM_NIRVANA_STATE {
    volatile LONG Initialized;
    volatile LONG ActiveThreadLock;
    volatile DWORD OwnerThreadId;
    uintptr_t NtdllBase;
    uintptr_t NtdllEnd;
    ULONG TotalSyscallsInspected;
    ULONG UnbackedSyscallsDetected;
    ULONG Int2ESyscallsDetected;
    VGM_NIRVANA_OBSERVER_NODE ObserverListHead;
} VGM_NIRVANA_STATE, *PVGM_NIRVANA_STATE;

/* ============================================================================
 * 5. CRYPTOGRAPHIC & HASHING PRIMITIVES (.riot1)
 * ============================================================================ */

typedef struct _VGM_SHA1_CTX {
    uint32_t State[5];
    uint64_t BitCount;
    uint8_t  Buffer[64];
} VGM_SHA1_CTX;

typedef struct _VGM_SHA256_CTX {
    uint32_t State[8];
    uint64_t BitCount;
    uint8_t  Buffer[64];
} VGM_SHA256_CTX;

typedef struct _VGM_SHA512_CTX {
    uint64_t State[8];
    uint64_t BitCountLow;
    uint64_t BitCountHigh;
    uint8_t  Buffer[128];
} VGM_SHA512_CTX;

typedef struct _VGM_XXH128_HASH {
    uint64_t Low64;
    uint64_t High64;
} VGM_XXH128_HASH;

/* ============================================================================
 * 6. COOPERATIVE FIBER SCHEDULER, IPC & WS2_32 TELEMETRY ENGINE
 * ============================================================================ */

#define VGM_MAX_FIBERS                  4U
#define VGM_FIBER_STACK_SIZE            (64 * 1024)
#define VGM_IPC_PIPE_NAME               L"\\\\.\\pipe\\vgk_vgm_ipc"

typedef struct _VGM_FIBER_CONTEXT {
    LPVOID   MainThreadFiber;
    LPVOID   WorkerFibers[VGM_MAX_FIBERS];
    uint32_t CurrentFiberIndex;
    uint32_t FiberTickCount[VGM_MAX_FIBERS];
    bool     ShouldTerminate;
} VGM_FIBER_CONTEXT, *PVGM_FIBER_CONTEXT;

typedef struct _VGM_HEARTBEAT_PACKET {
    uint32_t Magic;                     /* 0x56474D31 ('VGM1') */
    uint32_t SequenceNumber;
    uint64_t TickCount64;
    uint32_t SyscallsAudited;
    uint32_t UnbackedSyscalls;
    uint8_t  SessionNonce[16];
    uint8_t  Sha256Mac[32];
} VGM_HEARTBEAT_PACKET, *PVGM_HEARTBEAT_PACKET;

/* ============================================================================
 * 7. FUNCTION PROTOTYPES ACROSS MODULES
 * ============================================================================ */

/* packman_loader.c */
NTSTATUS VgmInitMaskedApis(PVGM_MASKED_API_TABLE Table);
void *VgmStubAlloc(size_t Size);
void VgmStubFree(void *Ptr);
__declspec(noreturn) void VgmFastFailCheck(uint32_t StatusCode);
void VgmKeccakF1600Permute(uint64_t State[25]);
uint64_t VgmKeccakChunkTableTag(const void *Data, size_t ByteLen, uint64_t Seed);
uint32_t VgmComputeAdler32(const uint8_t *Data, size_t Len);
uint32_t VgmComputeCrc32(const uint8_t *Data, size_t Len);
void VgmStreamCipherXor(uint8_t *Buffer, size_t Length, const uint8_t Key[32], const uint8_t Nonce[12]);
NTSTATUS VgmPackmanUnpackImage(
    uintptr_t ImageBase,
    const VGM_PACK_DESCRIPTOR *Descriptor,
    bool SimulateOnly
);
const VGM_PACK_DESCRIPTOR *VgmGetEmbeddedPackDescriptor(void);
const VGM_PACK_CHUNK_ENTRY *VgmGetEmbeddedChunkTable(uint32_t *OutCount);

/* nirvana_callback.c */
NTSTATUS VgmNirvanaInitialize(bool InstallProcessCallback);
void VgmNirvanaShutdown(void);
void VgmNirvanaRegisterObserver(PFN_VGM_SYSCALL_OBSERVER Callback, void *UserContext);
void VgmNirvanaDispatchInspection(uintptr_t PostSyscallRip, uint64_t ReturnValue);
bool VgmProbeHardwareDebugRegisters(void);
const VGM_NIRVANA_STATE *VgmGetNirvanaState(void);

/* crypto_hash.c */
void VgmSha1Compute(const void *Data, size_t Len, uint8_t Digest[20]);
void VgmSha256Compute(const void *Data, size_t Len, uint8_t Digest[32]);
void VgmSha512Compute(const void *Data, size_t Len, uint8_t Digest[64]);
uint64_t VgmXxh64Compute(const void *Data, size_t Len, uint64_t Seed);
VGM_XXH128_HASH VgmXxh3_128Compute(const void *Data, size_t Len, uint64_t Seed);
int VgmFormatUnicodeString(wchar_t *OutBuf, size_t MaxChars, const wchar_t *Format, ...);

/* crash_handler.c */
void VgmInstallCrashHandler(void);
LONG WINAPI VgmUnhandledExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo);
bool VgmWriteCrashMinidump(EXCEPTION_POINTERS *ExceptionInfo, bool FullDump, wchar_t *OutPath, size_t MaxChars);

/* fiber_ipc_net.c */
bool VgmRunCooperativeScheduler(uint32_t MaxIterations);
bool VgmIpcPollNamedPipe(const wchar_t *PipeName, VGM_HEARTBEAT_PACKET *Packet);
bool VgmNetPollTelemetrySocket(VGM_HEARTBEAT_PACKET *Packet);
bool VgmGenerateRandomBytes(uint8_t *Buffer, size_t Length);

#ifdef __cplusplus
}
#endif

#endif /* VGM_H */
