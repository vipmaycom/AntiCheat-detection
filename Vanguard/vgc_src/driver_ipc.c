/*
 * driver_ipc.c
 * Riot Vanguard User-Mode Service (vgc.exe v1.19.1.18)
 * Ring 3 <-> Ring 0 Communication Channel via \\.\vgk IOCTL Interface
 */

#include "vgc.h"

bool
VgcConnectToVgkDriver(void)
{
    if (g_Vgc.VgkDeviceHandle != INVALID_HANDLE_VALUE && g_Vgc.VgkDeviceHandle != NULL) {
        return true;
    }

    g_Vgc.VgkDeviceHandle = CreateFileW(
        VGC_DRIVER_DEVICE_PATH_W,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (g_Vgc.VgkDeviceHandle == INVALID_HANDLE_VALUE) {
        g_Vgc.DriverConnected = false;
        return false;
    }

    g_Vgc.DriverConnected = true;
    return true;
}

void
VgcDisconnectFromVgkDriver(void)
{
    if (g_Vgc.VgkDeviceHandle != INVALID_HANDLE_VALUE && g_Vgc.VgkDeviceHandle != NULL) {
        CloseHandle(g_Vgc.VgkDeviceHandle);
        g_Vgc.VgkDeviceHandle = INVALID_HANDLE_VALUE;
    }
    g_Vgc.DriverConnected = false;
}

bool
VgcBindServiceTokenToDriver(void)
{
    if (!VgcConnectToVgkDriver()) {
        return false;
    }

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        g_Vgc.VgkDeviceHandle,
        IOCTL_VGK_BIND_VGC,
        NULL,
        0,
        NULL,
        0,
        &bytesReturned,
        NULL
    );

    return (ok != FALSE);
}

bool
VgcRegisterGamePidWithDriver(DWORD Pid)
{
    if (!VgcConnectToVgkDriver()) {
        return false;
    }

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        g_Vgc.VgkDeviceHandle,
        IOCTL_VGK_PROTECT_PID,
        &Pid,
        sizeof(DWORD),
        NULL,
        0,
        &bytesReturned,
        NULL
    );

    return (ok != FALSE);
}

bool
VgcPollDriverTelemetry(PVGK_TELEMETRY_PACKET Packet)
{
    if (!VgcConnectToVgkDriver() || !Packet) {
        return false;
    }

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        g_Vgc.VgkDeviceHandle,
        IOCTL_VGK_POLL_TELEMETRY,
        NULL,
        0,
        Packet,
        sizeof(VGK_TELEMETRY_PACKET),
        &bytesReturned,
        NULL
    );

    if (ok && bytesReturned >= sizeof(VGK_TELEMETRY_PACKET)) {
        return true;
    }

    return false;
}

bool
VgcQueryDriverHardwareProfile(PVGK_HARDWARE_PROFILE Profile)
{
    if (!VgcConnectToVgkDriver() || !Profile) {
        return false;
    }

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        g_Vgc.VgkDeviceHandle,
        IOCTL_VGK_QUERY_HWID,
        NULL,
        0,
        Profile,
        sizeof(VGK_HARDWARE_PROFILE),
        &bytesReturned,
        NULL
    );

    if (ok && bytesReturned >= sizeof(VGK_HARDWARE_PROFILE)) {
        return true;
    }

    return false;
}

bool
VgcGenerateProofViaDriver(
    ULONG64 ServerNonce,
    PUCHAR  OutProof64
)
{
    if (!g_Vgc.VgkDeviceHandle || g_Vgc.VgkDeviceHandle == INVALID_HANDLE_VALUE || !OutProof64) {
        return false;
    }

    VGK_PROOF_CHALLENGE challenge = { 0 };
    challenge.ServerNonce = ServerNonce;

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        g_Vgc.VgkDeviceHandle,
        IOCTL_VGK_GENERATE_PROOF,
        &challenge,
        sizeof(challenge),
        &challenge,
        sizeof(challenge),
        &bytesReturned,
        NULL
    );

    if (ok && bytesReturned >= sizeof(VGK_PROOF_CHALLENGE)) {
        RtlCopyMemory(OutProof64, challenge.DriverProof, 64);
        return true;
    }

    return false;
}
