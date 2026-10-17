/*
 * nirvana_callback.c - ProcessInstrumentationCallback (Nirvana Syscall Hook) & DR7 Probe
 * Reconstructed via REA from vgm.exe:
 *   - FUN_1419cf400 @ 0x1419cf400 (ProcessInstrumentationCallback trampoline)
 *   - FUN_1419c7170 @ 0x1419c7170 (Syscall origin verifier: 0F 05 vs CD 2E, TEB lock, observer list)
 *   - FUN_1419a94dc @ 0x1419a94dc (CPUID GenuineIntel check + NtSetContextThread DR7=0x200 probe)
 */

#include "vgm.h"
#include <psapi.h>

static VGM_NIRVANA_STATE g_NirvanaState = { 0 };

/*
 * Locates ntdll.dll image bounds in the current process so FUN_1419c7170 can distinguish
 * legitimate ntdll.dll syscalls from direct/manual-mapped syscalls.
 */
static bool VgmResolveNtdllBounds(uintptr_t *OutBase, uintptr_t *OutEnd)
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll == NULL) {
        return false;
    }

    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hNtdll, &modInfo, sizeof(modInfo))) {
        return false;
    }

    *OutBase = (uintptr_t)modInfo.lpBaseOfDll;
    *OutEnd  = (uintptr_t)modInfo.lpBaseOfDll + modInfo.SizeOfImage;
    return true;
}

/*
 * FUN_1419c7170: Nirvana Syscall Origin Inspector
 * Inspects the 2 bytes preceding R10 (PostSyscallRip):
 *   - 0x0F 0x05 : standard x64 `syscall` instruction (OpcodeType = 0)
 *   - 0xCD 0x2E : legacy `int 0x2e` interrupt gate (OpcodeType = 1)
 * Uses a thread-reentrancy guard and dispatches to registered observer callbacks.
 */
void VgmNirvanaDispatchInspection(uintptr_t PostSyscallRip, uint64_t ReturnValue)
{
    if (PostSyscallRip < 0x10000ULL || PostSyscallRip > VGM_USER_MODE_LIMIT) {
        return;
    }

    DWORD currentTid = GetCurrentThreadId();
    if (g_NirvanaState.OwnerThreadId == currentTid) {
        /* Prevent recursive instrumentation callback reentrancy on the same thread */
        return;
    }

    if (InterlockedCompareExchange(&g_NirvanaState.ActiveThreadLock, 1, 0) != 0) {
        return;
    }
    g_NirvanaState.OwnerThreadId = currentTid;

    const uint8_t *insnBytes = (const uint8_t *)(PostSyscallRip - 2);
    uint8_t opcodeType = 0xFF;

    if (insnBytes[0] == 0x0F && insnBytes[1] == 0x05) {
        opcodeType = 0; /* `syscall` */
    } else if (insnBytes[0] == 0xCD && insnBytes[1] == 0x2E) {
        opcodeType = 1; /* `int 2Eh` */
        InterlockedIncrement((volatile LONG *)&g_NirvanaState.Int2ESyscallsDetected);
    }

    if (opcodeType != 0xFF) {
        InterlockedIncrement((volatile LONG *)&g_NirvanaState.TotalSyscallsInspected);

        if (PostSyscallRip < g_NirvanaState.NtdllBase || PostSyscallRip >= g_NirvanaState.NtdllEnd) {
            InterlockedIncrement((volatile LONG *)&g_NirvanaState.UnbackedSyscallsDetected);
        }

        PVGM_NIRVANA_OBSERVER_NODE node = g_NirvanaState.ObserverListHead.Next;
        while (node != NULL && node != &g_NirvanaState.ObserverListHead) {
            if (node->Callback != NULL) {
                node->Callback(ReturnValue, PostSyscallRip, opcodeType, node->UserContext);
            }
            node = node->Next;
        }
    }

    g_NirvanaState.OwnerThreadId = 0;
    InterlockedExchange(&g_NirvanaState.ActiveThreadLock, 0);
}

/*
 * Initializes the Nirvana callback subsystem and optionally registers it with
 * NtSetInformationProcess(ProcessInstrumentationCallback = 40).
 */
NTSTATUS VgmNirvanaInitialize(bool InstallProcessCallback)
{
    if (InterlockedCompareExchange(&g_NirvanaState.Initialized, 1, 0) == 0) {
        g_NirvanaState.ObserverListHead.Next = &g_NirvanaState.ObserverListHead;
        g_NirvanaState.ObserverListHead.Prev = &g_NirvanaState.ObserverListHead;
        VgmResolveNtdllBounds(&g_NirvanaState.NtdllBase, &g_NirvanaState.NtdllEnd);
    }

    if (!InstallProcessCallback) {
        return VGM_STATUS_SUCCESS;
    }

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll == NULL) {
        return VGM_STATUS_UNSUCCESSFUL;
    }

    PFN_NtSetInformationProcess fnSetInfo = (PFN_NtSetInformationProcess)(
        (uintptr_t)GetProcAddress(hNtdll, "NtSetInformationProcess")
    );
    if (fnSetInfo == NULL) {
        return VGM_STATUS_UNSUCCESSFUL;
    }

    PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION cbInfo;
    cbInfo.Version  = 0;
    cbInfo.Reserved = 0;
    cbInfo.Callback = NULL; /* Safe query/reset probe on current process */

    return fnSetInfo(GetCurrentProcess(), ProcessInstrumentationCallback, &cbInfo, sizeof(cbInfo));
}

