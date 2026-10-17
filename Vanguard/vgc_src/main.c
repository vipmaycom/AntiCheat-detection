/*
 * main.c
 * Riot Vanguard User-Mode Service (vgc.exe v1.19.1.18)
 * Service Control Manager (SCM) Dispatcher, Heartbeat Loop & Console Runner
 */

#include "vgc.h"

VGC_GLOBAL_CONTEXT g_Vgc = { 0 };

static VOID WINAPI VgcServiceCtrlHandler(DWORD Opcode)
{
    switch (Opcode) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        g_Vgc.ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_Vgc.ServiceStatusHandle, &g_Vgc.ServiceStatus);
        SetEvent(g_Vgc.StopEvent);
        break;
    default:
        break;
    }
}

static DWORD WINAPI VgcCoreWorkerThread(LPVOID Parameter)
{
    UNREFERENCED_PARAMETER(Parameter);

    wprintf(L"[vgc] Worker Thread iniciada. Conectando ao driver vgk.sys...\n");

    /* 1. Conecta e faz o handshake com o vgk.sys */
    if (VgcConnectToVgkDriver()) {
        wprintf(L"[vgc] Conexão estabelecida com \\\\.\\vgk com sucesso.\n");
        VgcBindServiceTokenToDriver();
        VgcQueryDriverHardwareProfile(&g_Vgc.HardwareProfile);
    } else {
        wprintf(L"[vgc] AVISO: Driver vgk.sys não está carregado ou inacessível no momento.\n");
    }

    VgcInitializeNetworkTransport();
    VgcStartGameIpcServer();

    /* 2. Loop de Heartbeat e Monitoramento contínuo (1 segundo por tick) */
    while (WaitForSingleObject(g_Vgc.StopEvent, 1000) == WAIT_TIMEOUT) {
        /* Monitora se o Valorant abriu ou fechou */
        VgcMonitorGameLifecycle();

        if (g_Vgc.DriverConnected) {
            VGK_TELEMETRY_PACKET pkt = { 0 };
            if (VgcPollDriverTelemetry(&pkt)) {
                g_Vgc.LastTelemetry = pkt;
                g_Vgc.LastHeartbeatSeq = pkt.SequenceId;
                g_Vgc.ConsecutiveHeartbeatDrops = 0;

                VgcDispatchEncryptedTelemetry(&pkt, &g_Vgc.HardwareProfile);

                /* Avalia se alguma violação requer execução de banimento / encerramento */
                VGC_ERROR_CODE directive = VGC_ERR_SUCCESS;
                VgcQueryBackendAuthorization(&directive);

                if (directive != VGC_ERR_SUCCESS && g_Vgc.GameRunning) {
                    if (directive == VGC_ERR_HARDWARE_BANNED) {
                        VgcTerminateGameWithReason(VGC_ERR_HARDWARE_BANNED, L"Violação de Kernel Crítica (VAN 152 / Hardware Ban)");
                    } else if (directive == VGC_ERR_REBOOT_REQUIRED) {
                        VgcTerminateGameWithReason(VGC_ERR_REBOOT_REQUIRED, L"Integridade de Boot Comprometida (VAN -81 / Reboot Required)");
                    } else if (directive == VGC_ERR_CONNECTION_LOST) {
                        VgcTerminateGameWithReason(VGC_ERR_CONNECTION_LOST, L"Perda de Sincronia de Heartbeat (VAN 128 / Desync)");
                    }
                }
            } else {
                g_Vgc.ConsecutiveHeartbeatDrops++;
                if (g_Vgc.ConsecutiveHeartbeatDrops >= 5 && g_Vgc.GameRunning) {
                    VgcTerminateGameWithReason(VGC_ERR_CONNECTION_LOST, L"Driver Parou de Responder (VAN 128 / Heartbeat Drop)");
                }
            }
        } else {
            /* Tenta reconectar se o driver for carregado a quente */
            if (VgcConnectToVgkDriver()) {
                wprintf(L"[vgc] Conexão tardia com \\\\.\\vgk estabelecida.\n");
                VgcBindServiceTokenToDriver();
                VgcQueryDriverHardwareProfile(&g_Vgc.HardwareProfile);
            }
        }
    }

    wprintf(L"[vgc] Sinal de encerramento recebido. Finalizando worker thread...\n");
    VgcStopGameIpcServer();
    VgcShutdownNetworkTransport();
    VgcDisconnectFromVgkDriver();

    return 0;
}

