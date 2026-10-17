/*
 * vgk.h
 * Riot Vanguard Kernel-Mode Driver (vgk.sys v1.19.1-18+20261001.143443) Master Header
 * Reconstructed from REA (Ghidra 12.1.4) analysis of C:\Program Files\Riot Vanguard\vgk.sys
 * SHA-256: 8ba32cfbc4a07fd41a082344e23e312515ffafef0e568ba72e7d33e83b558f13
 * ImageBase: 0x140000000 | Packer EP RVA: 0x061C213C | Exports: 'Egg' (0x140368), 'Egg2' (0x14062C)
 */

#ifndef VGK_MASTER_H
#define VGK_MASTER_H

#include <ntifs.h>
#include <ntddk.h>
#include <ntimage.h>
#include <bcrypt.h>
#include <ntstrsafe.h>
#include <stdarg.h>
#include <intrin.h>

#pragma warning(disable : 4201) /* nameless struct/union */
#pragma warning(disable : 4152) /* function/data pointer conversion */
#pragma warning(disable : 4214) /* bitfield types other than int */
#pragma warning(disable : 4996) /* ExAllocatePoolWithTag / HalGetBusDataByOffset */

/* =========================================================================
 * BINARY METADATA & CONSTANTS EXTRACTED FROM vgk.sys v1.19.1.18
 * ========================================================================= */

#define VGK_VERSION_STRING_W        L"1.19.1-18+20261001.143443 ; Thu Oct  1 14:35:29 2026\n" /* RVA 0x1598C0 */
#define VGK_LEGACY_BANNER_VER_W     L"1.18.1-0 ; Fri Jul 17 06:39:05 2026\n"                  /* RVA 0x162810 */
#define VGK_PDB_EASTER_EGG_A        "https://imgur.com/a/PiWvsB0"                             /* RVA 0x163798 */
#define VGK_POOL_TAG                'TKGV'  /* "VGKT" (0x544B4756) from .text RVA 0x1565C0 */
#define VGK_RTMP_POOL_TAG           'pMTR'  /* 0x52544D70 from Rule 4 (RVA 0x15A100) */

/* Constant Pool Strings at End of .text (RVA 0x156500..0x156E40) */
#define VGK_BOOT_LOG_DAT_W          L"\\SystemRoot\\vgk0001.dat"        /* RVA 0x156500 */
#define VGK_DEVICE_NAME_W           L"\\Device\\vgk"
#define VGK_DOS_DEVICE_NAME_W       L"\\DosDevices\\vgk"
#define VGK_GOBBLEDYGOOK_DEVICE_W   L"\\Device\\gobbledygook"           /* RVA 0x156E10 */
#define VGK_SYSDISPAFFINITY_W       L"SysDispAffinity"
#define VGK_KE_SET_BASE_PRIORITY_W  L"KeSetActualBasePriorityThread"    /* RVA 0x156BE0 */
#define VGK_IOMMU_DEVICE_A          "\\Device\\IommuChipsetEnabler"     /* RVA 0x1565E0 */
#define VGK_IOMMU_DEVICE_W          L"\\Device\\IommuChipsetEnabler"
#define VGK_HALP_IOMMU_FN_A         "HalpDmaGetIommuInterface"          /* RVA 0x156600 */
#define VGK_VGC_EXE_A               "vgc.exe"                           /* RVA 0x156A00 */
#define VGK_VALORANT_EXE_A          "VALORANT-Win64-Shipping.exe"       /* RVA 0x156A10 */
#define VGK_WIN32KBASE_SYS_A        "win32kbase.sys"                    /* RVA 0x156DB0 */
#define VGK_WIN32KFULL_SYS_A        "win32kfull.sys"                    /* RVA 0x156DC0 */
#define VGK_MS_PRIMITIVE_PROVIDER_W L"Microsoft Primitive Provider"     /* RVA 0x1568D0 */
#define VGK_OBJECT_LENGTH_W         L"ObjectLength"                     /* RVA 0x156770 */
#define VGK_HASH_DIGEST_LENGTH_W    L"HashDigestLength"                 /* RVA 0x156790 */
#define VGK_RSA_ALG_W               L"RSA"                              /* RVA 0x156910 */
#define VGK_RSAPUBLICBLOB_W         L"RSAPUBLICBLOB"                    /* RVA 0x156920 */
#define VGK_OB_ALTITUDE_W           L"388400"

/* PKCS#1 Signature OIDs from RVA 0x156830..0x1568A0 */
#define VGK_OID_SHA1_RSA_A          "1.2.840.113549.1.1.5"              /* RVA 0x156830 */
#define VGK_OID_SHA256_RSA_A        "1.2.840.113549.1.1.11"             /* RVA 0x156850 */
#define VGK_OID_SHA384_RSA_A        "1.2.840.113549.1.1.12"             /* RVA 0x156870 */
#define VGK_OID_SHA512_RSA_A        "1.2.840.113549.1.1.13"             /* RVA 0x156890 */

