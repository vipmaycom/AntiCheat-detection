/*
 * service_precheck.c
 * SCM Service Monitor (vgc/vgk) & Hardware Security Pre-Check Verifier
 * Reconstructed from FUN_140082ea0, FUN_14008ce00, FUN_1400b1970, FUN_1400b1cc0, FUN_1400b1ea0
 */

#include "vgtray.h"
#include <tbs.h>

typedef LONG NTSTATUS;
typedef NTSTATUS(NTAPI* PFN_RtlGetVersion)(PRTL_OSVERSIONINFOW);
typedef NTSTATUS(NTAPI* PFN_NtQuerySystemInformation)(ULONG, PVOID, ULONG, PULONG);

/* SystemBootEnvironmentInformation (90) & SystemCodeIntegrityInformation (103) */
#define SystemBootEnvironmentInformation 90
#define SystemCodeIntegrityInformation   103
#define SystemIsolatedUserModeInformation 165

typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION_LOCAL {
    ULONG Length;
    ULONG CodeIntegrityOptions;
} SYSTEM_CODEINTEGRITY_INFORMATION_LOCAL;

/* ============================================================================
 * VGK SERVICE START TYPE QUERY (FUN_14008ce00)
 * Queries OpenSCManagerW(1) -> OpenServiceW("vgk", 1) -> QueryServiceConfigW
 * ============================================================================ */
BOOL VgTrayQueryVgkStartType(DWORD* pdwStartType)
{
    if (pdwStartType == NULL) {
        return FALSE;
    }

    BOOL bResult = FALSE;
    SC_HANDLE hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (hScm != NULL) {
        SC_HANDLE hSvc = OpenServiceW(hScm, L"vgk", SERVICE_QUERY_CONFIG);
        if (hSvc != NULL) {
            DWORD cbNeeded = 0;
            if (!QueryServiceConfigW(hSvc, NULL, 0, &cbNeeded) &&
                GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                cbNeeded > 0) {
                LPQUERY_SERVICE_CONFIGW pCfg = (LPQUERY_SERVICE_CONFIGW)malloc(cbNeeded);
                if (pCfg != NULL) {
                    if (QueryServiceConfigW(hSvc, pCfg, cbNeeded, &cbNeeded)) {
                        *pdwStartType = pCfg->dwStartType;
                        bResult = TRUE;
                    }
                    free(pCfg);
                }
            }
            CloseServiceHandle(hSvc);
        }
        CloseServiceHandle(hScm);
    }

    return bResult;
}

/* ============================================================================
 * SERVICE STATUS & CONFIG SNAPSHOT POLLER (FUN_1400b1cc0)
 * Queries QueryServiceStatusEx(SC_STATUS_PROCESS_INFO) + QueryServiceConfigW
 * for "vgc" and "vgk".
 * ============================================================================ */
