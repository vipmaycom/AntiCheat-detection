/*
 * driver.c
 * Riot Vanguard (vgk.sys v1.19.1.18) Core Initialization, Exports 'Egg' (0x140368) & 'Egg2' (0x14062C),
 * Dynamic XOR API Table, BugCheck Reason Callback (0x1AC644), Cache-Aware Rundown Protection (0x1AC5AE),
 * \Device\gobbledygook (0x156E10), IOMMU Probe (0x1565E0), and LIFO Teardown.
 */

#include "vgk.h"

VGK_GLOBAL_CONTEXT g_Vgk = { 0 };

/* Riot Vanguard ASCII Fist Banner from vgk.sys v1.19.1.18 .rdata RVA 0x161A70 */
static const WCHAR g_VgkProtectionBanner[] =
    L"             #########################                                        \n"
    L"              #######################                                         \n"
    L"               #####################                                          \n"
    L"                ###################                                           \n"
    L"                  ################                                            \n"
    L"                   ##############                                             \n"
    L"                    ############                                              \n"
    L"                     ##########                                               \n"
    L"                      ########                                                \n"
    L"         ######        ######       ######                                    \n"
    L"         ########       ####      #########     Protected by Riot Vanguard.   \n"
    L"         ##########      ##      ##########                                   \n"
    L"           #########           ##########                                     \n"
    L"             #########       #########                                        \n"
    L"                ########    ########                                          \n"
    L"                  ###############                                             \n"
    L"                    ###########                                               \n"
    L"                       ######                                                 \n"
    L"                         ##                                                   \n";

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD     VgkDriverUnload;
_Dispatch_type_(IRP_MJ_CREATE)         DRIVER_DISPATCH VgkDispatchCreateClose;
_Dispatch_type_(IRP_MJ_CLOSE)          DRIVER_DISPATCH VgkDispatchCreateClose;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH VgkDispatchIoControl;

/* =========================================================================
 * EXPORTED ENTRIES [0, 1]: Egg (RVA 0x140368) & Egg2 (RVA 0x14062C)
 * Wrapped by .riot0 ExportDir (RVA 0x0616F250) -> .riot1 trampolines
 * (0x067C6ED0 -> jmp 0x140368, 0x067C6EE0 -> jmp 0x14062C)
 * ========================================================================= */

__declspec(dllexport)
NTSTATUS
NTAPI
Egg(
    _In_opt_ PVOID Context
)
{
    UNREFERENCED_PARAMETER(Context);

    /*
     * Exported symbol 'Egg' at RVA 0x140368 (.edata RVA 0x1AB000).
     * Bumps internal heartbeat sequence and validates driver liveness.
     */
    InterlockedIncrement64(&g_Vgk.HeartbeatSequence);
    return STATUS_SUCCESS;
}

__declspec(dllexport)
NTSTATUS
NTAPI
Egg2(
    _In_opt_ PVOID Context
)
{
    /*
     * Second exported symbol 'Egg2' added in vgk.sys v1.19.1.18 at RVA 0x14062C.
     * Queries elapsed QPC microseconds (FUN_1401406dc) and flushes active vgtrace telemetry
     * when invoked by external Vanguard kernel/packman dispatchers.
     */
    ULONG64 t0 = VgkQueryElapsedMicroseconds();
    InterlockedIncrement64(&g_Vgk.HeartbeatSequence);

    if (Context != NULL && MmIsAddressValid(Context)) {
        VgtraceFlushToRealtime(
            &g_Vgk.VgtraceLogger,
            Context,
            sizeof(ULONG64),
            0xE662UL
        );
    }

    ULONG64 t1 = VgkQueryElapsedMicroseconds();
    if (t1 >= t0) {
        VgkRecordProfilerSample(0, t1 - t0);
    }

    return STATUS_SUCCESS;
}

/* =========================================================================
 * NEW IN v1.19.1.18: BUGCHECK REASON CALLBACK (RVA 0x1AC61E..0x1AC644)
 * Captures crash state and active hook reference count on BSOD
 * ========================================================================= */

