/*
 * eac.h
 * Easy Anti-Cheat EOS Kernel-Mode Driver (EasyAntiCheat_EOS.sys v3.2.0) Master Header
 * Reconstructed via REA & algebraic MBA deobfuscation from C:\EasyAntiCheat_EOS.sys
 * SHA-256: e43e586a85b0101512b29508897075d3a12b58567b95d669f9d9016e8ab8ecbb
 * ImageBase: 0x140000000 | DriverEntry RVA: 0x274DF8 | PDB: Core.pdb
 *
 * Key Deobfuscated Routines Reconstructed:
 *   - SplitMix64/Murmur MBA Dispatcher (Constant: 0xC6BC279692B5C323ULL, .rdata 0x140307E90..0x14031B000)
 *   - FUN_14004a85a / FUN_14004a922: 8-byte Physical Memory Reader via MmCopyMemory(MM_COPY_MEMORY_PHYSICAL)
 *   - FUN_1400d9e46 (Key r14 = 0xA1BBB7A6F3A55BF7): 4-Level CR3 Kernel Page Table Walker (PML4->PDPT->PD->PT)
 *   - FUN_1400d97f3 (Key rsi = 0x7C25BF06AA521F89): Custom CONTEXT (0x4D0) + .pdata / Unbacked Leaf Stack Unwinder
 *   - FUN_1401e828c (0x1401e828c..0x1401e85bf): Control Register Reader (CR2, CR3, CR4, CR8) & CR3 PFN Validator
 */

#ifndef EAC_EOS_MASTER_H
#define EAC_EOS_MASTER_H

#pragma warning(disable : 4201) /* nameless struct/union */
#pragma warning(disable : 4152) /* function/data pointer conversion */
#pragma warning(disable : 4214) /* bitfield types other than int */
#pragma warning(disable : 4324) /* structure was padded due to alignment specifier in fltKernel.h */
#pragma warning(disable : 4996) /* ExAllocatePoolWithTag / HalGetBusDataByOffset */

#include <fltKernel.h>
#include <ntifs.h>
#include <ntddk.h>
#include <ntimage.h>
#include <bcrypt.h>
#include <ntstrsafe.h>
#include <stdarg.h>
#include <intrin.h>

#ifndef UNW_FLAG_NHANDLER
#define UNW_FLAG_NHANDLER 0x0
#endif

/* =========================================================================
 * BINARY METADATA, POOL TAGS & CONSTANTS FROM EasyAntiCheat_EOS.sys v3.2.0
 * ========================================================================= */

#define EAC_PRODUCT_VERSION_W       L"3.2.0"
#define EAC_DRIVER_FILENAME_W       L"EasyAntiCheat_EOS.sys"
#define EAC_SERVICE_FILENAME_A      "EasyAntiCheat_EOS.exe"
#define EAC_FORTNITE_EXE_A          "FortniteClient-Win64-Shipping.exe"
#define EAC_FORTNITE_EXE_W          L"FortniteClient-Win64-Shipping.exe"

#define EAC_DEVICE_NAME_W           L"\\Device\\EasyAntiCheat_EOS"
#define EAC_DOS_DEVICE_NAME_W       L"\\DosDevices\\EasyAntiCheat_EOS"
#define EAC_MINIFILTER_ALTITUDE_W   L"328010"
#define EAC_OB_ALTITUDE_W           L"328010"
#define EAC_CM_ALTITUDE_W           L"328010"

/* Pool Tags Observed in EasyAntiCheat_EOS.sys (.text / Section 7) */
#define EAC_POOL_TAG_CORE           'CflC'  /* "ClfC" - Core / Minifilter Context */
#define EAC_POOL_TAG_OB             'btdO'  /* "Obtb" - Object Callback Context */
#define EAC_POOL_TAG_SCAN           'iScP'  /* "PcSi" - Process & Page Table Scanner */
#define EAC_POOL_TAG_CALLBACK       'BCcS'  /* "ScCB" - Security Callback Buffer */
#define EAC_POOL_TAG_WFP_C          'CpfW'  /* "WfpC" - Network Filter Callout */
#define EAC_POOL_TAG_WFP_S          'SpfW'  /* "WfpS" - Network Filter State */

