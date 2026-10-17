/*
 * process_watcher.c
 * Riot Vanguard User-Mode Service (vgc.exe v1.19.1.18)
 * Process Lifecycle Tracking, Authenticode Signature Verification & Session Enforcement
 */

#include "vgc.h"

DWORD
VgcScanForValorantProcess(void)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    DWORD foundPid = 0;

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, VGC_VALORANT_EXE_NAME_W) == 0 ||
                _wcsicmp(pe.szExeFile, VGC_VALORANT_BOOTSTRAP_W) == 0) {
                foundPid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return foundPid;
}

bool
VgcAuditProcessSignature(PCWSTR FilePath)
{
    if (!FilePath) {
        return false;
    }

    WINTRUST_FILE_INFO fileData = { 0 };
    fileData.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    fileData.pcwszFilePath = FilePath;

    GUID policyGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    WINTRUST_DATA trustData = { 0 };
    trustData.cbStruct            = sizeof(WINTRUST_DATA);
    trustData.dwUIChoice          = WTD_UI_NONE;
    trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    trustData.dwUnionChoice       = WTD_CHOICE_FILE;
    trustData.pFile               = &fileData;
    trustData.dwStateAction       = WTD_STATEACTION_VERIFY;

    LONG status = WinVerifyTrust(NULL, &policyGuid, &trustData);

    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &policyGuid, &trustData);

    return (status == ERROR_SUCCESS);
}

void
VgcTerminateGameWithReason(VGC_ERROR_CODE ErrorCode, PCWSTR Reason)
{
    EnterCriticalSection(&g_Vgc.Lock);

    wprintf(L"[vgc] Punição/Desconexão Executada: Código VAN %d - Motivo: %s\n", (int)ErrorCode, Reason ? Reason : L"Violação de Integridade");

    if (g_Vgc.GameProcessHandle != NULL && g_Vgc.GameProcessHandle != INVALID_HANDLE_VALUE) {
        TerminateProcess(g_Vgc.GameProcessHandle, (UINT)ErrorCode);
        CloseHandle(g_Vgc.GameProcessHandle);
        g_Vgc.GameProcessHandle = NULL;
    } else if (g_Vgc.GamePid != 0) {
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, g_Vgc.GamePid);
        if (hProc != NULL) {
            TerminateProcess(hProc, (UINT)ErrorCode);
            CloseHandle(hProc);
        }
    }

    g_Vgc.GamePid     = 0;
    g_Vgc.GameRunning = false;

    LeaveCriticalSection(&g_Vgc.Lock);
}

void
VgcMonitorGameLifecycle(void)
{
    DWORD currentPid = VgcScanForValorantProcess();

    EnterCriticalSection(&g_Vgc.Lock);

    if (currentPid != 0) {
        if (!g_Vgc.GameRunning || g_Vgc.GamePid != currentPid) {
            g_Vgc.GamePid     = currentPid;
            g_Vgc.GameRunning = true;

            if (g_Vgc.GameProcessHandle != NULL) {
                CloseHandle(g_Vgc.GameProcessHandle);
            }
            g_Vgc.GameProcessHandle = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, currentPid);

            wprintf(L"[vgc] Instância do Valorant detectada (PID %u). Registrando com vgk.sys...\n", currentPid);

            /* Notifica o driver de kernel para ativar ObRegisterCallbacks e travar o processo */
            VgcRegisterGamePidWithDriver(currentPid);
        } else {
            /* Verifica se o processo ainda está vivo */
            DWORD exitCode = 0;
            if (GetExitCodeProcess(g_Vgc.GameProcessHandle, &exitCode) && exitCode != STILL_ACTIVE) {
                wprintf(L"[vgc] Processo do jogo encerrado normalmente (ExitCode %u).\n", exitCode);
                CloseHandle(g_Vgc.GameProcessHandle);
                g_Vgc.GameProcessHandle = NULL;
                g_Vgc.GamePid           = 0;
                g_Vgc.GameRunning       = false;
                VgcRegisterGamePidWithDriver(0);
            }
        }
    } else {
        if (g_Vgc.GameRunning) {
            wprintf(L"[vgc] Instância do jogo não encontrada. Resetando monitoramento.\n");
            if (g_Vgc.GameProcessHandle != NULL) {
                CloseHandle(g_Vgc.GameProcessHandle);
                g_Vgc.GameProcessHandle = NULL;
            }
            g_Vgc.GamePid     = 0;
            g_Vgc.GameRunning = false;
            VgcRegisterGamePidWithDriver(0);
        }
    }

    LeaveCriticalSection(&g_Vgc.Lock);
}
