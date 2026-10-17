/*
 * telemetry_tls.c
 * Riot Vanguard User-Mode Service (vgc.exe v1.19.1.18)
 * Telemetry Framing, Hardware ID Digest & Server-Side Ban Directive Evaluation
 */

#include "vgc.h"

static bool g_WsaInitialized = false;

bool
VgcInitializeNetworkTransport(void)
{
    WSADATA wsa;
    int res = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (res == 0) {
        g_WsaInitialized = true;
        return true;
    }
    return false;
}

void
VgcShutdownNetworkTransport(void)
{
    if (g_WsaInitialized) {
        WSACleanup();
        g_WsaInitialized = false;
    }
}

/*
 * Empacota a telemetria do kernel e o HWID composto para submissão à nuvem.
 * Simula a verificação de autenticação e avaliação de integridade de regras.
 */
bool
VgcDispatchEncryptedTelemetry(const VGK_TELEMETRY_PACKET *Telemetry, const VGK_HARDWARE_PROFILE *Hwid)
{
    if (!Telemetry || !Hwid) {
        return false;
    }

    /* Formata a carga de telemetria para gravação e envio */
    if (Telemetry->ViolationFlags != VGK_FLAG_NONE) {
        wprintf(L"[vgc-telemetry] ALERTA: Violação de Kernel Detectada! Mask = 0x%08X (Última Regra: %u)\n",
            Telemetry->ViolationFlags,
            Telemetry->LastMatchedRuleId);
    }

    return true;
}

/*
 * Avalia as flags locais reportadas pelo driver para determinar se a sessão deve ser terminada
 * com base na política de banimento instantâneo ou se o servidor mandou diretiva.
 */
bool
VgcQueryBackendAuthorization(VGC_ERROR_CODE *OutDirective)
{
    if (!OutDirective) {
        return false;
    }

    *OutDirective = VGC_ERR_SUCCESS;

    /* 1. Checagem de violações críticas instantâneas (1 Flag = Banimento/Kill) */
    ULONG flags = g_Vgc.LastTelemetry.ViolationFlags;

    if ((flags & VGK_FLAG_MC_PTE_UNBACKED) ||
        (flags & VGK_FLAG_KSS_UNBACKED_RIP) ||
        (flags & VGK_FLAG_BBL_BYOVD_MATCH) ||
        (flags & VGK_FLAG_PCI_DMA_FLAGGED) ||
        (flags & VGK_FLAG_HV_MSR_HOOKED)) {
        *OutDirective = VGC_ERR_HARDWARE_BANNED; /* VAN 152 */
        return true;
    }

    /* 2. Checagem de Perda de Integridade de Boot / Drivers */
    if ((flags & VGK_FLAG_KERNEL_DEBUGGER) ||
        (flags & VGK_FLAG_IOMMU_DISABLED) ||
        (flags & VGK_FLAG_TPM_ATTEST_FAIL)) {
        *OutDirective = VGC_ERR_REBOOT_REQUIRED; /* VAN -81 */
        return true;
    }

    /* 3. Checagem de Desync Crônico de Heartbeat (> 5 amostras perdidas) */
    if (g_Vgc.ConsecutiveHeartbeatDrops >= 5) {
        *OutDirective = VGC_ERR_CONNECTION_LOST; /* VAN 128 */
        return true;
    }

    return true;
}
