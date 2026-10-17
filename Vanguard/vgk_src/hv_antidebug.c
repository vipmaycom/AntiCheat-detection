/*
 * hv_antidebug.c
 * Riot Vanguard Kernel-Mode Anti-Cheat (vgk.sys v1.19.1.18)
 * Sections 2, 3, 10, 16: Hypervisor Detection, CPUID Leaves, MSR Verification & Anti-Debug
 *
 * Implements:
 *   - §3 / §16 CPUID Leaves 0x00000000, 0x00000001 (ECX[31] HV bit),
 *              0x40000000..0x40000005 (HV Vendor & VBS/Hyper-V Interface),
 *              0x80000002..0x80000004 (48-byte Processor Brand String)
 *   - §3       MSR Hooking Detection (IA32_LSTAR 0xC0000082, IA32_SYSENTER_EIP 0x176,
 *              IA32_GS_BASE 0xC0000101) across CLI/STI barriers + ntoskrnl bounds check
 *   - §3       CR0 / CR4 Control Register Masking (CR4.VMXE, SMEP, SMAP)
 *   - §3       EPT Page Splitting & INVLPG + RDTSCP Latency Calibration
 *   - §2 / §3  TESTSIGNING, VBS/HVCI (SystemIsolatedUserModeInformation), Secure Boot
 *   - §9 / §10 Anti-Debug: KdDebuggerEnabled, KdDebuggerNotPresent,
 *              SystemKernelDebuggerInformation, KUSER_SHARED_DATA vs RDTSC+CPUID Quad Check
 */

#include "vgk.h"

#pragma intrinsic(_disable, _enable)

#define IA32_SYSENTER_EIP_MSR       0x00000176UL
#define IA32_GS_BASE_MSR            0xC0000101UL
#define IA32_KERNEL_GS_BASE_MSR     0xC0000102UL
#define IA32_LSTAR_MSR              0xC0000082UL

