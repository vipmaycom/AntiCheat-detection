/*
 * usb_hid_mouse.c
 * Riot Vanguard Kernel-Mode Anti-Cheat (vgk.sys v1.19.1.18)
 * Sections 11, 23, 25: Hardware Mouse Emulation & Out-of-Band (OOB) Input Vector Analysis
 *
 * Implements:
 *   - §25.1 / §25.2 Microcontroller (MCU) Hardware VID/PID Blacklist:
 *                     Arduino Leonardo (VID_2341&PID_8036 / Bootloader VID_2341&PID_0036)
 *                     SparkFun Pro Micro ATmega32U4 (VID_1B4F&PID_9206)
 *                     RP2040 TinyUSB / Raspberry Pi Pico (VID_CAFE / VID_2E8A)
 *                     KMBox / Makemcu CH340/CH341 Bridge (VID_1A86&PID_7523)
 *   - §25.2         Stock 50-byte Arduino ATmega32U4 HID Mouse Report Descriptor signature
 *   - §25.2 / §25.3 USB Bus Topology Walk (\Driver\usbhub, \Driver\usbccgp, \Driver\MouHid,
 *                   \Driver\MouClass) via IoEnumerateDeviceObjectList & IoGetDeviceProperty
 *   - §25.3         Dual-Mouse Conflict Arbitration (\Device\PointerClass0 vs \Device\PointerClass1)
 */

#include "vgk.h"

/*
 * §25.2: Exact 50-byte Stock Arduino ATmega32U4 Mouse Report Descriptor
 * (PluggableUSB / HID.cpp standard descriptor flagged by Vanguard)
 */
const UCHAR g_VgkStockArduinoReportDescriptor[50] = {
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01,
    0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05,
    0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81,
    0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06, 0xC0, 0xC0
};

/*
 * §25.2: Blacklisted Hardware ID Substrings on USB Bus (\Driver\usbhub / \Driver\usbccgp / \Driver\HidUsb)
 */
static const PCWSTR g_VgkBlacklistedUsbHardwareIds[] = {
    L"VID_2341&PID_8036",   /* Arduino Leonardo ATmega32U4 */
    L"VID_2341&PID_0036",   /* Arduino Leonardo Caterina Bootloader */
    L"VID_2341&PID_8037",   /* Arduino Micro ATmega32U4 */
    L"VID_1B4F&PID_9206",   /* SparkFun Pro Micro 5V/16MHz */
    L"VID_1B4F&PID_9205",   /* SparkFun Pro Micro Bootloader */
    L"VID_CAFE&",           /* RP2040 TinyUSB Default Test VID */
    L"VID_2E8A&PID_0003",   /* Raspberry Pi Pico SDK Default CDC+HID */
    L"VID_2E8A&PID_000A",   /* Raspberry Pi Pico MicroPython/CircuitPython */
    L"VID_1A86&PID_7523",   /* KMBox B+ / Makemcu CH340 Serial Bridge */
    L"VID_1A86&PID_55D4",   /* KMBox CH9102 Serial Bridge */
    L"VID_16C0&PID_0486"    /* Teensy 3.x / 4.x RawHID / Serial+Mouse */
};

/*
 * Case-insensitive wide substring check for Hardware IDs
 */
