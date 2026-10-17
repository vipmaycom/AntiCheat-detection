# Riot Vanguard Kernel Driver (`vgk.sys`) — Reconstructed Source

This repository contains the complete C/WDK reconstruction of the **`vgk.sys`** kernel driver (Riot Vanguard v1.19.1-18), produced via Reverse Engineering Architecture (REA) analysis on the official binary located at `C:\Program Files\Riot Vanguard\vgk.sys`.

---

## 1. Architectural Overview

`vgk.sys` operates in **Ring 0** (Kernel Mode) as an early boot-start driver (Service Type `SERVICE_BOOT_START`), ensuring that the anti-cheat kernel subsystem initializes before any third-party drivers or user-mode processes are loaded.

It implements active memory protection, continuous kernel integrity verification, physical bus auditing against external DMA attack hardware, and strict handle-stripping over protected game processes.

---

## 2. File and Directory Structure

```
vgk_src/
├── vgk.sln                     # Visual Studio 2022 Solution (Kernel Driver)
├── vgk.vcxproj                 # MSVC/WDK Configuration (Release|x64, KMDF/WDM)
├── vgk.vcxproj.filters         # Visual Studio Filter Structure
├── vgk.def                     # PE Kernel Module Export Definitions
├── vgk.rc                      # Version Metadata (1.19.1.18, Riot Games, Inc.)
├── vgk.h                       # Master Kernel Architecture, IOCTLs, and Structures
├── driver.c                    # DriverEntry, Unload, IRP Dispatchers, and IOCTL Router
├── byovd_scanner.c             # BYOVD Scanner, Triple Redundancy, and 27th Root CA
├── callbacks.c                 # ObRegisterCallbacks, Process/Thread/Image Notify Routines
├── crypto_cng.c                # Kernel CNG Cryptography (BCrypt SHA-256, HMAC, ECDSA)
├── dpc_telemetry.c             # Periodic DPC Timers and Asynchronous Worker Queues
├── hv_antidebug.c              # Hypervisor Detection (VMX/SVM), CPUID, and Anti-Debug
├── hwid_tpm_smbios.c           # Hardware ID Engine (SMBIOS, TPM 2.0 TCG, NVMe, MAC)
├── ipi_apc_stackwalk.c         # Non-Cooperative Stack-Walking via IPI and Kernel APCs
├── memory_mdl.c                # Physical/Virtual Memory Auditing, MDLs, and PTEs
├── pci_dma_acpi.c              # PCI Bus Auditing, ACPI Tables, and IOMMU/DMA Defense
├── usb_hid_mouse.c             # USB/HID Input Packet Filter Against Synthetic Hardware
├── vgk_parity_matrix.md        # Binary Parity Matrix (REA Offsets vs Source Code)
└── build.bat                   # Automated MSBuild Script
```

---

## 3. Component Details

### 3.1 [`driver.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/driver.c)
- **`DriverEntry`**: Driver entry point. Initializes synchronization primitives, creates the device object `\Device\vgk`, and establishes the symbolic link `\DosDevices\vgk`.
- **`VgkDispatchCreateClose`**: Controls handle acquisition and closure for the device, validating caller privileges and identity.
- **`VgkDispatchDeviceControl`**: Central IOCTL request router dispatched from the user-mode service `vgc.exe` (e.g., initial cryptographic handshake, status queries, session nonces).
- **`DriverUnload`**: Performs orderly, safe LIFO teardown of all registered callbacks, timers, and allocated resources.

### 3.2 [`byovd_scanner.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/byovd_scanner.c)
- **BYOVD (*Bring Your Own Vulnerable Driver*) Mitigation**: Scans loaded kernel drivers, cross-referencing certificate signatures and cryptographic digests against known vulnerable driver catalogs.
- **Triple Redundancy in `.rdata`**: Maintains 3 mirrored copies of the driver signature table (at offsets `0x15A5C0`, `0x15AF40`, and `0x15BEC0`). If an external driver attempts DKOM to zero out the primary table in memory, the mirror copies immediately flag the tampering.
- **Root CA Validation**: Verifies authentic digital signatures, including support for the 27th embedded Root CA ("Gamerdoc Inc" at `0x16BF50`).

