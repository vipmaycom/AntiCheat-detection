/*
 * toast_registry.c
 * Kernel Registry Toast Notification Watcher (HKLM\Software\vgk)
 * Reconstructed from FUN_14008a430, FUN_14008e8f0, and FUN_140091470
 */

#include "vgtray.h"

/* ============================================================================
 * FNV-1a 32-BIT HASH (FUN_14008a430)
 * Seed: 0x811C9DC5 | Prime: 0x01000193
 * Used to deduplicate KBToastNotification binary blobs written by vgk.sys.
 * ============================================================================ */
uint32_t VgTrayComputeFnv1a32(const uint8_t* pData, size_t cbSize)
{
    uint32_t uHash = VGTRAY_FNV1A_32_OFFSET;
    for (size_t i = 0; i < cbSize; i++) {
        uHash ^= (uint32_t)pData[i];
        uHash *= VGTRAY_FNV1A_32_PRIME;
    }
    return uHash;
}

/* ============================================================================
 * KBToastNotification BINARY PARSER (FUN_14008a430)
 * Binary format in HKLM\Software\vgk -> "KBToastNotification" (max 0xA07 bytes):
 *   Offset 0x00: BYTE   bNotificationType (0 = Blocked file, 1 = Incompatible software)
 *   Offset 0x01: BYTE   bReserved
 *   Offset 0x02: USHORT cchField[3]       (lengths in WCHARs of up to 3 strings)
 *   Offset 0x08: WCHAR  wszPayload[...]   (packed UTF-16LE strings)
 * ============================================================================ */
BOOL VgTrayPollKbToastNotification(VGTRAY_TOAST_ITEM* pOutToast)
{
    if (pOutToast == NULL) {
        return FALSE;
    }

    HKEY hKey = NULL;
    LSTATUS lStatus = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        VGK_REGISTRY_KEY,
        0,
        KEY_QUERY_VALUE,
        &hKey);
    if (lStatus != ERROR_SUCCESS) {
        return FALSE;
    }

    uint8_t abBuffer[VGTRAY_MAX_TOAST_BLOB_SIZE + 1] = { 0 };
    DWORD cbData = VGTRAY_MAX_TOAST_BLOB_SIZE;
    DWORD dwType = 0;

    lStatus = RegQueryValueExW(
        hKey,
        VGK_VAL_TOAST_NOTIFICATION,
        NULL,
        &dwType,
        abBuffer,
        &cbData);

    /* Fallback check for BlockedImage / DetectedImage string values */
    WCHAR wszBlockedImage[MAX_PATH] = { 0 };
    DWORD cbBlocked = sizeof(wszBlockedImage);
    RegQueryValueExW(hKey, VGK_VAL_BLOCKED_IMAGE, NULL, NULL, (LPBYTE)wszBlockedImage, &cbBlocked);

    RegCloseKey(hKey);

    if (lStatus != ERROR_SUCCESS || cbData < 8 || cbData > VGTRAY_MAX_TOAST_BLOB_SIZE) {
        return FALSE;
    }

    BYTE bNotifType = abBuffer[0];
    if (bNotifType >= 2) {
        return FALSE;
    }

    /* Validate total packed UTF-16LE string length against cbData - 8 (FUN_14008a430) */
    const uint16_t* pu16Lengths = (const uint16_t*)(abBuffer + 2);
    uint32_t cchTotal = (uint32_t)pu16Lengths[0] + (uint32_t)pu16Lengths[1] + (uint32_t)pu16Lengths[2];
    if (cchTotal == 0 || (uint64_t)cchTotal * sizeof(WCHAR) > (uint64_t)(cbData - 8)) {
        return FALSE;
    }

    uint32_t uHash = VgTrayComputeFnv1a32(abBuffer, cbData);

    EnterCriticalSection(&g_VgTray.csToastLock);
    if (uHash == g_VgTray.dwLastToastHash) {
        LeaveCriticalSection(&g_VgTray.csToastLock);
        return FALSE;
    }
    g_VgTray.dwLastToastHash = uHash;
    LeaveCriticalSection(&g_VgTray.csToastLock);

    ZeroMemory(pOutToast, sizeof(*pOutToast));
    pOutToast->bNotificationType = bNotifType;
    pOutToast->dwFnvHash = uHash;

    const WCHAR* pCursor = (const WCHAR*)(abBuffer + 8);
    WCHAR wszFields[3][MAX_PATH] = { { 0 } };
    for (int i = 0; i < 3; i++) {
        uint16_t cch = pu16Lengths[i];
        if (cch > 0) {
            size_t cchCopy = (cch < (MAX_PATH - 1)) ? cch : (MAX_PATH - 1);
            wcsncpy_s(wszFields[i], MAX_PATH, pCursor, cchCopy);
            pCursor += cch;
        }
    }

    wcscpy_s(pOutToast->wszTitle, _countof(pOutToast->wszTitle), L"Riot Vanguard");
    wcsncpy_s(pOutToast->wszFilePath, _countof(pOutToast->wszFilePath),
              wszFields[0][0] ? wszFields[0] : wszBlockedImage, _TRUNCATE);
    wcscpy_s(pOutToast->wszHelpUrl, _countof(pOutToast->wszHelpUrl),
             wszFields[2][0] ? wszFields[2] : VANGUARD_SUPPORT_URL_REQ);

    if (bNotifType == 0) {
        swprintf_s(
            pOutToast->wszMessage,
            _countof(pOutToast->wszMessage),
            L"Vanguard has blocked the following file from loading on your system:\n%s\n\nClick here to learn more.",
            pOutToast->wszFilePath);
    } else {
        swprintf_s(
            pOutToast->wszMessage,
            _countof(pOutToast->wszMessage),
            L"Vanguard has detected incompatible software on your machine:\n%s\n\nClick here to learn more.",
            pOutToast->wszFilePath);
    }

    EnterCriticalSection(&g_VgTray.csToastLock);
    g_VgTray.lastToast = *pOutToast;
    g_VgTray.bHasPendingToast = TRUE;
    LeaveCriticalSection(&g_VgTray.csToastLock);

    VgTrayLogW(L"KBToastNotification received (type=%u, hash=0x%08X, file=%ls)",
               (unsigned)bNotifType, uHash, pOutToast->wszFilePath);
    return TRUE;
}