void VgmNirvanaRegisterObserver(PFN_VGM_SYSCALL_OBSERVER Callback, void *UserContext)
{
    if (Callback == NULL) {
        return;
    }

    PVGM_NIRVANA_OBSERVER_NODE node = (PVGM_NIRVANA_OBSERVER_NODE)VgmStubAlloc(sizeof(VGM_NIRVANA_OBSERVER_NODE));
    if (node == NULL) {
        return;
    }

    node->Callback    = Callback;
    node->UserContext = UserContext;

    while (InterlockedCompareExchange(&g_NirvanaState.ActiveThreadLock, 1, 0) != 0) {
        YieldProcessor();
    }

    PVGM_NIRVANA_OBSERVER_NODE tail = g_NirvanaState.ObserverListHead.Prev;
    node->Next = &g_NirvanaState.ObserverListHead;
    node->Prev = tail;
    tail->Next = node;
    g_NirvanaState.ObserverListHead.Prev = node;

    InterlockedExchange(&g_NirvanaState.ActiveThreadLock, 0);
}

void VgmNirvanaShutdown(void)
{
    while (InterlockedCompareExchange(&g_NirvanaState.ActiveThreadLock, 1, 0) != 0) {
        YieldProcessor();
    }

    PVGM_NIRVANA_OBSERVER_NODE node = g_NirvanaState.ObserverListHead.Next;
    while (node != NULL && node != &g_NirvanaState.ObserverListHead) {
        PVGM_NIRVANA_OBSERVER_NODE next = node->Next;
        VgmStubFree(node);
        node = next;
    }

    g_NirvanaState.ObserverListHead.Next = &g_NirvanaState.ObserverListHead;
    g_NirvanaState.ObserverListHead.Prev = &g_NirvanaState.ObserverListHead;
    InterlockedExchange(&g_NirvanaState.ActiveThreadLock, 0);
}

const VGM_NIRVANA_STATE *VgmGetNirvanaState(void)
{
    return &g_NirvanaState;
}

/*
 * FUN_1419a94dc: Hardware Debug Register (DR7) & CPUID ("GenuineIntel") Integrity Probe
 * 1. Executes __cpuid(0) and inspects EBX == 0x756E6547 ("GenuineIntel").
 * 2. Resolves NtGetContextThread / NtSetContextThread in ntdll.dll.
 * 3. Sets ContextFlags = CONTEXT_AMD64 | CONTEXT_DEBUG_REGISTERS (0x100010),
 *    applies Dr7 = 0x200, then restores Dr7 = 0 to detect hardware breakpoint hooks.
 */
bool VgmProbeHardwareDebugRegisters(void)
{
    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);
    bool isGenuineIntel = ((uint32_t)cpuInfo[1] == 0x756E6547U); /* "Genu" in EBX */
    (void)isGenuineIntel;

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll == NULL) {
        return false;
    }

    PFN_NtGetContextThread fnGetCtx = (PFN_NtGetContextThread)(
        (uintptr_t)GetProcAddress(hNtdll, "NtGetContextThread")
    );
    PFN_NtSetContextThread fnSetCtx = (PFN_NtSetContextThread)(
        (uintptr_t)GetProcAddress(hNtdll, "NtSetContextThread")
    );
    if (fnGetCtx == NULL || fnSetCtx == NULL) {
        return false;
    }

    CONTEXT ctx;
    ZeroMemory(&ctx, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    HANDLE hCurrentThread = GetCurrentThread();
    NTSTATUS st = fnGetCtx(hCurrentThread, &ctx);
    if (st < 0) {
        return false;
    }

    /* Check if any hardware breakpoints (DR0..DR3) are externally active */
    bool externalBpDetected = (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0);

    /* Perform the exact DR7 = 0x200 probe and restore from FUN_1419a94dc */
    DWORD64 savedDr7 = ctx.Dr7;
    ctx.Dr7 = 0x200ULL;
    st = fnSetCtx(hCurrentThread, &ctx);
    if (st >= 0) {
        ctx.Dr7 = 0ULL;
        fnSetCtx(hCurrentThread, &ctx);
        ctx.Dr7 = savedDr7;
        fnSetCtx(hCurrentThread, &ctx);
    }

    return !externalBpDetected;
}