static VOID
NTAPI
VgkBugCheckReasonCallbackRoutine(
    _In_ KBUGCHECK_CALLBACK_REASON Reason,
    _In_ struct _KBUGCHECK_REASON_CALLBACK_RECORD *Record,
    _Inout_ PVOID ReasonSpecificData,
    _In_ ULONG ReasonSpecificDataLength
)
{
    UNREFERENCED_PARAMETER(Reason);
    UNREFERENCED_PARAMETER(Record);
    UNREFERENCED_PARAMETER(ReasonSpecificData);
    UNREFERENCED_PARAMETER(ReasonSpecificDataLength);

    /* Record final crash state into Guarded Region header if mapped */
    if (g_Vgk.GuardedRegionKernelVa != NULL) {
        PULONG64 headerWords = (PULONG64)g_Vgk.GuardedRegionKernelVa;
        headerWords[2] = (ULONG64)g_Vgk.ViolationFlags;
        headerWords[3] = (ULONG64)g_Vgk.ActiveHookRefCount;
        headerWords[4] = (ULONG64)g_Vgk.HeartbeatSequence;
    }
}

/* =========================================================================
 * DYNAMIC XOR-ENCRYPTED API TABLE
 * ========================================================================= */

static PVOID
VgkResolveRoutineByName(
    _In_z_ PCWSTR RoutineName
)
{
    UNICODE_STRING uName;
    RtlInitUnicodeString(&uName, RoutineName);
    return MmGetSystemRoutineAddress(&uName);
}

VOID
VgkInitializeApiXorTable(VOID)
{
    LARGE_INTEGER perfCounter = KeQueryPerformanceCounter(NULL);
    ULONG64 xorKey = ((ULONG64)__rdtsc() << 13) ^ (ULONG64)perfCounter.QuadPart ^ 0xA55A9E3779B97F4AULL;
    if (xorKey == 0) {
        xorKey = 0x5A5AA5A5DEADBEEFULL;
    }
    g_Vgk.ApiXorKey = xorKey;

    struct {
        VGK_API_SLOT Slot;
        PCWSTR       Name;
    } apiBindings[] = {
        { VGK_API_KE_REGISTER_NMI_CALLBACK,             L"KeRegisterNmiCallback"              },
        { VGK_API_KE_DEREGISTER_NMI_CALLBACK,           L"KeDeregisterNmiCallback"            },
        { VGK_API_OB_REGISTER_CALLBACKS,                L"ObRegisterCallbacks"                },
        { VGK_API_OB_UNREGISTER_CALLBACKS,              L"ObUnRegisterCallbacks"              },
        { VGK_API_PS_SET_CREATE_PROCESS_NOTIFY_EX,      L"PsSetCreateProcessNotifyRoutineEx"  },
        { VGK_API_PS_SET_CREATE_THREAD_NOTIFY,          L"PsSetCreateThreadNotifyRoutine"     },
        { VGK_API_PS_REMOVE_CREATE_THREAD_NOTIFY,       L"PsRemoveCreateThreadNotifyRoutine"  },
        { VGK_API_PS_SET_LOAD_IMAGE_NOTIFY,             L"PsSetLoadImageNotifyRoutine"        },
        { VGK_API_PS_REMOVE_LOAD_IMAGE_NOTIFY,          L"PsRemoveLoadImageNotifyRoutine"     },
        { VGK_API_HALP_DMA_GET_IOMMU_INTERFACE,         L"HalpDmaGetIommuInterface"           },
        { VGK_API_SYS_DISP_AFFINITY,                    VGK_SYSDISPAFFINITY_W                 },
        { VGK_API_KE_SET_ACTUAL_BASE_PRIORITY_THREAD,   VGK_KE_SET_BASE_PRIORITY_W            }
    };

    for (ULONG i = 0; i < ARRAYSIZE(apiBindings); ++i) {
        PVOID rawPtr = VgkResolveRoutineByName(apiBindings[i].Name);
        g_Vgk.EncryptedApiTable[apiBindings[i].Slot] =
            (rawPtr != NULL) ? ((ULONG64)rawPtr ^ xorKey) : 0ULL;
    }
}

