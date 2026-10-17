/*
 * log_exporter.c
 * Vanguard Log Zip Exporter & vgtray-settings.json Serializer
 * Reconstructed from FUN_140083a40, FUN_140084560, and FUN_14009c7d0
 */

#include "vgtray.h"

/* ============================================================================
 * SETTINGS JSON SERIALIZER (FUN_14009c7d0)
 * Reads/writes %LOCALAPPDATA%\Riot Games\Riot Vanguard\vgtray-settings.json
 * Exact schema from FUN_14009c7d0:
 * {"theme":%d,"language":%d,"advanced_view":%d,"precheck_option":%d,
 *  "failure_reason":%d,"error_code":%lld,"vgtray_version":%d,
 *  "vanguard_version":"%s","precheck_opted_out":%d,
 *  "precheck_enrolled_but_unmet":%d,"precheck_completed_after_settings_changes":%d,
 *  "precheck_reverted_after_completion":%d,"precheck_initial_state_recorded":%d,
 *  "precheck_initially_unmet":%d,"precheck_completed_once":%d,
 *  "precheck_25h2_originally_missing":%d,"precheck_tpm_originally_missing":%d,
 *  "precheck_secure_boot_originally_missing":%d,"precheck_vbs_originally_missing":%d,
 *  "precheck_hvci_originally_missing":%d,"precheck_iommu_originally_missing":%d}
 * ============================================================================ */
static int VgTrayJsonReadInt(const char* szJson, const char* szKey, int iDefault)
{
    char szPattern[128] = { 0 };
    sprintf_s(szPattern, sizeof(szPattern), "\"%s\":", szKey);
    const char* pFound = strstr(szJson, szPattern);
    if (pFound == NULL) {
        return iDefault;
    }
    pFound += strlen(szPattern);
    while (*pFound == ' ' || *pFound == '\t') {
        pFound++;
    }
    return atoi(pFound);
}