static VOID WINAPI VgcServiceMain(DWORD Argc, LPWSTR *Argv)
{
    UNREFERENCED_PARAMETER(Argc);
    UNREFERENCED_PARAMETER(Argv);

    g_Vgc.ServiceStatusHandle = RegisterServiceCtrlHandlerW(VGC_SERVICE_NAME_W, VgcServiceCtrlHandler);
    if (!g_Vgc.ServiceStatusHandle) {
        return;
    }

    g_Vgc.ServiceStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    g_Vgc.ServiceStatus.dwCurrentState            = SERVICE_START_PENDING;
    g_Vgc.ServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_Vgc.ServiceStatus.dwWin32ExitCode           = 0;
    g_Vgc.ServiceStatus.dwServiceSpecificExitCode = 0;
    g_Vgc.ServiceStatus.dwCheckPoint              = 0;
    g_Vgc.ServiceStatus.dwWaitHint                = 3000;

    SetServiceStatus(g_Vgc.ServiceStatusHandle, &g_Vgc.ServiceStatus);

    g_Vgc.StopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    InitializeCriticalSection(&g_Vgc.Lock);

    g_Vgc.ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_Vgc.ServiceStatusHandle, &g_Vgc.ServiceStatus);

    /* Executa o worker thread síncrono no contexto do serviço */
    VgcCoreWorkerThread(NULL);

    g_Vgc.ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_Vgc.ServiceStatusHandle, &g_Vgc.ServiceStatus);

    DeleteCriticalSection(&g_Vgc.Lock);
    CloseHandle(g_Vgc.StopEvent);
}

int wmain(int argc, wchar_t *argv[])
{
    wprintf(L"===============================================================\n");
    wprintf(L"  Riot Vanguard User-Mode Service (vgc.exe v1.19.1.18)\n");
    wprintf(L"  Reconstructed Engine - Parity with C:\\vgc.exe\n");
    wprintf(L"===============================================================\n\n");

    /* Se executado com o argumento /console, roda de forma interativa sem depender do SCM */
    if (argc > 1 && (_wcsicmp(argv[1], L"/console") == 0 || _wcsicmp(argv[1], L"-console") == 0)) {
        wprintf(L"[vgc] Executando em modo interativo de console (/console). Pressione Ctrl+C para sair.\n\n");
        g_Vgc.StopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        InitializeCriticalSection(&g_Vgc.Lock);

        VgcCoreWorkerThread(NULL);

        DeleteCriticalSection(&g_Vgc.Lock);
        CloseHandle(g_Vgc.StopEvent);
        return 0;
    }

    /* Caso contrário, conecta-se ao SCM do Windows (Service Control Manager) */
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { (LPWSTR)VGC_SERVICE_NAME_W, (LPSERVICE_MAIN_FUNCTIONW)VgcServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcherW(serviceTable)) {
        DWORD err = GetLastError();
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            wprintf(L"[vgc] Inicializado fora do SCM. Alternando automaticamente para modo console...\n");
            g_Vgc.StopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
            InitializeCriticalSection(&g_Vgc.Lock);

            VgcCoreWorkerThread(NULL);

            DeleteCriticalSection(&g_Vgc.Lock);
            CloseHandle(g_Vgc.StopEvent);
        } else {
            wprintf(L"[vgc] Erro ao iniciar StartServiceCtrlDispatcher: %u\n", err);
        }
    }

    return 0;
}
