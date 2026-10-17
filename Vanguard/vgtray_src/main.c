/*
 * main.c
 * Entry Point, Single-Instance Guard, and System Tray Window for vgtray.exe
 * Reconstructed from FUN_140089200, FUN_140091060, FUN_140091470, FUN_140091850
 */

#include "vgtray.h"
#include <stdarg.h>

VGTRAY_CONTEXT g_VgTray = { 0 };
static UINT g_uTaskbarRestartMsg = 0;

/* ============================================================================
 * LOGGING SUBSYSTEM (%LOCALAPPDATA%\Riot Games\Riot Vanguard\vgtray.log)
 * Matches format "[%04u-%02u-%02u %02u:%02u:%02u] %ls\n"
 * ============================================================================ */
static void VgTrayInitPaths(void)
{
    WCHAR wszModulePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, wszModulePath, MAX_PATH);
    wcscpy_s(g_VgTray.wszInstallDir, MAX_PATH, wszModulePath);
    WCHAR* pLastSlash = wcsrchr(g_VgTray.wszInstallDir, L'\\');
    if (pLastSlash) {
        *pLastSlash = L'\0';
    }

    WCHAR wszLocalAppData[MAX_PATH] = { 0 };
    DWORD cch = GetEnvironmentVariableW(L"LOCALAPPDATA", wszLocalAppData, MAX_PATH);
    if (cch > 0 && cch < MAX_PATH) {
        swprintf_s(g_VgTray.wszLocalAppDataDir, MAX_PATH, L"%s\\Riot Games", wszLocalAppData);
        CreateDirectoryW(g_VgTray.wszLocalAppDataDir, NULL);
        swprintf_s(g_VgTray.wszLocalAppDataDir, MAX_PATH, L"%s\\Riot Games\\Riot Vanguard", wszLocalAppData);
        CreateDirectoryW(g_VgTray.wszLocalAppDataDir, NULL);

        swprintf_s(g_VgTray.wszLogFilePath, MAX_PATH, L"%s\\vgtray.log", g_VgTray.wszLocalAppDataDir);
        swprintf_s(g_VgTray.wszSettingsFilePath, MAX_PATH, L"%s\\vgtray-settings.json", g_VgTray.wszLocalAppDataDir);
    }
}

void VgTrayLogW(const wchar_t* fmt, ...)
{
    if (g_VgTray.wszLogFilePath[0] == L'\0') {
        return;
    }

    WCHAR wszBody[1024] = { 0 };
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(wszBody, _countof(wszBody), _TRUNCATE, fmt, args);
    va_end(args);

    SYSTEMTIME st;
    GetLocalTime(&st);

    WCHAR wszLine[1200] = { 0 };
    int cchLine = swprintf_s(
        wszLine,
        _countof(wszLine),
        L"[%04u-%02u-%02u %02u:%02u:%02u] %ls\r\n",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond,
        wszBody);

    if (cchLine <= 0) {
        return;
    }

    char szUtf8[2400] = { 0 };
    int cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, wszLine, cchLine, szUtf8, sizeof(szUtf8), NULL, NULL);
    if (cbUtf8 > 0) {
        HANDLE hFile = CreateFileW(
            g_VgTray.wszLogFilePath,
            FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD dwWritten = 0;
            WriteFile(hFile, szUtf8, (DWORD)cbUtf8, &dwWritten, NULL);
            CloseHandle(hFile);
        }
    }
}

/* ============================================================================
 * OLD INSTANCE TERMINATION GUARD (FUN_140089200)
 * Walks CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) and terminates any
 * previous vgtray.exe process before acquiring Global\vgtray_mutex.
 * ============================================================================ */
