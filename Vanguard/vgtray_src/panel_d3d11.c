/*
 * panel_d3d11.c
 * VanguardPanelClass & VanguardTrayContextMenuHostClass D3D11 + Dashboard Window
 * Reconstructed from FUN_1400b0ba0, FUN_1400aee10, FUN_140095a10, FUN_1400ae8b0
 */

#define COBJMACROS
#include "vgtray.h"
#include <initguid.h>
#pragma warning(push)
#pragma warning(disable: 4201)
#include <d3d11.h>
#include <dxgi.h>
#pragma warning(pop)
#include <dwmapi.h>

/* D3D11 SwapChain Context (FUN_1400b0ba0) */
static ID3D11Device*           s_pD3DDevice    = NULL;
static ID3D11DeviceContext*    s_pD3DContext   = NULL;
static IDXGISwapChain*         s_pSwapChain    = NULL;
static ID3D11RenderTargetView* s_pMainRTV      = NULL;

/* Interactive Button IDs inside VanguardPanelClass */
#define IDC_BTN_REFRESH_STATUS  2001
#define IDC_BTN_SAVE_LOGS       2002
#define IDC_BTN_TOGGLE_ADVANCED 2003
#define IDC_BTN_LEARN_MORE      2004
#define IDC_BTN_EXIT_VANGUARD   2005

/* ============================================================================
 * D3D11 DEVICE & SWAPCHAIN INITIALIZATION (FUN_1400b0ba0)
 * Mirrors vgtray.exe:
 *   1. CreateDXGIFactory1(&IID_IDXGIFactory1)
 *   2. D3D11CreateDevice(..., D3D_DRIVER_TYPE_HARDWARE) with fallback to
 *      D3D_DRIVER_TYPE_WARP
 *   3. CreateSwapChain(DXGI_FORMAT_R8G8B8A8_UNORM, BufferCount=2, SWAP_EFFECT_DISCARD)
 * ============================================================================ */
static void VgPanelCleanupRenderTarget(void)
{
    if (s_pMainRTV != NULL) {
        ID3D11RenderTargetView_Release(s_pMainRTV);
        s_pMainRTV = NULL;
    }
}

static void VgPanelCreateRenderTarget(void)
{
    if (s_pSwapChain == NULL || s_pD3DDevice == NULL) {
        return;
    }
    ID3D11Texture2D* pBackBuffer = NULL;
    if (SUCCEEDED(IDXGISwapChain_GetBuffer(s_pSwapChain, 0, &IID_ID3D11Texture2D, (void**)&pBackBuffer)) &&
        pBackBuffer != NULL) {
        ID3D11Device_CreateRenderTargetView(s_pD3DDevice, (ID3D11Resource*)pBackBuffer, NULL, &s_pMainRTV);
        ID3D11Texture2D_Release(pBackBuffer);
    }
}

static BOOL VgPanelInitD3D11(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = { 0 };
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevels[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL obtainedLevel = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        featureLevels,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &s_pSwapChain,
        &s_pD3DDevice,
        &obtainedLevel,
        &s_pD3DContext);

    if (FAILED(hr)) {
        /* Fallback to WARP software rasterizer just like FUN_1400b0ba0 */
        hr = D3D11CreateDeviceAndSwapChain(
            NULL,
            D3D_DRIVER_TYPE_WARP,
            NULL,
            0,
            featureLevels,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &s_pSwapChain,
            &s_pD3DDevice,
            &obtainedLevel,
            &s_pD3DContext);
    }

    if (FAILED(hr)) {
        VgTrayLogW(L"D3D11CreateDeviceAndSwapChain failed: 0x%08X", (unsigned)hr);
        return FALSE;
    }

    VgPanelCreateRenderTarget();
    return TRUE;
}

