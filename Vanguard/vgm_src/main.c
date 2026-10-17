/*
 * main.c - Entrypoint, TLS Callbacks, WindowStation Validator & OEP Handoff for vgm.exe
 * Reconstructed via REA from C:\Program Files\Riot Vanguard\vgm.exe (v1.19.1-18+20261001.143443):
 *   - TLS Callback 0 @ 0x141fd3ed0
 *   - TLS Callback 1 @ 0x141fd3f10
 *   - entry          @ 0x1419cf13c (calls thunk_FUN_141a54471(rcx, rdx, &LAB_1419cf0c0))
 *   - OEP            @ 0x14036785c (ImageBase + 0x36785C)
 */

#include "vgm.h"

/* ============================================================================
 * 1. TLS CALLBACKS (Reconstructed from .rdata TLS Directory @ 0x1404C4F58)
 * ============================================================================ */

static volatile LONG g_TlsInitialized = 0;

VOID NTAPI VgmTlsCallback0(PVOID DllHandle, DWORD Reason, PVOID Reserved)
{
    (void)DllHandle;
    (void)Reserved;
    if (Reason == DLL_PROCESS_ATTACH) {
        InterlockedExchange(&g_TlsInitialized, 1);
    }
}

VOID NTAPI VgmTlsCallback1(PVOID DllHandle, DWORD Reason, PVOID Reserved)
{
    (void)DllHandle;
    (void)Reserved;
    if (Reason == DLL_PROCESS_ATTACH || Reason == DLL_THREAD_ATTACH) {
        /* Verify thread debug registers on every new thread attach */
        VgmProbeHardwareDebugRegisters();
    }
}

#ifdef _MSC_VER
#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma section(".CRT$XLB", long, read)
__declspec(allocate(".CRT$XLB")) PIMAGE_TLS_CALLBACK g_VgmTlsCallbacks[] = {
    VgmTlsCallback0,
    VgmTlsCallback1,
    NULL
};
#endif

/* ============================================================================
 * 2. WINDOW STATION & EVENT LOG TELEMETRY (USER32.dll & ADVAPI32.dll imports)
 * ============================================================================ */

static bool VgmVerifyInteractiveWindowStation(wchar_t *OutStationName, size_t MaxChars)
{
    HWINSTA hWinSta = GetProcessWindowStation();
    if (hWinSta == NULL) {
        return false;
    }

    DWORD needed = 0;
    wchar_t nameBuf[128] = { 0 };
    if (!GetUserObjectInformationW(hWinSta, UOI_NAME, nameBuf, sizeof(nameBuf), &needed)) {
        return false;
    }

    if (OutStationName != NULL && MaxChars > 0) {
        wcsncpy_s(OutStationName, MaxChars, nameBuf, _TRUNCATE);
    }
    return true;
}

static void VgmReportStartupEvent(const wchar_t *Message)
{
    HANDLE hEventLog = RegisterEventSourceW(NULL, L"Vanguard User-mode Module");
    if (hEventLog != NULL) {
        LPCWSTR strings[1] = { Message };
        ReportEventW(
            hEventLog,
            EVENTLOG_INFORMATION_TYPE,
            0,
            0x00001001L,
            NULL,
            1,
            0,
            strings,
            NULL
        );
        DeregisterEventSource(hEventLog);
    }
}

/* ============================================================================
 * 3. NIRVANA SYSCALL OBSERVER CALLBACK
 * ============================================================================ */

static void VgmDefaultSyscallObserver(
    uint64_t ReturnValue,
    uintptr_t PostSyscallRip,
    uint8_t OpcodeType,
    void *UserContext
)
{
    (void)ReturnValue;
    (void)PostSyscallRip;
    (void)OpcodeType;
    uint32_t *counter = (uint32_t *)UserContext;
    if (counter != NULL) {
        (*counter)++;
    }
}

/* ============================================================================
 * 4. UNPACKED ORIGINAL ENTRY POINT (OEP @ 0x14036785C)
 * ============================================================================ */

static int VgmUnpackedOepMain(HINSTANCE hInstance, LPWSTR lpCmdLine)
{
    (void)hInstance;

    /* 1. Install VGN-4 Unhandled Exception Filter */
    VgmInstallCrashHandler();

    /* 2. Validate WindowStation context (WinSta0 vs Service Session 0) */
    wchar_t stationName[128] = { 0 };
    VgmVerifyInteractiveWindowStation(stationName, 128);

    /* 3. Initialize Nirvana Syscall Instrumentation state and register observer */
    uint32_t observedCalls = 0;
    VgmNirvanaInitialize(false);
    VgmNirvanaRegisterObserver(VgmDefaultSyscallObserver, &observedCalls);

    /* 4. Run hardware debug register (DR7 = 0x200) & CPUID ("GenuineIntel") check */
    bool drClean = VgmProbeHardwareDebugRegisters();

    /* 5. Execute cooperative fiber scheduler (4 fibers: DR7, IPC, WS2_32, Packman Hash) */
    uint32_t iterations = (lpCmdLine && wcsstr(lpCmdLine, L"--daemon")) ? 60U : 2U;
    VgmRunCooperativeScheduler(iterations);

    /* 6. Log status to Windows Event Log if requested */
    wchar_t logMsg[256] = { 0 };
    VgmFormatUnicodeString(
        logMsg,
        256,
        L"vgm.exe initialized on WindowStation=%s, DR7Clean=%d, TLS=%ld",
        stationName[0] ? stationName : L"Unknown",
        drClean ? 1 : 0,
        g_TlsInitialized
    );
    VgmReportStartupEvent(logMsg);

    /* 7. Clean up Nirvana observer nodes allocated on the 'Stub' 64KB slab */
    VgmNirvanaShutdown();
    return 0;
}

/* ============================================================================
 * 5. PACKMAN BOOTSTRAP ENTRYPOINT (entry @ 0x1419CF13C)
 * ============================================================================ */

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
)
{
    (void)hPrevInstance;
    (void)nShowCmd;

    /*
     * Reconstructs `entry` @ 0x1419cf13c:
     *   status = thunk_FUN_141a54471(rcx, rdx, &LAB_1419cf0c0);
     *   if (status != 0 && status != 0xD4494E49) -> jump to OEP (ImageBase + 0x36785C)
     */
    const VGM_PACK_DESCRIPTOR *packDesc = VgmGetEmbeddedPackDescriptor();
    NTSTATUS unpackStatus = VgmPackmanUnpackImage(
        (uintptr_t)hInstance,
        packDesc,
        true /* Validate Keccak-f[1600], Stub slab, Stream Cipher & Adler32/CRC32 */
    );

    if (unpackStatus < 0 && (uint32_t)unpackStatus != VGM_STATUS_ALREADY_INIT) {
        VgmFastFailCheck(VGM_FASTFAIL_MAGIC_VG | ((uint32_t)unpackStatus & 0xFFFFU));
    }

    /* Transfer control to unpacked OEP (0x14036785C) */
    return VgmUnpackedOepMain(hInstance, lpCmdLine);
}