static void VgTrayTerminatePreviousInstances(void)
{
    DWORD dwSelfPid = GetCurrentProcessId();
    WCHAR wszSelfPath[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, wszSelfPath, MAX_PATH);

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32W pe = { 0 };
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (pe.th32ProcessID != dwSelfPid &&
                _wcsicmp(pe.szExeFile, L"vgtray.exe") == 0) {
                /* OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE) = 0x101001 */
                HANDLE hProc = OpenProcess(0x101001, FALSE, pe.th32ProcessID);
                if (hProc != NULL) {
                    WCHAR wszOtherPath[MAX_PATH] = { 0 };
                    DWORD cchOther = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProc, 0, wszOtherPath, &cchOther)) {
                        if (_wcsicmp(wszSelfPath, wszOtherPath) == 0) {
                            TerminateProcess(hProc, 0);
                            WaitForSingleObject(hProc, 3000);
                        }
                    }
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }

    CloseHandle(hSnap);
}

/* ============================================================================
 * EXIT EVENT WATCHER THREAD (FUN_140089b20)
 * Waits on Global\vgtray_exit_event and posts WM_CLOSE to the tray window.
 * ============================================================================ */
static DWORD WINAPI VgTrayExitEventThread(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);
    HANDLE hEvents[2] = { g_VgTray.hExitEvent, g_VgTray.hStopWorkerEvent };
    DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
    if (dwWait == WAIT_OBJECT_0 && g_VgTray.hTrayWnd != NULL) {
        PostMessageW(g_VgTray.hTrayWnd, WM_CLOSE, 0, 0);
    }
    return 0;
}

/* ============================================================================
 * SYSTEM LIGHT / DARK THEME QUERY (FUN_140089960)
 * Reads HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Themes\Personalize
 * ============================================================================ */
BOOL VgTrayIsSystemLightTheme(void)
{
    HKEY hKey = NULL;
    DWORD dwLight = 0;
    DWORD cbData = sizeof(dwLight);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, THEME_PERSONALIZE_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, THEME_VAL_LIGHT_THEME, NULL, NULL, (LPBYTE)&dwLight, &cbData);
        RegCloseKey(hKey);
    }
    return (dwLight != 0);
}

/* ============================================================================
 * SHELL_NOTIFYICONW MANAGEMENT (FUN_140091060 / FUN_140091230 / FUN_140091360)
 * Uses official GUID {70D5F62D-0932-47D8-9E1A-BA4B95549E3E} and uFlags=0xA7.
 * ============================================================================ */
static BOOL VgTrayRegisterNotifyIcon(HWND hWnd)
{
    NOTIFYICONDATAW nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = 0;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_GUID | NIF_SHOWTIP; /* 0xA7 */
    nid.uCallbackMessage = WM_VGTRAY_NOTIFYICON;                            /* 0x8001 */
    nid.hIcon = g_VgTray.hTrayIcon;
    nid.guidItem = VGTRAY_NOTIFYICON_GUID;
    wcscpy_s(nid.szTip, _countof(nid.szTip), L"Riot Vanguard");

    /* Remove stale icon first, then add and set version 4 (FUN_140091060) */
    Shell_NotifyIconW(NIM_DELETE, &nid);
    if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
        return FALSE;
    }

    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

