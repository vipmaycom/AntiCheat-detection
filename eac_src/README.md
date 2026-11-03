# EasyAntiCheat Kernel Driver (`EasyAntiCheat_EOS.sys`) — Reconstructed Source

This repository contains the complete C/WDK reconstruction of the **`EasyAntiCheat_EOS.sys`** (v3.2.0 — Fortnite / Epic Online Services) multi-module Windows kernel-mode driver, produced via Reverse Engineering Architecture (REA) static analysis, symbol correlation against `Core.pdb`, and 64-bit Mixed Boolean Arithmetic (MBA) deobfuscation of the official protected binary (`SHA-256: e43e586a85b0101512b29508897075d3a12b58567b95d669f9d9016e8ab8ecbb`).

---

## 1. Architectural Overview

`EasyAntiCheat_EOS.sys` operates in **Ring 0** (Kernel Mode) combining standard Windows Driver Model (`WDM`) dispatching with a high-altitude Filter Manager (`FLTMGR.SYS`) Minifilter (Altitude `328010`).

Key operational subsystems include:
- **Direct CR3 Hardware Page Table Walking**: Traverses the 4-level paging hierarchy (`PML4 -> PDPT -> PD -> PT`) via physical memory reads (`MmCopyMemory`) to inspect page-level permissions, NX bits, and large-page transitions directly from hardware structures, bypassing OS APIs.
- **File System Minifilter**: Intercepts low-level I/O requests (`IRP_MJ_CREATE`, `IRP_MJ_WRITE`, `IRP_MJ_SET_INFORMATION`, and section synchronization) to block binary tampering, memory dumping, and unauthorized section creation against game processes.
- **Process Memory & VAD Auditing**: Scans user-mode Virtual Address Descriptors (VAD) for unbacked executable memory (`MEM_PRIVATE` + `PAGE_EXECUTE`), orphaned `MZ`/`PE` headers, and `.data`/`.rdata` code caves.
- **Active Kernel Integrity & Stack Unwinding**: Samples thread execution states using custom 64-bit CONTEXT structures and custom unwinders to identify kernel execution originating from unbacked pool memory.
- **Hardware Bus & Input Defense**: Audits PCI configuration spaces against PCIe DMA hardware (PCILeech), validates ACPI IOMMU tables, and inspects USB/HID mouse packet stacks against synthetic input injection.

---

## 2. File and Directory Structure

```
eac_src/
├── eac.sln                     # Visual Studio 2022 Solution (Kernel Driver)
├── eac.vcxproj                 # MSVC/WDK Configuration (Release|x64, KMDF/WDM)
├── eac.vcxproj.filters         # Visual Studio Filter Structure
├── eac.def                     # PE Kernel Module Export Definitions
├── eac.rc                      # Version Metadata (3.2.0, Epic Games / Kamu)
├── eac.h                       # Master Kernel Architecture, Deobfuscated Constants, Masks & IOCTLs
├── driver.c                    # DriverEntry, Unload, IRP Dispatchers, MBA Resolver, and Section 7 VM
├── minifilter.c                # FLTMGR.SYS File System Minifilter (Altitude 328010)
├── cr3_pagewalk_stack.c        # Hardware CR3 Page-Table Walker, Physical Memory Reader, and Stack Unwinder
├── memory_vad_scanner.c        # Process Memory Auditing, VAD Scanner, Code Caves, and SHA-256
├── callbacks.c                 # ObRegisterCallbacks, Process/Thread/Image Notify & Registry Filter
├── byovd_softpub.c             # BYOVD Detection, Root CA Validator, and MajorFunction Dispatch Auditor
├── dpc_telemetry.c             # Periodic Multi-Core DPC Timers, Worker Queues, and Chunk Serializer
├── hv_antidebug.c              # Hypervisor Detection, CPUID Latency (RDTSC), MSRs, and Anti-Debug
├── hwid_tpm_smbios.c           # Hardware ID Engine (SMBIOS, TPM 2.0, MBR Hash, SCSI LUN, MAC)
├── pci_dma_acpi.c              # PCI Bus Auditing (PCILeech Detection) and ACPI DMAR/IOMMU Verification
├── usb_hid_mouse.c             # USB/HID Pointer Filter Stack & MCU Hardware Emulator Detection
├── build.bat                   # Automated MSBuild Script (WDK Native Tools)
└── README.md                   # Reconstructed Source Documentation
```