#define VGK_MAX_CPUS                64
#define VGK_STACK_CAPTURE_DEPTH     32
#define VGK_GUARDED_REGION_SIZE     (2ULL * 1024ULL * 1024ULL)          /* 2MB Guarded MDL */
#define VGK_VGTRACE_SLOT_COUNT      64
#define VGK_VGTRACE_SLOT_SIZE       0x200
#define VGK_VGTRACE_MAX_CONSUMERS   8
#define VGK_PROFILER_MAX_BUCKETS    16
#define VGK_HOOK_TRAMPOLINE_COUNT   8

/* Access Mask Bits for ObRegisterCallbacks Handle Stripping */
#define VGK_PROCESS_STRIP_MASK      (0x0001 | 0x0002 | 0x0008 | 0x0010 | 0x0020 | 0x0040 | 0x0800)
#define VGK_THREAD_STRIP_MASK       (0x0001 | 0x0002 | 0x0008 | 0x0010 | 0x0200)

/* IOCTL Codes for vgc.exe <-> vgk.sys IPC */
#define IOCTL_VGK_BIND_VGC          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_PROTECT_PID       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_POLL_TELEMETRY    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_QUERY_HWID        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_MAP_GUARDED_VIEW  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_RUN_FULL_SWEEP    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_GENERATE_PROOF    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma pack(push, 1)
typedef struct _VGK_PROOF_CHALLENGE {
    ULONG64 ServerNonce;        /* Nonce de 64 bits recebido do servidor Riot */
    UCHAR   DriverProof[64];    /* Saída: 64 bytes gerados (HMAC Kernel + TPM 2.0) */
} VGK_PROOF_CHALLENGE, *PVGK_PROOF_CHALLENGE;
#pragma pack(pop)

/* Telemetry Violation Flags (MC / KSS / BBL / DCL / DF / WIC + Sections 1-25) */
#define VGK_FLAG_NONE               0x00000000
#define VGK_FLAG_KSS_UNBACKED_RIP   0x00000001  /* KSS %u: Kernel Stack Scan unbacked RIP */
#define VGK_FLAG_KSS_SPOOFED_FRAME  0x00000002  /* KSS %u: RVA == BeginAddress unwind mismatch */
#define VGK_FLAG_KSS_DR7_ACTIVE     0x00000004  /* KSS %u: Hardware debug registers active */
#define VGK_FLAG_BBL_BYOVD_MATCH    0x00000008  /* BBL %u: Boot Blacklist signature match */
#define VGK_FLAG_EBBL_EXT_MATCH     0x00000010  /* EBBL %u: Extended Boot Blacklist match */
#define VGK_FLAG_MC_PTE_UNBACKED    0x00000020  /* MC %08x: Executable PTE outside module list */
#define VGK_FLAG_MC_HASH_MISMATCH   0x00000040  /* MC %08x: On-disk vs In-memory SHA256 mismatch */
#define VGK_FLAG_DCL_DEVICE_FOUND   0x00000080  /* DCL %u: Blacklisted \Device or \DosDevices */
#define VGK_FLAG_EDCL_FILTER_FOUND  0x00000100  /* EDCL %u: Unauthorized AttachedDevice filter */
#define VGK_FLAG_DF_DRIVER_FLAG     0x00000200  /* DF %d: Driver characteristic falsified */
#define VGK_FLAG_WIC_ANOMALY        0x00000400  /* WIC %u: Work Item Check integrity anomaly */
#define VGK_FLAG_IOMMU_DISABLED     0x00000800  /* §4: IOMMU / HalpDmaGetIommuInterface absent */
#define VGK_FLAG_HV_FOREIGN_VENDOR  0x00001000  /* §3: Non-Hyper-V synthetic hypervisor detected */
#define VGK_FLAG_HV_MSR_HOOKED      0x00002000  /* §3: IA32_LSTAR / SYSENTER / GS_BASE hook */
#define VGK_FLAG_HV_TIMING_ANOMALY  0x00004000  /* §3/§10: RDTSC/CPUID/INVLPG VM-exit latency */
#define VGK_FLAG_KERNEL_DEBUGGER    0x00008000  /* §2/§10: KdDebuggerEnabled or TESTSIGNING */
#define VGK_FLAG_PCI_DMA_FLAGGED    0x00010000  /* §4/§17: Xilinx 10EE / VMware / VirtIO PCI */
#define VGK_FLAG_SMBIOS_MISMATCH    0x00020000  /* §15/§19: SMBIOS Type 4 vs CPUID mismatch */
#define VGK_FLAG_TPM_ATTEST_FAIL    0x00040000  /* §14.1: TPM 2.0 EK / PCR[7] attestation fail */
#define VGK_FLAG_USB_MCU_DETECTED   0x00080000  /* §25: Arduino/Pico/KMBox VID/PID or CDC+HID */
#define VGK_FLAG_USB_DUAL_MOUSE     0x00100000  /* §25.3: Dual-mouse arbitration / jitter spike */

/* =========================================================================
 * UNDOCUMENTED KERNEL DECLARATIONS MATCHING vgk.sys v1.19.1.18 IMPORT TABLE
 * ========================================================================= */

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation                  = 0x00,
    SystemProcessInformation                = 0x05,
    SystemModuleInformation                 = 0x0B,
    SystemHandleInformation                 = 0x10,
    SystemKernelDebuggerInformation         = 0x23,
    SystemFirmwareTableInformation          = 0x4C,
    SystemBootEnvironmentInformation        = 0x5A,
    SystemCodeIntegrityInformation          = 0x67,
    SystemIsolatedUserModeInformation       = 0x9C
} SYSTEM_INFORMATION_CLASS;