static void VgTrayRemoveNotifyIcon(HWND hWnd)
{
    NOTIFYICONDATAW nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uFlags = NIF_GUID;
    nid.guidItem = VGTRAY_NOTIFYICON_GUID;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void VgTrayShowBalloonNotification(const wchar_t* wszTitle, const wchar_t* wszBody, DWORD dwInfoFlags)
{
    if (g_VgTray.hTrayWnd == NULL) {
        return;
    }

    NOTIFYICONDATAW nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = g_VgTray.hTrayWnd;
    nid.uFlags = NIF_INFO | NIF_GUID | NIF_SHOWTIP;
    nid.guidItem = VGTRAY_NOTIFYICON_GUID;
    nid.dwInfoFlags = dwInfoFlags;
    wcsncpy_s(nid.szInfoTitle, _countof(nid.szInfoTitle), wszTitle ? wszTitle : L"Riot Vanguard", _TRUNCATE);
    wcsncpy_s(nid.szInfo, _countof(nid.szInfo), wszBody ? wszBody : L"", _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

/* ============================================================================
 * TRAY CONTEXT MENU & COMMAND HANDLER (FUN_1400912b0 / FUN_140091850)
 * ============================================================================ */
void VgTrayShowContextMenu(HWND hWnd, POINT pt)
{
    HMENU hMenuBar = LoadMenuW(g_VgTray.hInstance, MAKEINTRESOURCEW(IDR_TRAY_MENU));
    HMENU hPopup = hMenuBar ? GetSubMenu(hMenuBar, 0) : NULL;
    BOOL bCreatedDynamic = FALSE;

    if (hPopup == NULL) {
        hPopup = CreatePopupMenu();
        bCreatedDynamic = TRUE;
        HMENU hMoreMenu = CreatePopupMenu();
        AppendMenuW(hMoreMenu, MF_STRING, IDM_TRAY_EXIT_VANGUARD, L"Exit Vanguard");
        AppendMenuW(hMoreMenu, MF_STRING, IDM_TRAY_UNINSTALL_VANGUARD, L"Uninstall Vanguard");

        AppendMenuW(hPopup, MF_STRING, IDM_TRAY_SHOW_PANEL, L"Show Panel");
        AppendMenuW(hPopup, MF_STRING | (g_VgTray.settings.advanced_view ? MF_CHECKED : MF_UNCHECKED),
                    IDM_TRAY_TOGGLE_ADVANCED, L"Advanced View");
        AppendMenuW(hPopup, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hPopup, MF_STRING, IDM_TRAY_SAVE_LOGS, L"Save Logs...");
        AppendMenuW(hPopup, MF_POPUP, (UINT_PTR)hMoreMenu, L"More");
        AppendMenuW(hPopup, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hPopup, MF_STRING, IDM_TRAY_EXIT_TRAY, L"Exit Tray");
    } else {
        CheckMenuItem(hPopup, IDM_TRAY_TOGGLE_ADVANCED,
                      MF_BYCOMMAND | (g_VgTray.settings.advanced_view ? MF_CHECKED : MF_UNCHECKED));
    }

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hPopup, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
    PostMessageW(hWnd, WM_NULL, 0, 0);

    if (bCreatedDynamic) {
        DestroyMenu(hPopup);
    } else if (hMenuBar) {
        DestroyMenu(hMenuBar);
    }
}

static BOOL VgTrayHandleCommand(HWND hWnd, WORD wCommandId)
{
    UNREFERENCED_PARAMETER(hWnd);

    switch (wCommandId) {
    case IDM_TRAY_EXIT_VANGUARD: /* 0x9C41 */
    {
        int iResp = MessageBoxW(
            NULL,
            L"This will turn off Vanguard. To play games that depend on Vanguard functionality, you will need to restart your computer.\n\nDo you want to continue?",
            L"Are you sure you want to continue?",
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_SETFOREGROUND);
        if (iResp == IDYES) {
            VgTrayLogW(L"User confirmed Exit Vanguard (0x9C41); stopping vgc and vgk services.");
            VgTrayStopServiceByName(L"vgc");
            VgTrayStopServiceByName(L"vgk");
            if (g_VgTray.hTrayWnd != NULL) {
                PostMessageW(g_VgTray.hTrayWnd, WM_CLOSE, 0, 0);
            }
        }
        return TRUE;
    }

    case IDM_TRAY_UNINSTALL_VANGUARD: /* 0x9C42 */
    {
        /* Matches exact MessageBoxW flags 0x133 / confirmation in FUN_140091850 */
        int iResp = MessageBoxW(
            NULL,
            L"This will completely uninstall \"Riot Vanguard\" from your computer. Games that depend on its functionality will stop working.\n\nDo you want to continue?",
            L"Are you sure you want to continue?",
            0x133);
        if (iResp == IDYES) {
            WCHAR wszUninstaller[MAX_PATH] = { 0 };
            swprintf_s(wszUninstaller, MAX_PATH, L"%s\\uninstall.exe", g_VgTray.wszInstallDir);
            if (GetFileAttributesW(wszUninstaller) == INVALID_FILE_ATTRIBUTES) {
                swprintf_s(wszUninstaller, MAX_PATH, L"%s\\installer.exe", g_VgTray.wszInstallDir);
            }

            WCHAR wszCmdLine[MAX_PATH + 32] = { 0 };
            swprintf_s(wszCmdLine, _countof(wszCmdLine), L"\"%s\" --quiet", wszUninstaller);

            STARTUPINFOW si = { 0 };
            PROCESS_INFORMATION pi = { 0 };
            si.cb = sizeof(si);

            VgTrayLogW(L"Launching uninstaller: %ls", wszCmdLine);
            if (CreateProcessW(NULL, wszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
            PostQuitMessage(0);
        }
        return TRUE;
    }

    case IDM_TRAY_EXIT_TRAY: /* 0x9C43 */
        PostQuitMessage(0);
        return TRUE;

    case IDM_TRAY_SAVE_LOGS: /* 0x9C44 */
        VgTrayExportLogsZipInteractive(g_VgTray.hPanelWnd ? g_VgTray.hPanelWnd : g_VgTray.hTrayWnd);
        return TRUE;

    case IDM_TRAY_TOGGLE_ADVANCED: /* 0x9C45 */
        g_VgTray.settings.advanced_view = !g_VgTray.settings.advanced_view;
        VgTraySaveSettingsJson(&g_VgTray.settings);
        VgTrayRefreshPanelLayout();
        return TRUE;

    case IDM_TRAY_SHOW_PANEL: /* 0x9C4A */
        VgTrayShowPanelWindow();
        return TRUE;

    default:
        return FALSE;
    }
}

/* ============================================================================
 * TRAY CALLBACK DISPATCHER (FUN_140091470)
 * Handles WM_VGTRAY_NOTIFYICON (0x8001):
 * - 0x007B (WM_CONTEXTMENU): Open right-click menu
 * - 0x0400 (NIN_SELECT): Toggle Vanguard Panel window (300ms debounce)
 * - 0x0404 (NIN_BALLOONTIMEOUT): Clear balloon state
 * - 0x0405 (NIN_BALLOONUSERCLICK): Show MessageBox & open Riot Support URL
 * ============================================================================ */
static LRESULT VgTrayHandleNotifyIconEvent(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    static DWORD s_dwLastClickTick = 0;
    WORD wEvent = LOWORD(lParam);

    if (wEvent == WM_CONTEXTMENU || wEvent == WM_RBUTTONUP) { /* 0x7B */
        POINT pt = { 0 };
        if (!GetCursorPos(&pt)) {
            pt.x = (SHORT)LOWORD(wParam);
            pt.y = (SHORT)HIWORD(wParam);
        }
        VgTrayShowContextMenu(hWnd, pt);
        return 1;
    } else if (wEvent == NIN_SELECT || wEvent == (NIN_SELECT | NINF_KEY) || wEvent == WM_LBUTTONUP) { /* 0x400 */
        DWORD dwNow = GetTickCount();
        if (dwNow - s_dwLastClickTick > 299) {
            s_dwLastClickTick = dwNow;
            if (g_VgTray.hPanelWnd && IsWindowVisible(g_VgTray.hPanelWnd)) {
                VgTrayHidePanelWindow();
            } else {
                VgTrayShowPanelWindow();
            }
        }
        return 1;
    } else if (wEvent == NIN_BALLOONTIMEOUT) { /* 0x404 */
        NOTIFYICONDATAW nid = { 0 };
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = hWnd;
        nid.uFlags = NIF_GUID | NIF_SHOWTIP;
        nid.guidItem = VGTRAY_NOTIFYICON_GUID;
        Shell_NotifyIconW(NIM_MODIFY, &nid);
        return 1;
    } else if (wEvent == NIN_BALLOONUSERCLICK) { /* 0x405 */
        VGTRAY_TOAST_ITEM toast = { 0 };
        BOOL bHasToast = FALSE;

        EnterCriticalSection(&g_VgTray.csToastLock);
        if (g_VgTray.bHasPendingToast) {
            toast = g_VgTray.lastToast;
            bHasToast = TRUE;
        }
        LeaveCriticalSection(&g_VgTray.csToastLock);

        if (bHasToast) {
            int iResp = MessageBoxW(NULL, toast.wszMessage, L"Riot Vanguard", MB_YESNO | MB_ICONWARNING);
            if (iResp == IDYES && toast.wszHelpUrl[0] != L'\0') {
                ShellExecuteW(NULL, L"open", toast.wszHelpUrl, NULL, NULL, SW_SHOWNORMAL);
            }
        } else {
            ShellExecuteW(NULL, L"open", VANGUARD_SUPPORT_URL_REQ, NULL, NULL, SW_SHOWNORMAL);
        }
        return 1;
    }

    return 0;
}

/* ============================================================================
 * VANGUARD TRAY WINDOW PROCEDURE (FUN_140091060)
 * ============================================================================ */
static LRESULT CALLBACK VgTrayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == g_uTaskbarRestartMsg && g_uTaskbarRestartMsg != 0) {
        VgTrayRegisterNotifyIcon(hWnd);
        return 0;
    }

    switch (uMsg) {
    case WM_CREATE:
        if (!VgTrayRegisterNotifyIcon(hWnd)) {
            ExitProcess(0);
        }
        WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION);
        SetTimer(hWnd, 1, 500, NULL);
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            /* Periodic 500ms tray icon / toast check (FUN_140091360) */
            VGTRAY_TOAST_ITEM toast = { 0 };
            if (VgTrayPollKbToastNotification(&toast)) {
                VgTrayShowBalloonNotification(toast.wszTitle, toast.wszMessage, NIIF_WARNING);
            }
            return 0;
        }
        break;

    case WM_COMMAND:
        if (VgTrayHandleCommand(hWnd, LOWORD(wParam))) {
            return 0;
        }
        break;

    case WM_VGTRAY_NOTIFYICON:
        if (VgTrayHandleNotifyIconEvent(hWnd, wParam, lParam)) {
            return 0;
        }
        break;

    case WM_VGTRAY_SHOW_CONTEXT:
    {
        POINT pt = { 0 };
        GetCursorPos(&pt);
        VgTrayShowContextMenu(hWnd, pt);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        WTSUnRegisterSessionNotification(hWnd);
        VgTrayRemoveNotifyIcon(hWnd);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

/* ============================================================================
 * ENTRY POINT (wWinMain — FUN_140089200)
 * ============================================================================ */
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    /* 1. Per-Monitor DPI Awareness V2 (-4) */
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    ZeroMemory(&g_VgTray, sizeof(g_VgTray));
    g_VgTray.hInstance = hInstance;
    InitializeCriticalSection(&g_VgTray.csStateLock);
    InitializeCriticalSection(&g_VgTray.csToastLock);
    VgTrayInitPaths();

    VgTrayLogW(L"vgtray starting (%ls)", VGTRAY_VERSION_WSTR);

    /* 2. Terminate any older running vgtray.exe instance */
    VgTrayTerminatePreviousInstances();

    /* 3. Create Global\vgtray_mutex & Global\vgtray_exit_event (FUN_140089200) */
    g_VgTray.hMutex = CreateMutexW(NULL, TRUE, VGTRAY_MUTEX_NAME);
    if (g_VgTray.hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_VgTray.hMutex) {
            CloseHandle(g_VgTray.hMutex);
        }
        return 2;
    }

    g_VgTray.hExitEvent = CreateEventW(NULL, TRUE, FALSE, VGTRAY_EXIT_EVENT_NAME);
    g_VgTray.hStopWorkerEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    g_VgTray.hPanelReadyEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    /* 4. Load settings JSON & evaluate initial security pre-check state */
    VgTrayLoadSettingsJson(&g_VgTray.settings);
    VgTrayEvaluateSecurityPrecheck(&g_VgTray.precheck);
    VgTrayPollServiceSnapshot(L"vgc", &g_VgTray.svcVgc);
    VgTrayPollServiceSnapshot(L"vgk", &g_VgTray.svcVgk);
    VgTraySaveSettingsJson(&g_VgTray.settings);

    /* 5. Load tray icon matching system light/dark theme */
    g_VgTray.hTrayIcon = LoadIconW(NULL, IDI_SHIELD);
    g_uTaskbarRestartMsg = RegisterWindowMessageW(L"TaskbarCreated");

    /* 6. Register VanguardTrayWindowClass (FUN_140089200) */
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = VgTrayWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = g_VgTray.hTrayIcon;
    wc.lpszClassName = VGTRAY_WINDOW_CLASS;
    if (!RegisterClassExW(&wc)) {
        return 1;
    }

    g_VgTray.hTrayWnd = CreateWindowExW(
        0,
        VGTRAY_WINDOW_CLASS,
        L"Riot Vanguard",
        WS_OVERLAPPEDWINDOW,
        0, 0, 0, 0,
        NULL, NULL, hInstance, NULL);
    if (g_VgTray.hTrayWnd == NULL) {
        return 1;
    }

    /* 7. Spawn worker threads:
     *    - Exit event listener (FUN_140089b20)
     *    - Panel UI + D3D11 dashboard thread (FUN_1400b0ba0)
     *    - Service status monitor thread (FUN_1400b1ea0)
     *    - Registry toast watcher thread (FUN_14008e8f0)
     */
    g_VgTray.hExitWatcherThread = CreateThread(NULL, 0, VgTrayExitEventThread, NULL, 0, NULL);
    g_VgTray.hPanelThread = CreateThread(NULL, 0, VgTrayPanelThread, NULL, 0, NULL);
    if (g_VgTray.hPanelReadyEvent) {
        WaitForSingleObject(g_VgTray.hPanelReadyEvent, 5000);
    }
    g_VgTray.hServiceMonitorThread = CreateThread(NULL, 0, VgTrayServiceMonitorThread, NULL, 0, NULL);
    g_VgTray.hToastWatcherThread = CreateThread(NULL, 0, VgTrayToastWatcherThread, NULL, 0, NULL);

    /* 8. Standard Win32 Message Loop */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    /* 9. Orderly LIFO Teardown */
    SetEvent(g_VgTray.hStopWorkerEvent);
    if (g_VgTray.hPanelWnd) {
        PostMessageW(g_VgTray.hPanelWnd, WM_CLOSE, 0, 0);
    }

    HANDLE hThreads[4];
    DWORD cThreads = 0;
    if (g_VgTray.hToastWatcherThread)   hThreads[cThreads++] = g_VgTray.hToastWatcherThread;
    if (g_VgTray.hServiceMonitorThread) hThreads[cThreads++] = g_VgTray.hServiceMonitorThread;
    if (g_VgTray.hPanelThread)          hThreads[cThreads++] = g_VgTray.hPanelThread;
    if (g_VgTray.hExitWatcherThread)    hThreads[cThreads++] = g_VgTray.hExitWatcherThread;

    if (cThreads > 0) {
        WaitForMultipleObjects(cThreads, hThreads, TRUE, 3000);
        for (DWORD i = 0; i < cThreads; i++) {
            CloseHandle(hThreads[i]);
        }
    }

    if (g_VgTray.hPanelReadyEvent) CloseHandle(g_VgTray.hPanelReadyEvent);
    if (g_VgTray.hStopWorkerEvent) CloseHandle(g_VgTray.hStopWorkerEvent);
    if (g_VgTray.hExitEvent)       CloseHandle(g_VgTray.hExitEvent);
    if (g_VgTray.hMutex) {
        ReleaseMutex(g_VgTray.hMutex);
        CloseHandle(g_VgTray.hMutex);
    }

    DeleteCriticalSection(&g_VgTray.csToastLock);
    DeleteCriticalSection(&g_VgTray.csStateLock);
    return 0;
}