---

## 3. Component Details

### 3.1 [`driver.c`](file:///C:/Users/pc/Desktop/eac_src/driver.c)
- **`DriverEntry` (`RVA 0x274DF8`)**: Kernel entry point. Initializes synchronization locks, derives the runtime MBA deobfuscation key (`0xC6BC279692B5C323`), creates `\Device\EasyAntiCheat_EOS` and `\DosDevices\EasyAntiCheat_EOS`, and sets up the dispatch tables.
- **`EacDeriveMbaKey`**: Decrypts the dynamic kernel API function pointer table stored in `.rdata` (`0x140307E90`..`0x14031B000`) at runtime using algebraic SplitMix64 transformations.
- **`EacDispatchDeviceControl`**: Central IOCTL handler for user-mode communication (`EasyAntiCheat_EOS.exe`), servicing operations including `IOCTL_EAC_BIND_SERVICE` (`0x800`), `IOCTL_EAC_PROTECT_PROCESS` (`0x801`), `IOCTL_EAC_POLL_TELEMETRY` (`0x802`), and `IOCTL_EAC_HEARTBEAT` (`0x804`).
- **Section 7 VM Dispatcher (`RVA 0x383000`)**: Emulates execution of internal bytecode routines for sensitive driver initialization and challenge/response processing.
- **`DriverUnload`**: Manages strict LIFO deregistration of minifilters, object callbacks, timer routines, and memory allocations.

### 3.2 [`minifilter.c`](file:///C:/Users/pc/Desktop/eac_src/minifilter.c)
- **`FltRegisterFilter`**: Registers the file system filter at altitude `328010` (`FSFilter Anti-Virus` tier) via `FLTMGR.SYS!FltRegisterFilter` (`0x140311C18`).
- **`EacMinifilterPreCreate`**: Intercepts file creation/open requests targeting protected game executables (`FortniteClient-Win64-Shipping.exe`) and anti-cheat modules to deny unauthorized read, write, or delete handles.
- **`EacMinifilterPreWrite` & `PreSetInformation`**: Blocks external processes from modifying binary disk images or altering file size/timestamp metadata while the game is active.
- **`IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION`**: Audits memory-mapped file section creation (`SEC_IMAGE`) to detect reflective execution, manual DLL injection, or unauthorized code replacement before execution begins.

### 3.3 [`cr3_pagewalk_stack.c`](file:///C:/Users/pc/Desktop/eac_src/cr3_pagewalk_stack.c)
- **Physical Memory Reader (`FUN_14004a85a` / `FUN_14004a922`)**: Safely reads 8-byte physical memory values using `MmCopyMemory(MM_COPY_MEMORY_PHYSICAL)` with address verification.
- **4-Level CR3 Page Table Walker (`FUN_1400d9e46`)**:
  - Traverses `PML4 -> PDPT -> PD -> PT` using decrypted MBA constant `r14 = 0xA1BBB7A6F3A55BF7`.
  - Aggregates page flags (`Present`, `ReadWrite`, `UserSupervisor`, `NX`) across all four hierarchy levels.
  - Automatically identifies 2MB Large Pages (`LargePage` bit 7) and 4KB standard leaf PTEs.
  - Skips user-space virtual addresses (`TargetVa < MmUserProbeAddress`) to concentrate audits exclusively on kernel memory integrity.
- **Kernel Stack Unwinder (`FUN_1400d97f3`)**:
  - Reconstructs a custom `0x4D0`-byte `CONTEXT` structure (`0x0010000B`).
  - Unwinds active kernel call stacks using `RtlLookupFunctionEntry` and `RtlVirtualUnwind`.
  - Flags unbacked return addresses (`[RSP]`) lacking registered `.pdata` function runtime tables.
- **Control Register & Pool Scanner (`FUN_1401e828c`)**:
  - Captures `CR2`, `CR3`, `CR4`, and `CR8` snapshots to detect hypervisor shadow paging anomalies.
  - Sweeps `SystemBigPoolInformation` for executable non-paged pool memory allocations not backed by any registered kernel module.

