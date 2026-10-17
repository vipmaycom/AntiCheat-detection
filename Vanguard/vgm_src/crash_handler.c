/*
 * crash_handler.c - VGN-4 Crash Handler & Dynamic dbgcore.dll Minidump Generator
 * Reconstructed via REA from vgm.exe (.riot1 strings @ 0x1419d1b14 .. 0x1419d2118):
 *   - "VGN-4 An unrecoverable error has occurred and the process must be terminated!"
 *   - "Would you like to create a crash dump to aid the developers in troubleshooting this issue?"
 *   - "Would you like to create a FULL crash dump?"
 *   - "dbgcore.dll" -> "MiniDumpWriteDump"
 *   - "%s\\%s_%llu_%08x.%s" ("\Crashes" / "\Minidump")
 *   - "explorer.exe \"%s\""
 */

#include "vgm.h"

#define VGM_MSG_FATAL_TITLE             L"Riot Vanguard - Critical Error (VGN-4)"
#define VGM_MSG_VGN4_PROMPT             \
    L"VGN-4 An unrecoverable error has occurred and the process must be terminated!\r\n\r\n" \
    L"Would you like to create a crash dump to aid the developers in troubleshooting this issue? " \
    L"This may take up to 5 minutes."

#define VGM_MSG_FULL_DUMP_PROMPT        \
    L"Would you like to create a FULL crash dump?\r\n\r\n" \
    L"A FULL dump includes all process memory and is much larger, while a SMALL dump " \
    L"only includes thread stacks and context."

#define VGM_MSG_DUMP_SUCCESS_FMT        \
    L"The crash dump has been created here:\r\n%s\r\n\r\n" \
    L"Would you like to open the crash dump folder now?"

#define VGM_MSG_DUMP_FAILED_FMT         \
    L"Failed to create the dump file (Error: 0x%08X)."

/* Minimal MINIDUMP_EXCEPTION_INFORMATION and MINIDUMP_TYPE definitions for dynamic dbgcore.dll */
typedef enum _VGM_MINIDUMP_TYPE {
    VgmMiniDumpNormal         = 0x00000000,
    VgmMiniDumpWithDataSegs   = 0x00000001,
    VgmMiniDumpWithFullMemory = 0x00000002,
    VgmMiniDumpWithHandleData = 0x00000004
} VGM_MINIDUMP_TYPE;

#pragma pack(push, 4)
typedef struct _VGM_MINIDUMP_EXCEPTION_INFORMATION {
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL ClientPointers;
} VGM_MINIDUMP_EXCEPTION_INFORMATION, *PVGM_MINIDUMP_EXCEPTION_INFORMATION;
#pragma pack(pop)

typedef BOOL (WINAPI *PFN_MiniDumpWriteDump)(
    HANDLE hProcess,
    DWORD ProcessId,
    HANDLE hFile,
    VGM_MINIDUMP_TYPE DumpType,
    PVGM_MINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    PVOID UserStreamParam,
    PVOID CallbackParam
);

/*
 * Dynamically loads dbgcore.dll -> MiniDumpWriteDump and writes `%s\%s_%llu_%08x.%s`
 */