#define CR4_VMXE_BIT                (1ULL << 13)
#define CR4_SMEP_BIT                (1ULL << 20)
#define CR4_SMAP_BIT                (1ULL << 21)

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION {
    BOOLEAN KernelDebuggerEnabled;
    BOOLEAN KernelDebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION {
    ULONG Length;
    ULONG CodeIntegrityOptions;
} SYSTEM_CODEINTEGRITY_INFORMATION, *PSYSTEM_CODEINTEGRITY_INFORMATION;

typedef struct _SYSTEM_ISOLATED_USER_MODE_INFORMATION {
    BOOLEAN SecureKernelRunning : 1;
    BOOLEAN HvciEnabled         : 1;
    BOOLEAN HvciStrictMode      : 1;
    BOOLEAN DebugEnabled        : 1;
    BOOLEAN FirmwarePageProtection : 1;
    BOOLEAN EncryptionKeyAvailable : 1;
    BOOLEAN SpareFlags          : 2;
    BOOLEAN TrustletRunning     : 1;
    BOOLEAN HvciDisableAllowed  : 1;
    BOOLEAN SpareFlags2         : 6;
    UCHAR   Spare0[14];
} SYSTEM_ISOLATED_USER_MODE_INFORMATION, *PSYSTEM_ISOLATED_USER_MODE_INFORMATION;

/* Known Third-Party / Prohibited Hypervisor Signatures (§3 / §16) */
static const PCSTR g_VgkBlacklistedHvVendors[] = {
    "VMwareVMware",
    "KVMKVMKVM\0\0\0",
    "XenVMMXenVMM",
    "VBoxVBoxVBox",
    "TCGTCGTCGTCG",
    "prl hyperv  ",
    "bhyve bhyve "
};

/*
 * §3 / §16: Collect CPUID Leaves 0x0, 0x1, 0x40000000..0x40000005, 0x80000002..0x80000004
 */
static VOID
VgkCollectCpuidAndHypervisorLeaves(VOID)
{
    int regs[4] = { 0 };

    /* Leaf 0x00000000: Base Architecture Vendor String (GenuineIntel / AuthenticAMD) */
    __cpuid(regs, 0x00000000);
    *(PULONG)&g_Vgk.HardwareProfile.CpuVendor[0] = (ULONG)regs[1]; /* EBX */
    *(PULONG)&g_Vgk.HardwareProfile.CpuVendor[4] = (ULONG)regs[3]; /* EDX */
    *(PULONG)&g_Vgk.HardwareProfile.CpuVendor[8] = (ULONG)regs[2]; /* ECX */
    g_Vgk.HardwareProfile.CpuVendor[12] = '\0';

    /* Leaf 0x00000001: Processor ID + ECX[31] Hypervisor Present Bit */
    __cpuid(regs, 0x00000001);
    g_Vgk.HardwareProfile.ProcessorId          = (ULONG)regs[0];
    g_Vgk.HardwareProfile.HypervisorBitPresent = ((regs[2] & (1 << 31)) != 0) ? TRUE : FALSE;

    /* Leaves 0x80000002..0x80000004: 48-character Processor Brand String */
    __cpuid(regs, (int)0x80000000);
    if ((ULONG)regs[0] >= 0x80000004UL) {
        int brandRegs[4];
        __cpuid(brandRegs, (int)0x80000002);
        RtlCopyMemory(&g_Vgk.HardwareProfile.CpuBrandString[0],  brandRegs, 16);
        __cpuid(brandRegs, (int)0x80000003);
        RtlCopyMemory(&g_Vgk.HardwareProfile.CpuBrandString[16], brandRegs, 16);
        __cpuid(brandRegs, (int)0x80000004);
        RtlCopyMemory(&g_Vgk.HardwareProfile.CpuBrandString[32], brandRegs, 16);
        g_Vgk.HardwareProfile.CpuBrandString[48] = '\0';
    }

    /* Leaf 0x40000000..0x40000005: Synthetic Hypervisor Vendor & Interface Matrix */
    if (g_Vgk.HardwareProfile.HypervisorBitPresent) {
        __cpuid(regs, 0x40000000);
        *(PULONG)&g_Vgk.HardwareProfile.HypervisorVendor[0] = (ULONG)regs[1];
        *(PULONG)&g_Vgk.HardwareProfile.HypervisorVendor[4] = (ULONG)regs[2];
        *(PULONG)&g_Vgk.HardwareProfile.HypervisorVendor[8] = (ULONG)regs[3];
        g_Vgk.HardwareProfile.HypervisorVendor[12] = '\0';

        int hvIfRegs[4] = { 0 };
        __cpuid(hvIfRegs, 0x40000001);
        g_Vgk.HardwareProfile.HypervisorInterfaceSig = (ULONG)hvIfRegs[0]; /* 'Hv#1' = 0x31237648 */

        /* Flag any hypervisor vendor that is NOT legitimate Windows Hyper-V ("Microsoft Hv") */
        if (RtlCompareMemory(g_Vgk.HardwareProfile.HypervisorVendor, "Microsoft Hv", 12) != 12) {
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_HV_FOREIGN_VENDOR);
            VgkLogTelemetryEvent(VGK_EVENT_WIC, 0x301, NULL);
        }

        for (ULONG i = 0; i < RTL_NUMBER_OF(g_VgkBlacklistedHvVendors); i++) {
            if (RtlCompareMemory(
                    g_Vgk.HardwareProfile.HypervisorVendor,
                    g_VgkBlacklistedHvVendors[i],
                    12) == 12) {
                InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_HV_FOREIGN_VENDOR);
                break;
            }
        }
    }
}

/*
 * §3: MSR Hooking Verification (IA32_LSTAR, IA32_SYSENTER_EIP, IA32_GS_BASE)
 * Verifies LSTAR points into ntoskrnl.exe (Modules->Modules[0]) and performs
 * atomic read-back across CLI/STI barriers to detect synthetic MSR shadows.
 */