/* Exact SplitMix64/Murmur MBA Constants Extracted from .text Prologues */
#define EAC_MBA_MIX_CONSTANT        0xC6BC279692B5C323ULL
#define EAC_MBA_KEY_PAGEWALK        0xA1BBB7A6F3A55BF7ULL   /* Solved r14 in FUN_1400d9e46 */
#define EAC_MBA_KEY_STACKWALK       0x7C25BF06AA521F89ULL   /* Solved rsi in FUN_1400d97f3 */
#define EAC_MBA_SUB_MMCOPYMEMORY    0xA012EB3CD7034EC2ULL   /* Multiplier in FUN_14004a85a */
#define EAC_MBA_SUB_READPHYS        0x9B82A33C841D6FC5ULL   /* Multiplier in FUN_1400d9e46 */

/* Hardware Paging Masks Decrypted from FUN_1400d9e46 */
#define EAC_PTE_PFN_MASK            0x0000FFFFFFFFF000ULL   /* [rsp+0x20]: (r14 * 0x364191c12506d90) ^ 0xa5a4aa316fa415f0 */
#define EAC_PTE_INDEX_MASK          0x0000000000000FF8ULL   /* [rsp+0x28]: (r14 * -0x785662309b5fe3df) ^ 0x10b8ea93b2191bb9 */
#define EAC_PTE_PRESENT_BIT         0x0000000000000001ULL   /* [rsp+0x50]: (r14 * 0x3c0c8230a3c1656f) ^ 0xcd22250c7c325318 */
#define EAC_PTE_RW_BIT              0x0000000000000002ULL   /* (r14 * 0x0c5ab725662eaf79) ^ -0x622d8b7adc97af43 */
#define EAC_PTE_USER_BIT            0x0000000000000004ULL
#define EAC_PTE_LARGE_PAGE_BIT      0x0000000000000080ULL   /* (r14 * -0x1e5ec25080327f2a) ^ -0x2b6623ba77fe9f06 */
#define EAC_PTE_PRESENT_OR_LARGE    0x0000000000000081ULL   /* (r14 * -0x4aa62068d45f64bb) ^ 0x70940b01c1a05612 */
#define EAC_PTE_NX_BIT              0x8000000000000000ULL   /* (r14 * 0x42e32b56b4953029) ^ -0x44ffd2b1c682f571 */

/* Context Flags Used in FUN_1400d97f3 (0x1400d998d) */
#define EAC_CONTEXT_AMD64_FULL      0x0010000BUL            /* CONTEXT_AMD64 | CONTROL | INTEGER | FLOATING_POINT */

#define EAC_MAX_CPUS                64
#define EAC_MAX_UNWIND_FRAMES       32
#define EAC_MAX_TELEMETRY_CHUNKS    64

/* Access Mask Bits Stripped by ObRegisterCallbacks (§5) */
#define EAC_PROCESS_STRIP_MASK      (0x0001 | 0x0002 | 0x0008 | 0x0010 | 0x0020 | 0x0040 | 0x0200 | 0x0800)
#define EAC_THREAD_STRIP_MASK       (0x0001 | 0x0002 | 0x0008 | 0x0010 | 0x0020 | 0x0100 | 0x0200)