static void VgPanelShutdownD3D11(void)
{
    VgPanelCleanupRenderTarget();
    if (s_pSwapChain != NULL) {
        IDXGISwapChain_Release(s_pSwapChain);
        s_pSwapChain = NULL;
    }
    if (s_pD3DContext != NULL) {
        ID3D11DeviceContext_Release(s_pD3DContext);
        s_pD3DContext = NULL;
    }
    if (s_pD3DDevice != NULL) {
        ID3D11Device_Release(s_pD3DDevice);
        s_pD3DDevice = NULL;
    }
}

/* ============================================================================
 * PANEL STATUS RENDERING & GDI COMPOSITOR
 * Displays Service Status (vgc/vgk), Security Features (Win11 25H2, UEFI
 * Secure Boot, TPM 2.0, VBS, HVCI, IOMMU, Disk Partition Style), and actions.
 * ============================================================================ */
static const wchar_t* VgFormatServiceState(DWORD dwState)
{
    switch (dwState) {
    case SERVICE_RUNNING:          return L"RUNNING";
    case SERVICE_STOPPED:          return L"STOPPED";
    case SERVICE_START_PENDING:    return L"STARTING";
    case SERVICE_STOP_PENDING:     return L"STOPPING";
    default:                       return L"UNKNOWN";
    }
}

static const wchar_t* VgFormatStartType(DWORD dwStartType)
{
    switch (dwStartType) {
    case SERVICE_BOOT_START:       return L"BOOT START";
    case SERVICE_SYSTEM_START:     return L"SYSTEM START";
    case SERVICE_AUTO_START:       return L"AUTO START";
    case SERVICE_DEMAND_START:     return L"DEMAND START";
    case SERVICE_DISABLED:         return L"DISABLED";
    default:                       return L"UNKNOWN";
    }
}