PVOID
VgkResolveEncryptedApi(
    _In_ VGK_API_SLOT Slot
)
{
    if (Slot >= VGK_API_SLOT_COUNT) {
        return NULL;
    }
    ULONG64 encoded = g_Vgk.EncryptedApiTable[Slot];
    if (encoded == 0ULL) {
        return NULL;
    }
    return (PVOID)(encoded ^ g_Vgk.ApiXorKey);
}

/* =========================================================================
 * RECONSTRUCTED VgkQuerySysDispAffinity
 * ========================================================================= */

typedef USHORT (NTAPI *PFN_VGK_QUERY_AFFINITY)(_In_ PCWSTR KeyName, _In_ PVOID Context);

BOOLEAN
VgkQuerySysDispAffinity(
    _In_ PVOID Context,
    _Out_ PULONG OutAffinity
)
{
    if (OutAffinity) {
        *OutAffinity = 0;
    }

    PFN_VGK_QUERY_AFFINITY fnQuery = (PFN_VGK_QUERY_AFFINITY)VgkResolveEncryptedApi(VGK_API_SYS_DISP_AFFINITY);
    if (fnQuery == NULL) {
        KAFFINITY activeProcessors = KeQueryActiveProcessors();
        if (OutAffinity) {
            *OutAffinity = (ULONG)(activeProcessors & 0xFFFF);
        }
        return TRUE;
    }

    USHORT result = fnQuery(VGK_SYSDISPAFFINITY_W, Context);
    if (result == 0) {
        return FALSE;
    }

    if (OutAffinity) {
        *OutAffinity = (ULONG)result;
    }
    return TRUE;
}

/* =========================================================================
 * MODULE LIST ENUMERATION & BOUNDARY CHECKS
 * ========================================================================= */

PRTL_PROCESS_MODULES
VgkAcquireModuleList(VOID)
{
    ULONG requiredBytes = 0;
    NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &requiredBytes);
    if (status != STATUS_INFO_LENGTH_MISMATCH || requiredBytes == 0) {
        return NULL;
    }

    requiredBytes += 0x2000;
    PRTL_PROCESS_MODULES modList = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        requiredBytes,
        VGK_POOL_TAG
    );
    if (!modList) {
        VgkLogTelemetryEvent(VGK_EVENT_EDCL, 0xC000009A, L"%lX\n", STATUS_INSUFFICIENT_RESOURCES);
        return NULL;
    }

    status = ZwQuerySystemInformation(SystemModuleInformation, modList, requiredBytes, &requiredBytes);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(modList, VGK_POOL_TAG);
        return NULL;
    }

    return modList;
}

BOOLEAN
VgkIsAddressBackedByModule(
    _In_ PRTL_PROCESS_MODULES Modules,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 OutImageBase
)
{
    if (OutImageBase) {
        *OutImageBase = 0;
    }
    if (!Modules || Address == 0) {
        return FALSE;
    }

    for (ULONG i = 0; i < Modules->NumberOfModules; ++i) {
        ULONG64 base = (ULONG64)Modules->Modules[i].ImageBase;
        ULONG64 end  = base + (ULONG64)Modules->Modules[i].ImageSize;
        if (Address >= base && Address < end) {
            if (OutImageBase) {
                *OutImageBase = base;
            }
            return TRUE;
        }
    }
    return FALSE;
}

/* =========================================================================
 * BOOT-TIME HARDWARE FINGERPRINTING & IOMMU CHIPSET PROBE
 * (RVA 0x1565E0: "\Device\IommuChipsetEnabler" / "HalpDmaGetIommuInterface")
 * ========================================================================= */

