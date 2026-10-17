/*
 * game_ipc_server.c
 * Riot Vanguard User-Mode Service (vgc.exe v1.19.1.18)
 * Local Named Pipe IPC Server: Handshake & Polling directly with VALORANT-Win64-Shipping.exe
 *
 * Implements the receiver for VGC_GAME_IPC_POLL and sender of VGC_GAME_IPC_REPLY
 */

#include "vgc.h"

static DWORD WINAPI VgcGameIpcWorker(LPVOID Parameter)
{
    UNREFERENCED_PARAMETER(Parameter);

    wprintf(L"[vgc-ipc] Servidor Named Pipe iniciado em: " VGC_GAME_IPC_PIPE_NAME_W L"\n");

    while (WaitForSingleObject(g_Vgc.StopEvent, 0) == WAIT_TIMEOUT) {
        HANDLE hPipe = CreateNamedPipeW(
            VGC_GAME_IPC_PIPE_NAME_W,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            sizeof(VGC_GAME_IPC_REPLY),
            sizeof(VGC_GAME_IPC_POLL),
            5000,
            NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            Sleep(500);
            continue;
        }

        g_Vgc.IpcPipeHandle = hPipe;

        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected) {
            VGC_GAME_IPC_POLL pollMsg = { 0 };
            DWORD bytesRead = 0;

            BOOL readOk = ReadFile(hPipe, &pollMsg, sizeof(pollMsg), &bytesRead, NULL);

            if (readOk && bytesRead >= sizeof(VGC_GAME_IPC_POLL)) {
                if (pollMsg.Magic == VGC_IPC_MAGIC_POLL) {
                    /* Atualiza o PID do jogo se veio pelo handshake do IPC */
                    if (pollMsg.GamePid != 0 && g_Vgc.GamePid != pollMsg.GamePid) {
                        EnterCriticalSection(&g_Vgc.Lock);
                        g_Vgc.GamePid = pollMsg.GamePid;
                        g_Vgc.GameRunning = true;
                        LeaveCriticalSection(&g_Vgc.Lock);

                        VgcRegisterGamePidWithDriver(pollMsg.GamePid);
                    }

                    /* Prepara a resposta estruturada VGC_GAME_IPC_REPLY */
                    VGC_GAME_IPC_REPLY replyMsg = { 0 };
                    replyMsg.Magic        = VGC_IPC_MAGIC_REPLY;
                    replyMsg.HeartbeatSeq = g_Vgc.LastHeartbeatSeq;

                    /* Avalia se há diretiva de banimento/kick pendente */
                    VGC_ERROR_CODE directive = VGC_ERR_SUCCESS;
                    VgcQueryBackendAuthorization(&directive);

                    if (directive != VGC_ERR_SUCCESS) {
                        replyMsg.ErrorCode = (ULONG)directive;
                        replyMsg.StatusFlags = (directive == VGC_ERR_HARDWARE_BANNED) ? 2 : 1; /* Bit 1 = Ban, Bit 0 = Kick */
                    } else {
                        replyMsg.ErrorCode = 0;
                        replyMsg.StatusFlags = 0; /* OK */
                    }

                    /* 
                     * Gera o DriverProof real de 64 bytes via IOCTL_VGK_GENERATE_PROOF (0x806)
                     * misturando HMAC-SHA256 (CR3, QPC, Sequence, Violations) + TPM 2.0 EK/PCR[7]
                     */
                    if (!VgcGenerateProofViaDriver((ULONG64)pollMsg.SessionNonce, replyMsg.DriverProof)) {
                        /* Fallback de integridade se o driver estiver desincronizado */
                        RtlCopyMemory(replyMsg.DriverProof, g_Vgc.HardwareProfile.TpmEkPubSha256, 32);
                    }

                    DWORD bytesWritten = 0;
                    WriteFile(hPipe, &replyMsg, sizeof(replyMsg), &bytesWritten, NULL);
                    FlushFileBuffers(hPipe);
                }
            }

            DisconnectNamedPipe(hPipe);
        }

        CloseHandle(hPipe);
        g_Vgc.IpcPipeHandle = NULL;
    }

    wprintf(L"[vgc-ipc] Servidor Named Pipe finalizado.\n");
    return 0;
}

bool
VgcStartGameIpcServer(void)
{
    g_Vgc.IpcServerThreadHandle = CreateThread(
        NULL,
        0,
        VgcGameIpcWorker,
        NULL,
        0,
        NULL
    );

    return (g_Vgc.IpcServerThreadHandle != NULL);
}

void
VgcStopGameIpcServer(void)
{
    if (g_Vgc.IpcPipeHandle != NULL && g_Vgc.IpcPipeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_Vgc.IpcPipeHandle);
        g_Vgc.IpcPipeHandle = NULL;
    }

    if (g_Vgc.IpcServerThreadHandle != NULL) {
        WaitForSingleObject(g_Vgc.IpcServerThreadHandle, 2000);
        CloseHandle(g_Vgc.IpcServerThreadHandle);
        g_Vgc.IpcServerThreadHandle = NULL;
    }
}