static void VgPanelPaintRow(
    HDC hdc,
    int y,
    int width,
    const wchar_t* wszLabel,
    const wchar_t* wszValue,
    BOOL bReady)
{
    RECT rcRow = { 24, y, width - 24, y + 26 };
    HBRUSH hRowBrush = CreateSolidBrush(RGB(28, 31, 38));
    FillRect(hdc, &rcRow, hRowBrush);
    DeleteObject(hRowBrush);

    SetTextColor(hdc, RGB(215, 220, 230));
    RECT rcLeft = { 34, y + 4, width - 170, y + 24 };
    DrawTextW(hdc, wszLabel, -1, &rcLeft, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    SetTextColor(hdc, bReady ? RGB(46, 213, 115) : RGB(255, 71, 87));
    RECT rcRight = { width - 165, y + 4, width - 34, y + 24 };
    DrawTextW(hdc, wszValue, -1, &rcRight, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
}

static void VgPanelRenderDashboard(HWND hWnd, HDC hdc)
{
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    int width = rcClient.right - rcClient.left;

    /* Present D3D11 clear color if swapchain is active */
    if (s_pD3DContext != NULL && s_pMainRTV != NULL && s_pSwapChain != NULL) {
        const float clearColor[4] = { 0.07f, 0.08f, 0.10f, 1.0f };
        ID3D11DeviceContext_OMSetRenderTargets(s_pD3DContext, 1, &s_pMainRTV, NULL);
        ID3D11DeviceContext_ClearRenderTargetView(s_pD3DContext, s_pMainRTV, clearColor);
        IDXGISwapChain_Present(s_pSwapChain, 1, 0);
    }

    HBRUSH hBg = CreateSolidBrush(RGB(18, 20, 25));
    FillRect(hdc, &rcClient, hBg);
    DeleteObject(hBg);

    SetBkMode(hdc, TRANSPARENT);

    HFONT hFontTitle = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontSection = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontBody = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    /* Header Banner */
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFontTitle);
    SetTextColor(hdc, RGB(255, 70, 85));
    RECT rcTitle = { 24, 16, width - 24, 42 };
    DrawTextW(hdc, L"RIOT VANGUARD  —  PRE-CHECK & STATUS", -1, &rcTitle, DT_LEFT | DT_SINGLELINE);

    SelectObject(hdc, hFontBody);
    SetTextColor(hdc, RGB(145, 152, 165));
    WCHAR wszVerLine[128] = { 0 };
    swprintf_s(wszVerLine, _countof(wszVerLine), L"Version: %s", VGTRAY_VERSION_WSTR);
    RECT rcSub = { 24, 42, width - 24, 62 };
    DrawTextW(hdc, wszVerLine, -1, &rcSub, DT_LEFT | DT_SINGLELINE);

    EnterCriticalSection(&g_VgTray.csStateLock);
    VGTRAY_SERVICE_SNAPSHOT vgc = g_VgTray.svcVgc;
    VGTRAY_SERVICE_SNAPSHOT vgk = g_VgTray.svcVgk;
    VGTRAY_PRECHECK_STATE   pc  = g_VgTray.precheck;
    BOOL bAdvanced              = (g_VgTray.settings.advanced_view != 0);
    LeaveCriticalSection(&g_VgTray.csStateLock);

    /* Section 1: Service Status */
    int y = 74;
    SelectObject(hdc, hFontSection);
    SetTextColor(hdc, RGB(240, 242, 245));
    RECT rcSec1 = { 24, y, width - 24, y + 22 };
    DrawTextW(hdc, L"SERVICE STATUS", -1, &rcSec1, DT_LEFT | DT_SINGLELINE);
    y += 26;

    SelectObject(hdc, hFontBody);
    WCHAR wszVgcStatus[128] = { 0 };
    swprintf_s(wszVgcStatus, _countof(wszVgcStatus), L"%s (%s)",
               VgFormatServiceState(vgc.dwCurrentState), VgFormatStartType(vgc.dwStartType));
    VgPanelPaintRow(hdc, y, width, L"Vanguard User-Mode Service (vgc)",
                    wszVgcStatus, vgc.dwCurrentState == SERVICE_RUNNING);
    y += 30;

    WCHAR wszVgkStatus[128] = { 0 };
    swprintf_s(wszVgkStatus, _countof(wszVgkStatus), L"%s (%s)",
               VgFormatServiceState(vgk.dwCurrentState), VgFormatStartType(vgk.dwStartType));
    VgPanelPaintRow(hdc, y, width, L"Vanguard Kernel Driver (vgk.sys)",
                    wszVgkStatus, vgk.dwCurrentState == SERVICE_RUNNING);
    y += 38;

    /* Section 2: Security Features Pre-Check */
    SelectObject(hdc, hFontSection);
    SetTextColor(hdc, RGB(240, 242, 245));
    RECT rcSec2 = { 24, y, width - 24, y + 22 };
    DrawTextW(hdc, L"SECURITY REQUIREMENTS PRE-CHECK", -1, &rcSec2, DT_LEFT | DT_SINGLELINE);
    y += 26;

    SelectObject(hdc, hFontBody);
    WCHAR wszOsVal[64] = { 0 };
    swprintf_s(wszOsVal, _countof(wszOsVal), L"Build %lu (%s)",
               pc.dwOsBuild, pc.bWin11_25H2_Ready ? L"READY" : L"UPDATE REQUIRED");
    VgPanelPaintRow(hdc, y, width, L"Windows 11 (25H2+)", wszOsVal, pc.bWin11_25H2_Ready);
    y += 30;

    VgPanelPaintRow(hdc, y, width, L"UEFI Secure Boot",
                    pc.bSecureBootEnabled ? L"ENABLED" : L"DISABLED", pc.bSecureBootEnabled);
    y += 30;

    VgPanelPaintRow(hdc, y, width, L"TPM 2.0",
                    pc.bTpm20Ready ? L"READY" : L"MISSING", pc.bTpm20Ready);
    y += 30;

    if (bAdvanced) {
        VgPanelPaintRow(hdc, y, width, L"Virtualization-Based Security (VBS)",
                        pc.bVbsEnabled ? L"ENABLED" : L"DISABLED", pc.bVbsEnabled);
        y += 30;

        VgPanelPaintRow(hdc, y, width, L"Memory Integrity (HVCI)",
                        pc.bHvciEnabled ? L"ENABLED" : L"DISABLED", pc.bHvciEnabled);
        y += 30;

        VgPanelPaintRow(hdc, y, width, L"IOMMU / Kernel DMA Protection",
                        pc.bIommuEnabled ? L"ENABLED" : L"DISABLED", pc.bIommuEnabled);
        y += 30;

        const wchar_t* wszPartStyle = (pc.dwPartitionStyle == PARTITION_STYLE_GPT) ? L"GPT (UEFI)" :
                                      (pc.dwPartitionStyle == PARTITION_STYLE_MBR) ? L"MBR (Legacy)" : L"RAW";
        VgPanelPaintRow(hdc, y, width, L"System Disk Partition Style",
                        wszPartStyle, pc.dwPartitionStyle == PARTITION_STYLE_GPT);
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFontTitle);
    DeleteObject(hFontSection);
    DeleteObject(hFontBody);
}

/* ============================================================================
 * VANGUARD PANEL WINDOW PROCEDURE (FUN_1400aee10)
 * ============================================================================ */
static LRESULT CALLBACK VgPanelWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
    {
        CreateWindowExW(0, L"BUTTON", L"Refresh Status",
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        24, 395, 110, 30, hWnd, (HMENU)IDC_BTN_REFRESH_STATUS, g_VgTray.hInstance, NULL);
        CreateWindowExW(0, L"BUTTON", L"Save Logs...",
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        142, 395, 100, 30, hWnd, (HMENU)IDC_BTN_SAVE_LOGS, g_VgTray.hInstance, NULL);
        CreateWindowExW(0, L"BUTTON", L"Advanced View",
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        250, 395, 110, 30, hWnd, (HMENU)IDC_BTN_TOGGLE_ADVANCED, g_VgTray.hInstance, NULL);
        CreateWindowExW(0, L"BUTTON", L"Learn More",
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        368, 395, 95, 30, hWnd, (HMENU)IDC_BTN_LEARN_MORE, g_VgTray.hInstance, NULL);
        return 0;
    }

    case WM_SIZE:
        if (s_pSwapChain != NULL && wParam != SIZE_MINIMIZED) {
            VgPanelCleanupRenderTarget();
            IDXGISwapChain_ResizeBuffers(s_pSwapChain, 0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            VgPanelCreateRenderTarget();
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_REFRESH_STATUS:
            VgTrayEvaluateSecurityPrecheck(&g_VgTray.precheck);
            VgTrayPollServiceSnapshot(L"vgc", &g_VgTray.svcVgc);
            VgTrayPollServiceSnapshot(L"vgk", &g_VgTray.svcVgk);
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;

        case IDC_BTN_SAVE_LOGS:
            VgTrayExportLogsZipInteractive(hWnd);
            return 0;

        case IDC_BTN_TOGGLE_ADVANCED:
            if (g_VgTray.hTrayWnd) {
                SendMessageW(g_VgTray.hTrayWnd, WM_COMMAND, IDM_TRAY_TOGGLE_ADVANCED, 0);
            }
            return 0;

        case IDC_BTN_LEARN_MORE:
            ShellExecuteW(NULL, L"open", VANGUARD_SUPPORT_URL_REQ, NULL, NULL, SW_SHOWNORMAL);
            return 0;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        VgPanelRenderDashboard(hWnd, hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        /* Hide panel on X click unless application is shutting down */
        if (WaitForSingleObject(g_VgTray.hStopWorkerEvent, 0) == WAIT_TIMEOUT) {
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

/* ============================================================================
 * CONTEXT MENU HOST WINDOW PROCEDURE (FUN_140095a10)
 * ============================================================================ */
static LRESULT CALLBACK VgMenuHostWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

/* ============================================================================
 * PANEL VISIBILITY & LAYOUT HELPERS (FUN_1400ae8b0 / FUN_1400aeb70)
 * ============================================================================ */
void VgTrayShowPanelWindow(void)
{
    if (g_VgTray.hPanelWnd != NULL) {
        VgTrayRefreshPanelLayout();
        ShowWindow(g_VgTray.hPanelWnd, SW_SHOWNORMAL);
        SetForegroundWindow(g_VgTray.hPanelWnd);
        InvalidateRect(g_VgTray.hPanelWnd, NULL, FALSE);
    }
}

void VgTrayHidePanelWindow(void)
{
    if (g_VgTray.hPanelWnd != NULL) {
        ShowWindow(g_VgTray.hPanelWnd, SW_HIDE);
    }
}

void VgTrayRefreshPanelLayout(void)
{
    if (g_VgTray.hPanelWnd == NULL) {
        return;
    }

    int width = 500;
    int height = g_VgTray.settings.advanced_view ? 485 : 365;

    RECT rcWork = { 0 };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);
    int x = rcWork.right - width - 16;
    int y = rcWork.bottom - height - 16;

    SetWindowPos(
        g_VgTray.hPanelWnd,
        HWND_TOP,
        x, y, width, height,
        SWP_NOACTIVATE | SWP_NOZORDER);

    InvalidateRect(g_VgTray.hPanelWnd, NULL, FALSE);
}

/* ============================================================================
 * PANEL UI THREAD (FUN_1400b0ba0)
 * Registers VanguardPanelClass & VanguardTrayContextMenuHostClass, initializes
 * Direct3D 11 device/swapchain, signals hPanelReadyEvent, and pumps messages.
 * ============================================================================ */
DWORD WINAPI VgTrayPanelThread(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    WNDCLASSEXW wcPanel = { 0 };
    wcPanel.cbSize = sizeof(WNDCLASSEXW);
    wcPanel.style = CS_HREDRAW | CS_VREDRAW;
    wcPanel.lpfnWndProc = VgPanelWndProc;
    wcPanel.hInstance = g_VgTray.hInstance;
    wcPanel.hIcon = g_VgTray.hTrayIcon;
    wcPanel.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcPanel.lpszClassName = VGPANEL_WINDOW_CLASS;
    RegisterClassExW(&wcPanel);

    WNDCLASSEXW wcMenuHost = { 0 };
    wcMenuHost.cbSize = sizeof(WNDCLASSEXW);
    wcMenuHost.lpfnWndProc = VgMenuHostWndProc;
    wcMenuHost.hInstance = g_VgTray.hInstance;
    wcMenuHost.lpszClassName = VGMENU_HOST_WINDOW_CLASS;
    RegisterClassExW(&wcMenuHost);

    int width = 500;
    int height = g_VgTray.settings.advanced_view ? 485 : 365;
    RECT rcWork = { 0 };
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);

    g_VgTray.hPanelWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        VGPANEL_WINDOW_CLASS,
        L"Riot Vanguard",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        rcWork.right - width - 16,
        rcWork.bottom - height - 16,
        width,
        height,
        NULL,
        NULL,
        g_VgTray.hInstance,
        NULL);

    g_VgTray.hMenuHostWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        VGMENU_HOST_WINDOW_CLASS,
        L"",
        WS_POPUP,
        0, 0, 1, 1,
        NULL,
        NULL,
        g_VgTray.hInstance,
        NULL);

    if (g_VgTray.hPanelWnd != NULL) {
        VgPanelInitD3D11(g_VgTray.hPanelWnd);
    }

    if (g_VgTray.hPanelReadyEvent != NULL) {
        SetEvent(g_VgTray.hPanelReadyEvent);
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    VgPanelShutdownD3D11();
    return 0;
}