static VOID
VgkProbeIommuAndHardwareProfile(VOID)
{
    RtlZeroMemory(&g_Vgk.HardwareProfile, sizeof(g_Vgk.HardwareProfile));

    /* 1. Query Performance Counter Frequency (HAL.dll!KeQueryPerformanceCounter, stored at 0x19CB58) */
    LARGE_INTEGER freq = { 0 };
    KeQueryPerformanceCounter(&freq);
    g_Vgk.HardwareProfile.QpcFrequency = (ULONG64)freq.QuadPart;

    /* 2. CPUID Vendor, Processor ID, and Hypervisor Leaf 0x40000000 */
    int regs[4] = { 0 };
    __cpuid(regs, 0);
    *(int*)(&g_Vgk.HardwareProfile.CpuVendor[0]) = regs[1];
    *(int*)(&g_Vgk.HardwareProfile.CpuVendor[4]) = regs[3];
    *(int*)(&g_Vgk.HardwareProfile.CpuVendor[8]) = regs[2];
    g_Vgk.HardwareProfile.CpuVendor[12] = '\0';

    __cpuid(regs, 1);
    g_Vgk.HardwareProfile.ProcessorId          = (ULONG)regs[0];
    g_Vgk.HardwareProfile.HypervisorBitPresent = ((regs[2] & (1 << 31)) != 0);

    if (g_Vgk.HardwareProfile.HypervisorBitPresent) {
        __cpuid(regs, 0x40000000);
        *(int*)(&g_Vgk.HardwareProfile.HypervisorVendor[0]) = regs[1];
        *(int*)(&g_Vgk.HardwareProfile.HypervisorVendor[4]) = regs[2];
        *(int*)(&g_Vgk.HardwareProfile.HypervisorVendor[8]) = regs[3];
        g_Vgk.HardwareProfile.HypervisorVendor[12] = '\0';
    }

    /* 3. Calibrate RDTSC vs CPUID Serialization Latency */
    ULONG64 accDelta = 0;
    for (ULONG i = 0; i < 32; ++i) {
        ULONG64 t0 = __rdtsc();
        __cpuid(regs, 1);
        ULONG64 t1 = __rdtsc();
        if (t1 > t0) {
            accDelta += (t1 - t0);
        }
    }
    g_Vgk.HardwareProfile.TscCpuidRatio = accDelta / 32;

    /* 4. Probe \Device\IommuChipsetEnabler & HalpDmaGetIommuInterface (RVA 0x1565E0) */
    PVOID halpIommu = VgkResolveEncryptedApi(VGK_API_HALP_DMA_GET_IOMMU_INTERFACE);
    if (halpIommu != NULL) {
        g_Vgk.HardwareProfile.IommuInterfacePresent = TRUE;
    } else {
        UNICODE_STRING iommuDev;
        RtlInitUnicodeString(&iommuDev, VGK_IOMMU_DEVICE_W);
        PFILE_OBJECT fileObj = NULL;
        PDEVICE_OBJECT devObj = NULL;
        if (NT_SUCCESS(IoGetDeviceObjectPointer(&iommuDev, FILE_READ_ATTRIBUTES, &fileObj, &devObj))) {
            g_Vgk.HardwareProfile.IommuInterfacePresent = TRUE;
            ObDereferenceObject(fileObj);
        } else {
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_IOMMU_DISABLED);
        }
    }

    /* 5. Compute SHA-256 of on-disk ntoskrnl.exe header via CNG (cng.sys) */
    VgkHashDiskFileHeader(
        L"\\SystemRoot\\System32\\ntoskrnl.exe",
        g_Vgk.HardwareProfile.NtoskrnlHeaderSha256
    );
}

/* =========================================================================
 * IRP DISPATCHERS (\Device\vgk AND \Device\gobbledygook)
 * ========================================================================= */

_Use_decl_annotations_
NTSTATUS
VgkDispatchCreateClose(
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
VgkDispatchIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    return VgkHandleDeviceControl(DeviceObject, Irp);
}

