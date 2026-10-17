/*
 * vgc.h
 * Riot Vanguard User-Mode Service (vgc.exe v1.19.1.18)
 * Master Header & Architecture Mapping from C:\vgc.exe (SHA-256 c8f330a7...)
 */

#ifndef _VGC_MASTER_H_
#define _VGC_MASTER_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <bcrypt.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "version.lib")

#define VGC_SERVICE_NAME_W          L"vgc"
#define VGC_SERVICE_DISPLAY_NAME_W  L"Vanguard User-mode Service"
#define VGC_DRIVER_DEVICE_PATH_W    L"\\\\.\\vgk"

#define VGC_VALORANT_EXE_NAME_W     L"VALORANT-Win64-Shipping.exe"
#define VGC_VALORANT_BOOTSTRAP_W    L"VALORANT.exe"
#define VGC_RIOT_CLIENT_EXE_W       L"RiotClientServices.exe"

/* IOCTL Codes for vgc.exe <-> vgk.sys Communication (Parity with vgk.h) */
#define IOCTL_VGK_BIND_VGC          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_PROTECT_PID       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_POLL_TELEMETRY    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_QUERY_HWID        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_MAP_GUARDED_VIEW  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_RUN_FULL_SWEEP    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VGK_GENERATE_PROOF    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma pack(push, 1)
typedef struct _VGK_PROOF_CHALLENGE {
    ULONG64 ServerNonce;        /* Nonce de 64 bits recebido do servidor Riot */
    UCHAR   DriverProof[64];    /* Saída: 64 bytes gerados (HMAC Kernel + TPM 2.0) */
} VGK_PROOF_CHALLENGE, *PVGK_PROOF_CHALLENGE;
#pragma pack(pop)

/* Telemetry Violation Flags (Direct Parity with vgk.sys) */
#define VGK_FLAG_NONE               0x00000000
#define VGK_FLAG_KSS_UNBACKED_RIP   0x00000001
#define VGK_FLAG_KSS_SPOOFED_FRAME  0x00000002
#define VGK_FLAG_KSS_DR7_ACTIVE     0x00000004
#define VGK_FLAG_BBL_BYOVD_MATCH    0x00000008
#define VGK_FLAG_EBBL_EXT_MATCH     0x00000010
#define VGK_FLAG_MC_PTE_UNBACKED    0x00000020
#define VGK_FLAG_MC_HASH_MISMATCH   0x00000040
#define VGK_FLAG_DCL_DEVICE_FOUND   0x00000080
#define VGK_FLAG_EDCL_FILTER_FOUND  0x00000100
#define VGK_FLAG_DF_DRIVER_FLAG     0x00000200
#define VGK_FLAG_WIC_ANOMALY        0x00000400
#define VGK_FLAG_IOMMU_DISABLED     0x00000800
#define VGK_FLAG_HV_FOREIGN_VENDOR  0x00001000
#define VGK_FLAG_HV_MSR_HOOKED      0x00002000
#define VGK_FLAG_HV_TIMING_ANOMALY  0x00004000
#define VGK_FLAG_KERNEL_DEBUGGER    0x00008000
#define VGK_FLAG_PCI_DMA_FLAGGED    0x00010000
#define VGK_FLAG_SMBIOS_MISMATCH    0x00020000
#define VGK_FLAG_TPM_ATTEST_FAIL    0x00040000
#define VGK_FLAG_USB_MCU_DETECTED   0x00080000
#define VGK_FLAG_USB_DUAL_MOUSE     0x00100000

#pragma pack(push, 1)
typedef struct _VGK_TELEMETRY_PACKET {
    ULONG64         SequenceId;
    ULONG64         PreciseTimestamp;
    ULONG           ViolationFlags;
    ULONG           LastMatchedRuleId;
    ULONG           ActiveGamePid;
    ULONG           IpiSampleCount;
    ULONG           WorkItemPassCount;
} VGK_TELEMETRY_PACKET, *PVGK_TELEMETRY_PACKET;

typedef struct _VGK_HARDWARE_PROFILE {
    UCHAR           SmbiosUuid[16];
    CHAR            SmbiosBiosVendor[64];
    CHAR            SmbiosBiosVersion[64];
    CHAR            SmbiosSystemManufacturer[64];
    CHAR            SmbiosSystemProduct[64];
    CHAR            SmbiosSystemSerial[64];
    CHAR            SmbiosBaseboardSerial[64];
    CHAR            SmbiosChassisSerial[64];
    CHAR            SmbiosProcessorBrand[64];
    CHAR            SmbiosDimmSerials[4][32];
    ULONG           SmbiosDimmPopulatedCount;
    CHAR            CpuVendor[13];
    CHAR            CpuBrandString[49];
    CHAR            HypervisorVendor[13];
    ULONG           ProcessorId;
    ULONG           HypervisorInterfaceSig;
    ULONG64         QpcFrequency;
    ULONG64         TscCpuidRatio;
    ULONG64         InvlpgLatencyCycles;
    ULONG64         LstarMsrValue;
    UCHAR           TpmEkPubSha256[32];
    UCHAR           TpmEkCertSha256[32];
    UCHAR           TpmPcr0Sha256[32];
    UCHAR           TpmPcr7Sha256[32];
    CHAR            NvmeSerialNumber[64];
    UCHAR           NicPermanentMac[6];
    WCHAR           MachineGuid[40];
    WCHAR           HwProfileGuid[40];
    UCHAR           VgkImageSha256[32];
    UCHAR           NtoskrnlHeaderSha256[32];
    ULONG           AcpiDmarSignature;
    ULONG           AcpiMadtSignature;
    ULONG           FlaggedPciDeviceCount;
    ULONG           FlaggedUsbMcuCount;
    ULONG           ActiveMouseDeviceCount;
    BOOLEAN         IommuInterfacePresent;
    BOOLEAN         HypervisorBitPresent;
    BOOLEAN         VbsEnabled;
    BOOLEAN         HvciEnabled;
    BOOLEAN         SecureBootEnabled;
    BOOLEAN         TestSigningEnabled;
    BOOLEAN         KernelDebuggerPresent;
} VGK_HARDWARE_PROFILE, *PVGK_HARDWARE_PROFILE;
#pragma pack(pop)