bool VgmWriteCrashMinidump(
    EXCEPTION_POINTERS *ExceptionInfo,
    bool FullDump,
    wchar_t *OutPath,
    size_t MaxChars
)
{
    HMODULE hDbgCore = LoadLibraryW(L"dbgcore.dll");
    if (hDbgCore == NULL) {
        hDbgCore = LoadLibraryW(L"dbghelp.dll");
    }
    if (hDbgCore == NULL) {
        return false;
    }

    PFN_MiniDumpWriteDump fnMiniDumpWriteDump = (PFN_MiniDumpWriteDump)(
        (uintptr_t)GetProcAddress(hDbgCore, "MiniDumpWriteDump")
    );
    if (fnMiniDumpWriteDump == NULL) {
        FreeLibrary(hDbgCore);
        return false;
    }

    wchar_t tempDir[MAX_PATH] = { 0 };
    if (GetTempPathW(MAX_PATH, tempDir) == 0) {
        wcscpy_s(tempDir, MAX_PATH, L"C:\\Windows\\Temp\\");
    }

    wchar_t crashDir[MAX_PATH] = { 0 };
    _snwprintf_s(crashDir, MAX_PATH, _TRUNCATE, L"%sCrashes", tempDir);
    CreateDirectoryW(crashDir, NULL);

    FILETIME ft = { 0 };
    GetSystemTimeAsFileTime(&ft);
    unsigned long long timestamp = (((unsigned long long)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    DWORD exceptionCode = (ExceptionInfo && ExceptionInfo->ExceptionRecord)
        ? ExceptionInfo->ExceptionRecord->ExceptionCode
        : 0xE0000001U;

    wchar_t dumpFile[MAX_PATH] = { 0 };
    _snwprintf_s(
        dumpFile,
        MAX_PATH,
        _TRUNCATE,
        L"%s\\%s_%llu_%08x.%s",
        crashDir,
        L"vgm",
        timestamp,
        exceptionCode,
        L"dmp"
    );

    if (OutPath != NULL && MaxChars > 0) {
        wcsncpy_s(OutPath, MaxChars, dumpFile, _TRUNCATE);
    }

    HANDLE hFile = CreateFileW(
        dumpFile,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        FreeLibrary(hDbgCore);
        return false;
    }

    VGM_MINIDUMP_EXCEPTION_INFORMATION exParam;
    exParam.ThreadId          = GetCurrentThreadId();
    exParam.ExceptionPointers = ExceptionInfo;
    exParam.ClientPointers    = FALSE;

    VGM_MINIDUMP_TYPE dumpType = FullDump
        ? (VGM_MINIDUMP_TYPE)(VgmMiniDumpWithFullMemory | VgmMiniDumpWithHandleData)
        : (VGM_MINIDUMP_TYPE)(VgmMiniDumpNormal | VgmMiniDumpWithDataSegs);

    BOOL ok = fnMiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        dumpType,
        (ExceptionInfo != NULL) ? &exParam : NULL,
        NULL,
        NULL
    );

    CloseHandle(hFile);
    FreeLibrary(hDbgCore);
    return (ok != FALSE);
}

/*
 * UnhandledExceptionFilter implementing the VGN-4 dialog sequence
 */
LONG WINAPI VgmUnhandledExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
    int resp = MessageBoxW(
        NULL,
        VGM_MSG_VGN4_PROMPT,
        VGM_MSG_FATAL_TITLE,
        MB_YESNO | MB_ICONERROR | MB_SYSTEMMODAL
    );

    if (resp == IDYES) {
        int fullChoice = MessageBoxW(
            NULL,
            VGM_MSG_FULL_DUMP_PROMPT,
            VGM_MSG_FATAL_TITLE,
            MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL
        );

        wchar_t dumpPath[MAX_PATH] = { 0 };
        bool ok = VgmWriteCrashMinidump(ExceptionInfo, (fullChoice == IDYES), dumpPath, MAX_PATH);
        if (ok) {
            wchar_t msg[512] = { 0 };
            _snwprintf_s(msg, 512, _TRUNCATE, VGM_MSG_DUMP_SUCCESS_FMT, dumpPath);
            if (MessageBoxW(NULL, msg, VGM_MSG_FATAL_TITLE, MB_YESNO | MB_ICONINFORMATION) == IDYES) {
                wchar_t cmdLine[MAX_PATH + 32] = { 0 };
                _snwprintf_s(cmdLine, MAX_PATH + 32, _TRUNCATE, L"explorer.exe /select,\"%s\"", dumpPath);

                STARTUPINFOW si;
                PROCESS_INFORMATION pi;
                ZeroMemory(&si, sizeof(si));
                ZeroMemory(&pi, sizeof(pi));
                si.cb = sizeof(si);
                if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                }
            }
        } else {
            wchar_t errMsg[256] = { 0 };
            _snwprintf_s(errMsg, 256, _TRUNCATE, VGM_MSG_DUMP_FAILED_FMT, GetLastError());
            MessageBoxW(NULL, errMsg, VGM_MSG_FATAL_TITLE, MB_OK | MB_ICONWARNING);
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void VgmInstallCrashHandler(void)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(VgmUnhandledExceptionFilter);
}