BOOL VgTrayLoadSettingsJson(VGTRAY_SETTINGS* pSettings)
{
    if (pSettings == NULL) {
        return FALSE;
    }

    ZeroMemory(pSettings, sizeof(*pSettings));
    pSettings->theme = VgTrayIsSystemLightTheme() ? 1 : 0;
    pSettings->language = 0; /* English */
    pSettings->advanced_view = 0;
    pSettings->precheck_option = 0;
    pSettings->failure_reason = 0;
    pSettings->error_code = 0;
    pSettings->vgtray_version = VGTRAY_SCHEMA_VERSION;
    strcpy_s(pSettings->vanguard_version, sizeof(pSettings->vanguard_version), VGTRAY_VERSION_STRING);

    if (g_VgTray.wszSettingsFilePath[0] == L'\0') {
        return FALSE;
    }

    HANDLE hFile = CreateFileW(
        g_VgTray.wszSettingsFilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    char szBuf[2048] = { 0 };
    DWORD cbRead = 0;
    BOOL bOk = ReadFile(hFile, szBuf, sizeof(szBuf) - 1, &cbRead, NULL);
    CloseHandle(hFile);

    if (!bOk || cbRead == 0) {
        return FALSE;
    }

    szBuf[cbRead] = '\0';
    pSettings->theme             = VgTrayJsonReadInt(szBuf, "theme", pSettings->theme);
    pSettings->language          = VgTrayJsonReadInt(szBuf, "language", 0);
    pSettings->advanced_view     = VgTrayJsonReadInt(szBuf, "advanced_view", 0);
    pSettings->precheck_option   = VgTrayJsonReadInt(szBuf, "precheck_option", 0);
    pSettings->failure_reason    = VgTrayJsonReadInt(szBuf, "failure_reason", 0);
    pSettings->vgtray_version    = VgTrayJsonReadInt(szBuf, "vgtray_version", VGTRAY_SCHEMA_VERSION);
    pSettings->precheck_opted_out = VgTrayJsonReadInt(szBuf, "precheck_opted_out", 0);
    pSettings->precheck_enrolled_but_unmet = VgTrayJsonReadInt(szBuf, "precheck_enrolled_but_unmet", 0);
    pSettings->precheck_completed_after_settings_changes =
        VgTrayJsonReadInt(szBuf, "precheck_completed_after_settings_changes", 0);
    pSettings->precheck_reverted_after_completion =
        VgTrayJsonReadInt(szBuf, "precheck_reverted_after_completion", 0);
    pSettings->precheck_initial_state_recorded =
        VgTrayJsonReadInt(szBuf, "precheck_initial_state_recorded", 0);
    pSettings->precheck_initially_unmet =
        VgTrayJsonReadInt(szBuf, "precheck_initially_unmet", 0);
    pSettings->precheck_completed_once =
        VgTrayJsonReadInt(szBuf, "precheck_completed_once", 0);
    pSettings->precheck_25h2_originally_missing =
        VgTrayJsonReadInt(szBuf, "precheck_25h2_originally_missing", 0);
    pSettings->precheck_tpm_originally_missing =
        VgTrayJsonReadInt(szBuf, "precheck_tpm_originally_missing", 0);
    pSettings->precheck_secure_boot_originally_missing =
        VgTrayJsonReadInt(szBuf, "precheck_secure_boot_originally_missing", 0);
    pSettings->precheck_vbs_originally_missing =
        VgTrayJsonReadInt(szBuf, "precheck_vbs_originally_missing", 0);
    pSettings->precheck_hvci_originally_missing =
        VgTrayJsonReadInt(szBuf, "precheck_hvci_originally_missing", 0);
    pSettings->precheck_iommu_originally_missing =
        VgTrayJsonReadInt(szBuf, "precheck_iommu_originally_missing", 0);

    return TRUE;
}

BOOL VgTraySaveSettingsJson(const VGTRAY_SETTINGS* pSettings)
{
    if (pSettings == NULL || g_VgTray.wszSettingsFilePath[0] == L'\0') {
        return FALSE;
    }

    char szJson[2048] = { 0 };
    int cbLen = sprintf_s(
        szJson,
        sizeof(szJson),
        "{\"theme\":%d,\"language\":%d,\"advanced_view\":%d,\"precheck_option\":%d,"
        "\"failure_reason\":%d,\"error_code\":%lld,\"vgtray_version\":%d,"
        "\"vanguard_version\":\"%s\",\"precheck_opted_out\":%d,"
        "\"precheck_enrolled_but_unmet\":%d,\"precheck_completed_after_settings_changes\":%d,"
        "\"precheck_reverted_after_completion\":%d,\"precheck_initial_state_recorded\":%d,"
        "\"precheck_initially_unmet\":%d,\"precheck_completed_once\":%d,"
        "\"precheck_25h2_originally_missing\":%d,\"precheck_tpm_originally_missing\":%d,"
        "\"precheck_secure_boot_originally_missing\":%d,\"precheck_vbs_originally_missing\":%d,"
        "\"precheck_hvci_originally_missing\":%d,\"precheck_iommu_originally_missing\":%d}",
        pSettings->theme,
        pSettings->language,
        pSettings->advanced_view,
        pSettings->precheck_option,
        pSettings->failure_reason,
        (long long)pSettings->error_code,
        pSettings->vgtray_version,
        pSettings->vanguard_version[0] ? pSettings->vanguard_version : VGTRAY_VERSION_STRING,
        pSettings->precheck_opted_out,
        pSettings->precheck_enrolled_but_unmet,
        pSettings->precheck_completed_after_settings_changes,
        pSettings->precheck_reverted_after_completion,
        pSettings->precheck_initial_state_recorded,
        pSettings->precheck_initially_unmet,
        pSettings->precheck_completed_once,
        pSettings->precheck_25h2_originally_missing,
        pSettings->precheck_tpm_originally_missing,
        pSettings->precheck_secure_boot_originally_missing,
        pSettings->precheck_vbs_originally_missing,
        pSettings->precheck_hvci_originally_missing,
        pSettings->precheck_iommu_originally_missing);

    if (cbLen <= 0) {
        return FALSE;
    }

    HANDLE hFile = CreateFileW(
        g_VgTray.wszSettingsFilePath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DWORD dwWritten = 0;
    BOOL bOk = WriteFile(hFile, szJson, (DWORD)cbLen, &dwWritten, NULL);
    CloseHandle(hFile);
    return (bOk && dwWritten == (DWORD)cbLen);
}

/* ============================================================================
 * SAVE RIOT VANGUARD LOGS ZIP ARCHIVER (FUN_140083a40)
 * 1. Generates default archive name "vanguard_logs_%04u%02u%02u_%02u%02u%02u.zip"
 * 2. Prompts user via GetSaveFileNameW (Title: "Save Riot Vanguard Logs",
 *    Filter: "Zip Files (*.zip)\0*.zip\0", Flags: 0x806)
 * 3. Creates staging dir "%TEMP%\vanguard_logs_<timestamp>"
 * 4. Copies *.log files from "<InstallDir>\Logs" via FindFirstFileW + CopyFileW
 * 5. Invokes:
 *      powershell.exe -NoProfile -NonInteractive -Command
 *      "Compress-Archive -Path '<temp>\*.log' -DestinationPath '<dest>' -Force"
 *    with CREATE_NO_WINDOW (0x08000000) and 60,000ms WaitForSingleObject
 * 6. Cleans up staging dir and highlights output file via:
 *      ShellExecuteW(NULL, L"open", L"explorer.exe", L"/select,\"<dest>\"", NULL, SW_SHOWNORMAL)
 * ============================================================================ */
BOOL VgTrayExportLogsZipInteractive(HWND hOwnerWnd)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    WCHAR wszStamp[64] = { 0 };
    swprintf_s(
        wszStamp,
        _countof(wszStamp),
        L"%04u%02u%02u_%02u%02u%02u",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);

    WCHAR wszDestZip[MAX_PATH] = { 0 };
    swprintf_s(wszDestZip, _countof(wszDestZip), L"vanguard_logs_%s.zip", wszStamp);

    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hOwnerWnd;
    ofn.lpstrFilter = L"Zip Files (*.zip)\0*.zip\0\0";
    ofn.lpstrFile = wszDestZip;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Save Riot Vanguard Logs";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_EXPLORER; /* 0x806 */
    ofn.lpstrDefExt = L"zip";

    if (!GetSaveFileNameW(&ofn)) {
        return FALSE;
    }

    WCHAR wszTempRoot[MAX_PATH] = { 0 };
    DWORD cchTemp = GetTempPathW(MAX_PATH, wszTempRoot);
    if (cchTemp == 0 || cchTemp >= MAX_PATH) {
        return FALSE;
    }

    WCHAR wszStageDir[MAX_PATH] = { 0 };
    swprintf_s(wszStageDir, _countof(wszStageDir), L"%svanguard_logs_%s", wszTempRoot, wszStamp);
    if (!CreateDirectoryW(wszStageDir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return FALSE;
    }

    /* Copy Logs\*.log from Vanguard directory into staging folder */
    WCHAR wszLogsPattern[MAX_PATH] = { 0 };
    swprintf_s(wszLogsPattern, _countof(wszLogsPattern), L"%s\\Logs\\*.log", g_VgTray.wszInstallDir);

    DWORD cCopied = 0;
    WIN32_FIND_DATAW ffd = { 0 };
    HANDLE hFind = FindFirstFileW(wszLogsPattern, &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                WCHAR wszSrcFile[MAX_PATH] = { 0 };
                WCHAR wszDstFile[MAX_PATH] = { 0 };
                swprintf_s(wszSrcFile, _countof(wszSrcFile), L"%s\\Logs\\%s", g_VgTray.wszInstallDir, ffd.cFileName);
                swprintf_s(wszDstFile, _countof(wszDstFile), L"%s\\%s", wszStageDir, ffd.cFileName);
                if (CopyFileW(wszSrcFile, wszDstFile, FALSE)) {
                    cCopied++;
                }
            }
        } while (FindNextFileW(hFind, &ffd));
        FindClose(hFind);
    }

    /* Also copy %LOCALAPPDATA%\Riot Games\Riot Vanguard\vgtray.log if present */
    if (g_VgTray.wszLogFilePath[0] != L'\0') {
        WCHAR wszTrayLogDst[MAX_PATH] = { 0 };
        swprintf_s(wszTrayLogDst, _countof(wszTrayLogDst), L"%s\\vgtray.log", wszStageDir);
        if (CopyFileW(g_VgTray.wszLogFilePath, wszTrayLogDst, FALSE)) {
            cCopied++;
        }
    }

    BOOL bArchived = FALSE;
    if (cCopied > 0) {
        DeleteFileW(wszDestZip);

        WCHAR wszPsCmd[1024] = { 0 };
        swprintf_s(
            wszPsCmd,
            _countof(wszPsCmd),
            L"powershell.exe -NoProfile -NonInteractive -Command \"Compress-Archive -Path '%s\\*.log' -DestinationPath '%s' -Force\"",
            wszStageDir,
            wszDestZip);

        STARTUPINFOW si = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;

        if (CreateProcessW(
                NULL,
                wszPsCmd,
                NULL,
                NULL,
                FALSE,
                CREATE_NO_WINDOW,
                NULL,
                NULL,
                &si,
                &pi)) {
            WaitForSingleObject(pi.hProcess, 60000);
            DWORD dwExitCode = 1;
            GetExitCodeProcess(pi.hProcess, &dwExitCode);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);

            DWORD dwAttrs = GetFileAttributesW(wszDestZip);
            bArchived = (dwExitCode == 0 && dwAttrs != INVALID_FILE_ATTRIBUTES &&
                         (dwAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
        }
    }

    /* Cleanup staged *.log files and remove temp folder (FUN_140083a40) */
    WCHAR wszStageCleanup[MAX_PATH] = { 0 };
    swprintf_s(wszStageCleanup, _countof(wszStageCleanup), L"%s\\*", wszStageDir);
    hFind = FindFirstFileW(wszStageCleanup, &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                WCHAR wszTmpFile[MAX_PATH] = { 0 };
                swprintf_s(wszTmpFile, _countof(wszTmpFile), L"%s\\%s", wszStageDir, ffd.cFileName);
                DeleteFileW(wszTmpFile);
            }
        } while (FindNextFileW(hFind, &ffd));
        FindClose(hFind);
    }
    RemoveDirectoryW(wszStageDir);

    if (bArchived) {
        WCHAR wszSelectArg[MAX_PATH + 32] = { 0 };
        swprintf_s(wszSelectArg, _countof(wszSelectArg), L"/select,\"%s\"", wszDestZip);
        ShellExecuteW(NULL, L"open", L"explorer.exe", wszSelectArg, NULL, SW_SHOWNORMAL);
        VgTrayLogW(L"Saved Vanguard logs archive to %ls", wszDestZip);
    }

    return bArchived;
}
