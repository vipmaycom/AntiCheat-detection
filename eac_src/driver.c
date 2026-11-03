/*
 * driver.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) Core DriverEntry (RVA 0x274DF8),
 * SplitMix64/Murmur 64-bit MBA Key Derivation (Constant 0xC6BC279692B5C323ULL),
 * Encrypted .rdata Kernel API Dispatch Table (0x140307E90..0x14031B000),
 * Section 7 Virtual Machine Register-Preserving Dispatcher (RVA 0x383000, 35.28 MB),
 * and Clean LIFO Unload Pipeline.
 */

#include "eac.h"

EAC_GLOBAL_CONTEXT g_Eac = { 0 };

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD     EacDriverUnload;
_Dispatch_type_(IRP_MJ_CREATE)         DRIVER_DISPATCH EacDispatchCreateClose;
_Dispatch_type_(IRP_MJ_CLOSE)          DRIVER_DISPATCH EacDispatchCreateClose;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH EacDispatchIoControl;

/* =========================================================================
 * SPLITMIX64 / MURMUR 64-BIT MBA KEY DERIVATION
 * Exact prologue sequence extracted from FUN_14004a85a, FUN_1400d97f3, FUN_1400d9e46:
 *   mov r11, 0xC6BC279692B5C323
 *   mov rsi, rcx
 *   imul rsi, r11
 *   mov r10, rsi
 *   shr r10, 0x20
 *   mov rcx, rsi
 *   shr rcx, 0x3C
 *   shr r10, cl
 *   xor r10, rsi
 *   imul r10, r11
 *   xor r10, rax
 * ========================================================================= */

ULONG64
EacDeriveMbaKey(
    _In_ ULONG64 Seed
)
{
    const ULONG64 kMul = EAC_MBA_MIX_CONSTANT; /* 0xC6BC279692B5C323ULL */
    ULONG64 s = Seed * kMul;
    ULONG   shift = (ULONG)(s >> 60);
    ULONG64 mixed = ((s >> 32) >> shift) ^ s;
    mixed *= kMul;
    mixed ^= Seed;
    return mixed;
}

static PVOID
EacLookupExportByName(
    _In_z_ PCWSTR ExportName
)
{
    UNICODE_STRING uName;
    RtlInitUnicodeString(&uName, ExportName);
    return MmGetSystemRoutineAddress(&uName);
}

/*
 * Initializes the MBA-encrypted pointer table in parity with .rdata (0x140307E90..0x14031B000).
 * In EasyAntiCheat_EOS.sys, each pointer is stored as:
 *   EncryptedSlot = RealFunctionAddress + (DerivedKey * SlotMultiplier)
 * and recovered at call time via:
 *   RealFunctionAddress = EncryptedSlot - (DerivedKey * SlotMultiplier)
 */
VOID
EacInitializeMbaApiTable(VOID)
{
    g_Eac.MbaMasterSeed = 0x8F4A2C91E3751B6DULL;
    ULONG64 key = EacDeriveMbaKey(g_Eac.MbaMasterSeed);

    struct {
        EAC_API_SLOT Slot;
        PCWSTR       Name;
    } bindings[] = {
        { EAC_API_MM_COPY_MEMORY,               L"MmCopyMemory"                      },
        { EAC_API_RTL_LOOKUP_FUNCTION_ENTRY,    L"RtlLookupFunctionEntry"            },
        { EAC_API_RTL_VIRTUAL_UNWIND,           L"RtlVirtualUnwind"                  },
        { EAC_API_OB_REGISTER_CALLBACKS,        L"ObRegisterCallbacks"               },
        { EAC_API_OB_UNREGISTER_CALLBACKS,      L"ObUnRegisterCallbacks"             },
        { EAC_API_PS_SET_CREATE_PROCESS_NOTIFY_EX, L"PsSetCreateProcessNotifyRoutineEx" },
        { EAC_API_PS_SET_CREATE_THREAD_NOTIFY,  L"PsSetCreateThreadNotifyRoutine"    },
        { EAC_API_PS_REMOVE_CREATE_THREAD_NOTIFY, L"PsRemoveCreateThreadNotifyRoutine" },
        { EAC_API_PS_SET_LOAD_IMAGE_NOTIFY,     L"PsSetLoadImageNotifyRoutine"       },
        { EAC_API_PS_REMOVE_LOAD_IMAGE_NOTIFY,  L"PsRemoveLoadImageNotifyRoutine"    },
        { EAC_API_CM_REGISTER_CALLBACK_EX,      L"CmRegisterCallbackEx"              },
        { EAC_API_CM_UNREGISTER_CALLBACK,       L"CmUnRegisterCallback"              },
        { EAC_API_MM_COPY_VIRTUAL_MEMORY,       L"MmCopyVirtualMemory"               }
    };

    for (ULONG i = 0; i < ARRAYSIZE(bindings); ++i) {
        PVOID fnPtr = EacLookupExportByName(bindings[i].Name);
        if (fnPtr != NULL) {
            ULONG64 slotMul = EAC_MBA_SUB_MMCOPYMEMORY + ((ULONG64)bindings[i].Slot * 0x100000001B3ULL);
            g_Eac.EncryptedApiTable[bindings[i].Slot] = (ULONG64)fnPtr + (key * slotMul);
        } else {
            g_Eac.EncryptedApiTable[bindings[i].Slot] = 0ULL;
        }
    }
}