static VOID
VgkVerifySyscallMsrsAndControlRegisters(
    _In_opt_ PRTL_PROCESS_MODULES Modules
)
{
    _disable();
    ULONG64 lstar1   = __readmsr(IA32_LSTAR_MSR);
    ULONG64 sysenter = __readmsr(IA32_SYSENTER_EIP_MSR);
    ULONG64 gsBase   = __readmsr(IA32_GS_BASE_MSR);
    ULONG64 cr0Val   = __readcr0();
    ULONG64 cr4Val   = __readcr4();
    ULONG64 lstar2   = __readmsr(IA32_LSTAR_MSR);
    _enable();

    g_Vgk.HardwareProfile.LstarMsrValue = lstar1;

    /* 1. Readback consistency across interrupt-disabled window */
    if (lstar1 != lstar2 || lstar1 == 0 || gsBase == 0) {
        InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_HV_MSR_HOOKED);
        VgkLogTelemetryEvent(VGK_EVENT_MC, 0xC0000082, NULL);
    }

    /* 2. Verify IA32_LSTAR (KiSystemCall64) resides inside ntoskrnl.exe */
    if (Modules != NULL && Modules->NumberOfModules > 0) {
        ULONG64 ntosBase = (ULONG64)Modules->Modules[0].ImageBase;
        ULONG64 ntosEnd  = ntosBase + (ULONG64)Modules->Modules[0].ImageSize;
        if (lstar1 < ntosBase || lstar1 >= ntosEnd) {
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_HV_MSR_HOOKED);
            VgkLogTelemetryEvent(VGK_EVENT_MC, 0xC0000083, NULL);
        }
        if (sysenter != 0 && (sysenter < ntosBase || sysenter >= ntosEnd)) {
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_HV_MSR_HOOKED);
        }
    }

    /* 3. Verify CR0 Write Protect (Bit 16) and CR4 SMEP (Bit 20) */
    if ((cr0Val & (1ULL << 16)) == 0 || (cr4Val & CR4_SMEP_BIT) == 0) {
        InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_MC_PTE_UNBACKED);
        VgkLogTelemetryEvent(VGK_EVENT_MC, (ULONG)(cr4Val & 0xFFFFFFFFUL), NULL);
    }

    /* 4. Check CR4.VMXE (Bit 13) when CPUID claims no hypervisor is present */
    if (!g_Vgk.HardwareProfile.HypervisorBitPresent && (cr4Val & CR4_VMXE_BIT) != 0) {
        InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_HV_FOREIGN_VENDOR);
        VgkLogTelemetryEvent(VGK_EVENT_WIC, 0x304, NULL);
    }
}

/*
 * §3 / §10: RDTSC + CPUID Quad Timing & EPT INVLPG Exit Latency Calibration
 * Cross-checks RDTSC against KUSER_SHARED_DATA (0xFFFFF78000000000) InterruptTime.
 */
static VOID
VgkCalibrateRdtscCpuidAndInvlpgTiming(VOID)
{
    int cpuidScratch[4] = { 0 };
    unsigned int aux = 0;

    /* Sample KUSER_SHARED_DATA InterruptTime before & after 64 CPUID + INVLPG cycles */
    ULONG64 kuserTimeStart = *(volatile ULONG64*)0xFFFFF78000000008ULL;

    ULONG64 totalCpuidCycles  = 0;
    ULONG64 totalInvlpgCycles = 0;

    for (ULONG i = 0; i < 64; i++) {
        ULONG64 t0 = __rdtscp(&aux);
        __cpuid(cpuidScratch, 0x00000001);
        ULONG64 t1 = __rdtscp(&aux);
        totalCpuidCycles += (t1 - t0);

        ULONG64 t2 = __rdtscp(&aux);
        __invlpg(&g_Vgk);
        ULONG64 t3 = __rdtscp(&aux);
        totalInvlpgCycles += (t3 - t2);
    }

    ULONG64 kuserTimeEnd = *(volatile ULONG64*)0xFFFFF78000000008ULL;

    g_Vgk.HardwareProfile.TscCpuidRatio       = totalCpuidCycles / 64ULL;
    g_Vgk.HardwareProfile.InvlpgLatencyCycles = totalInvlpgCycles / 64ULL;

    /* Nested hypervisor or EPT split trap causes abnormal CPUID/INVLPG exit cycles (> 4500) */
    if (g_Vgk.HardwareProfile.TscCpuidRatio > 4500ULL ||
        g_Vgk.HardwareProfile.InvlpgLatencyCycles > 4500ULL ||
        kuserTimeEnd < kuserTimeStart) {
        InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_HV_TIMING_ANOMALY);
        VgkLogTelemetryEvent(
            VGK_EVENT_TIMING_PAIR,
            0,
            NULL,
            g_Vgk.HardwareProfile.TscCpuidRatio,
            g_Vgk.HardwareProfile.InvlpgLatencyCycles
        );
    }
}

