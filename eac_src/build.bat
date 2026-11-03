@echo off
REM build.bat - Build script for reconstructed EasyAntiCheat_EOS.sys v3.2.0 multi-module WDM/Minifilter driver
REM Requires Visual Studio + Windows Driver Kit (WDK 10/11) x64 Native Tools Command Prompt

rc.exe /nologo eac.rc

cl.exe /nologo /c /Zi /W4 /Gz /Gs- /kernel /D_WIN64 /D_AMD64_ /DAMD64 /DPOOL_NX_OPTIN=1 ^
    driver.c ^
    minifilter.c ^
    cr3_pagewalk_stack.c ^
    memory_vad_scanner.c ^
    callbacks.c ^
    byovd_softpub.c ^
    dpc_telemetry.c ^
    hv_antidebug.c ^
    pci_dma_acpi.c ^
    hwid_tpm_smbios.c ^
    usb_hid_mouse.c

link.exe /nologo /DRIVER:WDM /INTEGRITYCHECK /SUBSYSTEM:NATIVE /ENTRY:DriverEntry ^
    /BASE:0x140000000 /DEF:eac.def /PDBALTPATH:"Core.pdb" ^
    driver.obj ^
    minifilter.obj ^
    cr3_pagewalk_stack.obj ^
    memory_vad_scanner.obj ^
    callbacks.obj ^
    byovd_softpub.obj ^
    dpc_telemetry.obj ^
    hv_antidebug.obj ^
    pci_dma_acpi.obj ^
    hwid_tpm_smbios.obj ^
    usb_hid_mouse.obj ^
    eac.res ^
    ntoskrnl.lib hal.lib fltMgr.lib cng.lib BufferOverflowFastFailK.lib /OUT:EasyAntiCheat_EOS.sys