typedef struct _RTL_PROCESS_MODULE_INFORMATION {
    HANDLE Section;
    PVOID  MappedBase;
    PVOID  ImageBase;
    ULONG  ImageSize;
    ULONG  Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR  FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

typedef enum _KAPC_ENVIRONMENT {
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment,
    InsertApcEnvironment
} KAPC_ENVIRONMENT;

typedef VOID (NTAPI *PKNORMAL_ROUTINE)(
    _In_opt_ PVOID NormalContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
);

typedef VOID (NTAPI *PKKERNEL_ROUTINE)(
    _In_ PKAPC Apc,
    _Inout_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_ PVOID *NormalContext,
    _Inout_ PVOID *SystemArgument1,
    _Inout_ PVOID *SystemArgument2
);

typedef VOID (NTAPI *PKRUNDOWN_ROUTINE)(
    _In_ PKAPC Apc
);

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    ULONG UnwindData;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef union _HARDWARE_PTE_X64 {
    ULONG64 Value;
    struct {
        ULONG64 Present        : 1;
        ULONG64 Write          : 1;
        ULONG64 Owner          : 1;
        ULONG64 WriteThrough   : 1;
        ULONG64 CacheDisable   : 1;
        ULONG64 Accessed       : 1;
        ULONG64 Dirty          : 1;
        ULONG64 LargePage      : 1;
        ULONG64 Global         : 1;
        ULONG64 CopyOnWrite    : 1;
        ULONG64 Prototype      : 1;
        ULONG64 Reserved0      : 1;
        ULONG64 PageFrameNumber: 36;
        ULONG64 Reserved1      : 4;
        ULONG64 SoftwareWsIndex: 11;
        ULONG64 NoExecute      : 1;
    } Bits;
} HARDWARE_PTE_X64, *PHARDWARE_PTE_X64;

/* Kernel Exports Imported Directly by vgk.sys v1.19.1.18 (INIT RVA 0x1AC000) */
NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI NTSTATUS NTAPI ZwOpenDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI NTSTATUS NTAPI ZwQueryDirectoryObject(
    _In_ HANDLE DirectoryHandle,
    _Out_writes_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_ BOOLEAN RestartScan,
    _Inout_ PULONG Context,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI VOID NTAPI KeInitializeApc(
    _Out_ PRKAPC Apc,
    _In_ PRKTHREAD Thread,
    _In_ KAPC_ENVIRONMENT Environment,
    _In_ PKKERNEL_ROUTINE KernelRoutine,
    _In_opt_ PKRUNDOWN_ROUTINE RundownRoutine,
    _In_opt_ PKNORMAL_ROUTINE NormalRoutine,
    _In_opt_ KPROCESSOR_MODE ProcessorMode,
    _In_opt_ PVOID NormalContext
);

NTSYSAPI BOOLEAN NTAPI KeInsertQueueApc(
    _Inout_ PRKAPC Apc,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2,
    _In_ KPRIORITY Increment
);

NTSYSAPI PRUNTIME_FUNCTION NTAPI RtlLookupFunctionEntry(
    _In_ DWORD64 ControlPc,
    _Out_ PDWORD64 ImageBase,
    _Inout_opt_ PVOID HistoryTable
);

NTSYSAPI NTSTATUS NTAPI ObReferenceObjectByName(
    _In_ PUNICODE_STRING ObjectName,
    _In_ ULONG Attributes,
    _In_opt_ PACCESS_STATE AccessState,
    _In_opt_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode,
    _Inout_opt_ PVOID ParseContext,
    _Out_ PVOID *Object
);

extern POBJECT_TYPE *IoDriverObjectType;

/* =========================================================================
 * SECTION #2 ("        " / $$PUKE @ RVA 0x157000) PTE CLOAK & TLB FLUSH
 * Decompiled via REA analyze_function (ev_a75091e3e4d504b6ef69dce331014d2e0ddc1017be476a79167d733fac68288b)
 * ========================================================================= */

typedef struct _VGK_PUKE_CONTEXT {
    volatile LONG   StateFlags;     /* +0x00: Bit 0 = Active Lock, Bit 15 = Cloak Completed */
    ULONG           PteCount;       /* +0x04: Number of PTE pointers in array */
    PULONG64        PteArray[64];   /* +0x08: Array of kernel PTE virtual addresses */
} VGK_PUKE_CONTEXT, *PVGK_PUKE_CONTEXT;

/* =========================================================================
 * NEW IN v1.19.1.18: 600-BYTE (0x258) TRUSTED ROOT CA RECORD (.data RVA 0x168260..0x16C1A8)
 * ========================================================================= */

#pragma pack(push, 1)
typedef struct _VGK_TRUSTED_CA_RECORD {
    LIST_ENTRY  ListEntry;          /* +0x000: Doubly-linked list head/node */
    UCHAR       EnabledFlag;        /* +0x010: 0x00 = valid entry, 0xFF = sentinel */
    CHAR        SubjectName[64];    /* +0x011: Null-terminated Root CA Common Name */
    UCHAR       KeyType;            /* +0x051: 1 = RSA-2048 (256B), 2 = RSA-4096 (512B) */
    UCHAR       Modulus[512];       /* +0x052: Raw big-endian RSA public key modulus */
    UCHAR       ReservedTail[6];    /* +0x252..0x257: Padding to 0x258 (600 bytes) */
} VGK_TRUSTED_CA_RECORD, *PVGK_TRUSTED_CA_RECORD;
#pragma pack(pop)

/* =========================================================================
 * NEW IN v1.19.1.18: VGTRACE REAL-TIME KERNEL-TO-USER MDL RING-BUFFER
 * Reconstructed via REA from FUN_1401318f0, FUN_14013242c, FUN_1401326cc, FUN_14013da44
 * ========================================================================= */

typedef struct _VGK_VGTRACE_SLOT {
    volatile LONG   SeqLockVersion; /* +0x00: Odd while writing, Even when committed */
    ULONG           Reserved0;      /* +0x04 */
    ULONG64         SequenceNumber; /* +0x08: Monotonic event sequence */
    ULONG64         TimestampQpc;   /* +0x10: High-resolution QPC timestamp */
    ULONG64         ThreadContext;  /* +0x18: Caller thread / CPU context */
    ULONG           PayloadSize;    /* +0x20: Byte length of payload at +0xB0 */
    ULONG           EventSubCode;   /* +0x24: Telemetry subcode */
    ULONG64         BufferSequence; /* +0x28: Active buffer sequence */
    UCHAR           HeaderMeta[128];/* +0x30..0xAF: Context registers & metadata */
    UCHAR           Payload[336];   /* +0xB0..0x1FF: Raw telemetry payload */
} VGK_VGTRACE_SLOT, *PVGK_VGTRACE_SLOT;

typedef struct _VGK_VGTRACE_RING_CONTROL {
    volatile ULONG64 WriteIndex;        /* +0x00 (*puVar2) */
    volatile ULONG64 ReadIndex;         /* +0x08 (puVar2[1]) */
    volatile ULONG64 OverwriteCount;    /* +0x10 (puVar2[2]) */
    volatile ULONG64 DeliveredCount;    /* +0x18 (puVar2[3]) */
    ULONG            CapacitySlots;     /* +0x20 (puVar2[4]) */
    ULONG            SlotStrideBytes;   /* +0x24 */
} VGK_VGTRACE_RING_CONTROL, *PVGK_VGTRACE_RING_CONTROL;

typedef struct _VGK_VGTRACE_CONSUMER {
    LIST_ENTRY      ListEntry;          /* +0x00 */
    PVOID           CallbackRoutine;    /* +0x10 */
    PVOID           CallbackContext;    /* +0x18 */
    PKEVENT         NotifyEvent;        /* +0x20: Signaled via KeSetEvent in FlushToRealtime */
    volatile LONG   RefCount;           /* +0x28: Atomic consumer refcount */
    volatile LONG   OverflowFlag;       /* +0x2C: Set to 1 when ring wraps unread slots */
    PMDL            ConsumerMdl;        /* +0x30: Mapped via BuildConsumer */
    PVOID           KernelVa;           /* +0x38 */
    PVOID           UserVa;             /* +0x40 */
} VGK_VGTRACE_CONSUMER, *PVGK_VGTRACE_CONSUMER;

typedef struct _VGK_VGTRACE_LOGGER {
    ULONG                       LoggerId;           /* +0x00 (*param_1) */
    BOOLEAN                     CollectionActive;   /* +0x04 */
    ULONG                       FlushTimerMs;       /* +0x08 */
    ULONG                       FlushThreshold;     /* +0x0C */
    KSPIN_LOCK                  ConsumerListLock;   /* +0x18 (param_1 + 0x1e) */
    LIST_ENTRY                  ConsumerListHead;   /* +0x20 (param_1 + 0x70) */
    PVGK_VGTRACE_RING_CONTROL   RingControl;        /* +0x30 (param_1 + 0x6e) */
    PVGK_VGTRACE_SLOT           RingSlotsKernelVa;  /* +0x38 (param_1 + 0x66) */
    PMDL                        RingMdl;            /* +0x40 */
    ULONG                       SlotMask;           /* +0x48 (param_1[0x6a] - 1) */
    ULONG                       MaxPayloadBytes;    /* +0x4C (param_1[0x6c]) */
} VGK_VGTRACE_LOGGER, *PVGK_VGTRACE_LOGGER;

/* =========================================================================
 * NEW IN v1.19.1.18: HIGH-RESOLUTION QPC EXECUTION PROFILER (RVA 0x1406DC)
 * Logs "ID;Samples;Total(ms);Avg(ms);Max(ms)\n" (RVA 0x159EE0)
 * ========================================================================= */

typedef struct _VGK_PROFILER_BUCKET {
    ULONG           BucketId;
    volatile LONG64 Samples;
    volatile LONG64 TotalMicroseconds;
    volatile LONG64 MaxMicroseconds;
} VGK_PROFILER_BUCKET, *PVGK_PROFILER_BUCKET;

/* =========================================================================
 * DYNAMIC XOR-ENCRYPTED API DISPATCH TABLE
 * ========================================================================= */

typedef enum _VGK_API_SLOT {
    VGK_API_KE_REGISTER_NMI_CALLBACK = 0,
    VGK_API_KE_DEREGISTER_NMI_CALLBACK,
    VGK_API_OB_REGISTER_CALLBACKS,
    VGK_API_OB_UNREGISTER_CALLBACKS,
    VGK_API_PS_SET_CREATE_PROCESS_NOTIFY_EX,
    VGK_API_PS_SET_CREATE_THREAD_NOTIFY,
    VGK_API_PS_REMOVE_CREATE_THREAD_NOTIFY,
    VGK_API_PS_SET_LOAD_IMAGE_NOTIFY,
    VGK_API_PS_REMOVE_LOAD_IMAGE_NOTIFY,
    VGK_API_HALP_DMA_GET_IOMMU_INTERFACE,
    VGK_API_SYS_DISP_AFFINITY,
    VGK_API_KE_SET_ACTUAL_BASE_PRIORITY_THREAD, /* RVA 0x156BE0 in v1.19.1.18 */
    VGK_API_SLOT_COUNT
} VGK_API_SLOT;

/* =========================================================================
 * EXACT 32-BYTE BYOVD SIGNATURE STRUCTURE (RVA 0x15A5C0, 0x15AF40, 0x15BEC0)
 * ========================================================================= */

typedef enum _VGK_SIGNATURE_CATEGORY {
    VGK_SIG_CAT_BYOVD_DRIVER    = 8,    /* Type 8 in vgk.sys */
    VGK_SIG_CAT_KERNEL_EXPLORER = 9     /* Type 9 in vgk.sys */
} VGK_SIGNATURE_CATEGORY;

typedef struct _VGK_BYOVD_SIGNATURE_RULE {
    ULONG64     RuleId;         /* +0x00: 1 .. 30 */
    ULONG64     PatternLength;  /* +0x08: Exact byte length */
    const VOID *PatternData;    /* +0x10: Pointer to pattern in .rdata */
    ULONG64     Category;       /* +0x18: 8 or 9 */
} VGK_BYOVD_SIGNATURE_RULE, *PVGK_BYOVD_SIGNATURE_RULE;

/* =========================================================================
 * CNG HASH ALGORITHM SELECTOR (RVA 0x156730..0x156760 IN v1.19.1.18)
 * ========================================================================= */

typedef enum _VGK_CNG_HASH_TYPE {
    VGK_HASH_SHA1   = 1,    /* L"SHA1"   (RVA 0x156740) */
    VGK_HASH_SHA256 = 2,    /* L"SHA256" (RVA 0x156730) */
    VGK_HASH_SHA384 = 3,    /* L"SHA384" (RVA 0x156750) */
    VGK_HASH_SHA512 = 4     /* L"SHA512" (RVA 0x156760) */
} VGK_CNG_HASH_TYPE;

/* =========================================================================
 * TELEMETRY LOG EVENT IDS (MAPPING .rdata RVA 0x162F00..0x1635B8 IN v1.19.1.18)
 * ========================================================================= */

typedef enum _VGK_LOG_EVENT_ID {
    VGK_EVENT_VERSION_BANNER    = 0,    /* "1.19.1-18+20261001.143443 ; Thu Oct  1 14:35:29 2026\n" (0x1598C0) */
    VGK_EVENT_KSS               = 1,    /* "KSS %u\n"  - Kernel Stack Scan (0x1631A8 -> 0x159CE0) */
    VGK_EVENT_BBL               = 2,    /* "BBL %u\n"  - Boot Blacklist (0x1631B0 -> 0x159CF0) */
    VGK_EVENT_EBBL              = 3,    /* "EBBL %u\n" - Extended Boot Blacklist (0x1631B8 -> 0x159D00) */
    VGK_EVENT_MC                = 4,    /* "MC %08x\n" - Memory Check (0x1631C0 -> 0x159D18) */
    VGK_EVENT_DCL               = 5,    /* "DCL %u\n"  - Device Check List (0x1631D8 -> 0x159D50) */
    VGK_EVENT_EDCL              = 6,    /* "EDCL %u\n" - Extended Device Check List (0x1631E0 -> 0x159D60) */
    VGK_EVENT_DF                = 7,    /* "DF %d\n"   - Driver Flag (0x1631F8 -> 0x159D78) */
    VGK_EVENT_WIC               = 8,    /* "WIC %u\n"  - Work Item Check (0x163328 -> 0x159E68) */
    VGK_EVENT_PTR_DETAIL        = 9,    /* "%p ; %S ; 0x%X\n" (0x1631C8 -> 0x159D30) */
    VGK_EVENT_MEM_RANGE         = 10,   /* "0x%p ; %llu ; %#010x ; %#010x\n" (0x163210 -> 0x159DA0) */
    VGK_EVENT_TIMING_PAIR       = 11,   /* "%llu ; %llu\n" (0x163368 -> 0x159E78) */
    VGK_EVENT_PROFILER_HEADER   = 12,   /* "ID;Samples;Total(ms);Avg(ms);Max(ms)\n" (0x163428 -> 0x159EE0) */
    VGK_EVENT_PROFILER_ROW      = 13,   /* "%u;%llu;%llu;%llu;%llu\n" (0x163430 -> 0x159F30) */
    VGK_EVENT_MAX
} VGK_LOG_EVENT_ID;

/* =========================================================================
 * PER-CORE IPI / NMI CAPTURE RECORD & GLOBAL DRIVER CONTEXT
 * ========================================================================= */

typedef struct _VGK_CPU_STACK_SAMPLE {
    volatile LONG   SampleValid;
    ULONG           ProcessorIndex;
    HANDLE          ProcessId;
    HANDLE          ThreadId;
    ULONG64         InstructionPointer;
    ULONG64         StackPointer;
    ULONG64         Cr3Register;
    ULONG64         Dr7Register;
    ULONG           CapturedFramesCount;
    ULONG64         StackWords[VGK_STACK_CAPTURE_DEPTH];
} VGK_CPU_STACK_SAMPLE, *PVGK_CPU_STACK_SAMPLE;

typedef struct _VGK_HARDWARE_PROFILE {
    /* §15 / §19 SMBIOS Physical & RSMB Identifiers */
    UCHAR           SmbiosUuid[16];
    CHAR            SmbiosBiosVendor[64];
    CHAR            SmbiosBiosVersion[64];
    CHAR            SmbiosSystemManufacturer[64];
    CHAR            SmbiosSystemProduct[64];
    CHAR            SmbiosSystemSerial[64];
    CHAR            SmbiosBaseboardSerial[64];
    CHAR            SmbiosChassisSerial[64];
    CHAR            SmbiosProcessorBrand[64];
    CHAR            SmbiosDimmSerials[4][32];
    ULONG           SmbiosDimmPopulatedCount;

    /* §3 / §16 CPUID & Hypervisor Leaves */
    CHAR            CpuVendor[13];
    CHAR            CpuBrandString[49];
    CHAR            HypervisorVendor[13];
    ULONG           ProcessorId;
    ULONG           HypervisorInterfaceSig;
    ULONG64         QpcFrequency;
    ULONG64         TscCpuidRatio;
    ULONG64         InvlpgLatencyCycles;
    ULONG64         LstarMsrValue;

    /* §14 / §14.1 TPM 2.0 TBS (EK & PCRs), NVMe, NDIS MAC & Registry GUIDs */
    UCHAR           TpmEkPubSha256[32];
    UCHAR           TpmEkCertSha256[32];
    UCHAR           TpmPcr0Sha256[32];
    UCHAR           TpmPcr7Sha256[32];
    CHAR            NvmeSerialNumber[64];
    UCHAR           NicPermanentMac[6];
    WCHAR           MachineGuid[40];
    WCHAR           HwProfileGuid[40];

    /* §2 / §4 / §17 / §24 / §25 Integrity, PCI/DMA, ACPI & USB HID State */
    UCHAR           VgkImageSha256[32];
    UCHAR           NtoskrnlHeaderSha256[32];
    ULONG           AcpiDmarSignature;
    ULONG           AcpiMadtSignature;
    ULONG           FlaggedPciDeviceCount;
    ULONG           FlaggedUsbMcuCount;
    ULONG           ActiveMouseDeviceCount;
    BOOLEAN         IommuInterfacePresent;
    BOOLEAN         HypervisorBitPresent;
    BOOLEAN         VbsEnabled;
    BOOLEAN         HvciEnabled;
    BOOLEAN         SecureBootEnabled;
    BOOLEAN         TestSigningEnabled;
    BOOLEAN         KernelDebuggerPresent;
} VGK_HARDWARE_PROFILE, *PVGK_HARDWARE_PROFILE;

typedef struct _VGK_TELEMETRY_PACKET {
    ULONG64         SequenceId;
    ULONG64         PreciseTimestamp;
    ULONG           ViolationFlags;
    ULONG           LastMatchedRuleId;
    ULONG           ActiveGamePid;
    ULONG           IpiSampleCount;
    ULONG           WorkItemPassCount;
} VGK_TELEMETRY_PACKET, *PVGK_TELEMETRY_PACKET;

typedef struct _VGK_GLOBAL_CONTEXT {
    PDRIVER_OBJECT                  DriverObject;
    PDEVICE_OBJECT                  VgkDeviceObject;
    PDEVICE_OBJECT                  GobbledygookDeviceObject;
    KSPIN_LOCK                      TelemetryLock;
    KMUTEX                          StateMutex;

    /* Dynamic XOR-Encrypted Kernel API Table */
    ULONG64                         ApiXorKey;
    ULONG64                         EncryptedApiTable[VGK_API_SLOT_COUNT];

    /* Protected Process State */
    volatile HANDLE                 VgcProcessId;
    volatile HANDLE                 ValorantProcessId;

    /* Registered Security & BugCheck Handles */
    PVOID                           ObCallbackHandle;
    PVOID                           NmiCallbackHandle;
    PEX_RUNDOWN_REF_CACHE_AWARE     CacheAwareRundownRef;   /* New in v1.19.1.18 (RVA 0x1AC5AE) */
    KBUGCHECK_REASON_CALLBACK_RECORD BugCheckRecord;        /* New in v1.19.1.18 (RVA 0x1AC644) */
    BOOLEAN                         BugCheckRegistered;
    BOOLEAN                         ProcessNotifyActive;
    BOOLEAN                         ThreadNotifyActive;
    BOOLEAN                         ImageNotifyActive;

    /* Guarded Region MDL State */
    PMDL                            GuardedRegionMdl;
    PVOID                           GuardedRegionKernelVa;
    PVOID                           GuardedRegionUserVa;

    /* Section #2 ($$PUKE @ RVA 0x157000) & Atomic Hook Trampolines (RVA 0x155AD0..0x155D9F) */
    VGK_PUKE_CONTEXT                PukeContext;
    volatile LONG64                 ActiveHookRefCount;     /* RVA 0x18FF08 */
    PVOID                           OriginalHookTargets[VGK_HOOK_TRAMPOLINE_COUNT]; /* RVA 0x1985B8 */

    /* New in v1.19.1.18: vgtrace Real-Time Ring-Buffer & QPC Profiler (RVA 0x19CB58) */
    VGK_VGTRACE_LOGGER              VgtraceLogger;
    VGK_PROFILER_BUCKET             ProfilerBuckets[VGK_PROFILER_MAX_BUCKETS];

    /* DPC, Timer & WorkItem Pipeline */
    KTIMER                          PeriodicTimer;
    KDPC                            PeriodicDpc;
    PIO_WORKITEM                    PeriodicWorkItem;
    volatile LONG                   WorkItemActive;
    volatile BOOLEAN                Unloading;

    /* Telemetry Counters & Violation State */
    volatile LONG                   ViolationFlags;
    volatile LONG                   LastMatchedRuleId;
    volatile LONG64                 HeartbeatSequence;
    volatile LONG                   IpiSampleCount;
    volatile LONG                   WorkItemPassCount;
    ULONG                           Pml4SweepCursor;

    /* Hardware Profile & Per-CPU Stack Captures */
    VGK_HARDWARE_PROFILE            HardwareProfile;
    VGK_CPU_STACK_SAMPLE            CpuSamples[VGK_MAX_CPUS];
} VGK_GLOBAL_CONTEXT, *PVGK_GLOBAL_CONTEXT;

extern VGK_GLOBAL_CONTEXT g_Vgk;
extern const VGK_BYOVD_SIGNATURE_RULE g_VgkByovdRules[30];
extern const PCWSTR g_VgkLogFormatTable[VGK_EVENT_MAX];
extern const PCSTR  g_VgkTrustedRootCas[];
extern const ULONG  g_VgkTrustedRootCaCount;

/* =========================================================================
 * SUBSYSTEM MODULE PROTOTYPES
 * ========================================================================= */

/* driver.c */
__declspec(dllexport) NTSTATUS NTAPI Egg(_In_opt_ PVOID Context);
__declspec(dllexport) NTSTATUS NTAPI Egg2(_In_opt_ PVOID Context);
VOID VgkInitializeApiXorTable(VOID);
PVOID VgkResolveEncryptedApi(_In_ VGK_API_SLOT Slot);
BOOLEAN VgkQuerySysDispAffinity(_In_ PVOID Context, _Out_ PULONG OutAffinity);
PRTL_PROCESS_MODULES VgkAcquireModuleList(VOID);
BOOLEAN VgkIsAddressBackedByModule(
    _In_ PRTL_PROCESS_MODULES Modules,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 OutImageBase
);

/* crypto_cng.c (Reconstructed from RVA 0x156730..0x156930 & 0x168260..0x16C1A8) */
NTSTATUS VgkCngComputeDigestByType(
    _In_ VGK_CNG_HASH_TYPE HashType,
    _In_reads_bytes_(DataLength) PUCHAR Data,
    _In_ ULONG DataLength,
    _Out_writes_bytes_(DigestBufferSize) PUCHAR DigestOut,
    _In_ ULONG DigestBufferSize
);
NTSTATUS VgkCngVerifyRsaSignature(
    _In_reads_bytes_(KeyBlobLength) PUCHAR RsaPublicBlob,
    _In_ ULONG KeyBlobLength,
    _In_z_ PCSTR OidString,
    _In_reads_bytes_(HashLength) PUCHAR HashDigest,
    _In_ ULONG HashLength,
    _In_reads_bytes_(SignatureLength) PUCHAR SignatureBytes,
    _In_ ULONG SignatureLength
);
NTSTATUS VgkHashDiskFileHeader(
    _In_ PCWSTR NtFilePath,
    _Out_writes_bytes_(32) PUCHAR OutSha256
);

/* byovd_scanner.c (Reconstructed from RVA 0x15A5C0, 0x15AF40, 0x15BEC0) */
extern const VGK_BYOVD_SIGNATURE_RULE g_VgkByovdRulesMirror1[30];
extern const VGK_BYOVD_SIGNATURE_RULE g_VgkByovdRulesMirror2[30];
VOID VgkScanObjectDirectoriesForByovd(VOID);
VOID VgkScanLoadedModulesAgainstRules(_In_ PRTL_PROCESS_MODULES Modules);
BOOLEAN VgkVerifyByovdRuleTableIntegrity(VOID);

/* ipi_apc_stackwalk.c */
VOID VgkBroadcastIpiStackWalk(VOID);
BOOLEAN VgkNmiInterruptHandler(_In_opt_ PVOID Context, _In_ BOOLEAN Handled);
VOID VgkEvaluateCapturedStacks(_In_ PRTL_PROCESS_MODULES Modules);
NTSTATUS VgkProbeThreadViaKernelApc(_In_ PETHREAD TargetThread);

/* memory_mdl.c (Guarded Region MDL + Section #2 $$PUKE @ 0x157000 + 8 Hook Trampolines @ 0x155AD0) */
NTSTATUS VgkInitializeGuardedRegionMdl(VOID);
VOID VgkFreeGuardedRegionMdl(VOID);
NTSTATUS VgkMapGuardedRegionIntoProcess(_Out_ PVOID *OutUserAddress);
VOID VgkSweepKernelPageTables(_In_ PRTL_PROCESS_MODULES Modules);
VOID VgkVerifyWin32kAndMouseStack(_In_ PRTL_PROCESS_MODULES Modules);
VOID VgkPukeCloakPages(_Inout_ PVGK_PUKE_CONTEXT PukeCtx);
VOID VgkPukeSpinWait(_In_ volatile LONG *LockWord);
BOOLEAN VgkHookPreHandler(
    _In_ ULONG64 Arg1,
    _In_ ULONG Arg2,
    _In_ ULONG64 Arg3,
    _In_ ULONG64 Arg4,
    _Out_opt_ PULONG64 OutResult,
    _In_ ULONG SyscallTag
);
NTSTATUS NTAPI VgkHookTrampoline0(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9);
NTSTATUS NTAPI VgkHookTrampoline1(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9);
NTSTATUS NTAPI VgkHookTrampoline2(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9, _In_ ULONG64 Arg5, _In_ ULONG64 Arg6, _In_ ULONG64 Arg7);
NTSTATUS NTAPI VgkHookTrampoline3(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9, _In_ ULONG64 Arg5, _In_ ULONG64 Arg6);
NTSTATUS NTAPI VgkHookTrampoline4(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9);
NTSTATUS NTAPI VgkHookTrampoline5(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9);
NTSTATUS NTAPI VgkHookTrampoline6(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9);
NTSTATUS NTAPI VgkHookTrampoline7(_In_ ULONG64 Rcx, _In_ ULONG Rdx, _In_ ULONG64 R8, _In_ ULONG64 R9);

/* callbacks.c */
NTSTATUS VgkInstallSecurityCallbacks(VOID);
VOID VgkRemoveSecurityCallbacks(VOID);

/* dpc_telemetry.c (vgtrace Real-Time Ring-Buffer @ 0x1318F0..0x13DA44 + Profiler @ 0x1406DC) */
NTSTATUS VgkInitializeDpcAndWorkItem(VOID);
VOID VgkShutdownDpcAndWorkItem(VOID);
NTSTATUS VgtraceInitializeLogger(VOID);
VOID VgtraceShutdownLogger(VOID);
BOOLEAN VgtraceCheckConsumerCapacity(_In_ PVGK_VGTRACE_LOGGER Logger);
NTSTATUS VgtraceBuildConsumer(
    _Inout_ PVGK_VGTRACE_LOGGER Logger,
    _In_opt_ HANDLE UserEventHandle,
    _Out_ PVOID *OutMappedUserVa
);
NTSTATUS VgtraceFlushActiveBuffers(
    _Inout_ PVGK_VGTRACE_LOGGER Logger,
    _In_ BOOLEAN FlushAll
);
VOID VgtraceNotifyConsumers(
    _Inout_ PVGK_VGTRACE_LOGGER Logger,
    _In_ ULONG64 EventContext
);
VOID VgtraceFlushToRealtime(
    _Inout_ PVGK_VGTRACE_LOGGER Logger,
    _In_reads_bytes_(PayloadSize) const VOID *Payload,
    _In_ ULONG PayloadSize,
    _In_ ULONG EventSubCode
);
ULONG64 VgkQueryElapsedMicroseconds(VOID);
VOID VgkRecordProfilerSample(_In_ ULONG BucketId, _In_ ULONG64 ElapsedMicroseconds);
VOID VgkFlushProfilerMetrics(VOID);
VOID VgkLogTelemetryEvent(
    _In_ VGK_LOG_EVENT_ID EventId,
    _In_ ULONG SubCode,
    _In_opt_ PCWSTR CustomFormat,
    ...
);
NTSTATUS VgkHandleDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);

/* hv_antidebug.c (Sections 2, 3, 10, 16 - Hypervisor, MSR, EPT/TSC & Anti-Debug) */
VOID VgkAuditHypervisorAndAntiDebug(_In_opt_ PRTL_PROCESS_MODULES Modules);

/* pci_dma_acpi.c (Sections 4, 17 - PCI Bus/ECAM DMA Scanner & ACPI Tables) */
VOID VgkAuditPciBusDmaAndAcpi(VOID);

/* hwid_tpm_smbios.c (Sections 14, 14.1, 15, 18, 19, 20.1 - TPM 2.0 TBS, SMBIOS, NVMe, MAC) */
VOID VgkCollectFullHardwareIdentity(VOID);
VOID VgkGenerateDriverProof(_In_ ULONG64 Nonce, _Out_writes_bytes_(64) PUCHAR OutProof);

/* usb_hid_mouse.c (Sections 11, 23, 25 - USB HID Descriptors, MCU Blacklist & Dual-Mouse) */
VOID VgkAuditUsbHidAndMouseTopology(VOID);

#endif /* VGK_MASTER_H */
