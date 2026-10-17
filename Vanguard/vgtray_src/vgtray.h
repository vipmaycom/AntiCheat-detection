/*
 * vgtray.h
 * Master header for Riot Vanguard System Tray & Pre-Check Dashboard (vgtray.exe)
 * Reconstructed from C:\Program Files\Riot Vanguard\vgtray.exe (v1.19.1-18+20261001.143443)
 * SHA-256: 825b9b89c7528d52f877ac2f9a3275da67191a5effe69a43725da753f94ccaef
 */

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <wtsapi32.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>

/* ============================================================================
 * 1. BUILD METADATA & GLOBAL IDENTIFIERS
 * ============================================================================ */
#define VGTRAY_VERSION_STRING        "1.19.1-18+20261001.143443"
#define VGTRAY_VERSION_WSTR          L"1.19.1-18+20261001.143443"
#define VGTRAY_BUILD_BANNER_FMT      ": 1.19.1-18+20261001.143443 ; Thu Oct  1 14:35:29 2026 ; %llu\n"
#define VGTRAY_SCHEMA_VERSION        1

/* Named Synchronization Objects (FUN_140089200) */
#define VGTRAY_MUTEX_NAME            L"Global\\vgtray_mutex"
#define VGTRAY_EXIT_EVENT_NAME       L"Global\\vgtray_exit_event"

/* Window Classes (FUN_140089200 / FUN_1400b0ba0) */
#define VGTRAY_WINDOW_CLASS          L"VanguardTrayWindowClass"
#define VGPANEL_WINDOW_CLASS         L"VanguardPanelClass"
#define VGMENU_HOST_WINDOW_CLASS     L"VanguardTrayContextMenuHostClass"

/* Tray Notification Icon Callback & Custom Messages (FUN_140091060 / FUN_140091470) */
#define WM_VGTRAY_NOTIFYICON         0x8001   /* WM_APP + 1 */
#define WM_VGTRAY_SHOW_CONTEXT       0x8002   /* WM_APP + 2 */
#define WM_VGPANEL_OPEN_CONTEXT      0x8120

/* Resource IDs (PE RT_GROUP_ICON / RT_MENU) */
#define IDI_VGTRAY_ICON_RED          101      /* 0x65 */
#define IDI_VGTRAY_ICON_LIGHT        102      /* 0x66 */
#define IDR_TRAY_MENU                103      /* 0x67 */
#define IDI_VGTRAY_ICON_DARK         104      /* 0x68 */
#define IDI_VGTRAY_ICON_ALERT        105      /* 0x69 */

/* Menu Command IDs (FUN_140091850) */
#define IDM_TRAY_EXIT_VANGUARD       0x9C41   /* 40001: Turn off Vanguard / stop vgc+vgk */
#define IDM_TRAY_UNINSTALL_VANGUARD  0x9C42   /* 40002: Uninstall Riot Vanguard */
#define IDM_TRAY_EXIT_TRAY           0x9C43   /* 40003: Exit Tray */
#define IDM_TRAY_SAVE_LOGS           0x9C44   /* 40004: Save Riot Vanguard Logs (.zip) */
#define IDM_TRAY_TOGGLE_ADVANCED     0x9C45   /* 40005: Toggle Advanced View */
#define IDM_TRAY_SHOW_PANEL          0x9C4A   /* 40010: Show Vanguard Status & Pre-Check Panel */

/* Official Shell_NotifyIconW GUID from FUN_140091060:
 * {70D5F62D-0932-47D8-9E1A-BA4B95549E3E}
 * (local_50=0x70d5f62d, uStack_4c=0x47d80932, uStack_48=0x4bba1a9e, uStack_44=0x3e9e5495)
 */
static const GUID VGTRAY_NOTIFYICON_GUID = {
    0x70d5f62d, 0x0932, 0x47d8, { 0x9e, 0x1a, 0xba, 0x4b, 0x95, 0x54, 0x9e, 0x3e }
};

