# Easy Anti-Cheat EOS (Fortnite) — Kernel Architecture, Detection & Telemetry Master Reference

Comprehensive mapping of Easy Anti-Cheat EOS (`EasyAntiCheat_EOS.sys` / `EasyAntiCheat_EOS.exe`) kernel instrumentation, filesystem minifilter interception, memory scanning, execution profiling, hardware identification, ban enforcement, and forensic trace footprint. Structured in direct parity with the Vanguard technical reference baseline, incorporating all empirical findings and authenticated Evidence IDs extracted via REA (Reverse Engineer Anything).

---

## 1. Detected Injection Types

| Injection Method | Detected? | How / Kernel Mechanism |
|---|---|---|
| `LoadLibrary` | **YES** | `PsSetLoadImageNotifyRoutine`, dynamic module whitelist/blacklist, Authenticode verification via in-kernel Softpub engine (`SoftpubAuthenticode`, `WintrustCertificateTrust`). |
| Manual map (MDL / physical allocation) | **YES** | Periodic DPC/work item memory walks. CR3 page table audits verify that executable pages (`NX=0`) map to physical VAD allocations backed by a valid signed image on disk. |
| Reflective DLL | **YES** | Memory scanning checks `MEM_COMMIT` regions lacking `SEC_IMAGE` backing; flags PE headers (`MZ`/`PE`), unlinked sections, and unbacked executable allocations. |
| APC injection | **YES** | `PsSetCreateThreadNotifyRoutine` and thread dispatch inspection monitor alertable thread states; stack unwinding catches APC delivery to unbacked addresses. |
| Thread hijack / `SetThreadContext` | **YES** | `ObRegisterCallbacks` strips `THREAD_SET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_DIRECT_IMPERSONATION`; periodic thread context checks validate instruction pointers. |
| Process hollowing | **YES** | `PsSetCreateProcessNotifyRoutineEx` intercepts process creation; cross-checks file section mapping against `PEB->ImageBaseAddress`. |
| Module stomping | **YES** | Compares on-disk signed binary page hashes against in-memory mapped pages; flags modified `.text` code sections and export tables. |
| DLL sideloading | **YES** | `PsSetLoadImageNotifyRoutine` hashes every loaded library against known Epic Games, Microsoft, and trusted software catalogs. |
| Shellcode in allocated memory | **YES** | Thread stack unwinding traps active instruction pointers (`RIP`) resolving outside legitimate module bounds in `PsLoadedModuleList`. |
| `NtCreateSection`-based mapping | **YES** | Minifilter callback `IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION` and handle filtering flag cross-process section mappings into `FortniteClient-Win64-Shipping.exe`. |
| Atom bombing | **PARTIAL** | Memory content scan detects unbacked payload; callback execution caught if target thread unwinds to dynamic memory. |
| ThreadlessInject / PoolParty | **YES** | Worker Factory and Threadpool completion ports audited for dispatch pointers resolving to unbacked addresses. |

---

## 2. Detected Driver Types

| Driver Type | Detected? | How |
|---|---|---|
| Unsigned driver (`NtLoadDriver`) | **YES** | Windows Driver Signature Enforcement (DSE) validation + in-kernel Authenticode engine (`SoftpubCheckCert`) checking `\Registry\Machine\SOFTWARE\Microsoft\SystemCertificates\ROOT`. |
| Test-signed driver (`TESTSIGNING`) | **YES** | Registry callback (`CmRegisterCallbackEx`) blocks `DisableIntegrityChecks` / `Testsigning`; verifies `SystemBootEnvironmentInformation`. |
| Known BYOVD (Gdrv, mhyprot2, Capcom, RTCore64, procexp, dbutil_2_3, eneio64, iqvw64e) | **YES** | Embedded encrypted certificate and SHA-256 driver blacklist. Driver names and cert hashes checked synchronously in `PsSetLoadImageNotifyRoutine`. |
| Unknown BYOVD with valid WHQL | **PARTIAL** | Monitored dynamically: physical memory mapping operations, cross-process handle creation, and dispatch table detours trigger heuristic alerts. |
| Cheat Engine kernel (`dbk64.sys`) | **YES** | Hardcoded certificate and module name blacklist; active object directory scanning (`\Device\DBK64`). |
| Manual-mapped driver (KDMapper / Capcom) | **YES** | Periodic kernel stack unwinding + page table audits flag `NX=0` executable code on kernel pool memory (`NonPagedPoolNx`). |
| Driver with dev build path PDB | **YES** | Kernel memory scanner checks driver PE headers for suspicious debug paths and linker signatures. |
| Revoked-cert driver | **YES** | Real-time CRL validation against Microsoft Code Integrity revocation lists and Windows Flight/AuthRoot catalogs (`FlightRoot`, `AuthRoot`). |

---

## 3. Detected Hypervisor Techniques

| Technique | Detected? | How |
|---|---|---|
| `CPUID.1:ECX[31]` (HV present) | **YES** | Direct instruction execution: bit 31 set flags hypervisor presence. |
| Custom HV vendor string | **YES** | `CPUID 0x40000000` query: unknown or synthetic vendor strings flagged. |
| CPUID feature filtering | **YES** | Cross-leaf consistency checks: missing architectural features paired with synthetic leaves trigger anomalies. |
| MSR hooking (`LSTAR`, `SYSENTER_EIP`, `GS_BASE`) | **YES** | Read-modify-write cycle across CLI/STI barriers: readback mismatch indicates guest virtualization trap. |
| EPT page splitting (execute vs read divergence) | **YES** | Forced `INVLPG` execution timing checks combined with physical memory cross-reads; VM-exit latency measured. |
| TSC manipulation | **YES** | `RDTSC`/`RDTSCP` multi-sample delta compared against hardware ACPI Power Management Timer (`PMT`) and APIC timer. |
| Hyper-V / VBS binary patching | **YES** | Disk vs memory hash comparison of `\SystemRoot\System32\ntoskrnl.exe`, `\SystemRoot\System32\secur32.dll`, `\SystemRoot\System32\hvix64.exe`, and `\SystemRoot\System32\hvax64.exe`. |
| Type-1 / Nested HV below Windows | **YES / PARTIAL** | Windows 11 mandates VBS/HVCI; nested hypervisors induce VM-exit latency penalties caught during calibration loops. |
| VBS / HVCI state change | **YES** | Direct verification via `ZwQuerySystemInformation(SystemIsolatedUserModeInformation)`. |
| CR4 / CR0 register masking | **YES** | Verification of reserved control register bits; attempts to shadow `CR4.VMXE` flag timing discrepancies. |
| ACPI table manipulation | **YES** | Physical table reads checking MADT, SLIC, FADT, and DMAR tables. |

---

## 4. Detected DMA / Firmware Techniques

