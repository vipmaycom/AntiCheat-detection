@echo off
REM build.bat - Build script for reconstructed vgk.sys v1.17.12.4 multi-module WDM driver
REM Requires Visual Studio + Windows Driver Kit (WDK 10/11) x64 Native Tools Command Prompt

rc.exe /nologo vgk.rc

cl.exe /nologo /c /Zi /W4 /WX /Gz /Gs- /kernel /D_WIN64 /D_AMD64_ ^
    driver.c ^
    crypto_cng.c ^
    byovd_scanner.c ^
    ipi_apc_stackwalk.c ^
    memory_mdl.c ^
    callbacks.c ^
    dpc_telemetry.c ^
    hv_antidebug.c ^
    pci_dma_acpi.c ^
    hwid_tpm_smbios.c ^
    usb_hid_mouse.c

link.exe /nologo /DRIVER:WDM /INTEGRITYCHECK /SUBSYSTEM:NATIVE /ENTRY:DriverEntry ^
    /BASE:0x140000000 /DEF:vgk.def /PDBALTPATH:"https://imgur.com/a/PiWvsB0" ^
    driver.obj ^
    crypto_cng.obj ^
    byovd_scanner.obj ^
    ipi_apc_stackwalk.obj ^
    memory_mdl.obj ^
    callbacks.obj ^
    dpc_telemetry.obj ^
    hv_antidebug.obj ^
    pci_dma_acpi.obj ^
    hwid_tpm_smbios.obj ^
    usb_hid_mouse.obj ^
    vgk.res ^
    ntoskrnl.lib hal.lib cng.lib /OUT:vgk.sys