/* Registry Paths & Values (FUN_14008a430 / FUN_140089960) */
#define VGK_REGISTRY_KEY             L"Software\\vgk"
#define VGK_VAL_TOAST_NOTIFICATION   L"KBToastNotification"
#define VGK_VAL_BLOCKED_IMAGE        L"BlockedImage"
#define VGK_VAL_DETECTED_IMAGE       L"DetectedImage"
#define VGK_VAL_DETECTED_IMAGE_ID    L"DetectedImageId"
#define THEME_PERSONALIZE_KEY        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"
#define THEME_VAL_LIGHT_THEME        L"AppsUseLightTheme"

/* Support URLs */
#define VANGUARD_SUPPORT_URL_REQ     L"https://support.riotgames.com/riot/performance/vanguard-security-requirements"
#define VANGUARD_SUPPORT_URL_HOME    L"https://support-valorant.riotgames.com/hc/articles/360044648213"

/* FNV-1a 32-bit Hash Constants for KBToastNotification Deduplication (FUN_14008a430) */
#define VGTRAY_FNV1A_32_OFFSET       0x811C9DC5u
#define VGTRAY_FNV1A_32_PRIME        0x01000193u
#define VGTRAY_MAX_TOAST_BLOB_SIZE   0xA07u

/* ============================================================================
 * 2. DATA STRUCTURES
 * ============================================================================ */

/* Service Monitor Snapshot (FUN_1400b1cc0) */
typedef struct _VGTRAY_SERVICE_SNAPSHOT {
    DWORD dwCurrentState;     /* SERVICE_RUNNING (4), SERVICE_STOPPED (1), etc. */
    DWORD dwStartType;        /* SERVICE_SYSTEM_START (1), SERVICE_DEMAND_START (3), etc. */
    DWORD dwWin32ExitCode;
    DWORD dwServiceSpecificExitCode;
    DWORD dwProcessId;
} VGTRAY_SERVICE_SNAPSHOT;

/* Hardware & OS Pre-Check State (FUN_140082ea0 / FUN_14009c7d0 / FUN_1400b0ba0) */
typedef struct _VGTRAY_PRECHECK_STATE {
    DWORD dwOsMajor;
    DWORD dwOsMinor;
    DWORD dwOsBuild;
    WCHAR wszEditionId[64];
    DWORD dwPartitionStyle;   /* PARTITION_STYLE_MBR (0), PARTITION_STYLE_GPT (1), RAW (2) */
    BOOL  bWin11_25H2_Ready;
    BOOL  bUefiMode;
    BOOL  bSecureBootEnabled;
    BOOL  bTpm20Ready;
    BOOL  bVbsEnabled;
    BOOL  bHvciEnabled;
    BOOL  bIommuEnabled;
    BOOL  bAllRequirementsMet;
} VGTRAY_PRECHECK_STATE;

/* Settings & Telemetry JSON State (FUN_14009c7d0: vgtray-settings.json) */
typedef struct _VGTRAY_SETTINGS {
    int      theme;
    int      language;
    int      advanced_view;
    int      precheck_option;
    int      failure_reason;
    int64_t  error_code;
    int      vgtray_version;
    char     vanguard_version[64];
    int      precheck_opted_out;
    int      precheck_enrolled_but_unmet;
    int      precheck_completed_after_settings_changes;
    int      precheck_reverted_after_completion;
    int      precheck_initial_state_recorded;
    int      precheck_initially_unmet;
    int      precheck_completed_once;
    int      precheck_25h2_originally_missing;
    int      precheck_tpm_originally_missing;
    int      precheck_secure_boot_originally_missing;
    int      precheck_vbs_originally_missing;
    int      precheck_hvci_originally_missing;
    int      precheck_iommu_originally_missing;
} VGTRAY_SETTINGS;