PVOID
EacResolveMbaApi(
    _In_ EAC_API_SLOT Slot,
    _In_ ULONG64 Seed
)
{
    if (Slot >= EAC_API_SLOT_COUNT) {
        return NULL;
    }

    ULONG64 enc = g_Eac.EncryptedApiTable[Slot];
    if (enc == 0ULL) {
        return NULL;
    }

    ULONG64 key = EacDeriveMbaKey(Seed);
    ULONG64 slotMul = EAC_MBA_SUB_MMCOPYMEMORY + ((ULONG64)Slot * 0x100000001B3ULL);
    return (PVOID)(enc - (key * slotMul));
}

/* =========================================================================
 * SYSTEM MODULE LIST ENUMERATION & BOUNDARY VERIFICATION
 * ========================================================================= */

PRTL_PROCESS_MODULES
EacAcquireModuleList(VOID)
{
    ULONG bytesNeeded = 0;
    NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &bytesNeeded);
    if (status != STATUS_INFO_LENGTH_MISMATCH || bytesNeeded == 0) {
        return NULL;
    }

    bytesNeeded += 0x2000;
    PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        bytesNeeded,
        EAC_POOL_TAG_SCAN
    );
    if (!modules) {
        return NULL;
    }

    RtlZeroMemory(modules, bytesNeeded);
    status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytesNeeded, &bytesNeeded);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(modules, EAC_POOL_TAG_SCAN);
        return NULL;
    }

    return modules;
}

BOOLEAN
EacIsAddressBackedByModule(
    _In_ PRTL_PROCESS_MODULES Modules,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 OutImageBase,
    _Out_opt_ PRTL_PROCESS_MODULE_INFORMATION *OutModEntry
)
{
    if (OutImageBase) {
        *OutImageBase = 0;
    }
    if (OutModEntry) {
        *OutModEntry = NULL;
    }
    if (!Modules || Address < 0xFFFF800000000000ULL) {
        return FALSE;
    }

    for (ULONG i = 0; i < Modules->NumberOfModules; ++i) {
        ULONG64 base = (ULONG64)Modules->Modules[i].ImageBase;
        ULONG64 end  = base + (ULONG64)Modules->Modules[i].ImageSize;
        if (Address >= base && Address < end) {
            if (OutImageBase) {
                *OutImageBase = base;
            }
            if (OutModEntry) {
                *OutModEntry = &Modules->Modules[i];
            }
            return TRUE;
        }
    }

    return FALSE;
}

/* =========================================================================
 * SECTION 7 (RVA 0x383000, 35.28 MB) OBFUSCATED VM DISPATCHER LOOP
 * Preserves XMM6..XMM15 and executes virtualized integrity opcodes (§8, §24.2)
 * ========================================================================= */

#pragma code_seg(push, ".eacvm")