static BOOLEAN
VgkWideStringContainsInsensitive(
    _In_reads_or_z_(HaystackChars) PCWSTR Haystack,
    _In_ ULONG HaystackChars,
    _In_z_ PCWSTR Needle
)
{
    SIZE_T needleLen = wcslen(Needle);
    if (needleLen == 0 || HaystackChars < needleLen) {
        return FALSE;
    }

    for (ULONG i = 0; i + needleLen <= HaystackChars; i++) {
        if (Haystack[i] == L'\0') {
            continue;
        }
        BOOLEAN match = TRUE;
        for (SIZE_T j = 0; j < needleLen; j++) {
            WCHAR c1 = RtlUpcaseUnicodeChar(Haystack[i + j]);
            WCHAR c2 = RtlUpcaseUnicodeChar(Needle[j]);
            if (c1 != c2) {
                match = FALSE;
                break;
            }
        }
        if (match) {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * §25.2 / §25.3: Inspect a DriverObject's PDOs / FDOs for Blacklisted MCU VID/PIDs
 * and Composite CDC (COM Port) + HID Mouse Profiles
 */
static ULONG
VgkInspectUsbDriverDevices(
    _In_ PCWSTR DriverObjectPath,
    _Out_opt_ PULONG OutTotalDeviceCount
)
{
    if (OutTotalDeviceCount != NULL) {
        *OutTotalDeviceCount = 0;
    }

    UNICODE_STRING drvName;
    RtlInitUnicodeString(&drvName, DriverObjectPath);

    PDRIVER_OBJECT drvObj = NULL;
    NTSTATUS status = ObReferenceObjectByName(
        &drvName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        0,
        *IoDriverObjectType,
        KernelMode,
        NULL,
        (PVOID*)&drvObj
    );
    if (!NT_SUCCESS(status) || !drvObj) {
        return 0;
    }

    PDEVICE_OBJECT devList[64] = { 0 };
    ULONG devCount = 0;
    status = IoEnumerateDeviceObjectList(drvObj, devList, sizeof(devList), &devCount);
    if (!NT_SUCCESS(status) && status != STATUS_BUFFER_TOO_SMALL) {
        ObDereferenceObject(drvObj);
        return 0;
    }

    ULONG actualCount = (devCount > 64) ? 64 : devCount;
    if (OutTotalDeviceCount != NULL) {
        *OutTotalDeviceCount = actualCount;
    }

    ULONG flaggedMcus = 0;

    for (ULONG i = 0; i < actualCount; i++) {
        PDEVICE_OBJECT devObj = devList[i];
        if (!devObj) {
            continue;
        }

        /* Query DevicePropertyHardwareID on physical/lower PDO if possible */
        WCHAR hwIdBuf[512] = { 0 };
        ULONG resultLen    = 0;
        NTSTATUS propStatus = IoGetDeviceProperty(
            devObj,
            DevicePropertyHardwareID,
            sizeof(hwIdBuf) - sizeof(WCHAR),
            hwIdBuf,
            &resultLen
        );

        if (NT_SUCCESS(propStatus) && resultLen >= sizeof(WCHAR)) {
            ULONG charsCount = resultLen / sizeof(WCHAR);
            for (ULONG k = 0; k < RTL_NUMBER_OF(g_VgkBlacklistedUsbHardwareIds); k++) {
                if (VgkWideStringContainsInsensitive(
                        hwIdBuf,
                        charsCount,
                        g_VgkBlacklistedUsbHardwareIds[k])) {
                    flaggedMcus++;
                    InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_USB_MCU_DETECTED);
                    VgkLogTelemetryEvent(VGK_EVENT_DCL, 0x2500 + k, NULL);
                    break;
                }
            }
        }

        ObDereferenceObject(devObj);
    }

    ObDereferenceObject(drvObj);
    return flaggedMcus;
}

/*
 * §25.2: Scan buffer against the 50-byte Stock Arduino ATmega32U4 HID Report Descriptor
 */
BOOLEAN
VgkMatchStockArduinoHidDescriptor(
    _In_reads_bytes_(DescriptorLength) const UCHAR *DescriptorBytes,
    _In_ ULONG DescriptorLength
)
{
    if (!DescriptorBytes || DescriptorLength < sizeof(g_VgkStockArduinoReportDescriptor)) {
        return FALSE;
    }

    for (ULONG offset = 0;
         offset + sizeof(g_VgkStockArduinoReportDescriptor) <= DescriptorLength;
         offset++) {
        if (RtlCompareMemory(
                &DescriptorBytes[offset],
                g_VgkStockArduinoReportDescriptor,
                sizeof(g_VgkStockArduinoReportDescriptor)) ==
            sizeof(g_VgkStockArduinoReportDescriptor)) {
            InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_USB_MCU_DETECTED);
            VgkLogTelemetryEvent(VGK_EVENT_EDCL, 0x2502, NULL);
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * §25.3: Audit USB Bus Topology (\Driver\usbhub, \Driver\usbccgp, \Driver\HidUsb)
 * and Check \Driver\MouClass / \Driver\MouHid for Dual-Mouse Injection Conflict
 */
VOID
VgkAuditUsbHidAndMouseTopology(VOID)
{
    ULONG flaggedTotal = 0;
    ULONG mouHidCount  = 0;
    ULONG mouClassCount = 0;

    /* 1. Walk USB Composite Parent Driver (\Driver\usbccgp) & Hub Drivers */
    flaggedTotal += VgkInspectUsbDriverDevices(L"\\Driver\\usbccgp", NULL);
    flaggedTotal += VgkInspectUsbDriverDevices(L"\\Driver\\usbhub",  NULL);
    flaggedTotal += VgkInspectUsbDriverDevices(L"\\Driver\\USBHUB3", NULL);
    flaggedTotal += VgkInspectUsbDriverDevices(L"\\Driver\\HidUsb",  NULL);

    /* 2. Walk \Driver\MouHid and \Driver\MouClass for Dual-Mouse Conflict (§25.3) */
    flaggedTotal += VgkInspectUsbDriverDevices(L"\\Driver\\MouHid",   &mouHidCount);
    flaggedTotal += VgkInspectUsbDriverDevices(L"\\Driver\\MouClass", &mouClassCount);

    g_Vgk.HardwareProfile.FlaggedUsbMcuCount     = flaggedTotal;
    g_Vgk.HardwareProfile.ActiveMouseDeviceCount = (mouHidCount != 0) ? mouHidCount : mouClassCount;

    /* If multiple physical mouse HID stacks exist alongside a flagged MCU/composite node */
    if (g_Vgk.HardwareProfile.ActiveMouseDeviceCount > 1 && flaggedTotal > 0) {
        InterlockedOr(&g_Vgk.ViolationFlags, VGK_FLAG_USB_DUAL_MOUSE);
        VgkLogTelemetryEvent(
            VGK_EVENT_EDCL,
            g_Vgk.HardwareProfile.ActiveMouseDeviceCount,
            NULL
        );
    }
}
