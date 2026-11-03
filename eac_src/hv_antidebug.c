/*
 * hv_antidebug.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) Hypervisor, MSR, EPT/TSC Timing,
 * VBS/HVCI State, and Kernel Anti-Debug Engine (§3, §10, §16).
 */

#include "eac.h"

#define IA32_SYSENTER_EIP_MSR   0x00000176UL
#define IA32_LSTAR_MSR          0xC0000082UL
#define IA32_GS_BASE_MSR        0xC0000101UL

typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION_LOCAL {
    ULONG Length;
    ULONG CodeIntegrityOptions;
} SYSTEM_CODEINTEGRITY_INFORMATION_LOCAL;

typedef struct _SYSTEM_ISOLATED_USER_MODE_INFORMATION_LOCAL {
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
} SYSTEM_ISOLATED_USER_MODE_INFORMATION_LOCAL;

VOID
EacAuditHypervisorAndAntiDebug(VOID)
{
    if (g_Eac.Unloading) {
        return;
    }

    int regs[4] = { 0 };

    /* 1. CPUID Leaf 0x00000000: Base CPU Vendor String (§16) */
    __cpuid(regs, 0);
    *(PULONG)&g_Eac.HwProfile.CpuVendor[0] = (ULONG)regs[1]; /* EBX */
    *(PULONG)&g_Eac.HwProfile.CpuVendor[4] = (ULONG)regs[3]; /* EDX */
    *(PULONG)&g_Eac.HwProfile.CpuVendor[8] = (ULONG)regs[2]; /* ECX */
    g_Eac.HwProfile.CpuVendor[12] = '\0';

    /* 2. CPUID Leaf 0x00000001: Feature Bits & Hypervisor Present Bit 31 (§3, §16) */
    __cpuid(regs, 1);
    g_Eac.HwProfile.CpuFeatureEcx     = (ULONG)regs[2];
    g_Eac.HwProfile.CpuFeatureEdx     = (ULONG)regs[3];
    g_Eac.HwProfile.HypervisorPresent = ((regs[2] & (1 << 31)) != 0);

    /* 3. CPUID Leaf 0x40000000: Hypervisor Vendor Signature (§3, §16) */
    if (g_Eac.HwProfile.HypervisorPresent) {
        __cpuid(regs, 0x40000000);
        *(PULONG)&g_Eac.HwProfile.HvVendorString[0] = (ULONG)regs[1];
        *(PULONG)&g_Eac.HwProfile.HvVendorString[4] = (ULONG)regs[2];
        *(PULONG)&g_Eac.HwProfile.HvVendorString[8] = (ULONG)regs[3];
        g_Eac.HwProfile.HvVendorString[12] = '\0';

        /* Only "Microsoft Hv" is authorized on standard Windows VBS/Hyper-V hosts */
        if (strcmp(g_Eac.HwProfile.HvVendorString, "Microsoft Hv") != 0) {
            InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_HV_CPUID_OR_VENDOR);
            EacEmitTelemetryChunk(EAC_CHUNK_OP_ALERT, 3, g_Eac.HwProfile.HvVendorString, 0, 1);
        }
    }

    /* 4. CPUID Leaves 0x80000002..0x80000004: Processor Brand String (§16) */
    __cpuid(regs, (int)0x80000000);
    if ((ULONG)regs[0] >= 0x80000004UL) {
        for (int leaf = 0; leaf < 3; ++leaf) {
            __cpuid(regs, (int)(0x80000002UL + leaf));
            RtlCopyMemory(&g_Eac.HwProfile.CpuBrandString[leaf * 16], regs, 16);
        }
        g_Eac.HwProfile.CpuBrandString[48] = '\0';
    }

    /* 5. MSR Integrity Check (IA32_LSTAR, IA32_SYSENTER_EIP, IA32_GS_BASE) (§3) */
    ULONG64 lstar = __readmsr(IA32_LSTAR_MSR);
    g_Eac.HwProfile.Ia32Lstar = lstar;
    if (lstar < 0xFFFF800000000000ULL) {
        InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_HV_MSR_HOOKED);
    }

    /* 6. RDTSC vs CPUID & INVLPG VM-Exit Latency Calibration (§3, §10) */
    ULONG64 totalCpuidCycles  = 0;
    ULONG64 totalInvlpgCycles = 0;
    for (ULONG iter = 0; iter < 16; ++iter) {
        ULONG64 t0 = __rdtsc();
        __cpuid(regs, 1);
        ULONG64 t1 = __rdtsc();
        if (t1 > t0) {
            totalCpuidCycles += (t1 - t0);
        }

        ULONG64 t2 = __rdtsc();
        __invlpg(&g_Eac.ControlRegs);
        ULONG64 t3 = __rdtsc();
        if (t3 > t2) {
            totalInvlpgCycles += (t3 - t2);
        }
    }

    g_Eac.HwProfile.TscCpuidDelta = totalCpuidCycles / 16ULL;
    g_Eac.HwProfile.InvlpgDelta   = totalInvlpgCycles / 16ULL;

    /* Flag extreme VM-exit overhead indicative of EPT hook trapping */
    if (g_Eac.HwProfile.TscCpuidDelta > 15000ULL || g_Eac.HwProfile.InvlpgDelta > 15000ULL) {
        InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_HV_EPT_TIMING_SPLIT);
    }

    /* 7. KdDebuggerEnabled & Hardware Debug Registers (§9, §10) */
    if (*KdDebuggerEnabled || (*KdDebuggerNotPresent == FALSE) || ((__readdr(7) & 0xFFULL) != 0ULL)) {
        g_Eac.HwProfile.KernelDebuggerEnabled = TRUE;
        InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_KERNEL_DEBUGGER);
    }

    /* 8. VBS / HVCI & CodeIntegrity State (§2, §3) */
    if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
        SYSTEM_ISOLATED_USER_MODE_INFORMATION_LOCAL iumInfo;
        RtlZeroMemory(&iumInfo, sizeof(iumInfo));
        ULONG retLen = 0;
        if (NT_SUCCESS(ZwQuerySystemInformation(
            SystemIsolatedUserModeInformation,
            &iumInfo,
            sizeof(iumInfo),
            &retLen
        ))) {
            g_Eac.HwProfile.VbsEnabled  = (iumInfo.SecureKernelRunning != 0);
            g_Eac.HwProfile.HvciEnabled = (iumInfo.HvciEnabled != 0);
        }

        SYSTEM_CODEINTEGRITY_INFORMATION_LOCAL ciInfo;
        ciInfo.Length               = sizeof(ciInfo);
        ciInfo.CodeIntegrityOptions = 0;
        if (NT_SUCCESS(ZwQuerySystemInformation(
            SystemCodeIntegrityInformation,
            &ciInfo,
            sizeof(ciInfo),
            &retLen
        ))) {
            /* CODEINTEGRITY_OPTION_TESTSIGN == 0x02 */
            if (ciInfo.CodeIntegrityOptions & 0x02UL) {
                g_Eac.HwProfile.TestSigningEnabled = TRUE;
                InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_KERNEL_DEBUGGER);
            }
        }
    }
}