### 3.4 [`memory_vad_scanner.c`](file:///C:/Users/pc/Desktop/eac_src/memory_vad_scanner.c)
- **`EacAuditProtectedProcessMemory`**: Sweeps the virtual address space of the protected game process (`0x10000` to `0x7FFFFFFE0000`) via `ZwQueryVirtualMemory`.
- **Unbacked Memory & MZ Header Auditing**: Detects `MEM_PRIVATE` or `MEM_COMMIT` regions possessing executable permissions (`PAGE_EXECUTE_READ`, `PAGE_EXECUTE_READWRITE`) and inspects them for unbacked `MZ`/`PE` headers (`EAC_VIO_MDL_USER_NO_VAD`).
- **Code Cave & Permission Tampering**: Cross-references `.data` and `.rdata` sections of game modules using CR3 page walking to detect executable patches (`NX=0`) applied to normally non-executable memory.
- **Kernel Cryptographic Hashing**: Leverages CNG (`BCryptCreateHash`, `BCryptHashData`) to compute SHA-256 hashes of critical code sections against known legitimate baselines.

### 3.5 [`callbacks.c`](file:///C:/Users/pc/Desktop/eac_src/callbacks.c)
- **`ObRegisterCallbacks` (Altitude `328010`)**: Strips handle permissions from unauthorized processes attempting to open handles to the game process or its threads:
  - Process access stripped: `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`, `PROCESS_SUSPEND_RESUME`, `PROCESS_DUP_HANDLE`.
  - Thread access stripped: `THREAD_GET_CONTEXT`, `THREAD_SET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_DIRECT_IMPERSONATION`.
- **`PsSetCreateProcessNotifyRoutineEx`**: Tracks all process creation and termination events across the OS, validating command lines and parent/child lineages.
- **`PsSetCreateThreadNotifyRoutine`**: Detects remote thread creation (`CreateRemoteThread`, `NtCreateThreadEx`) targeting the game process.
- **`PsSetLoadImageNotifyRoutine`**: Audits all loaded kernel drivers and DLL modules, validating base addresses and certificate validity in real time.
- **`CmRegisterCallbackEx`**: Registry filter callback guarding Driver Signature Enforcement (DSE), `Testsigning` flags, and system certificate stores (`HKLM\SYSTEM\CurrentControlSet\Control\CI`).

### 3.6 [`byovd_softpub.c`](file:///C:/Users/pc/Desktop/eac_src/byovd_softpub.c)
- **BYOVD (*Bring Your Own Vulnerable Driver*) Mitigation**: Detects vulnerable third-party drivers by cross-referencing known device names (`\Device\DBK64`, `iqvw64e.sys`, `gdrv.sys`, `mhyprot2.sys`) and certificate hashes.
- **Root Certificate Store Validation**: Audits system Root CAs (`ROOT`, `AuthRoot`, `FlightRoot`) to detect unauthorized or self-signed test certificates injected into the kernel.
- **`MajorFunction` Dispatch Table Auditing**: Regularly verifies that `DriverObject->MajorFunction` pointers of critical Windows drivers point strictly within their respective legitimate image bounds, exposing hook trampolines and rootkit redirections.

### 3.7 [`dpc_telemetry.c`](file:///C:/Users/pc/Desktop/eac_src/dpc_telemetry.c)
- **Periodic DPC Timers**: High-priority `1000ms` periodic timer (`KeSetTimerEx`) rotating execution across all CPU cores (`KeSetTargetProcessorDpc`) to avoid predictability.
- **Asynchronous Worker Queues**: Defers non-critical processing from `DISPATCH_LEVEL` to `PASSIVE_LEVEL` using `IoAllocateWorkItem` / `IoQueueWorkItem`.
- **Chunk Streaming Protocol**: Packs and encrypts detected security violations into a serialized multiarray streaming format using delimiters (`newl+`, `newl?`, `newl!`) for dispatch to the user-mode service.
- **`IOCTL_EAC_HEARTBEAT` (`0x804`)**: Bidirectional cryptographic handshake verifying responsiveness of the user-mode anti-cheat service; timeouts trigger game shutdown.