/* =========================================================================
 * LIFO REVERSE-PATH TEARDOWN & DRIVER ENTRY
 * ========================================================================= */

_Use_decl_annotations_
VOID
VgkDriverUnload(
    PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    g_Vgk.Unloading = TRUE;

    /* 1. Stop DPC Timer, flush profiler metrics, and drain deferred WorkItems */
    VgkFlushProfilerMetrics();
    VgkShutdownDpcAndWorkItem();

    /* 2. Wait for SMP Cache-Aware Rundown Protection and active hook trampolines (RVA 0x18FF08) */
    if (g_Vgk.CacheAwareRundownRef != NULL) {
        ExWaitForRundownProtectionReleaseCacheAware(g_Vgk.CacheAwareRundownRef);
        ExFreeCacheAwareRundownProtection(g_Vgk.CacheAwareRundownRef);
        g_Vgk.CacheAwareRundownRef = NULL;
    }

    while (InterlockedCompareExchange64(&g_Vgk.ActiveHookRefCount, 0, 0) != 0) {
        LARGE_INTEGER shortWait;
        shortWait.QuadPart = -10000LL; /* 1ms */
        KeDelayExecutionThread(KernelMode, FALSE, &shortWait);
    }

    /* 3. Shutdown vgtrace real-time MDL ring-buffer logger */
    VgtraceShutdownLogger();

    /* 4. Remove Ob, Process, Thread, Image, and NMI callbacks */
    VgkRemoveSecurityCallbacks();

    /* 5. Deregister BugCheck Reason Callback (RVA 0x1AC61E) */
    if (g_Vgk.BugCheckRegistered) {
        KeDeregisterBugCheckReasonCallback(&g_Vgk.BugCheckRecord);
        g_Vgk.BugCheckRegistered = FALSE;
    }

    /* 6. Release Guarded Region MDL */
    VgkFreeGuardedRegionMdl();

    /* 7. Destroy Symbolic Link and Device Objects (\Device\gobbledygook & \Device\vgk) */
    UNICODE_STRING dosName;
    RtlInitUnicodeString(&dosName, VGK_DOS_DEVICE_NAME_W);
    IoDeleteSymbolicLink(&dosName);

    if (g_Vgk.GobbledygookDeviceObject != NULL) {
        IoDeleteDevice(g_Vgk.GobbledygookDeviceObject);
        g_Vgk.GobbledygookDeviceObject = NULL;
    }

    if (g_Vgk.VgkDeviceObject != NULL) {
        IoDeleteDevice(g_Vgk.VgkDeviceObject);
        g_Vgk.VgkDeviceObject = NULL;
    }
}