| Technique | Detected? | How |
|---|---|---|
| PCILeech default FPGA (Xilinx VID/PID) | **YES** | PCI configuration space scan (`HalGetBusDataByOffset` + MMIO ECAM): default IDs (`10EE`) flagged immediately. |
| Custom FPGA with spoofed VID/PID | **PARTIAL** | Bridge topology walking, Subsystem ID validation, class code verification, and abnormal Base Address Register (BAR) window size detection. |
| Thunderbolt DMA adapter | **YES** | PCI bus hierarchy inspection: external hot-plug PCIe bridges audited for DMA capabilities. |
| IOMMU / VT-d / AMD-Vi bypass | **YES** | IOMMU activation enforced on Windows 11; DMA remapping blocks unmapped host memory access. |
| DMA device with unusual BAR config | **YES** | Bus/Device/Function iteration audits BAR allocation sizes against valid hardware database. |
| Modified UEFI firmware | **YES** | TPM PCR[0] measurement verification + SMBIOS cross-validation against ACPI tables. |
| Bootkit (MBR / VBR modification) | **YES** | MBR signature hashing (`MBR:%08x`), Secure Boot chain validation, disk boot sector hashing. |
| UEFI bootkit (Firmware DXE driver) | **YES** | TPM PCR[0-7] measurement verification via `tbs.dll` (`Tbsip_Submit_Command`). |
| Early-launch driver injection | **YES** | Early boot measurement comparison via TPM PCR[11]. |
| BCD test-signing enabled | **YES** | Registry BCD element check (`SystemBootEnvironmentInformation`). |
| Modified `ntoskrnl` / `securekernel` on disk | **YES** | On-disk digital signature validation vs in-memory image hashing (`SoftpubAuthenticode`). |

---

## 5. Kernel Calls EAC Monitors

Calls EAC monitors for unauthorized interaction with protected processes (`FortniteClient-Win64-Shipping.exe` / `EasyAntiCheat_EOS.sys`):

| Call | Monitoring Method |
|---|---|
| `NtOpenProcess` | `ObRegisterCallbacks` — strips `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`, `PROCESS_DUP_HANDLE`, `PROCESS_CREATE_THREAD`, `PROCESS_TERMINATE`, `PROCESS_SUSPEND_RESUME`, `PROCESS_SET_INFORMATION`. |
| `NtOpenThread` | `ObRegisterCallbacks` — strips `THREAD_SET_CONTEXT`, `THREAD_GET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_DIRECT_IMPERSONATION`, `THREAD_TERMINATE`, `THREAD_SET_INFORMATION`. |
| `NtDuplicateObject` | `ObRegisterCallbacks` — enforces handle rights stripping on duplicated descriptors. |
| `NtWriteVirtualMemory` | Blocked via handle stripping; direct kernel callers intercepted via kernel stack walking. |
| `NtAllocateVirtualMemory` | Blocked via handle stripping; kernel-mode allocations visible in VAD audits. |
| `NtProtectVirtualMemory` | Blocked via handle stripping; PTE walker flags unexpected `RWX` transitions. |
| `PsCreateSystemThread` | Thread start address verified; must reside within legitimate loaded module bounds. |
| `NtCreateThreadEx` | Process notify routines intercept new thread staging within protected instance. |
| `NtSetContextThread` | Blocked via handle stripping; kernel modifications caught during DPC thread context validation. |
| `NtLoadDriver` / `ZwLoadDriver` | Image load callback validates signatures pre-execution; blocks blacklisted certificates. |
| `MmCopyVirtualMemory` | Audited via kernel stack trace inspection; caller address must reside in authorized system module. |

---

## 6. Kernel Calls EAC Makes (Detection Tools)

| Routine | Purpose / Detection Functionality |
|---|---|
| `FltRegisterFilter` | Registers filesystem minifilter driver with Filter Manager (`FLTMGR.SYS`). |
| `FltStartFiltering` | Activates filesystem filtering across all storage volumes. |
| `FltGetFileNameInformation` | Normalizes and parses file paths accessed across disk volumes. |
| `ObRegisterCallbacks` | Registers process and thread pre/post handle creation and duplication interception. |
| `PsSetCreateProcessNotifyRoutineEx` | Intercepts process creation/termination system-wide; audits image path, parent PID, command line. |
| `PsSetCreateThreadNotifyRoutine` | Intercepts thread creation; validates `StartAddress` against module boundaries. |
| `PsSetLoadImageNotifyRoutine` | Audits every DLL and driver image loaded into user or kernel space before entry execution. |
| `CmRegisterCallbackEx` | Intercepts registry key creation and value modifications (`\Registry\Machine\...`). |
| `KeSetTimerEx` | High-frequency kernel timer scheduling for background integrity audits. |
| `KeInitializeDpc` | Initializes Deferred Procedure Call routine for asynchronous memory scanning. |
| `PsLookupProcessByProcessId` | Resolves target PID to `EPROCESS` object for entity tracking. |
| `Tbsi_Submit_Command` / `Tbsi_Context_Create` | Submits raw TPM 2.0 command bytes (via `tbs.dll` in `EasyAntiCheat_EOS.exe`) to query Endorsement Keys and PCR banks. |
| `NCryptOpenStorageProvider` / `NCryptImportKey` | Cryptography Next Generation (CNG via `ncrypt.dll`) for cryptographic token sealing. |
| `CertGetCertificateChain` | Validates Authenticode certificate chains via `CRYPT32.dll`. |
| `MmCopyVirtualMemory` | Dynamically resolved kernel routine to read and verify user-mode process pages. |

---

## 7. Kernel Callbacks Registered

| Callback | API Registration | Scope & Operational Target |
|---|---|---|
| Filesystem Filter | `FltRegisterFilter` (Altitude `328010`) | Intercepts `IRP_MJ_CREATE`, `IRP_MJ_WRITE`, `IRP_MJ_SET_INFORMATION`, `IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION` targeting game binaries and EAC certificates. |
| Handle Filter | `ObRegisterCallbacks` | Intercepts every handle creation/duplication targeting `FortniteClient-Win64-Shipping.exe` and protected game threads. |
| Process Notify | `PsSetCreateProcessNotifyRoutineEx` | Intercepts process creation/termination system-wide; audits image path, parent PID, and command line. |
| Thread Notify | `PsSetCreateThreadNotifyRoutine` | Intercepts every thread creation; validates `StartAddress` against module boundaries. |
| Image Load Notify | `PsSetLoadImageNotifyRoutine` | Audits every DLL and driver image loaded into user or kernel space before entry execution. |
| Registry Callback | `CmRegisterCallbackEx` | Monitors registry tampering targeting driver signing and certificate stores. |
| I/O Control Interceptor | Dispatch Table (`IRP_MJ_DEVICE_CONTROL`) | Restricts `\Device\EasyAntiCheat_EOS` exclusively to authenticated `EasyAntiCheat_EOS.exe` tokens. |

