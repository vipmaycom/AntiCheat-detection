/*
 * usb_hid_mouse.c
 * Easy Anti-Cheat EOS (EasyAntiCheat_EOS.sys v3.2.0) Hardware Mouse Emulation & Out-of-Band
 * (OOB) Input Vector Auditor (§11, §23.3, §25).
 *
 * Audits:
 *   1. \Driver\MouClass & \Driver\HidUsb device stacks for unauthorized filter attachments
 *      and dispatch table detours.
 *   2. Active \Device\PointerClass0, \Device\PointerClass1, ... device count for
 *      dual-mouse hardware arbitration anomalies (§25.2).
 *   3. USB Enum registry tree for default microcontroller VID/PID pairs:
 *      - VID_2341 (Arduino Leonardo / Micro ATmega32U4)
 *      - VID_1B4F (SparkFun Pro Micro)
 *      - VID_1A86 (QinHeng CH340 / KMBox serial bridge)
 *      - VID_2E8A (Raspberry Pi Pico RP2040 TinyUSB)
 */

#include "eac.h"

VOID
EacAuditUsbHidAndMouseStack(
    _In_ PRTL_PROCESS_MODULES Modules
)
{
    if (!Modules || g_Eac.Unloading || KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return;
    }

    /* 1. Count active \Device\PointerClassN endpoints & verify device stack (§25.2) */
    ULONG activePointerDevices = 0;

    for (ULONG idx = 0; idx < 8; ++idx) {
        WCHAR devNameBuf[64];
        RtlStringCbPrintfW(devNameBuf, sizeof(devNameBuf), L"\\Device\\PointerClass%u", idx);

        UNICODE_STRING uDevName;
        RtlInitUnicodeString(&uDevName, devNameBuf);

        PFILE_OBJECT   fileObj = NULL;
        PDEVICE_OBJECT devObj  = NULL;
        if (NT_SUCCESS(IoGetDeviceObjectPointer(&uDevName, FILE_READ_ATTRIBUTES, &fileObj, &devObj))) {
            activePointerDevices++;

            /* Walk AttachedDevice chain and verify every filter driver's DriverStart is backed */
            PDEVICE_OBJECT currDev = devObj;
            while (currDev != NULL) {
                if (currDev->DriverObject != NULL) {
                    ULONG64 drvStart = (ULONG64)currDev->DriverObject->DriverStart;
                    if (drvStart != 0ULL && !EacIsAddressBackedByModule(Modules, drvStart, NULL, NULL)) {
                        InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_DUAL_MOUSE_ARBITRATION);
                        EacEmitTelemetryChunk(
                            EAC_CHUNK_OP_ALERT,
                            25,
                            "unbacked_mouse_filter_device",
                            idx,
                            (ULONG)(drvStart & 0xFFFFFFFFUL)
                        );
                    }
                }
                currDev = currDev->AttachedDevice;
            }

            ObDereferenceObject(fileObj);
        }
    }

    g_Eac.HwProfile.PointerClassDeviceCount = activePointerDevices;

    /* 2. Probe USB Enum Registry Keys for Default Microcontroller VID/PIDs (§25.1, §25.2) */
    PCWSTR flaggedUsbKeys[] = {
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_2341&PID_8036", /* Arduino Leonardo */
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_2341&PID_8037", /* Arduino Micro */
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_1B4F&PID_9206", /* SparkFun Pro Micro */
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_1A86&PID_7523", /* CH340 KMBox Serial */
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_2E8A&PID_000A"  /* RP2040 Pico SDK */
    };

    ULONG mcuMatches = 0;
    for (ULONG i = 0; i < ARRAYSIZE(flaggedUsbKeys); ++i) {
        UNICODE_STRING uKey;
        RtlInitUnicodeString(&uKey, flaggedUsbKeys[i]);

        OBJECT_ATTRIBUTES oa;
        InitializeObjectAttributes(&oa, &uKey, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        HANDLE hKey = NULL;
        if (NT_SUCCESS(ZwOpenKey(&hKey, KEY_READ, &oa)) && hKey != NULL) {
            mcuMatches++;
            ZwClose(hKey);
            InterlockedOr(&g_Eac.ViolationFlags, EAC_VIO_USB_MCU_DETECTED);
            EacEmitTelemetryChunk(EAC_CHUNK_OP_ALERT, 25, "usb_mcu_vid_pid", i, activePointerDevices);
        }
    }

    g_Eac.HwProfile.FlaggedUsbMcuCount = mcuMatches;
}