/* Parsed KBToastNotification Entry (FUN_14008a430 / FUN_140091470) */
typedef struct _VGTRAY_TOAST_ITEM {
    BYTE  bNotificationType;  /* 0 = Blocked driver/file, 1 = Incompatible software */
    DWORD dwFnvHash;          /* FNV-1a 32-bit digest of raw registry value */
    WCHAR wszTitle[128];
    WCHAR wszMessage[512];
    WCHAR wszFilePath[MAX_PATH];
    WCHAR wszHelpUrl[256];
} VGTRAY_TOAST_ITEM;

/* Global Runtime Context */
typedef struct _VGTRAY_CONTEXT {
    HINSTANCE               hInstance;
    HWND                    hTrayWnd;
    HWND                    hPanelWnd;
    HWND                    hMenuHostWnd;
    HANDLE                  hMutex;
    HANDLE                  hExitEvent;
    HANDLE                  hStopWorkerEvent;
    HANDLE                  hPanelReadyEvent;
    HANDLE                  hExitWatcherThread;
    HANDLE                  hServiceMonitorThread;
    HANDLE                  hToastWatcherThread;
    HANDLE                  hPanelThread;
    HICON                   hTrayIcon;
    CRITICAL_SECTION        csStateLock;
    CRITICAL_SECTION        csToastLock;
    VGTRAY_SERVICE_SNAPSHOT svcVgc;
    VGTRAY_SERVICE_SNAPSHOT svcVgk;
    VGTRAY_PRECHECK_STATE   precheck;
    VGTRAY_SETTINGS         settings;
    VGTRAY_TOAST_ITEM       lastToast;
    BOOL                    bHasPendingToast;
    DWORD                   dwLastToastHash;
    WCHAR                   wszInstallDir[MAX_PATH];
    WCHAR                   wszLocalAppDataDir[MAX_PATH];
    WCHAR                   wszLogFilePath[MAX_PATH];
    WCHAR                   wszSettingsFilePath[MAX_PATH];
} VGTRAY_CONTEXT;

extern VGTRAY_CONTEXT g_VgTray;

/* ============================================================================
 * 3. SUBSYSTEM PROTOTYPES
 * ============================================================================ */

/* main.c */
void VgTrayLogW(const wchar_t* fmt, ...);
BOOL VgTrayIsSystemLightTheme(void);
void VgTrayShowBalloonNotification(const wchar_t* wszTitle, const wchar_t* wszBody, DWORD dwInfoFlags);
void VgTrayShowContextMenu(HWND hWnd, POINT pt);

/* toast_registry.c */
uint32_t VgTrayComputeFnv1a32(const uint8_t* pData, size_t cbSize);
BOOL VgTrayPollKbToastNotification(VGTRAY_TOAST_ITEM* pOutToast);
DWORD WINAPI VgTrayToastWatcherThread(LPVOID lpParam);

/* service_precheck.c */
BOOL VgTrayQueryVgkStartType(DWORD* pdwStartType);
void VgTrayPollServiceSnapshot(LPCWSTR wszServiceName, VGTRAY_SERVICE_SNAPSHOT* pSnapshot);
BOOL VgTrayStopServiceByName(LPCWSTR wszServiceName);
int  VgTrayEnsureVgcServiceMode(BOOL bDemandStart);
BOOL VgTrayQuerySystemPartitionStyle(DWORD* pdwPartitionStyle);
void VgTrayEvaluateSecurityPrecheck(VGTRAY_PRECHECK_STATE* pState);
DWORD WINAPI VgTrayServiceMonitorThread(LPVOID lpParam);

/* log_exporter.c */
BOOL VgTrayLoadSettingsJson(VGTRAY_SETTINGS* pSettings);
BOOL VgTraySaveSettingsJson(const VGTRAY_SETTINGS* pSettings);
BOOL VgTrayExportLogsZipInteractive(HWND hOwnerWnd);

/* panel_d3d11.c */
DWORD WINAPI VgTrayPanelThread(LPVOID lpParam);
void VgTrayShowPanelWindow(void);
void VgTrayHidePanelWindow(void);
void VgTrayRefreshPanelLayout(void);