### 3.8 [`hv_antidebug.c`](file:///C:/Users/pc/Desktop/eac_src/hv_antidebug.c)
- **Hypervisor Presence & CPUID Sweeps**: Inspects `CPUID` leaves (`0x0`, `0x1`, `0x40000000`, `0x80000002`..`0x80000004`), checking ECX bit 31 (hypervisor present) and identifying virtualization signatures (`VMwareVMware`, `Microsoft Hv`, `KVMKVMKVM`).
- **RDTSC / VM-Exit Latency Timing**: Measures execution cycles of `CPUID` and `INVLPG` against a baseline to detect VM-Exit interception overhead introduced by hypervisor-based tools.
- **System Call Hook Detection**: Validates `IA32_LSTAR` MSR to ensure it points directly to `nt!KiSystemCall64` and has not been hooked.
- **Kernel Debugging Probes**: Monitors `KdDebuggerEnabled`, `KdDebuggerNotPresent`, and hardware debug registers (`DR0`-`DR7`).
- **VBS / HVCI Auditing**: Queries `SystemIsolatedUserModeInformation` to verify Virtualization-Based Security status.

### 3.9 [`hwid_tpm_smbios.c`](file:///C:/Users/pc/Desktop/eac_src/hwid_tpm_smbios.c)
- **Physical Disk Identification**: Reads raw Master Boot Record (MBR) sector 0 hash (`MBR:%08x`), SCSI LUN identifiers (`:lun=%llx`), and volume serial numbers directly from disk controllers.
- **SMBIOS Table Parser**: Maps and parses raw physical SMBIOS memory (`0xF0000` range), extracting immutable hardware identifiers (Type 0 BIOS, Type 1 System UUID, Type 2 Baseboard, Type 3 Chassis, Type 4 Processor, Type 17 Memory).
- **TPM 2.0 Cryptographic Telemetry**: Communicates with the hardware TPM via ACPI/TCG interfaces to query Endorsement Keys (EK) and PCR bank digests.
- **Network & Registry Telemetry**: Retrieves permanent hardware MAC addresses via NDIS OIDs (`OID_802_3_PERMANENT_ADDRESS`) and reads registry install identifiers (`MachineGuid`, `HwProfileGuid`).

### 3.10 [`pci_dma_acpi.c`](file:///C:/Users/pc/Desktop/eac_src/pci_dma_acpi.c)
- **PCI Configuration Space Enumeration**: Enumerates 256 PCI buses via `HalGetBusDataByOffset` / PCI Enhanced Configuration Mechanism.
- **Direct Memory Access (DMA) Defense**: Detects unauthorized bus-mastering hardware, checking vendor/device IDs associated with PCIe leeches and FPGA boards (Xilinx `0x10EE`, VMware `0x15AD`, VirtIO `0x1AF4`, QEMU `0x1B36`).
- **ACPI DMAR & IOMMU Verification**: Parses ACPI tables (`DMAR`, `MADT`, `FADT`) to verify whether hardware IOMMU DMA remapping (Intel VT-d / AMD-Vi) is active in the firmware.

### 3.11 [`usb_hid_mouse.c`](file:///C:/Users/pc/Desktop/eac_src/usb_hid_mouse.c)
- **Pointer Class Driver Stack Auditing**: Traverses device objects attached to `\Device\PointerClass0`..`\Device\PointerClassN` to detect unauthorized filter drivers inserted into the input stack.
- **Dual-Mouse & Synthetic Arbitration**: Detects concurrent active mouse devices injecting programmatic input into the operating system.
- **USB Microcontroller Enumeration**: Scans USB device descriptors for VID/PID signatures commonly used by microcontroller-based hardware emulators (Arduino Leonardo `VID_2341`, Teensy `VID_1B4F`, CH340 `VID_1A86`, Raspberry Pi Pico `VID_2E8A`).

---

## 4. Prerequisites & Build Instructions

### Prerequisites
- **Visual Studio 2022** with the **Desktop development with C++** workload.
- **Windows Driver Kit (WDK)** compatible with Windows 10/11 and Visual Studio 2022.
- Administrator privileges for building and managing kernel driver solutions.

### Build via Command Line
Run the automated build script inside an **x64 Native Tools Command Prompt for VS 2022**:
```cmd
cd /d C:\Users\pc\Desktop\eac_src
build.bat
```

### Build via MSBuild
```cmd
msbuild eac.sln /p:Configuration=Release /p:Platform=x64
```

### Build Output
- Binary: `eac_src\build\x64\Release\EasyAntiCheat_EOS.sys`
- Symbol: `eac_src\build\x64\Release\EasyAntiCheat_EOS.pdb`