/* IOCTL Codes for EasyAntiCheat_EOS.exe <-> EasyAntiCheat_EOS.sys IPC */
#define IOCTL_EAC_BIND_SERVICE      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EAC_PROTECT_PROCESS   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EAC_POLL_TELEMETRY    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EAC_QUERY_HWID        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EAC_HEARTBEAT         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EAC_RUN_PAGEWALK      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_EAC_EXEC_VM_CHUNK     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Violation Bitmask Flags (Mapping Sections 1-25 of eacfortinite-detection.md) */
#define EAC_VIO_NONE                    0x00000000UL
#define EAC_VIO_UNBACKED_KERNEL_RIP     0x00000001UL /* §2/§8: FUN_1400d97f3 + FUN_1400d9e46 NX=0 unbacked kernel RIP */
#define EAC_VIO_MMCOPY_PHYS_CONFLICT    0x00000002UL /* §8: Active RIP failed MmCopyMemory(MM_COPY_MEMORY_PHYSICAL) */
#define EAC_VIO_MDL_USER_NO_VAD         0x00000004UL /* §8/§11: User PTE Present without EPROCESS->VadRoot backing */
#define EAC_VIO_TEXT_HASH_MISMATCH      0x00000008UL /* §1/§8: On-disk vs in-memory .text section hash mismatch */
#define EAC_VIO_CODE_CAVE_EXEC          0x00000010UL /* §8: Executable PTE inside .data/.rdata section */
#define EAC_VIO_BYOVD_BLACKLIST         0x00000020UL /* §2/§9: Known vulnerable driver / \Device\DBK64 detected */
#define EAC_VIO_REVOKED_OR_UNSIGNED     0x00000040UL /* §2: SoftpubAuthenticode / FlightRoot / AuthRoot failure */
#define EAC_VIO_DISPATCH_TABLE_HOOK     0x00000080UL /* §11/§12: DriverObject->MajorFunction outside module .text */
#define EAC_VIO_THREAD_START_UNBACKED   0x00000100UL /* §5/§6: Thread StartAddress outside signed module bounds */
#define EAC_VIO_MINIFILTER_TAMPER       0x00000200UL /* §7/§24: Unauthorized write/section sync on game/EAC binary */
#define EAC_VIO_REGISTRY_DSE_TAMPER     0x00000400UL /* §2/§7: Testsigning / DisableIntegrityChecks registry edit */
#define EAC_VIO_HV_CPUID_OR_VENDOR      0x00000800UL /* §3/§16: Foreign hypervisor vendor or synthetic CPUID anomaly */
#define EAC_VIO_HV_MSR_HOOKED           0x00001000UL /* §3: IA32_LSTAR / SYSENTER_EIP / GS_BASE readback mismatch */
#define EAC_VIO_HV_EPT_TIMING_SPLIT     0x00002000UL /* §3/§10: RDTSC vs CPUID / INVLPG VM-exit latency spike */
#define EAC_VIO_KERNEL_DEBUGGER         0x00004000UL /* §9/§10: KdDebuggerEnabled, DR7 active, or TESTSIGNING */
#define EAC_VIO_VBS_HVCI_DISABLED       0x00008000UL /* §3: SystemIsolatedUserModeInformation VBS/HVCI disabled */
#define EAC_VIO_PCI_DMA_DEVICE          0x00010000UL /* §4/§17: Xilinx 10EE / VMware / VirtIO / abnormal BAR */
#define EAC_VIO_ACPI_DMAR_MISSING       0x00020000UL /* §3/§4: ACPI DMAR/MADT table missing or manipulated */
#define EAC_VIO_USB_MCU_DETECTED        0x00040000UL /* §25: Arduino/RP2040/KMBox VID/PID or CDC+HID composite */
#define EAC_VIO_DUAL_MOUSE_ARBITRATION  0x00080000UL /* §11/§23/§25: Dual mouse or synthetic MouClass packet jitter */
#define EAC_VIO_HEARTBEAT_TIMEOUT       0x00100000UL /* §12/§18: EasyAntiCheat_EOS.exe heartbeat desync */

/* =========================================================================
 * UNDOCUMENTED KERNEL DECLARATIONS & STRUCTURES
 * ========================================================================= */

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation                  = 0x00,
    SystemProcessInformation                = 0x05,
    SystemModuleInformation                 = 0x0B,
    SystemHandleInformation                 = 0x10,
    SystemKernelDebuggerInformation         = 0x23,
    SystemBigPoolInformation                = 0x42,
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

typedef struct _SYSTEM_BIGPOOL_ENTRY {
    PVOID     VirtualAddress;
    ULONG_PTR NonPaged : 1;
    ULONG_PTR SizeInBytes;
    union {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
} SYSTEM_BIGPOOL_ENTRY, *PSYSTEM_BIGPOOL_ENTRY;

typedef struct _SYSTEM_BIGPOOL_INFORMATION {
    ULONG Count;
    SYSTEM_BIGPOOL_ENTRY AllocatedInfo[1];
} SYSTEM_BIGPOOL_INFORMATION, *PSYSTEM_BIGPOOL_INFORMATION;

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    ULONG UnwindData;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    PM128A FloatingContext[16];
    PULONG64 IntegerContext[16];
} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

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

NTSYSAPI PRUNTIME_FUNCTION NTAPI RtlLookupFunctionEntry(
    _In_ DWORD64 ControlPc,
    _Out_ PDWORD64 ImageBase,
    _Inout_opt_ PVOID HistoryTable
);