---

## 8. Memory Scanning Methods

| Method | Architecture & Operational Details |
|---|---|
| CR3 Page Directory Walking | Reads target process CR3 register; manually walks PML4 -> PDPT -> PD -> PT in physical RAM, bypassing software API hooks. |
| System PTE Attribute Verification | Audits kernel Page Table Entries: flags any page where `NoExecute (NX) == 0` that falls outside legitimate `.text` sections of signed modules. |
| Section 7 Obfuscated VM Core | Protected core code resides in an unnamed section (Offset `0x32f000`, 35.28 MB, Entropy 6.6633) executed via an x86_64 register dispatch loop preserving XMM and GPR registers. |
| VAD Tree Cross-Referencing | Traverses `EPROCESS->VadRoot` AVL tree; cross-checks `VadFlags` with physical PTE protections to detect hidden memory regions (MDL allocations). |
| SIMD Content Scanner (AVX2/SSE) | Scans committed process memory at 32-64 bytes/cycle searching for known cheat signatures, strings, and hook trampolines. |
| PE On-Disk vs In-Memory Comparison | Hashes `.text` sections of critical modules (`ntoskrnl.exe`, `FortniteClient-Win64-Shipping.exe`, graphics drivers) and compares directly against on-disk signed files. |
| Code Cave Detection | Cross-references PE section characteristics; flags executable execution occurring inside `.data` or `.rdata` sections of signed modules. |
| Periodic Thread Stack Unwinding | Captures thread context; unwinds frames using `.pdata`; flags return addresses in unbacked memory (`PAGE_EXECUTE_READWRITE`). |

---

## 9. Cheat Tool Detection

| Tool | Detection Method |
|---|---|
| Cheat Engine (`dbk64.sys` + process) | Process name scan, window title inspection, `\Device\DBK64` driver enumeration, certificate blacklist. |
| x64dbg | Process name scan, window class checks, debug register hook monitoring. |
| ReClass.NET | Process name scan, handle table enumeration, memory read pattern tracking. |
| Process Hacker / System Informer | Process name scan, driver blacklist (`kprocesshacker.sys`), open handle scans. |
| IDA Pro / Ghidra | Window title detection, process name enumeration. |
| Kernel Debuggers (WinDbg) | `KdDebuggerEnabled` kernel variable checks, BCD `/debug` flag validation, INT 1/3 SEH traps. |
| Frida / Dynamic Binary Instrumentation | RDTSC timing detection, unbacked runtime code generation flags. |
| Custom Debug / Mapping Tools | PDB build path scanning in PE headers, `PoolTag` audits (`ClfC`, `Obtb`, `PcSi`, `ScCB`), unbacked thread start address tracking. |

---

## 10. Anti-Debug

| Check | Kernel / Hardware Method |
|---|---|
| INT 1 SEH Trap | Driver initialization gate: debugger consumption causes driver load abort. |
| INT 3 SEH Trap | Embedded breakpoint exceptions checked periodically; active debuggers consume interrupt and fail parity tests. |
| RDTSC Multi-Sample Timing | Back-to-back `RDTSC` loops with randomized iterations: single-step and VM-exit overhead detected. |
| `KdDebuggerEnabled` Check | Direct kernel variable read; if set at any point, machine is flagged. |
| RDTSC + CPUID Quad Check | Ratio comparison across CPUID instruction boundaries: hypervisor exit latency detected. |
| APIC / PMT Calibration | Hardware timers (HPET, ACPI PMT) cross-referenced against CPU TSC to detect synthetic timing spoofing. |
| `KUSER_SHARED_DATA` Timestamps | Read directly from unhookable system memory page (`0x7FFE0000`). |

---

## 11. Communication Channel Detection