/* ============================================================================
 * REGISTRY WATCHER THREAD (FUN_14008e8f0)
 * Monitors HKLM\Software\vgk via RegNotifyChangeKeyValue and dispatches
 * balloon notifications immediately when vgk.sys updates KBToastNotification.
 * ============================================================================ */
DWORD WINAPI VgTrayToastWatcherThread(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    HANDLE hRegEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (hRegEvent == NULL) {
        return 1;
    }

    while (WaitForSingleObject(g_VgTray.hStopWorkerEvent, 0) == WAIT_TIMEOUT) {
        HKEY hKey = NULL;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, VGK_REGISTRY_KEY, 0, KEY_NOTIFY | KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
            ResetEvent(hRegEvent);
            if (RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, hRegEvent, TRUE) == ERROR_SUCCESS) {
                HANDLE hWait[2] = { g_VgTray.hStopWorkerEvent, hRegEvent };
                DWORD dwWait = WaitForMultipleObjects(2, hWait, FALSE, 5000);
                if (dwWait == WAIT_OBJECT_0 + 1) {
                    VGTRAY_TOAST_ITEM toast = { 0 };
                    if (VgTrayPollKbToastNotification(&toast)) {
                        VgTrayShowBalloonNotification(toast.wszTitle, toast.wszMessage, NIIF_WARNING);
                    }
                }
            } else {
                WaitForSingleObject(g_VgTray.hStopWorkerEvent, 2000);
            }
            RegCloseKey(hKey);
        } else {
            if (WaitForSingleObject(g_VgTray.hStopWorkerEvent, 2000) == WAIT_OBJECT_0) {
                break;
            }
        }
    }

    CloseHandle(hRegEvent);
    return 0;
}