NTSYSAPI PEXCEPTION_ROUTINE NTAPI RtlVirtualUnwind(
    _In_ DWORD HandlerType,
    _In_ DWORD64 ImageBase,
    _In_ DWORD64 ControlPc,
    _In_ PRUNTIME_FUNCTION FunctionEntry,
    _Inout_ PCONTEXT ContextRecord,
    _Out_ PVOID *HandlerData,
    _Out_ PDWORD64 EstablisherFrame,
    _Inout_opt_ PKNONVOLATILE_CONTEXT_POINTERS ContextPointers
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

NTSYSAPI PCHAR NTAPI PsGetProcessImageFileName(
    _In_ PEPROCESS Process
);

extern POBJECT_TYPE *IoDriverObjectType;

/* =========================================================================
 * MBA-ENCRYPTED KERNEL API TABLE (.rdata 0x140307E90..0x14031B000)
 * ========================================================================= */

typedef enum _EAC_API_SLOT {
    EAC_API_MM_COPY_MEMORY = 0,                 /* [0x140307E90] -> ntoskrnl!MmCopyMemory */
    EAC_API_RTL_LOOKUP_FUNCTION_ENTRY,          /* [0x140309060] -> ntoskrnl!RtlLookupFunctionEntry */
    EAC_API_RTL_VIRTUAL_UNWIND,                 /* [0x140309068] -> ntoskrnl!RtlVirtualUnwind */
    EAC_API_OB_REGISTER_CALLBACKS,              /* ntoskrnl!ObRegisterCallbacks */
    EAC_API_OB_UNREGISTER_CALLBACKS,            /* ntoskrnl!ObUnRegisterCallbacks */
    EAC_API_PS_SET_CREATE_PROCESS_NOTIFY_EX,    /* ntoskrnl!PsSetCreateProcessNotifyRoutineEx */
    EAC_API_PS_SET_CREATE_THREAD_NOTIFY,        /* ntoskrnl!PsSetCreateThreadNotifyRoutine */
    EAC_API_PS_REMOVE_CREATE_THREAD_NOTIFY,     /* ntoskrnl!PsRemoveCreateThreadNotifyRoutine */
    EAC_API_PS_SET_LOAD_IMAGE_NOTIFY,           /* ntoskrnl!PsSetLoadImageNotifyRoutine */
    EAC_API_PS_REMOVE_LOAD_IMAGE_NOTIFY,        /* ntoskrnl!PsRemoveLoadImageNotifyRoutine */
    EAC_API_CM_REGISTER_CALLBACK_EX,            /* ntoskrnl!CmRegisterCallbackEx */
    EAC_API_CM_UNREGISTER_CALLBACK,             /* ntoskrnl!CmUnRegisterCallback */
    EAC_API_MM_COPY_VIRTUAL_MEMORY,             /* ntoskrnl!MmCopyVirtualMemory */
    EAC_API_SLOT_COUNT
} EAC_API_SLOT;

/* =========================================================================
 * TELEMETRY MULTIARRAY CHUNK STREAMING PROTOCOL (.rdata STRINGS)
 *   "multiarray %s %d->%d : {\n"
 *   "newl+ chunk type %d = %s [%d->%d]\n"
 *   "newl? chunk type %d = %s [%d->%d]\n"
 *   "newl! chunk type %d = %s [%d->%d]\n"
 * ========================================================================= */

typedef enum _EAC_CHUNK_OP {
    EAC_CHUNK_OP_ADD   = 0, /* "newl+ chunk type %d = %s [%d->%d]\n" */
    EAC_CHUNK_OP_QUERY = 1, /* "newl? chunk type %d = %s [%d->%d]\n" */
    EAC_CHUNK_OP_ALERT = 2  /* "newl! chunk type %d = %s [%d->%d]\n" */
} EAC_CHUNK_OP;

typedef struct _EAC_TELEMETRY_CHUNK {
    ULONG        SequenceId;
    ULONG64      TimestampQpc;
    EAC_CHUNK_OP Operation;
    ULONG        ChunkType;
    ULONG        StartIndex;
    ULONG        EndIndex;
    CHAR         ChunkTag[32];
    CHAR         FormattedLine[160];
} EAC_TELEMETRY_CHUNK, *PEAC_TELEMETRY_CHUNK;

/* =========================================================================
 * DEOBFUSCATED STACK UNWIND & CONTROL REGISTER STRUCTURES
 * ========================================================================= */

typedef struct _EAC_UNWIND_FRAME {
    ULONG64 Rip;
    ULONG64 Rsp;
    ULONG64 ImageBase;
    ULONG64 EffectivePteFlags;
    ULONG64 PhysicalAddress;
    BOOLEAN HasRuntimeFunction;
    BOOLEAN IsBackedByModule;
    BOOLEAN IsExecutableUnbacked;
} EAC_UNWIND_FRAME, *PEAC_UNWIND_FRAME;

typedef struct _EAC_UNWIND_RESULT {
    ULONG            FrameCount;
    ULONG            UnbackedExecCount;
    EAC_UNWIND_FRAME Frames[EAC_MAX_UNWIND_FRAMES];
} EAC_UNWIND_RESULT, *PEAC_UNWIND_RESULT;

typedef struct _EAC_CONTROL_REGISTERS {
    ULONG64 Cr2;    /* 0x1401e85a6: mov rax, cr2 */
    ULONG64 Cr3;    /* 0x1401e859e: mov rax, cr3 */
    ULONG64 Cr4;    /* 0x1401e85ae: mov rax, cr4 */
    ULONG64 Cr8;    /* 0x1401e85b6: mov rax, cr8 */
    ULONG64 Cr3Pfn; /* 0x140147ddb: (cr3 >> 12) & 0xFFFFFFFFF */
} EAC_CONTROL_REGISTERS, *PEAC_CONTROL_REGISTERS;

/* =========================================================================
 * HARDWARE IDENTIFICATION PROFILE (§14, §15, §16, §17)
 * ========================================================================= */

typedef struct _EAC_HARDWARE_PROFILE {
    /* §14 Disk MBR ("MBR:%08x"), SCSI LUN (":lun=%llx"), Partitions ("Part:%u", "CD:%u") */
    ULONG   MbrCrc32Hash;
    CHAR    MbrFormatted[24];
    ULONG64 ScsiLunId;
    CHAR    ScsiLunFormatted[32];
    ULONG   PartitionCount;
    ULONG   CdRomCount;
    CHAR    PartitionFormatted[32];
    CHAR    NvmeSerialNumber[64];

    /* §14 / §15 SMBIOS Types 0, 1, 2, 3, 4, 17, 127 */
    UCHAR   SmbiosUuid[16];
    CHAR    SmbiosUuidFormatted[40];
    CHAR    BiosVendor[64];
    CHAR    BiosVersion[64];
    CHAR    SystemManufacturer[64];
    CHAR    SystemProduct[64];
    CHAR    SystemSerial[64];
    CHAR    BaseboardManufacturer[64];
    CHAR    BaseboardProduct[64];
    CHAR    BaseboardSerial[64];
    CHAR    ChassisSerial[64];
    CHAR    ProcessorBrandSmbios[64];
    CHAR    MemoryDimmSerials[4][32];
    ULONG   MemoryDimmCount;
    BOOLEAN SmbiosEndSentinelValid;

    /* §14 TPM 2.0 EK & PCR[0..7], NDIS MAC, Registry GUIDs */
    UCHAR   TpmEkPublicSha256[32];
    UCHAR   TpmPcr0Sha256[32];
    UCHAR   TpmPcr7Sha256[32];
    UCHAR   NicPermanentMac[6];
    CHAR    NicMacFormatted[24];
    WCHAR   MachineGuid[40];
    WCHAR   HwProfileGuid[40];

    /* §3 / §16 CPUID & Control Registers */
    CHAR    CpuVendor[13];
    CHAR    CpuBrandString[49];
    CHAR    HvVendorString[13];
    ULONG   CpuFeatureEcx;
    ULONG   CpuFeatureEdx;
    ULONG64 TscCpuidDelta;
    ULONG64 InvlpgDelta;
    ULONG64 Ia32Lstar;
    BOOLEAN HypervisorPresent;
    BOOLEAN VbsEnabled;
    BOOLEAN HvciEnabled;
    BOOLEAN SecureBootEnabled;
    BOOLEAN TestSigningEnabled;
    BOOLEAN KernelDebuggerEnabled;

    /* §4 / §17 / §25 PCI DMA, ACPI & USB HID State */
    ULONG   AcpiDmarSig;
    ULONG   AcpiMadtSig;
    ULONG   AcpiFadtSig;
    ULONG   FlaggedPciCount;
    ULONG   FlaggedUsbMcuCount;
    ULONG   PointerClassDeviceCount;
} EAC_HARDWARE_PROFILE, *PEAC_HARDWARE_PROFILE;

/* =========================================================================
 * HEARTBEAT & IPC PACKETS
 * ========================================================================= */

#pragma pack(push, 1)
typedef struct _EAC_HEARTBEAT_PACKET {
    ULONG64 ClientChallenge;
    ULONG64 DriverResponseToken;
    ULONG   ActiveViolationFlags;
    ULONG   HeartbeatSequence;
} EAC_HEARTBEAT_PACKET, *PEAC_HEARTBEAT_PACKET;

typedef struct _EAC_PAGEWALK_REQUEST {
    ULONG64 TargetKernelVa;
    ULONG64 OutEffectivePteFlags;
    ULONG64 OutPhysicalAddress;
    BOOLEAN WalkSucceeded;
    BOOLEAN IsBackedBySignedModule;
} EAC_PAGEWALK_REQUEST, *PEAC_PAGEWALK_REQUEST;
#pragma pack(pop)

/* =========================================================================
 * GLOBAL DRIVER CONTEXT
 * ========================================================================= */

typedef struct _EAC_GLOBAL_CONTEXT {
    PDRIVER_OBJECT          DriverObject;
    PDEVICE_OBJECT          DeviceObject;
    PFLT_FILTER             MinifilterHandle;
    KSPIN_LOCK              TelemetryLock;

    /* MBA Runtime Seed & Encrypted Function Pointer Table (.rdata 0x140307E90) */
    ULONG64                 MbaMasterSeed;
    ULONG64                 EncryptedApiTable[EAC_API_SLOT_COUNT];

    /* Protected Process & Service State */
    volatile HANDLE         ServiceProcessId;       /* EasyAntiCheat_EOS.exe */
    volatile HANDLE         ProtectedGameProcessId; /* FortniteClient-Win64-Shipping.exe */

    /* Security Callback Registration Handles */
    PVOID                   ObCallbackHandle;
    LARGE_INTEGER           CmCallbackCookie;
    BOOLEAN                 CmCallbackRegistered;
    BOOLEAN                 ProcessNotifyRegistered;
    BOOLEAN                 ThreadNotifyRegistered;
    BOOLEAN                 ImageNotifyRegistered;

    /* DPC (1s tick + Multi-Core Rotation) & WorkItem */
    KTIMER                  PeriodicTimer;
    KDPC                    PeriodicDpc;
    PIO_WORKITEM            PeriodicWorkItem;
    ULONG                   CurrentDpcTargetCpu;
    volatile LONG           WorkItemActive;
    volatile BOOLEAN        Unloading;

    /* Violation Bitmask, Counters & Telemetry Ring */
    volatile LONG           ViolationFlags;
    volatile LONG           HeartbeatSequence;
    volatile LONG           DpcPassCount;
    volatile LONG64         LastHeartbeatTimestamp;
    ULONG                   TelemetryWriteIndex;
    EAC_TELEMETRY_CHUNK     TelemetryRing[EAC_MAX_TELEMETRY_CHUNKS];

    /* Hardware Profile & Control Registers Snapshot */
    EAC_CONTROL_REGISTERS   ControlRegs;
    EAC_HARDWARE_PROFILE    HwProfile;
} EAC_GLOBAL_CONTEXT, *PEAC_GLOBAL_CONTEXT;

extern EAC_GLOBAL_CONTEXT g_Eac;

/* =========================================================================
 * SUBSYSTEM PROTOTYPES
 * ========================================================================= */

/* driver.c */
ULONG64 EacDeriveMbaKey(_In_ ULONG64 Seed);
VOID EacInitializeMbaApiTable(VOID);
PVOID EacResolveMbaApi(_In_ EAC_API_SLOT Slot, _In_ ULONG64 Seed);
PRTL_PROCESS_MODULES EacAcquireModuleList(VOID);
BOOLEAN EacIsAddressBackedByModule(
    _In_ PRTL_PROCESS_MODULES Modules,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 OutImageBase,
    _Out_opt_ PRTL_PROCESS_MODULE_INFORMATION *OutModEntry
);
NTSTATUS EacExecuteSection7VmDispatcher(_In_ ULONG VmOpcode, _Inout_opt_ PULONG64 VmRegs);

/* minifilter.c (§6, §7, §24 - FltRegisterFilter Altitude 328010) */
NTSTATUS EacInitializeMinifilter(_In_ PDRIVER_OBJECT DriverObject);
VOID EacTeardownMinifilter(VOID);

/* cr3_pagewalk_stack.c (Deobfuscated FUN_14004a85a, FUN_1400d9e46, FUN_1400d97f3, FUN_1401e828c) */
NTSTATUS __fastcall EacMmCopyPhysical_14004a85a(
    _In_  ULONG64 MbaSeed,
    _Out_ PSIZE_T OutBytesTransferred,
    _In_  ULONG64 PhysicalAddress,
    _Out_writes_bytes_(8) PVOID TargetBuffer
);
BOOLEAN __fastcall EacReadPhysicalQword_14004a922(
    _In_  ULONG64 PhysicalAddress,
    _Out_ PULONG64 OutValue,
    _In_  ULONG64 MbaSeed
);
BOOLEAN __fastcall EacWalkKernelPageTable_1400d9e46(
    _In_      ULONG64  TargetVa,
    _Out_opt_ PULONG64 OutEffectivePteFlags,
    _In_      ULONG64  MbaSeed,
    _Out_opt_ PULONG64 OutPhysicalAddress
);
NTSTATUS __fastcall EacUnwindKernelStack_1400d97f3(
    _In_opt_ PCONTEXT InitialContext,
    _In_     PRTL_PROCESS_MODULES Modules,
    _In_     ULONG64 MbaSeed,
    _Out_    PEAC_UNWIND_RESULT OutResult
);
VOID __fastcall EacReadControlRegisters_1401e828c(_Out_ PEAC_CONTROL_REGISTERS OutRegs);
VOID EacAuditKernelPageTablesAndStacks(_In_ PRTL_PROCESS_MODULES Modules);

/* memory_vad_scanner.c (§1, §8, §11 - VAD vs CR3, Disk vs Memory .text Hash, Code Caves) */
NTSTATUS EacComputeSha256(
    _In_reads_bytes_(DataLength) const UCHAR *Data,
    _In_ ULONG DataLength,
    _Out_writes_bytes_(32) PUCHAR OutDigest
);
VOID EacAuditModuleTextIntegrityAndCodeCaves(_In_ PRTL_PROCESS_MODULES Modules);
VOID EacAuditProtectedProcessMemory(_In_ HANDLE ProtectedPid);

/* callbacks.c (§1, §2, §5, §6, §7 - Ob/Ps/Cm Callbacks) */
NTSTATUS EacInstallSecurityCallbacks(VOID);
VOID EacRemoveSecurityCallbacks(VOID);

/* byovd_softpub.c (§2, §9, §11, §12 - SoftpubAuthenticode, BYOVD, MajorFunction Audit) */
VOID EacScanByovdAndDeviceObjects(_In_ PRTL_PROCESS_MODULES Modules);
VOID EacAuditDriverDispatchTables(_In_ PRTL_PROCESS_MODULES Modules);
BOOLEAN EacVerifyRegistryRootCertificates(VOID);

/* dpc_telemetry.c (§12, §13, §18 - Multi-Core DPC, Multiarray Telemetry, IOCTLs) */
NTSTATUS EacInitializeDpcAndTimer(VOID);
VOID EacShutdownDpcAndTimer(VOID);
VOID EacEmitTelemetryChunk(
    _In_ EAC_CHUNK_OP Operation,
    _In_ ULONG ChunkType,
    _In_z_ PCSTR ChunkTag,
    _In_ ULONG StartIndex,
    _In_ ULONG EndIndex
);
NTSTATUS EacHandleDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);

/* hv_antidebug.c (§3, §10, §16 - CPUID, MSRs, EPT/TSC Timing, VBS/HVCI, Debugger) */
VOID EacAuditHypervisorAndAntiDebug(VOID);

/* pci_dma_acpi.c (§4, §17 - PCI Bus DMA Enumeration & ACPI DMAR/MADT/FADT) */
VOID EacAuditPciBusDmaAndAcpi(VOID);

/* hwid_tpm_smbios.c (§14, §15, §19, §20 - MBR:%08x, :lun=%llx, SMBIOS, TPM, NDIS MAC) */
VOID EacCollectHardwareTelemetry(VOID);

/* usb_hid_mouse.c (§11, §23, §25 - USB HID MCU Scanner & MouClass Arbitration) */
VOID EacAuditUsbHidAndMouseStack(_In_ PRTL_PROCESS_MODULES Modules);

#endif /* EAC_EOS_MASTER_H */
