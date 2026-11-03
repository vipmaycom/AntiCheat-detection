# EasyAntiCheat_EOS.sys (v3.2.0 — Fortnite) Reconstructed Kernel Source

Multi-module Windows Kernel-Mode Driver (`WDM` + `FLTMGR.SYS` Minifilter) reconstructed via REA static analysis and algebraic 64-bit Mixed Boolean Arithmetic (MBA) deobfuscation of `C:\EasyAntiCheat_EOS.sys` (`SHA-256: e43e586a85b0101512b29508897075d3a12b58567b95d669f9d9016e8ab8ecbb`, `Core.pdb`).

## Module Map

| File | Reconstructed Subsystems & Deobfuscated RVA Anchors |
| :--- | :--- |
| `eac.h` | Master structures, SplitMix64/Murmur MBA constants (`0xC6BC279692B5C323`), hardware PTE masks, pool tags (`ClfC`, `Obtb`, `PcSi`, `ScCB`, `WfpC`, `WfpS`), IOCTLs (`0x800`..`0x806`), and violation flags. |
| `driver.c` | `DriverEntry` (`RVA 0x274DF8`), `EacDeriveMbaKey` & `.rdata` encrypted API table (`0x140307E90`..`0x14031B000`), Section 7 VM dispatcher (`RVA 0x383000`), and LIFO unload teardown. |
| `minifilter.c` | Static import anchor `FLTMGR.SYS!FltRegisterFilter` (`0x140311C18`), altitude `328010`, intercepting `IRP_MJ_CREATE`, `IRP_MJ_WRITE`, `IRP_MJ_SET_INFORMATION`, and `IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION`. |
| `cr3_pagewalk_stack.c` | Exact deobfuscated reconstructions of:<br>- `FUN_14004a85a` & `FUN_14004a922` (`0x14004a85a`..`0x14004a990`): 8-byte physical reader via `MmCopyMemory(MM_COPY_MEMORY_PHYSICAL)`<br>- `FUN_1400d9e46` (`0x1400d9e46`..`0x1400da430`, key `r14 = 0xA1BBB7A6F3A55BF7`): 4-level CR3 kernel page table walker (`PML4 -> PDPT -> PD -> PT`) with 2MB `LargePage` & 4KB `PTE` hierarchical `NX`/`RW` aggregation<br>- `FUN_1400d97f3` (`0x1400d97f3`..`0x1400d9e20`, key `rsi = 0x7C25BF06AA521F89`): custom `0x4D0`-byte `CONTEXT` (`0x0010000B`) + `RtlLookupFunctionEntry`/`RtlVirtualUnwind` + leaf/unbacked `[RSP]` pop unwinder<br>- `FUN_1401e828c` (`0x1401e828c`..`0x1401e85bf`): `CR2`/`CR3`/`CR4`/`CR8` snapshot + `SystemBigPoolInformation` executable pool scanner. |
| `memory_vad_scanner.c` | Kernel CNG SHA-256 (`BCrypt`), code cave detection (`.data`/`.rdata` `NX=0` check via `FUN_1400d9e46`), and protected process (`FortniteClient-Win64-Shipping.exe`) private executable / MDL memory scanner. |
| `callbacks.c` | `ObRegisterCallbacks` (handle stripping), `PsSetCreateProcessNotifyRoutineEx`, `PsSetCreateThreadNotifyRoutine`, `PsSetLoadImageNotifyRoutine`, and `CmRegisterCallbackEx` (DSE / `Testsigning` / certificate store protection). |
| `byovd_softpub.c` | Root certificate store validator (`ROOT`, `AuthRoot`, `FlightRoot`), `\Device\DBK64` & BYOVD scanner, and `DriverObject->MajorFunction` dispatch table auditor. |
| `dpc_telemetry.c` | 1000ms `KeSetTimerEx` + `KeSetTargetProcessorDpc` multi-core rotation, `IoQueueWorkItem` deferral, `.rdata` multiarray chunk streaming serializer (`newl+`, `newl?`, `newl!`), and `IOCTL_EAC_HEARTBEAT` (`0x804`). |
| `hv_antidebug.c` | `CPUID` (`0x0`, `0x1`, `0x40000000`, `0x80000002`..`0x80000004`), `IA32_LSTAR` MSR check, `RDTSC` vs `CPUID`/`INVLPG` VM-exit latency, `KdDebuggerEnabled`, `DR7`, and `SystemIsolatedUserModeInformation` (VBS/HVCI). |
| `pci_dma_acpi.c` | 256-bus PCI configuration space enumeration (`0x10EE` Xilinx PCILeech, `0x15AD` VMware, `0x1AF4` VirtIO, `0x1B36` QEMU) and ACPI `DMAR`/`MADT`/`FADT` verification. |
| `hwid_tpm_smbios.c` | `MBR:%08x` raw sector 0 hash, `:lun=%llx` SCSI LUN, `Part:%u CD:%u`, SMBIOS Types 0/1/2/3/4/17/127 physical parser, TPM 2.0 EK/PCR SHA-256, NDIS MAC, and Registry `MachineGuid` / `HwProfileGuid`. |
| `usb_hid_mouse.c` | `\Device\PointerClassN` dual-mouse arbitration & filter stack check + USB MCU VID/PID (`VID_2341`, `VID_1B4F`, `VID_1A86`, `VID_2E8A`) enumeration. |

## Build

Open Visual Studio 2022 (`eac.sln`) or run `msbuild eac.sln /p:Configuration=Release /p:Platform=x64` (or `build.bat` inside an x64 WDK Native Tools Command Prompt).