| Channel | Detected? | EAC Monitoring Mechanism |
|---|---|---|
| Named Device Object (`\Device\...`) + `CreateFile` | **YES** | Object Manager directory walk (`\Device\` & `\DosDevices\`), `IoEnumerateDeviceObjectList`, and global handle sweeps. |
| `MajorFunction` Dispatch Table Hooking (`beep.sys`, `null.sys`) | **YES** | Dispatch Table Integrity Audits: periodic sweeps of `DriverObject->MajorFunction[IRP_MJ_*]` verifying pointers reside within `.text` module boundaries. |
| Subsystem Dispatch Pointer Hooks (`win32k.sys`, `dxgkrnl.sys`) | **YES** | Function pointer hashing and boundary checking across graphics dispatch tables. |
| Kernel MDL Mapping (`MmMapLockedPagesSpecifyCache` with `UserMode`) | **YES** | Paging Hierarchy (CR3) vs VAD Tree Cross-Check: PTE has `User/Supervisor = 1` without a corresponding `VAD` node in `EPROCESS->VadRoot`. |
| Named Shared Memory (`\BaseNamedObjects\...`) | **YES** | System-wide Object Manager scans and handle table iteration. |
| Usermode-Allocated Shared Memory (Physical Translation / CR3) | **NO** | Zero kernel object footprint; memory allocated via `VirtualAlloc` in external process (valid VAD node); driver reads/writes via physical PFN mapping. |
| Data-Only Buffer / Queue in Non-Executable Kernel Memory | **NO** | Raw data read/write in non-executable buffers (`NX=1`) without modifying code sections (`.text`) or dispatch pointers leaves no hook signature. |
| Windows Named Pipes (`\Device\NamedPipe\...`) | **YES** | `\Device\NamedPipe\` directory enumeration and process handle table audits. |
| WNF (Windows Notification Facility) State Data | **PARTIAL** | Transport is unmonitored (`ExpWnf*`), but callback routines and reader threads inside protected processes caught via stack walking. |
| Local Network Sockets / Loopback (`127.0.0.1`) | **YES / HEURISTIC** | Winsock transport layer inspection, TDI/WFP kernel filters (`WfpC`, `WfpS`), and network telemetry. |
| Synthetic Input Injection (`SendInput` / Software `mouclass` Packets) | **YES** | Input subsystem interception: audits mouse packets at `\Driver\MouClass` and HID layers; flags packets lacking physical hardware bus interrupt timestamps. |
| Hardware Microcontroller Out-of-Band (KMBox / Arduino / Pico HID) | **NO** | Real hardware bus endpoints via physical USB host controller; presents authentic USB composite HID descriptors and physical bus interrupt frames. |

---

## 12. DPC (Deferred Procedure Call) & Kernel Timer Auditing

EAC uses DPCs (`KDPC`) running at `DISPATCH_LEVEL` (IRQL 2) scheduled via `KeSetTimerEx` (1-second tick) to perform continuous background memory analysis and system integrity verification:

### 12.1 Scheduling & Execution Pipeline

| Mechanism | Kernel API / Interface | Operational Behavior |
|---|---|---|
| **Periodic Timer (1s default)** | `KeSetTimerEx`, `KeInitializeDpc` | Schedules DPC routines at regular 1000ms intervals to execute memory audits without starving user threads. |
| **Multi-Core Distribution** | `KeSetTargetProcessorDpc` | Cycles execution across all logical CPU cores, ensuring page table walks and system checks are distributed. |
| **Work Item Deferral** | `IoQueueWorkItem`, `ExQueueWorkItem` | When an audit routine requires file I/O or blocking operations that cannot execute at `DISPATCH_LEVEL`, the DPC delegates tasks to system worker threads at `PASSIVE_LEVEL`. |

### 12.2 Audit Operations Conducted by DPCs

1. **System Page Table (PTE) Sweeps:**
   - Iterates physical page table structures (PML4 -> PDPT -> PD -> PT).
   - Extracts Page Table Entries where `Present == 1` and `NoExecute (NX) == 0`.
   - Cross-references virtual addresses with `PsLoadedModuleList`. Any executable kernel page unbacked by a signed `.text` section triggers a security violation.
2. **Dispatch Table Integrity Verification:**
   - Sweeps `DriverObject->MajorFunction[IRP_MJ_*]` pointers across critical Windows subsystem drivers (`ntoskrnl.exe`, `hal.dll`, `fltmgr.sys`, `mouclass.sys`).
   - Verifies that target addresses strictly reside within the original signed binary boundaries on disk.
3. **Handle Table Iteration:**
   - Scans system-wide handle tables to identify processes holding open handles to unregistered `DEVICE_OBJECT` endpoints.
4. **Heartbeat & Attestation Verification:**
   - Calculates rolling integrity hashes over monitored memory regions.
   - Synchronizes challenge/response tokens with `EasyAntiCheat_EOS.exe` via `IOCTL_EAC_HEARTBEAT` (`0x804`).

---

## 13. Detection Timing & Frequencies

| Detection Mechanism | Execution Frequency & Cadence |
|---|---|
| **Filesystem Minifilter Interception** | Real-time: intercepts all file operations on game binaries at `IRP_MJ_CREATE` / `WRITE`. |
| **`ObRegisterCallbacks`** | Synchronous: executes on every process/thread handle operation system-wide. |
| **CR3 Physical Memory Sweeps** | Continuous background DPC loops: batched per tick. |
| **Kernel Dispatch Table Auditing** | Periodic: full cycle across major subsystem drivers every 5–15 seconds. |
| **Handle Table Scanning** | Periodic: walks system handle tables every 10–30 seconds. |
| **Heartbeat Token Verification** | High-frequency: bidirectional handshake between `EasyAntiCheat_EOS.exe` and driver every 1000ms. |
| **TPM / PCR Attestation** | Boot-time validation + session start attestation via `tbs.dll`. |
| **Hardware Interrupt Timestamp Auditing** | Real-time: evaluates input packets continuously on arrival at `mouclass`. |

---

## 14. EAC EOS — HWID Identifiers & Spoofing

Comprehensive mapping of every hardware identifier collected by EAC EOS (`EasyAntiCheat_EOS.sys` / `EasyAntiCheat_EOS.exe`), the internal kernel extraction mechanism, data format, and hardware/spoofing constraints:

| Identifier | Kernel Source / Extraction Mechanism | Format String / Representation | Detection Seam & Spoof Constraint |
|---|---|---|---|
| **TPM 2.0 Endorsement Key (EK Certificate / Hash)** | TBS User-Mode API (`tbs.dll` -> `Tbsi_Submit_Command`) + TPM command `TPM2_CC_ReadPublic` (Index `0x81010001` / `0x81010002`). | 256-bit SHA-256 Public Key Hash / X.509 Certificate | **Hardware-Sealed Silicon:** Baked directly into physical TPM 2.0 cryptoprocessor / AMD fTPM / Intel PTT. Cannot be modified via software hooks under Windows 11 HVCI without physical motherboard replacement or fTPM NVRAM clear. |
| **Master Boot Record (MBR) Hash** | Direct disk sector read via `\Device\Harddisk0\DR0` + hash calculation. | `MBR:%08x` | Boot sector hash calculated directly from physical disk sector 0. Bypasses volume hooks. |
| **SCSI LUN & Storage Controller Identification** | Dispatches direct I/O request packets: `IOCTL_SCSI_GET_ADDRESS`, `IOCTL_STORAGE_QUERY_PROPERTY` (`StorageDeviceProperty`). | `:lun=%llx` | Queried directly from storage miniport (`stornvme.sys`, `iaStorAC.sys`). |
| **Disk Partition & Media Metrics** | Queries partition table geometry and media type. | `Part:%u`, `CD:%u` | Formatted disk geometry metrics. |
| **Network Interface Card (NIC) MAC Address** | Direct NDIS miniport OID requests: `OID_802_3_PERMANENT_ADDRESS` and `OID_GEN_PERMANENT_ADDRESS`. | `%02x:%02x:%02x:%02x:%02x:%02x` / `%02x%02x:%02x%02x...` | Bypasses local adapter registry spoofing (`NetworkAddress`). Requires NDIS Light-Weight Filter (LWF) intercept or permanent EEPROM re-flashing. |
| **SMBIOS UUID** | Direct physical memory mapping of SMBIOS tables (`0x000F0000`–`0x000FFFFF`) + `HalAcpiGetTableEx('RSMB')`. | `%08X-%04X-%04X-%02X%02X-%012llX` | Bypasses user-mode WMI hooks. Requires physical memory table re-writing or UEFI NVRAM flashing. |
| **SMBIOS Baseboard & System Serials** | Parsed from Type 1 (System) and Type 2 (Baseboard) tables. | Variable length ASCII string | Physical memory table rewrite / DMI BIOS mod required before driver boot. |
| **Storage Drive Serial Numbers (NVMe / SSD)** | Direct storage queries via `SMART_RCV_DRIVE_DATA` and ATA Pass-Through. | 20-character ASCII string (Byte-swapped for ATA) | Miniport driver intercept required (`\Driver\stornvme`). |
| **GPU Hardware UUID & PCIe Serial** | Queries `dxgkrnl.sys!DxgkQueryAdapterInfo` (`DXGK_QUERYADAPTERINFO`). | Hexadecimal UUID string + PCIe device serial | Extracted via kernel graphics runtime; user-mode registry GPU ID overrides are bypassed. |
| **Monitor Display EDID Serial** | Direct I2C/DDC bus extraction via graphics miniport + `WmiGetMonitorRawEEdidV1Block`. | Encoded 128-byte EDID block (Offset `0x0C`–`0x11` + Serial Descriptor `0x36`) | Queried from hardware EEPROM inside physical monitor. Software registry EDID overrides are ignored. |

---

## 15. SMBIOS Types Parsed by EAC

| Type | Name | Key Monitored Fields |
|---|---|---|
| **0** | BIOS Information | BIOS Vendor, BIOS Version, Release Date, ROM Size, System BIOS Major/Minor Release. |
| **1** | System Information | Manufacturer, Product Name, Version, System Serial Number, System UUID, Wake-up Type. |
| **2** | Baseboard (Motherboard) | Manufacturer, Product, Version, Board Serial Number, Asset Tag, Chassis Location. |
| **3** | System Enclosure / Chassis | Manufacturer, Type, Version, Enclosure Serial Number, Asset Tag Number. |
| **4** | Processor Information | Socket Designation, Processor Family, Manufacturer, Processor ID, Version, Core Count. |
| **17** | Memory Device (per-DIMM) | Device Locator, Bank Locator, Manufacturer, Serial Number, Asset Tag, Part Number, Configured Clock Speed. |
| **127** | End-of-Table | Sentinel structure validating structural termination. |

---

## 16. CPUID Leaves Collected

| Leaf | Collected Data | Ban & Anti-Analysis Relevance |
|---|---|---|
| **`0x00000000`** | `EBX:EDX:ECX` Vendor String (`GenuineIntel` / `AuthenticAMD`) | Base architecture verification. |
| **`0x00000001`** | `EAX` Family/Model/Stepping; `ECX:EDX` Feature Bits (bit 31 = HV) | CPU Identity + Hypervisor detection bit. |
| **`0x00000002`–`0x0000000F`** | Cache, TLB, and Core Topology Descriptors | Hardware topology cross-checked with ACPI MADT table. |
| **`0x40000000`** | Synthetic Hypervisor Vendor String | Identifies Hyper-V (`Microsoft Hv`), VMware, KVM, Xen. |
| **`0x40000001`–`0x40000005`** | Hypervisor Interface & Feature Matrix | Evaluates VBS / SecureKernel hypervisor configuration. |
| **`0x80000002`–`0x80000004`** | Processor Brand String (48 ASCII characters) | Cross-checked with SMBIOS Type 4 processor name. |

---

## 17. PCI Devices Fingerprinted

Full iteration across 256 buses, 32 devices, 8 functions:

### Known Flagged Virtual / Hypervisor Devices:
| Vendor ID | Device ID | Subsystem ID | Hardware Entity |
|---|---|---|---|
| `0x15AD` | `0x0405` | Any | VMware SVGA II Adapter |
| `0x15AD` | `0x0740` | Any | VMware VMXNET3 Virtual Ethernet |
| `0x15AD` | `0x07B0` | Any | VMware PVSCSI Controller |
| `0x1AF4` | `0x1000`–`0x107F` | Any | Red Hat / VirtIO Device Family (KVM / QEMU) |
| `0x1414` | `0x5353` | Any | Microsoft Hyper-V Video Adapter |
| `0x1B36` | Any | Any | QEMU PCI Interconnect / Bridge |
| `0x10EE` | Default | Any | Xilinx Default DMA Bitstream (PCILeech / Screamer) |

---

## 18. Ban Enforcement & Telemetry Pipeline

EAC's ban execution and client-server enforcement workflow for Fortnite:

1. **Hardware Telemetry Assembly:**
   - Driver samples TPM EK public hash, MBR hash (`MBR:%08x`), SCSI LUN (`:lun=%llx`), SMBIOS UUID, Baseboard Serial, NVMe serials, and NIC MAC addresses.
   - Collects boot state measurements: Secure Boot state, PCR[7], HVCI status.
2. **Encrypted Telemetry Dispatch & Chunk Streaming Protocol:**
   - Serializes telemetry using EAC's multiarray chunk streaming protocol discovered in `.rdata`:
     ```text
     multiarray %s %d->%d : {
     newl+ chunk type %d = %s [%d->%d]
     newl? chunk type %d = %s [%d->%d]
     newl! chunk type %d = %s [%d->%d]
     ```
   - Dispatches payload over TLS via `EasyAntiCheat_EOS.exe` directly to Epic Games DataRouter endpoints:
     - `https://api.epicgames.dev/telemetry/data/datarouter/api/v1/public/data`
     - `https://datarouter.ol.epicgames.com/datarouter/api/v1/public/data`
3. **Server-Side Evaluation & Check Bitmask:**
   - Epic Games / EAC backend evaluates composite hardware hash against the global ban database.
   - If a match is detected:
     - Issues a **Hardware Ban Directive** back to client.
     - Triggers match kickoff with error: `"You were removed from the match due to internet lag, your IP or machine, VPN usage, or for cheating."`
4. **Heartbeat Desynchronization Traps:**
   - If the user hooks or freezes the reporting loop, `EasyAntiCheat_EOS.exe` fails to provide the required challenge response to `EasyAntiCheat_EOS.sys`.
   - Driver terminates game process handle with `STATUS_ACCESS_DENIED`.

---

## 19. Spoof Priority Checklist & Cross-Validation Requirements

### Priority Checklist:
| Priority | Hardware Identifier | Architectural Justification |
|---|---|---|
| **CRITICAL** | **TPM 2.0 Endorsement Key (EK)** | Primary hardware-root identifier on Windows 11. Bypassing requires fTPM NVRAM clear or motherboard replacement. |
| **CRITICAL** | **SMBIOS UUID & System Serial (Type 1)** | Foundational composite fingerprint anchor. |
| **CRITICAL** | **SMBIOS Baseboard Serial (Type 2)** | Core motherboard fingerprint; must match manufacturer serial format. |
| **CRITICAL** | **Master Boot Record (MBR) Hash** | Queried via raw physical sector read (`MBR:%08x`). |
| **HIGH** | **Storage Drive Serial Numbers (NVMe / SCSI LUN)** | Queried directly via miniport IRPs (`\Driver\stornvme`); must spoof SCSI inquiry (`:lun=%llx`) and SMART data. |
| **HIGH** | **NIC Permanent MAC Address** | Queried via NDIS OIDs (`OID_802_3_PERMANENT_ADDRESS`); local registry changes are ignored. |
| **HIGH** | **Monitor Display EDID Serial** | Queried from hardware monitor EEPROM; must be intercepted in graphics miniport. |
| **MEDIUM** | **GPU UUID / PCIe Serial** | Queried via `dxgkrnl.sys`; requires driver-level spoofing. |
| **MEDIUM** | **SMBIOS Memory Serials (Type 17)** | Per-DIMM serials cross-checked against channel topology. |
| **LOW** | **Disk Volume / Partition GUIDs** | Re-created upon filesystem reformat / volume recreation (`Part:%u`). |

---

## 20. EAC EOS — Ban Traces & Forensic Cleaner Reference

When a ban is issued, EAC EOS and Fortnite establish persistent markers across filesystem journals, cryptographic registries, and system caches:

### 20.1 Registry Artifacts & Persistent Keys

| Registry Path | Key / Value Audited | Purpose & Persistence Mechanism |
|---|---|---|
| `HKLM\SOFTWARE\Epic Games` | `InstallLocation`, `MachineId`, `AppVersion` | Core client identity tokens. Cross-referenced against cloud ban database on client startup. |
| `HKLM\SOFTWARE\EasyAntiCheat_EOS` | `InstallLocation`, `ProductVersion` | Driver service registration state, install timestamp. |
| `HKLM\SOFTWARE\Microsoft\Cryptography` | `MachineGuid` | Windows global cryptographic installation GUID. Captured at install and hashed into HWID payload. |
| `HKLM\SYSTEM\CurrentControlSet\Control\IDConfigDB\Hardware Profiles\0001` | `HwProfileGuid` | Hardware profile identifier generated by Windows HAL during installation. |
| `HKLM\SYSTEM\MountedDevices` | `\??\Volume{GUID}` | System volume GUID associations and drive signatures. |
| `HKCU\Software\Epic Games` | `SessionData`, `TrackingId` | User-mode state tracking and telemetry tokens. |
| `HKLM\SYSTEM\CurrentControlSet\Services\EasyAntiCheat_EOS` | `Start`, `Type`, `ImagePath` | Driver service registration state (`EasyAntiCheat_EOS.sys`). |

### 20.2 Filesystem Traces & Persistent Cache Blobs

| Directory / File Path | Artifact Content & Telemetry Role |
|---|---|
| `C:\ProgramData\Epic\` | Persistent machine configuration, telemetry caches, and Epic Games launcher private tokens. |
| `C:\Program Files (x86)\EasyAntiCheat_EOS\` | Runtime service directory; contains `EasyAntiCheat_EOS.exe`, `EasyAntiCheat_EOS.sys`, and `.bak` binaries. |
| `C:\Users\<User>\AppData\Local\FortniteGame\Saved\` | Config files, crash report context blobs (`CrashContext.runtime-xml`), and encrypted telemetry cookies. |
| `C:\Users\<User>\AppData\Local\EpicGamesLauncher\Saved\` | Web cache, local installation metadata, and telemetry manifests. |

### 20.3 Cleaner Execution Order

To fully sanitize a machine following a Fortnite hardware ban:
1. **Uninstall Game & Services:** Uninstall Fortnite, Epic Games Launcher, and Easy Anti-Cheat EOS via official uninstaller.
2. **Purge Filesystem Traces:** Delete all directories under `ProgramData\Epic`, `AppData\Local\FortniteGame`, `AppData\Local\EpicGamesLauncher`, and `C:\Program Files (x86)\EasyAntiCheat_EOS`.
3. **Clean System Registries:** Delete `HKLM\SOFTWARE\Epic Games`, `HKLM\SOFTWARE\EasyAntiCheat_EOS`, `HKCU\Software\Epic Games`, and reset `MachineGuid` / `HwProfileGuid`.
4. **Purge Windows Forensic Artifacts:** Flush ShimCache, clear Amcache (`Amcache.hve`), clear Prefetch (`Windows\Prefetch\`), and clear Windows Event Logs (`System.evtx`, `Application.evtx`).
5. **Reset NTFS Change Journal:** Reset or truncate `$UsnJrnl` via `fsutil usn deletejournal /d C:`.
6. **Flush Network Caches:** Flush DNS cache, reset ARP table, obtain new DHCP lease / public IP.
7. **Hardware & Firmware Re-attestation:**
   - Clear and reset TPM 2.0 state (fTPM clear in BIOS).
   - Re-flash / spoof SMBIOS tables (UUID, Baseboard Serial, System Serial).
   - Spoof NVMe drive serials and MBR hash at storage miniport level.
   - Spoof NIC permanent MAC address.
   - Re-format drive partitions to regenerate Volume GUIDs.
8. **Fresh Account & Installation:** Install game on authenticated, unlinked account on the clean hardware state.

---

## 21. Direct3D 11, DXGI & DWM Overlay Detection Architecture

EAC employs a multi-tiered inspection pipeline to detect external HUDs, ESP boxes, and menu canvases drawn over Fortnite:

### 21.1 Window Manager & Composition Auditing (DWM)
- **Window Extended Style Scanning:** `EasyAntiCheat_EOS.exe` audits the system window hierarchy using `GetWindowLongPtr(GWL_EXSTYLE)`.
- Flags any external window possessing the following composite style combination positioned over `FortniteClient-Win64-Shipping.exe`'s screen bounding box:
  $$\text{WS\_EX\_TOPMOST} \mid \text{WS\_EX\_TRANSPARENT} \mid \text{WS\_EX\_LAYERED}$$
- **DWM Occlusion Queries:** Leverages Desktop Window Manager composition telemetry (`DwmGetCompositionTimingInfo` / DirectComposition APIs) to detect transparent swapchains rendering between the desktop composition root and the game canvas.

### 21.2 DXGI SwapChain & DirectX Hook Verification
- **`IDXGISwapChain::Present` VTable Inspection:** Fortnite audits its own DirectX 11 / DirectX 12 swapchain virtual method table. Any detour or trampoline redirecting `Present` (VTable Index 8) or `ResizeBuffers` (VTable Index 13) to unbacked memory triggers immediate abort.
- **Overlay DLL Whitelist:** Only cryptographically signed, pre-approved third-party overlay hooks are permitted to touch the swapchain:
  - Epic Online Services Overlay (`EOSOverlayRenderer-Win64-Shipping.dll`)
  - Discord (`DiscordOverlayHook64.dll`)
  - OBS Studio (`graphics-hook64.dll`)
  - NVIDIA GeForce Experience (`nvspcap64.dll`)
  - Steam In-Game Overlay (`GameOverlayRenderer64.dll`)
- **Hook Hijacking Detection:** If a payload injects into a whitelisted host to borrow its overlay rendering pipeline, EAC verifies the call chain and integrity of the hook module's code section against its signed on-disk binary.

---

## 22. Fortnite Game-Specific Engine Architecture & Network Culling

To render client-side memory scraping ineffective against distant enemies, Fortnite (Unreal Engine 5) implements server-authoritative spatial occlusion and replication graphs:

### 22.1 Replication Graph & Dynamic Dormancy
- The dedicated game server executes real-time 3D visibility raycasts between every player's eye coordinates and all other actors on the map.
- If an enemy player is completely occluded behind building structures, terrain, or outside perception range:
  - **The server sets the actor to dormant and halts network serialization.**
  - Enemy coordinate fields $(X, Y, Z)$, bone transforms, and entity states **do not exist in client RAM**.

### 22.2 The Replication Envelope
- To compensate for network round-trip time (RTT / Ping) and prevent popping, the server maintains a dynamic predictive envelope:
  - As an enemy approaches an angle, corner, or threshold where they could become visible within $\Delta t = \text{Ping} + \text{InterpolationBuffer}$, the server begins replicating position data.
  - Only when this spatial envelope is broken do enemy player pawns appear in client process memory, bounding ESP visualization exclusively to immediately peekable ranges.

---

## 23. Input Heuristics, Trajectory Modeling & Machine Learning Detection

Beyond driver and hardware validation, Epic Games deploys server-side and client-side behavioral modeling (Cerberus / EAC analytics) to detect algorithmic aiming aids:

### 23.1 Vector Delta & Angular Distribution Analysis
- Continuously samples view angle changes per tick:
  $$\vec{\Delta} = (\Delta \text{Pitch}, \Delta \text{Yaw})$$
- **Human Biological Baseline:** Human mouse movement exhibits natural acceleration, deceleration, micro-overshoot, and biological tremors (sensor noise / micro-jitter).
- **Synthetic Signature Penalties:**
  - **Linear Interpolation:** Straight-line trajectories $(\Delta \text{Pitch} / \Delta \text{Yaw} = C)$ lacking variance.
  - **Over-Smoothed Curves:** Symmetrical polynomial curves (Bézier, Hermite) with zero jitter entropy.
  - **Instantaneous Snap:** Single-tick angular shifts exceeding physical human slew rates followed by zero settling time.

### 23.2 Reaction Time & Standard Deviation ($\sigma$) Profiling
- Measures the time delta between an enemy entering the line of sight and the first click/shot event:
  - Normal human reaction distribution: $160\text{ms}–240\text{ms}$ with standard deviation $\sigma \approx 25\text{ms}–40\text{ms}$.
  - Machine assistance signature: Consistent sub-120ms trigger response times with sub-5ms standard deviation ($\sigma < 5\text{ms}$) across consecutive rounds.

### 23.3 Packet Arrival Jitter vs Hardware Interrupt Timestamps
- Audits the timing interval between consecutive `MOUSE_INPUT_DATA` packets at `mouclass.sys`.
- Software loops (`while(true) SendInput(...)`) create synthetic periodic timing clusters ($1.000\text{ms}, 2.000\text{ms}$) that diverge from the asynchronous clock drift of physical USB polling.

---

## 24. Boot Lifecycle, Load Order & Self-Integrity

### 24.1 Execution Sequence
```text
1. UEFI Firmware Initialization (Secure Boot & TPM PCR measurements)
   │
2. Windows Boot Manager (bootmgr)
   │
3. Windows OS Kernel (ntoskrnl.exe)
   │
4. Service Startup:
   ├── EasyAntiCheat_EOS Service (EasyAntiCheat_EOS.exe)
   └── Kernel Driver Load (EasyAntiCheat_EOS.sys via Service Manager)
   │
5. Minifilter Attachment (FLTMGR.SYS!FltRegisterFilter, Altitude 328010)
   │
6. Kernel Callbacks Registration (Ps/Ob/Cm Callbacks)
   │
7. Fortnite Launcher Handshake & Protected Process Spawning (FortniteClient-Win64-Shipping.exe)
```

### 24.2 Self-Integrity Lock & Section 7 Virtual Machine
1. **Dynamic Minifilter Shield:** `EasyAntiCheat_EOS.sys` attaches at altitude `328010` to block unauthorized modification or read access to its own disk image and game binaries.
2. **Encrypted VM Core Execution:** Critical anti-cheat logic resides in Section 7 (Offset `0x32f000`, 35.28 MB, Entropy `6.6633`), executed via a register-preserving dispatcher loop (`movups [rsp+18h], xmm6; movups [rsp+28h], xmm8; movups [rsp+38h], xmm15`).
3. **Dual Verification Pipeline:** The driver verifies the user-mode service via private cryptographic handshake tokens, while the user-mode service attests the driver's signature and integrity via `CRYPT32.dll` and `WinVerifyTrust`.

---

## 25. Hardware Mouse Emulation & Out-of-Band (OOB) Input Vector Analysis

External cheats attempt to bypass software synthetic input detection (`SendInput`, `mouse_event`, `MouseClassServiceCallback` tampering) by using physical USB microcontrollers (MCU) acting as human interface devices (HID). EAC deploys hardware validation and bus topology inspection to identify out-of-band input devices:

### 25.1 Target Microcontroller Hardware Profiles
- **Arduino Leonardo / Pro Micro (ATmega32U4):** Uses AVR USB Core (`PluggableUSB` / `HID.cpp`).
- **Raspberry Pi Pico / RP2040:** Employs TinyUSB stack for dual CDC (serial) + HID composite profiles.
- **KMBox (B+ / NET / Pro) & Makemcu:** Commercial breakout boards pairing an onboard UART/Ethernet bridge with an MCU running hardware pass-through and coordinate injection firmware.

### 25.2 USB Descriptor & Endpoint Validation Pipeline
1. **VID / PID Fingerprinting:**
   - Default IDs (`VID_2341&PID_8036`, `VID_1B4F&PID_9206`, `VID_1A86&PID_7523` CH340) flagged.
   - Dual interface inspection: devices declaring both **CDC Serial (Class `0x02`)** and **HID Mouse (Class `0x03`)** on the same composite root flagged as automation interfaces.
2. **Report Descriptor Anomalies:**
   - Stock Arduino 44-byte mouse report descriptors lack high-resolution scrolling and vendor usage pages present in authentic gaming mice (Logitech, Razer).
3. **Dual-Mouse Conflict & Arbitration:**
   - If inputs arrive simultaneously from two distinct physical device objects (`\Device\PointerClass0` and `\Device\PointerClass1`) while aiming, EAC flags the session for hardware arbitration failure.
4. **Packet Jitter & Polling Intervals:**
   - Authentic 1000Hz gaming mice produce hardware IRQs at $1.000\text{ms} \pm 10\mu\text{s}$. MCU injection via serial/COM produces bursty deltas ($\Delta t$ clustering around $16\text{ms}$ or $8\text{ms}$) failing hardware bus consistency checks.

---

## 26. Complete REA Reverse Engineering Ledger & Static Artifact Manifest

Empirical data extracted directly via REA (`inspect_pe_resources`, `inspect_managed_artifact`, `inspect_rust_binary`, `inspect_go_binary`) and structural PE32/PE32+ disassemblers:

### 26.1 Driver Artifact (`C:\EasyAntiCheat_EOS.sys`)
* **SHA-256**: `e43e586a85b0101512b29508897075d3a12b58567b95d669f9d9016e8ab8ecbb`
* **File Size**: 40,434,848 bytes (~38.56 MB)
* **Architecture**: AMD64 (x86_64, Machine `0x8664`, Subsystem 1 `NATIVE`)
* **Compilation Timestamp**: `2026-10-29 12:36:30 UTC` (`0x6ae33dce`)
* **REA Evidence Ledger IDs**:
  * PE Resources: `ev_aa5cebc620f72faa2ec2fc824eda07c38fb00cdaa96fc5bcce033701702157c1`
  * Managed / CLI Classification: `ev_11f44cde23583a931bff873c53343b5b086d9591963f710bcd74e3fdbf2c98ac` (`not-managed`)
  * Rust Auditable: `ev_a260b577f2e41b48fbdd8e0287a967ef56b33a8f9e93c7b5500ebce3ddd876fc` (`.dep-v0` absent)
  * Go Build Info: `ev_8f912cdfed4a855c2d361d07e153a661e99863e13f5662d2035e63ad22eda83c` (`absent`)
* **Debug RSDS PDB**:
  * Signature: `RSDS`
  * GUID: `d49cb764456c49584c4c44205044422e` (LLD-Link Clang)
  * Age: `1`
  * PDB Path: `Core.pdb`
* **Authenticode Digital Signature**:
  * Signer: `CN=Epic Games Inc., O=Epic Games Inc., L=Cary, S=North Carolina, C=US`
  * Issuer: `CN=DigiCert Trusted G4 Code Signing RSA4096 SHA384 2021 CA1`
  * Serial Number: `098C9339B51A60B6D52F36B70B5FAAA7`
  * Thumbprint: `3CD980E3BACD8FB6FB3F532AFF62753FB7DE47A8`
  * Validity: 2026-02-01 to 2029-02-01
  * Status: Valid (Signed & Timestamper Verified)
* **Version Resource Information**:
  * CompanyName: `EasyAntiCheat Oy`
  * ProductName: `Easy Anti-Cheat (EOS)`
  * ProductVersion: `3.2.0`
  * FileDescription: `Easy Anti-Cheat (EOS) driver`
  * InternalName: `EasyAntiCheat_EOS.sys`
  * LegalCopyright: `Copyright Epic Games, Inc. All Rights Reserved.`
* **PE Section Table & Entropy Profile**:
  * `[0] .text`: VAddr `0x1000`, VSize `0x273e24`, RawOffset `0x400`, RawSize `0x274000`, Flags `0x68000020`, Entropy `6.7207`
  * `[1] .rdata`: VAddr `0x275000`, VSize `0xa5524`, RawOffset `0x274400`, RawSize `0xa5600`, Flags `0x48000040`, Entropy `6.0217`
  * `[2] .data`: VAddr `0x31b000`, VSize `0x51738`, RawOffset `0x319a00`, RawSize `0x2200`, Flags `0xc8000040`, Entropy `3.3755`
  * `[3] [unnamed]`: VAddr `0x36d000`, VSize `0x7b84`, RawOffset `0x31bc00`, RawSize `0x7c00`, Flags `0x48000040`, Entropy `0.0000` (Zero pad)
  * `[4] .rsrc`: VAddr `0x375000`, VSize `0x330`, RawOffset `0x323800`, RawSize `0x400`, Flags `0x48000040`, Entropy `2.7522`
  * `[5] .reloc`: VAddr `0x376000`, VSize `0x30a8`, RawOffset `0x323c00`, RawSize `0x3200`, Flags `0x42000040`, Entropy `5.4146`
  * `[6] [unnamed]`: VAddr `0x37a000`, VSize `0x8200`, RawOffset `0x326e00`, RawSize `0x8200`, Flags `0x68000020`, Entropy `6.4326`
  * `[7] [unnamed]`: VAddr `0x383000`, VSize `0x234c000`, RawOffset `0x32f000`, RawSize `0x234b600` (36,994,560 bytes = 35.28 MB), Flags `0x68000000`, Entropy `6.6633` (Core Obfuscated Virtual Machine)
  * `[8] .pdata`: VAddr `0x26cf000`, VSize `0x11000`, RawOffset `0x267a600`, RawSize `0x10400`, Flags `0x48000000`, Entropy `6.5270`
* **Static Import Anchor**:
  * `FLTMGR.SYS` -> `[178] FltRegisterFilter`
* **DriverEntry**:
  * RVA `0x274df8` (File offset `0x2741f8`)
  * Prologue: `48 89 5c 24 08 57 48 83 ec 20 48 8b da 48 8b f9`
* **Security Directory / Authenticode Offset**:
  * Raw File Offset: `0x268aa00`
  * Length: 21,152 bytes (`0x52a0`)

### 26.2 User-Mode Service Artifact (`C:\Program Files (x86)\EasyAntiCheat_EOS\EasyAntiCheat_EOS.exe`)
* **SHA-256**: `0bbe6d48f45fc155f1c1d029df8de67f221b7329010164eda2dd06d7978340b9`
* **File Size**: 1,041,336 bytes
* **Architecture**: x86 (32-bit PE, Machine `0x014c`)
* **Compilation Timestamp**: `2026-09-09 14:20:48 UTC` (`0x6aa16b40`)
* **REA Evidence Ledger ID**: `ev_1de4a38bd7a72b4e1633a9e6afe918a63f29a374d3cede235092fe74fbcf3a42`
* **Version**: `1.5.9` (`EasyAntiCheat_EOS_Setup.pdb`)
* **Key Imported Subsystems**:
  * TPM Hardware Attestation: `tbs.dll` (`Tbsi_Context_Create`, `Tbsi_GetDeviceInfo`, `Tbsip_Cancel_Commands`, `Tbsip_Context_Close`, `Tbsip_Submit_Command`)
  * Cryptography Next Generation: `ncrypt.dll` (`NCryptOpenStorageProvider`, `NCryptImportKey`, `NCryptExportKey`, `NCryptFreeObject`)
  * Certificate Validation: `CRYPT32.dll` (`CertGetCertificateChain`, `CertVerifyCertificateChainPolicy`)
  * Service Management: `ADVAPI32.dll` (`CreateServiceW`, `ChangeServiceConfig2W`, `ControlService`)
  * Telemetry Networking: `WS2_32.dll` (`getaddrinfo`, `WSAIoctl`)

### 26.3 Backup Service Artifact (`C:\Program Files (x86)\EasyAntiCheat_EOS\EasyAntiCheat_EOS.exe.bak`)
* **SHA-256**: `b289b022c438cbf6b8729585eb66c247567336522e72f64c968346c95d4bf864`
* **File Size**: 1,039,800 bytes
* **Compilation Timestamp**: `2026-06-04 11:18:11 UTC`
* **REA Evidence Ledger ID**: `ev_83c0638ca9fbb7c6ba28daa14b95024d1dea95adf0dea6c3766d89b8ce9ffe89`
* **Version**: `1.5.8`