### 3.3 [`callbacks.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/callbacks.c)
- **`ObRegisterCallbacks`**: Process and thread handle filter. Strips dangerous access masks (`PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`, `THREAD_SUSPEND_RESUME`) from unauthorized callers attempting to interact with the game.
- **`PsSetCreateProcessNotifyRoutineEx`**: Tracks the creation and termination of all operating system processes.
- **`PsSetCreateThreadNotifyRoutine`**: Audits newly spawned threads across all processes in real time.
- **`PsSetLoadImageNotifyRoutine`**: Intercepts the loading of DLLs and kernel-mode drivers across the system.

### 3.4 [`crypto_cng.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/crypto_cng.c)
- Provides kernel-mode cryptographic primitives using the Windows **CNG (Cryptography Next Generation)** API (`KsecDD` / `BCrypt`).
- Utilized for signing telemetry records, verifying module SHA-256 hashes, and authenticating ECDSA signatures.

### 3.5 [`dpc_telemetry.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/dpc_telemetry.c)
- **Periodic DPC Timers**: Schedules high-priority kernel timer routines at `DISPATCH_LEVEL` for integrity verification at randomized, unpredictable intervals.
- **Worker Queues**: Enqueues kernel-gathered telemetry records for asynchronous serialization and transmission to the user-mode service.

### 3.6 [`hv_antidebug.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/hv_antidebug.c)
- **Hypervisor Detection**: Executes `CPUID` instruction sweeps and checks the hypervisor presence bit and synthetic interfaces.
- **Control Register Integrity**: Inspects `CR0`, `CR4`, and critical MSRs to identify unauthorized VM-Exit interceptions.
- **Kernel Debugger Probes**: Audits kernel debugging state flags such as `KdDebuggerEnabled` and `KdDebuggerNotPresent`.

### 3.7 [`hwid_tpm_smbios.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/hwid_tpm_smbios.c)
- Collects immutable hardware identity metrics directly from physical system tables:
  - **SMBIOS** tables (motherboard serial, system UUID).
  - **TPM 2.0** measurements (PCR registers and TCG event logs).
  - Physical drive serial numbers (**NVMe / SATA** controller inquiry).
  - Network interface MAC addresses.

### 3.8 [`ipi_apc_stackwalk.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/ipi_apc_stackwalk.c)
- **Inter-Processor Interrupts (IPI)**: Issues inter-processor interrupts to freeze target CPU cores simultaneously, capturing instantaneous instruction pointers (`RIP`) across all active threads.
- **Kernel APC Stack-Walking**: Enqueues special kernel APCs to walk call stacks without relying on exported APIs, isolating unbacked code execution and shellcode injection.

### 3.9 [`memory_mdl.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/memory_mdl.c)
- **Memory Management via MDL**: Safely probes and locks physical memory pages (`IoAllocateMdl`, `MmProbeAndLockPages`).
- **PTE Self-Cloaking Analysis**: Reconstructs logic corresponding to `VgkPukeCloakPages`, manipulating page table entries (PTEs) and flushing the TLB cache (`INVLPG`) to prevent external reads or memory dumps of driver-internal structures.

### 3.10 [`pci_dma_acpi.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/pci_dma_acpi.c)
- **Direct Memory Access (DMA) Mitigation**: Scans PCI configuration space to identify unauthorized bus-mastering devices (FPGA cards, PCIe leeches).
- **ACPI and IOMMU Auditing**: Confirms active hardware virtualization and IOMMU protection (VT-d / AMD-Vi) to block unauthorized direct physical RAM access.

### 3.11 [`usb_hid_mouse.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgk_src/usb_hid_mouse.c)
- Monitors and filters input packets within the USB/HID driver stack.
- Analyzes mouse packet sources to differentiate authentic USB mouse hardware from synthetic mouse injection generated by virtual drivers or hardware emulators (e.g., Arduino Leonardo, Raspberry Pi Pico).

---

## 4. Prerequisites

- **Visual Studio 2022** with the **Desktop development with C++** workload installed.
- **Windows Driver Kit (WDK)** compatible with Windows 10/11 and Visual Studio 2022.
- Administrator privileges for building and managing kernel driver solutions.