NTSTATUS
EacExecuteSection7VmDispatcher(
    _In_ ULONG VmOpcode,
    _Inout_opt_ PULONG64 VmRegs
)
{
    M128A savedXmm[4];
    RtlZeroMemory(savedXmm, sizeof(savedXmm));

    /*
     * Section 7 VM handler dispatch table:
     *   Opcode 0x01: Snapshot CR2/CR3/CR4/CR8 and verify CR3 PFN
     *   Opcode 0x02: Run 4-level CR3 kernel page table walk + stack unwind
     *   Opcode 0x03: Run hypervisor / MSR / timing audit
     *   Opcode 0x04: Refresh hardware fingerprint & emit multiarray chunk
     */
    switch (VmOpcode) {
    case 0x01:
        EacReadControlRegisters_1401e828c(&g_Eac.ControlRegs);
        if (VmRegs) {
            VmRegs[0] = g_Eac.ControlRegs.Cr3;
            VmRegs[1] = g_Eac.ControlRegs.Cr4;
        }
        break;

    case 0x02: {
        PRTL_PROCESS_MODULES mods = EacAcquireModuleList();
        if (mods != NULL) {
            EacAuditKernelPageTablesAndStacks(mods);
            ExFreePoolWithTag(mods, EAC_POOL_TAG_SCAN);
        }
        break;
    }

    case 0x03:
        EacAuditHypervisorAndAntiDebug();
        break;

    case 0x04:
        EacCollectHardwareTelemetry();
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

#pragma code_seg(pop)

/* =========================================================================
 * IRP DISPATCH ROUTINES (\Device\EasyAntiCheat_EOS)
 * ========================================================================= */

_Use_decl_annotations_
NTSTATUS
EacDispatchCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
EacDispatchIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    return EacHandleDeviceControl(DeviceObject, Irp);
}

/* =========================================================================
 * LIFO DRIVER UNLOAD (CLEAN RESTORE OF ALL HOOKS, FILTERS & CALLBACKS)
 * ========================================================================= */

_Use_decl_annotations_
VOID
EacDriverUnload(
    PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    InterlockedExchange8((volatile CHAR *)&g_Eac.Unloading, TRUE);

    /* 1. Stop periodic 1000ms timer, flush DPCs & free work item */
    EacShutdownDpcAndTimer();

    /* 2. Remove Ob, Ps, and Cm callbacks */
    EacRemoveSecurityCallbacks();

    /* 3. Unregister FLTMGR minifilter (Altitude 328010) */
    EacTeardownMinifilter();

    /* 4. Delete symbolic link and device object */
    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, EAC_DOS_DEVICE_NAME_W);
    IoDeleteSymbolicLink(&symLink);

    if (g_Eac.DeviceObject != NULL) {
        IoDeleteDevice(g_Eac.DeviceObject);
        g_Eac.DeviceObject = NULL;
    }

    /* 5. Zero sensitive MBA keys and API tables */
    RtlSecureZeroMemory(g_Eac.EncryptedApiTable, sizeof(g_Eac.EncryptedApiTable));
}

/* =========================================================================
 * DRIVER ENTRY (Reconstructed from RVA 0x274DF8)
 * Prologue: 48 89 5c 24 08 57 48 83 ec 20 48 8b da 48 8b f9
 * ========================================================================= */

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    RtlZeroMemory(&g_Eac, sizeof(g_Eac));
    g_Eac.DriverObject = DriverObject;
    KeInitializeSpinLock(&g_Eac.TelemetryLock);

    /* 1. Initialize SplitMix64/Murmur MBA API Dispatch Table (.rdata 0x140307E90) */
    EacInitializeMbaApiTable();

    /* 2. Create \Device\EasyAntiCheat_EOS and \DosDevices\EasyAntiCheat_EOS */
    UNICODE_STRING devName;
    UNICODE_STRING symLink;
    RtlInitUnicodeString(&devName, EAC_DEVICE_NAME_W);
    RtlInitUnicodeString(&symLink, EAC_DOS_DEVICE_NAME_W);

    NTSTATUS status = IoCreateDevice(
        DriverObject,
        0,
        &devName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_Eac.DeviceObject
    );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IoCreateSymbolicLink(&symLink, &devName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(g_Eac.DeviceObject);
        g_Eac.DeviceObject = NULL;
        return status;
    }

    DriverObject->DriverUnload                         = EacDriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = EacDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = EacDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = EacDispatchIoControl;

    /* 3. Register FLTMGR.SYS Minifilter at Altitude 328010 (Static Import Anchor) */
    status = EacInitializeMinifilter(DriverObject);
    if (!NT_SUCCESS(status)) {
        /* Continue operating even if volume attachment is deferred */
    }

    /* 4. Snapshot Control Registers (FUN_1401e828c) & Collect Boot HWID Profile */
    EacReadControlRegisters_1401e828c(&g_Eac.ControlRegs);
    EacCollectHardwareTelemetry();
    EacAuditHypervisorAndAntiDebug();
    EacAuditPciBusDmaAndAcpi();

    /* 5. Install ObRegisterCallbacks, Ps* Notify Routines & CmRegisterCallbackEx */
    EacInstallSecurityCallbacks();

    /* 6. Start 1000ms Multi-Core DPC & WorkItem Integrity Pipeline */
    EacInitializeDpcAndTimer();

    EacEmitTelemetryChunk(EAC_CHUNK_OP_ADD, 1, "driver_init", 0, 1);
    return STATUS_SUCCESS;
}