void VgTrayPollServiceSnapshot(LPCWSTR wszServiceName, VGTRAY_SERVICE_SNAPSHOT* pSnapshot)
{
    if (wszServiceName == NULL || pSnapshot == NULL) {
        return;
    }

    SC_HANDLE hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (hScm == NULL) {
        return;
    }

    SC_HANDLE hSvc = OpenServiceW(hScm, wszServiceName, SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
    if (hSvc != NULL) {
        SERVICE_STATUS_PROCESS ssp = { 0 };
        DWORD cbNeeded = 0;
        if (QueryServiceStatusEx(
                hSvc,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssp,
                sizeof(ssp),
                &cbNeeded)) {
            pSnapshot->dwCurrentState = ssp.dwCurrentState;
            pSnapshot->dwWin32ExitCode = ssp.dwWin32ExitCode;
            pSnapshot->dwServiceSpecificExitCode = ssp.dwServiceSpecificExitCode;
            pSnapshot->dwProcessId = ssp.dwProcessId;
        }

        cbNeeded = 0;
        if (!QueryServiceConfigW(hSvc, NULL, 0, &cbNeeded) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
            cbNeeded > 0) {
            LPQUERY_SERVICE_CONFIGW pCfg = (LPQUERY_SERVICE_CONFIGW)malloc(cbNeeded);
            if (pCfg != NULL) {
                if (QueryServiceConfigW(hSvc, pCfg, cbNeeded, &cbNeeded)) {
                    pSnapshot->dwStartType = pCfg->dwStartType;
                }
                free(pCfg);
            }
        }
        CloseServiceHandle(hSvc);
    }

    CloseServiceHandle(hScm);
}

/* ============================================================================
 * STOP SERVICE HELPER (FUN_14008cd40)
 * Called when user confirms "Exit Vanguard" (0x9C41).
 * ============================================================================ */
BOOL VgTrayStopServiceByName(LPCWSTR wszServiceName)
{
    BOOL bOk = FALSE;
    SC_HANDLE hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (hScm != NULL) {
        SC_HANDLE hSvc = OpenServiceW(hScm, wszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (hSvc != NULL) {
            SERVICE_STATUS ss = { 0 };
            if (ControlService(hSvc, SERVICE_CONTROL_STOP, &ss)) {
                bOk = TRUE;
            }
            CloseServiceHandle(hSvc);
        }
        CloseServiceHandle(hScm);
    }
    return bOk;
}

/* ============================================================================
 * VGC "--start-type" CONFIGURATOR (FUN_1400b1970)
 * Starts vgc with "--start-type" "1" (SYSTEM_START) or "3" (DEMAND_START)
 * and waits up to 50,000ms for vgk start-type to match, enforcing a 3,000ms
 * minimum UI transition duration.
 * ============================================================================ */
int VgTrayEnsureVgcServiceMode(BOOL bDemandStart)
{
    ULONGLONG ullStartTick = GetTickCount64();
    DWORD dwTargetStartType = bDemandStart ? SERVICE_DEMAND_START : SERVICE_SYSTEM_START;
    DWORD dwCurrentStartType = 0;
    int iResult = 1;

    if (VgTrayQueryVgkStartType(&dwCurrentStartType)) {
        if (dwCurrentStartType == dwTargetStartType) {
            iResult = 0;
        } else {
            SC_HANDLE hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
            if (hScm != NULL) {
                SC_HANDLE hSvc = OpenServiceW(hScm, L"vgc", SERVICE_QUERY_STATUS | SERVICE_START);
                if (hSvc != NULL) {
                    SERVICE_STATUS ss = { 0 };
                    if (QueryServiceStatus(hSvc, &ss) && ss.dwCurrentState == SERVICE_RUNNING) {
                        CloseServiceHandle(hSvc);
                        CloseServiceHandle(hScm);
                        ULONGLONG ullElapsed = GetTickCount64() - ullStartTick;
                        if (ullElapsed < 3000) {
                            Sleep((DWORD)(3000 - ullElapsed));
                        }
                        return 2;
                    }

                    LPCWSTR apwszArgs[2] = {
                        L"--start-type",
                        bDemandStart ? L"3" : L"1"
                    };
                    if (StartServiceW(hSvc, 2, apwszArgs)) {
                        ULONGLONG ullWaitStart = GetTickCount64();
                        while (GetTickCount64() - ullWaitStart < 50000) {
                            if (VgTrayQueryVgkStartType(&dwCurrentStartType) &&
                                dwCurrentStartType == dwTargetStartType) {
                                iResult = 0;
                                break;
                            }
                            Sleep(100);
                        }
                    }
                    CloseServiceHandle(hSvc);
                }
                CloseServiceHandle(hScm);
            }
        }
    }

    ULONGLONG ullElapsed = GetTickCount64() - ullStartTick;
    if (ullElapsed < 3000) {
        Sleep((DWORD)(3000 - ullElapsed));
    }
    return iResult;
}

/* ============================================================================
 * DISK PARTITION STYLE CHECK (FUN_140082ea0)
 * 1. Resolves Windows directory drive (e.g. "\\.\C:") via GetWindowsDirectoryW
 * 2. Sends IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS (0x2D1080) to get DiskNumber
 * 3. Opens "\\.\PhysicalDrive%lu" and sends IOCTL_DISK_GET_DRIVE_LAYOUT_EX
 *    (0x70050) with a 0x48C0-byte buffer to read PartitionStyle (MBR=0, GPT=1).
 * ============================================================================ */
BOOL VgTrayQuerySystemPartitionStyle(DWORD* pdwPartitionStyle)
{
    if (pdwPartitionStyle == NULL) {
        return FALSE;
    }

    *pdwPartitionStyle = PARTITION_STYLE_RAW; /* 2 */

    WCHAR wszWinDir[MAX_PATH] = { 0 };
    UINT cchWinDir = GetWindowsDirectoryW(wszWinDir, MAX_PATH);
    if (cchWinDir < 2 || wszWinDir[1] != L':') {
        return FALSE;
    }

    WCHAR wszVolumePath[16] = { 0 };
    swprintf_s(wszVolumePath, _countof(wszVolumePath), L"\\\\.\\%c:", wszWinDir[0]);

    HANDLE hVolume = CreateFileW(
        wszVolumePath,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (hVolume == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    VOLUME_DISK_EXTENTS vde = { 0 };
    DWORD cbReturned = 0;
    BOOL bOk = FALSE;

    /* IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS = 0x002D1080 (or 0x00560000) */
    if (DeviceIoControl(
            hVolume,
            0x002D1080,
            NULL,
            0,
            &vde,
            sizeof(vde),
            &cbReturned,
            NULL) ||
        DeviceIoControl(
            hVolume,
            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
            NULL,
            0,
            &vde,
            sizeof(vde),
            &cbReturned,
            NULL)) {
        WCHAR wszDrivePath[64] = { 0 };
        swprintf_s(
            wszDrivePath,
            _countof(wszDrivePath),
            L"\\\\.\\PhysicalDrive%lu",
            vde.Extents[0].DiskNumber);

        HANDLE hDisk = CreateFileW(
            wszDrivePath,
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        if (hDisk != INVALID_HANDLE_VALUE) {
            /* Allocate exact 0x48C0 byte buffer from FUN_140082ea0 */
            const DWORD cbLayoutBuf = 0x48C0;
            BYTE* pLayoutBuf = (BYTE*)calloc(1, cbLayoutBuf);
            if (pLayoutBuf != NULL) {
                /* IOCTL_DISK_GET_DRIVE_LAYOUT_EX = 0x00070050 */
                if (DeviceIoControl(
                        hDisk,
                        IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                        NULL,
                        0,
                        pLayoutBuf,
                        cbLayoutBuf,
                        &cbReturned,
                        NULL)) {
                    *pdwPartitionStyle = *(DWORD*)pLayoutBuf;
                    bOk = TRUE;
                }
                free(pLayoutBuf);
            }
            CloseHandle(hDisk);
        }
    }

    CloseHandle(hVolume);
    return bOk;
}

/* ============================================================================
 * HARDWARE & OS SECURITY PRE-CHECK VERIFIER
 * Evaluates:
 *   1. Windows 11 25H2+ (RtlGetVersion + EditionID)
 *   2. Disk Partition Style (GPT vs MBR via FUN_140082ea0)
 *   3. UEFI Secure Boot (HKLM\SYSTEM\CurrentControlSet\Control\SecureBoot\State)
 *   4. TPM 2.0 (Tbsi_GetDeviceInfo)
 *   5. VBS & HVCI (SystemCodeIntegrityInformation / DeviceGuard registry)
 *   6. IOMMU / Kernel DMA Protection
 * ============================================================================ */
void VgTrayEvaluateSecurityPrecheck(VGTRAY_PRECHECK_STATE* pState)
{
    if (pState == NULL) {
        return;
    }

    ZeroMemory(pState, sizeof(*pState));

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll != NULL) {
        PFN_RtlGetVersion pfnRtlGetVersion =
            (PFN_RtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
        if (pfnRtlGetVersion != NULL) {
            RTL_OSVERSIONINFOW osvi = { 0 };
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (pfnRtlGetVersion(&osvi) == 0) {
                pState->dwOsMajor = osvi.dwMajorVersion;
                pState->dwOsMinor = osvi.dwMinorVersion;
                pState->dwOsBuild = osvi.dwBuildNumber;
            }
        }
    }

    /* Windows EditionID */
    HKEY hKeyNt = NULL;
    if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            0,
            KEY_READ,
            &hKeyNt) == ERROR_SUCCESS) {
        DWORD cbEdition = sizeof(pState->wszEditionId);
        RegQueryValueExW(hKeyNt, L"EditionID", NULL, NULL, (LPBYTE)pState->wszEditionId, &cbEdition);
        RegCloseKey(hKeyNt);
    }

    /* Windows 11 build >= 22000 (25H2 = 26200+) */
    pState->bWin11_25H2_Ready = (pState->dwOsMajor >= 10 && pState->dwOsBuild >= 22000);

    /* Partition style (MBR vs GPT) */
    VgTrayQuerySystemPartitionStyle(&pState->dwPartitionStyle);
    pState->bUefiMode = (pState->dwPartitionStyle == PARTITION_STYLE_GPT);

    /* UEFI Secure Boot */
    HKEY hKeySb = NULL;
    if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State",
            0,
            KEY_READ,
            &hKeySb) == ERROR_SUCCESS) {
        DWORD dwSbEnabled = 0;
        DWORD cbSb = sizeof(dwSbEnabled);
        if (RegQueryValueExW(hKeySb, L"UEFISecureBootEnabled", NULL, NULL, (LPBYTE)&dwSbEnabled, &cbSb) == ERROR_SUCCESS) {
            pState->bSecureBootEnabled = (dwSbEnabled == 1);
            pState->bUefiMode = TRUE;
        }
        RegCloseKey(hKeySb);
    }

    /* TPM 2.0 via TBS (Tbsi_GetDeviceInfo) */
    TPM_DEVICE_INFO tpmInfo = { 0 };
    if (Tbsi_GetDeviceInfo(sizeof(tpmInfo), &tpmInfo) == TBS_SUCCESS) {
        pState->bTpm20Ready = (tpmInfo.tpmVersion == TPM_VERSION_20);
    }

    /* VBS & HVCI via NtQuerySystemInformation(SystemCodeIntegrityInformation = 103) */
    if (hNtdll != NULL) {
        PFN_NtQuerySystemInformation pfnNtQuerySystemInformation =
            (PFN_NtQuerySystemInformation)GetProcAddress(hNtdll, "NtQuerySystemInformation");
        if (pfnNtQuerySystemInformation != NULL) {
            SYSTEM_CODEINTEGRITY_INFORMATION_LOCAL sci = { 0 };
            sci.Length = sizeof(sci);
            ULONG cbRet = 0;
            if (pfnNtQuerySystemInformation(SystemCodeIntegrityInformation, &sci, sizeof(sci), &cbRet) == 0) {
                /* HVCI (CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED = 0x400) */
                if (sci.CodeIntegrityOptions & 0x400) {
                    pState->bHvciEnabled = TRUE;
                    pState->bVbsEnabled = TRUE;
                }
            }
        }
    }

    /* DeviceGuard Registry fallback for VBS / HVCI / IOMMU */
    HKEY hKeyDg = NULL;
    if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\DeviceGuard",
            0,
            KEY_READ,
            &hKeyDg) == ERROR_SUCCESS) {
        DWORD dwVbs = 0;
        DWORD cbVbs = sizeof(dwVbs);
        if (RegQueryValueExW(hKeyDg, L"EnableVirtualizationBasedSecurity", NULL, NULL, (LPBYTE)&dwVbs, &cbVbs) == ERROR_SUCCESS &&
            dwVbs != 0) {
            pState->bVbsEnabled = TRUE;
        }
        RegCloseKey(hKeyDg);
    }

    HKEY hKeyDma = NULL;
    if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\DmaSecurity",
            0,
            KEY_READ,
            &hKeyDma) == ERROR_SUCCESS) {
        pState->bIommuEnabled = TRUE;
        RegCloseKey(hKeyDma);
    } else {
        pState->bIommuEnabled = pState->bVbsEnabled;
    }

    pState->bAllRequirementsMet = (
        pState->bWin11_25H2_Ready &&
        pState->bSecureBootEnabled &&
        pState->bTpm20Ready &&
        pState->bVbsEnabled &&
        pState->bHvciEnabled);
}