_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(g_VgkProtectionBanner);

    RtlZeroMemory(&g_Vgk, sizeof(g_Vgk));
    g_Vgk.DriverObject = DriverObject;

    KeInitializeSpinLock(&g_Vgk.TelemetryLock);
    KeInitializeMutex(&g_Vgk.StateMutex, 0);

    for (ULONG i = 0; i < VGK_PROFILER_MAX_BUCKETS; ++i) {
        g_Vgk.ProfilerBuckets[i].BucketId = i + 1;
    }

    /* 1. Initialize XOR-Encrypted Kernel API Dispatch Table */
    VgkInitializeApiXorTable();

    /* 2. Allocate Cache-Aware Rundown Protection (RVA 0x1AC5AE) & BugCheck Callback (RVA 0x1AC644) */
    g_Vgk.CacheAwareRundownRef = ExAllocateCacheAwareRundownProtection(NonPagedPoolNx, VGK_POOL_TAG);

    KeInitializeCallbackRecord(&g_Vgk.BugCheckRecord);
    g_Vgk.BugCheckRegistered = KeRegisterBugCheckReasonCallback(
        &g_Vgk.BugCheckRecord,
        VgkBugCheckReasonCallbackRoutine,
        KbCallbackSecondaryDumpData,
        (PUCHAR)"VGKT"
    );

    /* 3. Create Primary Device (\Device\vgk) and Auxiliary Device (\Device\gobbledygook, RVA 0x156E10) */
    UNICODE_STRING vgkDevName;
    UNICODE_STRING vgkDosName;
    UNICODE_STRING gobbleDevName;
    RtlInitUnicodeString(&vgkDevName,    VGK_DEVICE_NAME_W);
    RtlInitUnicodeString(&vgkDosName,    VGK_DOS_DEVICE_NAME_W);
    RtlInitUnicodeString(&gobbleDevName, VGK_GOBBLEDYGOOK_DEVICE_W);

    NTSTATUS status = IoCreateDevice(
        DriverObject,
        0,
        &vgkDevName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_Vgk.VgkDeviceObject
    );
    if (!NT_SUCCESS(status)) {
        if (g_Vgk.BugCheckRegistered) {
            KeDeregisterBugCheckReasonCallback(&g_Vgk.BugCheckRecord);
        }
        if (g_Vgk.CacheAwareRundownRef != NULL) {
            ExFreeCacheAwareRundownProtection(g_Vgk.CacheAwareRundownRef);
        }
        return status;
    }

    status = IoCreateSymbolicLink(&vgkDosName, &vgkDevName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(g_Vgk.VgkDeviceObject);
        g_Vgk.VgkDeviceObject = NULL;
        if (g_Vgk.BugCheckRegistered) {
            KeDeregisterBugCheckReasonCallback(&g_Vgk.BugCheckRecord);
        }
        if (g_Vgk.CacheAwareRundownRef != NULL) {
            ExFreeCacheAwareRundownProtection(g_Vgk.CacheAwareRundownRef);
        }
        return status;
    }

    /* Create \Device\gobbledygook (RVA 0x156E10 in vgk.sys v1.19.1.18) */
    IoCreateDevice(
        DriverObject,
        0,
        &gobbleDevName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_Vgk.GobbledygookDeviceObject
    );

    DriverObject->DriverUnload                         = VgkDriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = VgkDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = VgkDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VgkDispatchIoControl;

    /* 4. Lock Boot-Time Hardware Profile, CNG Hashes & IOMMU State */
    ULONG64 t0 = VgkQueryElapsedMicroseconds();
    VgkProbeIommuAndHardwareProfile();

    /* 5. Allocate 2MB Non-Paged MDL for Guarded Region Mapping & Initialize vgtrace Ring-Buffer */
    VgkInitializeGuardedRegionMdl();
    VgtraceInitializeLogger();

    /* 6. Perform Initial Boot Blacklist (BBL / DCL), Hypervisor, PCI/DMA, HWID/TPM & USB HID Sweeps */
    VgkScanObjectDirectoriesForByovd();
    PRTL_PROCESS_MODULES bootModules = VgkAcquireModuleList();
    if (bootModules != NULL) {
        VgkScanLoadedModulesAgainstRules(bootModules);
        VgkAuditHypervisorAndAntiDebug(bootModules);
        ExFreePoolWithTag(bootModules, VGK_POOL_TAG);
    } else {
        VgkAuditHypervisorAndAntiDebug(NULL);
    }

    VgkAuditPciBusDmaAndAcpi();
    VgkCollectFullHardwareIdentity();
    VgkAuditUsbHidAndMouseTopology();

    ULONG64 t1 = VgkQueryElapsedMicroseconds();
    if (t1 >= t0) {
        VgkRecordProfilerSample(1, t1 - t0);
    }

    /* 7. Install ObRegisterCallbacks, Process/Thread/Image Notifiers & NMI Handler */
    VgkInstallSecurityCallbacks();

    /* 8. Start Periodic KDPC Timer & WorkItem Pipeline */
    VgkInitializeDpcAndWorkItem();

    /* 9. Emit Boot Version Telemetry Record (RVA 0x1598C0 -> \SystemRoot\vgk0001.dat) */
    VgkLogTelemetryEvent(VGK_EVENT_VERSION_BANNER, 0, NULL);

    return STATUS_SUCCESS;
}