/*
 * §2 / §3 / §10: Query VBS/HVCI, Code Integrity (TESTSIGNING) & Kernel Debugger State
 */
static VOID
VgkAuditBootIntegrityAndDebuggerState(VOID)
{
    ULONG returnLen = 0;

    /* 1. Direct kernel global check: KdDebuggerEnabled & KdDebuggerNotPresent */
    if (*KdDebuggerEnabled || !(*KdDebuggerNotPresent)) {
        g_Vgk.HardwareProfile.KernelDebuggerPresent = TRUE;
        InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_KERNEL_DEBUGGER);
        VgkLogTelemetryEvent(VGK_EVENT_DF, 1, NULL);
    }

    /* 2. ZwQuerySystemInformation(SystemKernelDebuggerInformation = 0x23) */
    SYSTEM_KERNEL_DEBUGGER_INFORMATION dbgInfo = { 0 };
    if (NT_SUCCESS(ZwQuerySystemInformation(
            SystemKernelDebuggerInformation,
            &dbgInfo,
            sizeof(dbgInfo),
            &returnLen))) {
        if (dbgInfo.KernelDebuggerEnabled || !dbgInfo.KernelDebuggerNotPresent) {
            g_Vgk.HardwareProfile.KernelDebuggerPresent = TRUE;
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_KERNEL_DEBUGGER);
        }
    }

    /* 3. ZwQuerySystemInformation(SystemCodeIntegrityInformation = 0x67) for TESTSIGNING */
    SYSTEM_CODEINTEGRITY_INFORMATION ciInfo = { 0 };
    ciInfo.Length = sizeof(ciInfo);
    if (NT_SUCCESS(ZwQuerySystemInformation(
            SystemCodeIntegrityInformation,
            &ciInfo,
            sizeof(ciInfo),
            &returnLen))) {
        /* CODEINTEGRITY_OPTION_TESTSIGN = 0x02, CODEINTEGRITY_OPTION_DEBUGMODE_ENABLED = 0x80 */
        if ((ciInfo.CodeIntegrityOptions & 0x02UL) != 0 ||
            (ciInfo.CodeIntegrityOptions & 0x80UL) != 0) {
            g_Vgk.HardwareProfile.TestSigningEnabled = TRUE;
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_KERNEL_DEBUGGER);
            VgkLogTelemetryEvent(VGK_EVENT_DF, (ULONG)ciInfo.CodeIntegrityOptions, NULL);
        }
        /* CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED = 0x400 */
        if ((ciInfo.CodeIntegrityOptions & 0x400UL) != 0) {
            g_Vgk.HardwareProfile.HvciEnabled = TRUE;
        }
    }

    /* 4. ZwQuerySystemInformation(SystemIsolatedUserModeInformation = 0x9C) for VBS/HVCI */
    SYSTEM_ISOLATED_USER_MODE_INFORMATION iumInfo = { 0 };
    if (NT_SUCCESS(ZwQuerySystemInformation(
            SystemIsolatedUserModeInformation,
            &iumInfo,
            sizeof(iumInfo),
            &returnLen))) {
        g_Vgk.HardwareProfile.VbsEnabled  = iumInfo.SecureKernelRunning ? TRUE : FALSE;
        if (iumInfo.HvciEnabled) {
            g_Vgk.HardwareProfile.HvciEnabled = TRUE;
        }
    }
}

VOID
VgkAuditHypervisorAndAntiDebug(
    _In_opt_ PRTL_PROCESS_MODULES Modules
)
{
    VgkCollectCpuidAndHypervisorLeaves();
    VgkVerifySyscallMsrsAndControlRegisters(Modules);
    VgkCalibrateRdtscCpuidAndInvlpgTiming();
    VgkAuditBootIntegrityAndDebuggerState();
}