/* ============================================================================
 * SERVICE MONITOR THREAD (FUN_1400b1ea0)
 * Polls vgc & vgk service status every 1000ms until stop event is signaled.
 * ============================================================================ */
DWORD WINAPI VgTrayServiceMonitorThread(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    while (WaitForSingleObject(g_VgTray.hStopWorkerEvent, 1000) == WAIT_TIMEOUT) {
        VGTRAY_SERVICE_SNAPSHOT snapVgc = { 0 };
        VGTRAY_SERVICE_SNAPSHOT snapVgk = { 0 };
        VgTrayPollServiceSnapshot(L"vgc", &snapVgc);
        VgTrayPollServiceSnapshot(L"vgk", &snapVgk);

        EnterCriticalSection(&g_VgTray.csStateLock);
        BOOL bChanged = (
            g_VgTray.svcVgc.dwCurrentState != snapVgc.dwCurrentState ||
            g_VgTray.svcVgc.dwStartType    != snapVgc.dwStartType    ||
            g_VgTray.svcVgk.dwCurrentState != snapVgk.dwCurrentState ||
            g_VgTray.svcVgk.dwStartType    != snapVgk.dwStartType);
        g_VgTray.svcVgc = snapVgc;
        g_VgTray.svcVgk = snapVgk;
        LeaveCriticalSection(&g_VgTray.csStateLock);

        if (bChanged && g_VgTray.hPanelWnd != NULL) {
            InvalidateRect(g_VgTray.hPanelWnd, NULL, FALSE);
        }
    }

    return 0;
}