/* Vanguard Client-Server Directive / Error Codes */
typedef enum _VGC_ERROR_CODE {
    VGC_ERR_SUCCESS          = 0,
    VGC_ERR_CONNECTION_LOST  = 128,  /* VAN 128: Driver Heartbeat Desync */
    VGC_ERR_DRIVER_NOT_FOUND = 138,  /* VAN 138: Driver Not Running */
    VGC_ERR_HARDWARE_BANNED  = 152,  /* VAN 152: Machine HWID Restricted */
    VGC_ERR_SESSION_REVOKED  = 5,    /* VAL 5: Account Logged In Elsewhere / Kicked */
    VGC_ERR_REBOOT_REQUIRED  = -81   /* VAN -81: Boot driver state desync */
} VGC_ERROR_CODE;

#define VGC_GAME_IPC_PIPE_NAME_W    L"\\\\.\\pipe\\vgc_game_ipc"
#define VGC_IPC_MAGIC_POLL          0x544E4156  /* 'VANT' */
#define VGC_IPC_MAGIC_REPLY         0x52434756  /* 'VGCR' */

#pragma pack(push, 1)
typedef struct _VGC_GAME_IPC_POLL {
    ULONG   Magic;              /* 'VANT' (0x544E4156) */
    ULONG   SessionNonce;       /* Valor gerado aleatoriamente pelo jogo */
    ULONG   GameSequence;       /* Contador incremental de frames/ticks da UE4 */
    ULONG   GamePid;            /* PID atual do VALORANT-Win64-Shipping.exe */
} VGC_GAME_IPC_POLL, *PVGC_GAME_IPC_POLL;

typedef struct _VGC_GAME_IPC_REPLY {
    ULONG   Magic;              /* 'VGCR' (0x52434756) */
    ULONG   StatusFlags;        /* 0 = OK, Bit 0 = Kick, Bit 1 = Ban */
    ULONG   ErrorCode;          /* Ex: 152 (VAN 152), 128 (VAN 128), -81, 5 */
    ULONG64 HeartbeatSeq;       /* Última sequência de batimento cardíaco do vgk.sys */
    UCHAR   DriverProof[64];    /* Assinatura criptográfica / hash de integridade */
} VGC_GAME_IPC_REPLY, *PVGC_GAME_IPC_REPLY;
#pragma pack(pop)

typedef struct _VGC_GLOBAL_CONTEXT {
    SERVICE_STATUS          ServiceStatus;
    SERVICE_STATUS_HANDLE   ServiceStatusHandle;
    HANDLE                  StopEvent;
    HANDLE                  VgkDeviceHandle;
    HANDLE                  WorkerThreadHandle;
    HANDLE                  IpcServerThreadHandle;
    HANDLE                  IpcPipeHandle;
    CRITICAL_SECTION        Lock;

    DWORD                   GamePid;
    HANDLE                  GameProcessHandle;
    bool                    GameRunning;
    bool                    DriverConnected;

    ULONG64                 LastHeartbeatSeq;
    DWORD                   ConsecutiveHeartbeatDrops;

    VGK_HARDWARE_PROFILE    HardwareProfile;
    VGK_TELEMETRY_PACKET    LastTelemetry;
} VGC_GLOBAL_CONTEXT, *PVGC_GLOBAL_CONTEXT;

extern VGC_GLOBAL_CONTEXT g_Vgc;

/* Module Prototypes */
/* driver_ipc.c */
bool VgcConnectToVgkDriver(void);
void VgcDisconnectFromVgkDriver(void);
bool VgcBindServiceTokenToDriver(void);
bool VgcRegisterGamePidWithDriver(DWORD Pid);
bool VgcPollDriverTelemetry(PVGK_TELEMETRY_PACKET Packet);
bool VgcQueryDriverHardwareProfile(PVGK_HARDWARE_PROFILE Profile);
bool VgcGenerateProofViaDriver(ULONG64 ServerNonce, PUCHAR OutProof64);

/* process_watcher.c */
DWORD VgcScanForValorantProcess(void);
bool VgcAuditProcessSignature(PCWSTR FilePath);
void VgcMonitorGameLifecycle(void);
void VgcTerminateGameWithReason(VGC_ERROR_CODE ErrorCode, PCWSTR Reason);

/* telemetry_tls.c */
bool VgcInitializeNetworkTransport(void);
void VgcShutdownNetworkTransport(void);
bool VgcDispatchEncryptedTelemetry(const VGK_TELEMETRY_PACKET *Telemetry, const VGK_HARDWARE_PROFILE *Hwid);
bool VgcQueryBackendAuthorization(VGC_ERROR_CODE *OutDirective);

/* game_ipc_server.c */
bool VgcStartGameIpcServer(void);
void VgcStopGameIpcServer(void);

#endif /* _VGC_MASTER_H_ */
