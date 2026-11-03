# Denuvo Anti-Cheat (DAC) — Kernel Architecture, Detection & Telemetry Master Reference

Comprehensive mapping of Denuvo Anti-Cheat (`denuvo-anti-cheat.sys` v6.15.3.7013 / `Denuvo Anti-Cheat Update Service`) kernel instrumentation, filesystem minifilter interception, Winsock Kernel (WSK) WebSocket communication, CR3 memory scanning, execution profiling, hardware identification, ban enforcement, and forensic trace footprint. Structured in direct parity with the Easy Anti-Cheat EOS master reference baseline, incorporating all empirical findings and authenticated Evidence IDs extracted via REA (Reverse Engineer Anything).

---

## 1. Detected Injection Types

| Injection Method | Detected? | How / Kernel Mechanism |
|---|---|---|
| `LoadLibrary` | **YES** | `PsSetLoadImageNotifyRoutine` (registered inside Denuvo VM at VA `0x1404B9963`), module whitelist/blacklist, Authenticode verification via `cng.sys` (`BCryptVerifySignature`). |
| Manual map (MDL / physical allocation) | **YES** | `KeStackAttachProcess` + CR3 page table audits. Verifies that executable pages (`NX=0`) map to valid VAD nodes backed by a signed on-disk image (`SEC_IMAGE`). |
| Reflective DLL | **YES** | Memory scanning checks `MEM_COMMIT` regions lacking `SEC_IMAGE` backing; flags PE headers (`MZ`/`PE`), unlinked sections, and unbacked executable allocations. |
| APC injection | **YES** | Thread dispatch inspection monitors alertable thread states; kernel stack unwinding traps APC delivery targets executing in unbacked memory pools. |
| Thread hijack / `SetThreadContext` | **YES** | `ObRegisterCallbacks` (registered at VA `0x1404B903A`) strips `THREAD_SET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_DIRECT_IMPERSONATION`; audits hardware debug registers (`DR0`-`DR7`). |
| Process hollowing | **YES** | `PsSetCreateProcessNotifyRoutineEx` (handler at VA `0x140092460`) intercepts process creation; cross-checks file section mapping against `PEB->ImageBaseAddress`. |
| Module stomping | **YES** | Compares on-disk signed binary page hashes (via `FLTMGR.SYS!FltReadFile`) against mapped in-memory pages; flags modified `.text` code sections and broken Copy-on-Write (COW). |
| DLL sideloading | **YES** | `PsSetLoadImageNotifyRoutine` hashes every loaded library against known publisher and trusted operating system catalogs. |
| Shellcode in allocated memory | **YES** | `IoGetInitialStack` / `IoGetStackLimits` and user-mode TEB inspection (`PsGetCurrentThreadTeb` at VA `0x1404DB469`) trap instruction pointers (`RIP`) resolving outside legitimate module bounds in `PsLoadedModuleList`. |
| `NtCreateSection`-based mapping | **YES** | Minifilter callback (`denuvo-anti-cheat-minifilter-%u`) and handle filtering flag cross-process section mappings into protected game processes. |
| Atom bombing | **PARTIAL** | Memory content scan detects unbacked payload in committed heap; callback execution caught if target thread unwinds to dynamic memory. |
| ThreadlessInject / PoolParty | **YES** | Worker Factory and Threadpool completion ports audited for dispatch pointers resolving to unbacked addresses. |

---

## 2. Detected Driver Types

| Driver Type | Detected? | How |
|---|---|---|
| Unsigned driver (`NtLoadDriver`) | **YES** | Windows Driver Signature Enforcement (DSE) validation + in-kernel signature validation via `cng.sys` (`BCryptVerifySignature`). |
| Test-signed driver (`TESTSIGNING`) | **YES** | Verifies `SystemBootEnvironmentInformation` and registry boot keys (`RtlCheckRegistryKey`). |
| Known BYOVD (Gdrv, mhyprot2, Capcom, RTCore64, procexp, dbutil_2_3, eneio64, iqvw64e) | **YES** | Blacklist of known vulnerable driver certificates and SHA-256 hashes evaluated synchronously in `PsSetLoadImageNotifyRoutine`. |
| Unknown BYOVD with valid WHQL | **PARTIAL** | Monitored dynamically: physical memory mapping operations (`MmMapIoSpace`), cross-process handle creation, and dispatch table detours trigger heuristic alerts. |
| Cheat Engine kernel (`dbk64.sys`) | **YES** | Hardcoded certificate and module name blacklist; active object directory scanning (`\Device\DBK64`). |
| Manual-mapped driver (KDMapper / Capcom) | **YES** | Periodic kernel stack unwinding (`IoGetInitialStack`) + page table audits flag `NX=0` executable code on kernel pool memory (`NonPagedPoolNx`). |
| Driver with dev build path PDB | **YES** | Kernel memory scanner checks driver PE headers for suspicious debug paths and linker signatures. |
| Revoked-cert driver | **YES** | Real-time CRL validation against Microsoft Code Integrity revocation lists and GlobalSign/DigiCert CRL endpoints. |

---

## 3. Detected Hypervisor Techniques

| Technique | Detected? | How |
|---|---|---|
| `CPUID.1:ECX[31]` (HV present) | **YES** | Direct instruction execution: bit 31 set flags hypervisor presence. |
| Custom HV vendor string | **YES** | `CPUID 0x40000000` query: unknown or synthetic vendor strings flagged. |
| CPUID feature filtering | **YES** | Cross-leaf consistency checks: missing architectural features paired with synthetic leaves trigger anomalies. |
| MSR hooking (`LSTAR`, `SYSENTER_EIP`, `GS_BASE`) | **YES** | Read-modify-write cycle across CLI/STI barriers: readback mismatch indicates guest virtualization trap. |
| EPT page splitting (execute vs read divergence) | **YES** | Forced `INVLPG` execution timing checks combined with physical memory cross-reads; VM-exit latency measured. |
| TSC manipulation | **YES** | `RDTSC`/`RDTSCP` multi-sample delta compared against hardware ACPI Power Management Timer (`PMT`) and APIC timer via `KeQueryPerformanceCounter` (`HAL.dll`). |
| Hyper-V / VBS binary patching | **YES** | Disk vs memory hash comparison of `\SystemRoot\System32\ntoskrnl.exe`, `\SystemRoot\System32\hvix64.exe`, and `\SystemRoot\System32\hvax64.exe`. |
| Type-1 / Nested HV below Windows | **YES / PARTIAL** | Windows 11 mandates VBS/HVCI; nested hypervisors induce VM-exit latency penalties caught during calibration loops. |
| VBS / HVCI state change | **YES** | Direct verification via `ZwQuerySystemInformation(SystemIsolatedUserModeInformation)`. |
| CR4 / CR0 register masking | **YES** | Verification of reserved control register bits; attempts to shadow `CR4.VMXE` flag timing discrepancies. |
| ACPI table manipulation | **YES** | Physical table reads checking MADT, SLIC, FADT, and DMAR tables. |

---

## 4. Detected DMA / Firmware Techniques

| Technique | Detected? | How |
|---|---|---|
| PCILeech default FPGA (Xilinx VID/PID) | **YES** | PCI configuration space scan (`ZwDeviceIoControlFile` to bus driver): default IDs (`10EE`) flagged immediately. |
| Custom FPGA with spoofed VID/PID | **PARTIAL** | Bridge topology walking, Subsystem ID validation, class code verification, and abnormal Base Address Register (BAR) window size detection. |
| Thunderbolt DMA adapter | **YES** | PCI bus hierarchy inspection: external hot-plug PCIe bridges audited for DMA capabilities. |
| IOMMU / VT-d / AMD-Vi bypass | **YES** | IOMMU activation enforced on Windows 11; DMA remapping blocks unmapped host memory access. |
| DMA device with unusual BAR config | **YES** | Bus/Device/Function iteration audits BAR allocation sizes against valid hardware database. |
| Modified UEFI firmware | **YES** | TPM PCR[0] measurement verification + SMBIOS cross-validation against ACPI tables. |
| Bootkit (MBR / VBR modification) | **YES** | MBR signature hashing, Secure Boot chain validation, disk boot sector hashing via `\DosDevices\PhysicalDrive`. |
| UEFI bootkit (Firmware DXE driver) | **YES** | TPM PCR[0-7] measurement verification and firmware volume integrity checks. |
| Early-launch driver injection | **YES** | Early boot measurement comparison via TPM PCR[11]. |
| BCD test-signing enabled | **YES** | Registry BCD element check (`SystemBootEnvironmentInformation`). |
| Modified `ntoskrnl` / `securekernel` on disk | **YES** | On-disk digital signature validation vs in-memory image hashing (`BCryptVerifySignature`). |

---

## 5. Kernel Calls Denuvo Monitors

Calls Denuvo monitors for unauthorized interaction with protected processes and its own driver:

| Call | Monitoring Method |
|---|---|
| `NtOpenProcess` | `ObRegisterCallbacks` (VA `0x1404B903A`) — strips `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`, `PROCESS_DUP_HANDLE`, `PROCESS_CREATE_THREAD`, `PROCESS_TERMINATE`, `PROCESS_SUSPEND_RESUME`, `PROCESS_SET_INFORMATION`. |
| `NtOpenThread` | `ObRegisterCallbacks` — strips `THREAD_SET_CONTEXT`, `THREAD_GET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_DIRECT_IMPERSONATION`, `THREAD_TERMINATE`, `THREAD_SET_INFORMATION`. |
| `NtDuplicateObject` | `ObRegisterCallbacks` — enforces handle rights stripping on duplicated descriptors. |
| `NtWriteVirtualMemory` | Blocked via handle stripping; direct kernel callers intercepted via kernel stack walking (`IoGetInitialStack`). |
| `NtAllocateVirtualMemory` | Blocked via handle stripping; kernel-mode allocations visible in VAD audits. |
| `NtProtectVirtualMemory` | Blocked via handle stripping; PTE walker flags unexpected `RWX` transitions. |
| `PsCreateSystemThread` | Thread start address verified; must reside within legitimate loaded module bounds. |
| `NtCreateThreadEx` | Process notify routines (`PsSetCreateProcessNotifyRoutineEx`) intercept new thread staging within protected instance. |
| `NtSetContextThread` | Blocked via handle stripping; kernel modifications caught during thread context and debug register validation. |
| `NtLoadDriver` / `ZwLoadDriver` | Image load callback validates signatures pre-execution; blocks blacklisted certificates. |
| `MmCopyVirtualMemory` | Audited via kernel stack trace inspection; caller address must reside in authorized system module. |

---

## 6. Kernel Calls Denuvo Makes (Detection Tools)

| Routine | Module | Purpose / Detection Functionality |
|---|---|---|
| `WskRegister` / `WskCaptureProviderNPI` | `NETIO.SYS` | Initializes Winsock Kernel (WSK) network provider for in-kernel WebSocket HTTP/WS server. |
| `WskDeregister` / `WskReleaseProviderNPI` | `NETIO.SYS` | Tears down WSK kernel network binding upon driver shutdown. |
| `FltCreateFile` / `FltReadFile` | `FLTMGR.SYS` | Filesystem minifilter file operations: reads binary on-disk pages for integrity comparisons. |
| `FltQueryInformationFile` / `FltGetFileNameInformation` | `FLTMGR.SYS` | Normalizes and parses file paths accessed across disk volumes. |
| `ObRegisterCallbacks` | `ntoskrnl.exe` | Registers process and thread pre/post handle creation and duplication interception (Offset `0x1404B903A`). |
| `PsSetCreateProcessNotifyRoutineEx` | `ntoskrnl.exe` | Intercepts process creation/termination system-wide (Handler at VA `0x140092460`). |
| `PsSetLoadImageNotifyRoutine` | `ntoskrnl.exe` | Audits every DLL and driver image loaded into user or kernel space before entry execution (Offset `0x1404B9963`). |
| `KeStackAttachProcess` / `KeUnstackDetachProcess` | `ntoskrnl.exe` | Attaches driver context directly to target process CR3 for deep user-mode memory scanning. |
| `MmMapIoSpace` / `MmUnmapIoSpace` | `ntoskrnl.exe` | Maps physical memory ranges directly into kernel virtual address space for hardware/page table audits. |
| `MmGetPhysicalAddress` | `ntoskrnl.exe` | Translates virtual addresses to physical Page Frame Numbers (PFNs) to audit page table manipulation. |
| `MmIsAddressValid` | `ntoskrnl.exe` | Memory probe (11 calls in Denuvo VM) preventing page faults during hostile memory scans. |
| `IoGetInitialStack` / `IoGetStackLimits` | `ntoskrnl.exe` | Thread stack boundaries inspection for ROP and kernel stack spoofing detection. |
| `PsGetCurrentThreadTeb` | `ntoskrnl.exe` | User-mode Thread Environment Block (TEB) inspection from ring-0 (`0x1404DB469`). |
| `KeRegisterBugCheckReasonCallback` | `ntoskrnl.exe` | Registers crash callback to harvest forensic crash state via `KeAddTriageDumpDataBlock`. |
| `ExCreateCallback` / `ExRegisterCallback` | `ntoskrnl.exe` | Registers `\Callback\PowerState` listener to recalibrate timers upon power state changes. |
| `BCryptVerifySignature` / `BCryptCreateHash` | `cng.sys` | Cryptography Next Generation: validates packet signatures, driver hashes, and attestation tokens. |
| `MmGetSystemRoutineAddress` | `ntoskrnl.exe` | Dynamically resolves 44 unexported/sensitive kernel routines at runtime inside Section 9 (VM). |

---

## 7. Kernel Callbacks Registered

| Callback | API Registration | Scope & Operational Target |
|---|---|---|
| Filesystem Filter | `FLTMGR.SYS` (`denuvo-anti-cheat-minifilter-%u`) | Intercepts file I/O targeting game binaries, data archives, configuration files, and driver logs. |
| Handle Filter | `ObRegisterCallbacks` (VA `0x1404B903A`) | Intercepts handle creation/duplication targeting protected game processes and threads. |
| Process Notify | `PsSetCreateProcessNotifyRoutineEx` (VA `0x1400922CA`) | Intercepts process creation/termination system-wide; audits image path, parent PID, command line. |
| Image Load Notify | `PsSetLoadImageNotifyRoutine` (VA `0x1404B9963`) | Audits every DLL and driver image loaded into user or kernel space before entry execution. |
| Power State Callback | `ExRegisterCallback` (`\Callback\PowerState`) | Detects sleep, hibernation, and resume events to recalibrate timing references. |
| BugCheck Reason Callback | `KeRegisterBugCheckReasonCallback` | Adds triage dump blocks (`KeAddTriageDumpDataBlock`) upon BSOD to harvest crash state and crash telemetry. |
| I/O Control & Create Interceptor | Dispatch Table (`0x14004A1F0` / `0x14004A1C0`) | Restricts `\Device\DAC-DEFAULT-{GUID}` access and establishes session bindings. |

---

## 8. Memory Scanning Methods

| Method | Architecture & Operational Details |
|---|---|
| CR3 Page Directory Walking | Attaches via `KeStackAttachProcess`; walks PML4 -> PDPT -> PD -> PT in physical RAM, bypassing software API hooks. |
| System PTE Attribute Verification | Audits Page Table Entries: flags any page where `NoExecute (NX) == 0` that falls outside legitimate `.text` sections of signed modules. |
| Section 9 Obfuscated VM Core | Protected core code resides in an unnamed section (Offset `0x103200`, 3.64 MB, Entropy 6.7340) executed via Denuvo's proprietary bytecode engine with Mixed Boolean Arithmetic (MBA). |
| VAD Tree Cross-Referencing | Traverses `EPROCESS->VadRoot`; cross-checks `VadFlags` with physical PTE protections to detect hidden memory regions (`MEM_COMMIT` vs `SEC_IMAGE`). |
| SIMD Content Scanner (AVX2/SSE) | Scans committed process memory searching for known cheat signatures, strings, and hook trampolines. |
| PE On-Disk vs In-Memory Comparison | Hashes `.text` sections of critical modules (`ntoskrnl.exe`, game executable, graphics drivers) and compares directly against on-disk files via `FLTMGR.SYS`. |
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
| Kernel Debuggers (WinDbg) | `KdDebuggerEnabled` / `KdRefreshDebuggerNotPresent` kernel variable checks, BCD `/debug` flag validation, INT 1/3 SEH traps. |
| Frida / Dynamic Binary Instrumentation | RDTSC timing detection, unbacked runtime code generation flags. |
| Custom Debug / Mapping Tools | PDB build path scanning in PE headers, unbacked thread start address tracking, pool tag audits. |

---

## 10. Anti-Debug

| Check | Kernel / Hardware Method |
|---|---|
| INT 1 SEH Trap | Driver initialization gate: debugger consumption causes driver load abort. |
| INT 3 SEH Trap | Embedded breakpoint exceptions checked periodically; active debuggers consume interrupt and fail parity tests. |
| RDTSC Multi-Sample Timing | Back-to-back `RDTSC` loops with randomized iterations: single-step and VM-exit overhead detected. |
| `KdDebuggerEnabled` Check | Direct kernel variable read; if set at any point, machine is flagged. |
| `KdRefreshDebuggerNotPresent` | Refreshes debugger presence state directly from kernel debugger block. |
| RDTSC + CPUID Quad Check | Ratio comparison across CPUID instruction boundaries: hypervisor exit latency detected. |
| APIC / PMT Calibration | Hardware timers (HPET, ACPI PMT) cross-referenced against CPU TSC via `KeQueryPerformanceCounter`. |
| `KUSER_SHARED_DATA` Timestamps | Read directly from unhookable system memory page (`0x7FFE0000`). |

---

## 11. Communication Channel Detection

| Channel | Detected? | Denuvo Monitoring Mechanism |
|---|---|---|
| In-Kernel WSK WebSocket (`127.0.0.1:<Port>`) | **PRIMARY** | Native communication bus: RFC 6455 WebSocket server in ring-0 (`NETIO.SYS`); authenticates clients via challenge keys (`Sec-WebSocket-Key`). |
| Named Device Object (`\Device\DAC-DEFAULT-...`) + `CreateFile` | **YES** | Object Manager directory walk (`\Device\` & `\DosDevices\`), `IoCreateDevice` (`0x14004A104`), parameterized by `DeploymentGuid`. |
| `MajorFunction` Dispatch Table Hooking | **YES** | Dispatch Table Integrity Audits: periodic sweeps of `DriverObject->MajorFunction[IRP_MJ_*]` verifying pointers reside within `.text` module boundaries. |
| Subsystem Dispatch Pointer Hooks | **YES** | Function pointer hashing and boundary checking across graphics and video dispatch tables (`\Device\Video%u`). |
| Kernel MDL Mapping (`MmMapLockedPagesSpecifyCache`) | **YES** | Paging Hierarchy (CR3) vs VAD Tree Cross-Check: PTE has `User/Supervisor = 1` without a corresponding `VAD` node in `EPROCESS->VadRoot`. |
| Named Shared Memory (`\BaseNamedObjects\...`) | **YES** | System-wide Object Manager scans and handle table iteration. |
| Usermode-Allocated Shared Memory (Physical Translation / CR3) | **NO** | Zero kernel object footprint; memory allocated via `VirtualAlloc` in external process (valid VAD node); driver reads/writes via physical PFN mapping. |
| Data-Only Buffer / Queue in Non-Executable Kernel Memory | **NO** | Raw data read/write in non-executable buffers (`NX=1`) without modifying code sections (`.text`) or dispatch pointers leaves no hook signature. |
| Windows Named Pipes (`\Device\NamedPipe\...`) | **YES** | `\Device\NamedPipe\` directory enumeration and process handle table audits. |
| WNF (Windows Notification Facility) State Data | **PARTIAL** | Transport is unmonitored (`ExpWnf*`), but callback routines and reader threads inside protected processes caught via stack walking. |
| Local Network Sockets / Loopback (`127.0.0.1`) | **MONITORED** | Native transport utilized by Denuvo itself; external unauthorized listeners on `ClientPort` flagged. |
| Synthetic Input Injection (`SendInput` / Software `mouclass` Packets) | **YES** | Input subsystem interception: audits mouse packets at `\Driver\MouClass` and HID layers; flags packets lacking physical hardware bus interrupt timestamps. |
| Hardware Microcontroller Out-of-Band (KMBox / Arduino / Pico HID) | **NO** | Real hardware bus endpoints via physical USB host controller; presents authentic USB composite HID descriptors and physical bus interrupt frames. |

---

## 12. DPC (Deferred Procedure Call) & Kernel Timer Auditing

Denuvo uses DPCs and kernel worker threads running at `DISPATCH_LEVEL` scheduled via `KeDelayExecutionThread` and timer callbacks to perform continuous background memory analysis and system integrity verification:

### 12.1 Scheduling & Execution Pipeline

| Mechanism | Kernel API / Interface | Operational Behavior |
|---|---|---|
| **Periodic Timer Tick** | `KeDelayExecutionThread`, `KeInitializeEvent` | Cycles background memory audits and heartbeat sweeps at regular intervals without starving user threads. |
| **Multi-Core Distribution** | `KeSetSystemAffinityThreadEx` / `KeRevertToUserAffinityThreadEx` | Cycles execution across all logical CPU cores (`KeQueryActiveProcessorCountEx`), ensuring page table walks and system checks are distributed. |
| **Thread Priority Elevation** | `KeSetPriorityThread` | Elevates worker thread priority during critical integrity passes to guarantee deterministic execution time. |

### 12.2 Audit Operations Conducted by DPCs

1. **System Page Table (PTE) Sweeps:**
   - Iterates physical page table structures (PML4 -> PDPT -> PD -> PT).
   - Extracts Page Table Entries where `Present == 1` and `NoExecute (NX) == 0`.
   - Cross-references virtual addresses with `PsLoadedModuleList`. Any executable kernel page unbacked by a signed `.text` section triggers a security violation.
2. **Dispatch Table Integrity Verification:**
   - Sweeps `DriverObject->MajorFunction[IRP_MJ_*]` pointers across critical Windows subsystem drivers (`ntoskrnl.exe`, `hal.dll`, `fltmgr.sys`).
   - Verifies that target addresses strictly reside within the original signed binary boundaries on disk.
3. **Handle Table Iteration:**
   - Scans system-wide handle tables to identify processes holding open handles to unregistered `DEVICE_OBJECT` endpoints.
4. **WebSocket Heartbeat & Attestation Verification:**
   - Calculates rolling integrity hashes over monitored memory regions.
   - Synchronizes challenge/response tokens over the local WebSocket connection (`127.0.0.1:<ClientPort>`).

---

## 13. Detection Timing & Frequencies

| Detection Mechanism | Execution Frequency & Cadence |
|---|---|
| **Filesystem Minifilter Interception** | Real-time: intercepts all file operations on game binaries at `IRP_MJ_CREATE` / `WRITE`. |
| **`ObRegisterCallbacks`** | Synchronous: executes on every process/thread handle operation system-wide. |
| **CR3 Physical Memory Sweeps** | Continuous background DPC loops: batched per tick. |
| **Kernel Dispatch Table Auditing** | Periodic: full cycle across major subsystem drivers every 5–15 seconds. |
| **Handle Table Scanning** | Periodic: walks system handle tables every 10–30 seconds. |
| **WebSocket Heartbeat Token Verification** | High-frequency: bidirectional handshake between user-mode client and driver every 1000ms. |
| **Hardware ID Attestation** | Session startup + periodic background telemetry sync. |
| **Hardware Interrupt Timestamp Auditing** | Real-time: evaluates input packets continuously on arrival at `mouclass`. |

---

## 14. Denuvo Anti-Cheat — HWID Identifiers & Spoofing

Comprehensive mapping of every hardware identifier collected by Denuvo Anti-Cheat (`denuvo-anti-cheat.sys`), the internal kernel extraction mechanism, data format, and hardware/spoofing constraints:

| Identifier | Kernel Source / Extraction Mechanism | Format String / Representation | Detection Seam & Spoof Constraint |
|---|---|---|---|
| **Physical Storage Drive Serial** | Direct sector read and SMART query via `\DosDevices\PhysicalDrive`. | Variable length ASCII string | Bypasses user-mode volume queries. Requires storage miniport interception (`stornvme.sys`). |
| **GPU Adapter Description String** | Queried via `\Registry\Machine\System\CurrentControlSet\Hardware Profiles\UnitedVideo\CONTROL\VIDEO\%s\%04llu`. | `HardwareInformation.AdapterString` | Graphics hardware identity. User-mode registry overrides bypassed via kernel query. |
| **GPU Dedicated Video Memory** | Queried via `HardwareInformation.MemorySize` in video profile. | 32-bit / 64-bit integer (VRAM Bytes) | Cross-checked against DXGI display adapter properties. |
| **Computer Hardware ID** | Global hardware identity string queried via `ComputerHardwareId`. | Registry GUID / Hardware Hash | Composite fingerprint anchored in Windows HAL hardware profile. |
| **SMBIOS UUID & System Serials** | Direct physical memory mapping of SMBIOS tables (`0x000F0000`–`0x000FFFFF`). | `%08X-%04X-%04X-%02X%02X-%012llX` | Bypasses user-mode WMI hooks. Requires physical memory table re-writing or UEFI NVRAM flashing. |
| **Deployment GUID** | Generated/stored in service key `DeploymentGuid`. | Standard GUID String | Re-generated per game installation / session context. |
| **Telemetry Session Key** | Stored in `\Registry\User\%s\SOFTWARE\DenuvoAntiCheat\telemetry\%llu-%llu\%llu`. | Numeric 64-bit tuple representation | Session identifier linked to user SID and hardware state. |

---

## 15. SMBIOS Types Parsed by Denuvo

| Type | Name | Key Monitored Fields |
|---|---|---|
| **0** | BIOS Information | BIOS Vendor, BIOS Version, Release Date, ROM Size, System BIOS Major/Minor Release. |
| **1** | System Information | Manufacturer, Product Name, Version, System Serial Number, System UUID, Wake-up Type. |
| **2** | Baseboard (Motherboard) | Manufacturer, Product, Version, Board Serial Number, Asset Tag, Chassis Location. |
| **3** | System Enclosure / Chassis | Manufacturer, Type, Version, Enclosure Serial Number, Asset Tag Number. |
| **4** | Processor Information | Socket Designation, Processor Family, Manufacturer, Processor ID, Version, Core Count. |
| **17** | Memory Device (per-DIMM) | Device Locator, Bank Locator, Manufacturer, Serial Number, Asset Tag, Part Number. |
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

Full iteration across PCI bus topology via video controller queries and device enumeration:

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

Denuvo Anti-Cheat's ban execution and client-server enforcement workflow:

1. **Hardware Telemetry Assembly:**
   - Driver samples physical drive serial (`\DosDevices\PhysicalDrive`), GPU adapter string, SMBIOS UUID, Baseboard Serial, and `ComputerHardwareId`.
   - Collects boot state measurements: Secure Boot state, HVCI status, debugger presence (`KdDebuggerEnabled`).
2. **Encrypted Telemetry Dispatch & Cloud Pipeline:**
   - Driver communicates with user-mode agent over in-kernel WebSocket (`127.0.0.1:<ClientPort>`).
   - Dispatches payload over TLS directly to Denuvo / Irdeto Codefusion infrastructure endpoints:
     - `https://ac-knock.codefusion.technology` (Heartbeat / Keepalive ping)
     - `https://ac-prp-ap.codefusion.technology` (Preparation / Attestation proxy)
     - `https://ac-trg-1-19-0.codefusion.technology` (Trigger / Detection report pipeline v1.19.0)
   - Telemetry payload tags discovered in `.rdata`:
     - `-app/ctl/shd: 48fba9e9f4240375_forced_telemetry=true, source=driver-ping`
     - `nc/zd: 48fba9e9f4240375_forced_telemetry=true, source=driver-zd`
     - `CurrentLogTicket`
3. **Server-Side Evaluation & Check Bitmask:**
   - Codefusion backend evaluates composite hardware hash against the global ban database.
   - If a match or anomaly is detected:
     - Issues a **Hardware Ban Directive** back to client.
     - Causes game client termination or match disconnection.
4. **Heartbeat Desynchronization Traps:**
   - If the user hooks, pauses, or freezes the WebSocket loop, the driver terminates the game process handle with `STATUS_ACCESS_DENIED`.

---

## 19. Spoof Priority Checklist & Cross-Validation Requirements

### Priority Checklist:
| Priority | Hardware Identifier | Architectural Justification |
|---|---|---|
| **CRITICAL** | **SMBIOS UUID & System Serial (Type 1)** | Foundational composite fingerprint anchor. |
| **CRITICAL** | **SMBIOS Baseboard Serial (Type 2)** | Core motherboard fingerprint; must match manufacturer serial format. |
| **CRITICAL** | **Physical Storage Drive Serial** | Queried via raw physical device query (`\DosDevices\PhysicalDrive`). |
| **HIGH** | **GPU Adapter String & VRAM Metrics** | Queried via video hardware profiles in registry. |
| **HIGH** | **Computer Hardware ID (`ComputerHardwareId`)** | Windows global hardware profile hash. |
| **MEDIUM** | **Deployment GUID (`DeploymentGuid`)** | Driver service instance parameter; must be regenerated on cleanup. |
| **MEDIUM** | **SMBIOS Memory Serials (Type 17)** | Per-DIMM serials cross-checked against channel topology. |
| **LOW** | **Telemetry Cache Keys** | Purged from `\Registry\User\<SID>\SOFTWARE\DenuvoAntiCheat\`. |

---

## 20. Denuvo Anti-Cheat — Ban Traces & Forensic Cleaner Reference

When a ban or detection occurs, Denuvo establishes persistent markers across registry hives and filesystem log directories:

### 20.1 Registry Artifacts & Persistent Keys

| Registry Path | Key / Value Audited | Purpose & Persistence Mechanism |
|---|---|---|
| `HKLM\SYSTEM\CurrentControlSet\Services\denuvo anti-cheat` | `DeploymentGuid`, `ClientPort`, `DriverConfigured` | Core driver service configuration and device instance binding. |
| `HKCU\SOFTWARE\DenuvoAntiCheat\telemetry\<ID>-<ID>\<ID>` | `telemetry` | User-specific persistent telemetry cache and tracking tokens. |
| `HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion` | `ProductId`, `InstallDate` | Windows installation metrics hashed into client profile. |
| `HKLM\SYSTEM\CurrentControlSet\Hardware Profiles\UnitedVideo\CONTROL\VIDEO\...` | `HardwareInformation.AdapterString` | Graphics adapter profile cache. |

### 20.2 Filesystem Traces & Persistent Cache Blobs

| Directory / File Path | Artifact Content & Telemetry Role |
|---|---|
| `C:\Logs\denuvo-anti-cheat-driver-...log` | Primary root driver log file containing session timestamps and errors. |
| `C:\Logs\acd-...log` | Anti-cheat diagnostic logs. |
| `%LOCALAPPDATA%\Denuvo Anti-Cheat\Logs\` | User-mode service log directory (`denuvo-anti-cheat-driver-...log`). |
| `C:\Program Files\Denuvo Anti-Cheat\` | Runtime service directory; contains service binaries and update agents. |

### 20.3 Cleaner Execution Order

To fully sanitize a machine following a Denuvo Anti-Cheat detection:
1. **Uninstall Game & Services:** Stop and uninstall `denuvo-anti-cheat` service (`sc stop "denuvo anti-cheat"`, `sc delete "denuvo anti-cheat"`).
2. **Purge Filesystem Logs:** Delete `C:\Logs\denuvo-anti-cheat-driver-*.log`, `C:\Logs\acd-*.log`, and `%LOCALAPPDATA%\Denuvo Anti-Cheat\`.
3. **Clean System Registries:**
   - Delete `HKLM\SYSTEM\CurrentControlSet\Services\denuvo anti-cheat`.
   - Delete `HKCU\SOFTWARE\DenuvoAntiCheat`.
   - Reset `DeploymentGuid` and `MachineGuid`.
4. **Purge Windows Forensic Artifacts:** Flush ShimCache, clear Amcache (`Amcache.hve`), clear Prefetch (`denuvo-anti-cheat*.pf`), and clear Windows Event Logs.
5. **Reset NTFS Change Journal:** Reset `$UsnJrnl` via `fsutil usn deletejournal /d C:`.
6. **Hardware & Firmware Re-attestation:**
   - Re-flash / spoof SMBIOS tables (UUID, Baseboard Serial, System Serial).
   - Spoof physical drive serials at storage miniport level (`\DosDevices\PhysicalDrive`).
   - Spoof MAC address and reset network leases.
7. **Fresh Account & Installation:** Install game on authenticated, unlinked account on clean hardware state.

---

## 21. Direct3D 11, DXGI & DWM Overlay Detection Architecture

Denuvo employs a multi-tiered inspection pipeline to detect external HUDs, ESP boxes, and menu canvases drawn over protected games:

### 21.1 Window Manager & Composition Auditing (DWM)
- **Window Extended Style Scanning:** Audits system window hierarchy using `GetWindowLongPtr(GWL_EXSTYLE)`.
- Flags any external window possessing the following composite style combination positioned over the game screen bounding box:
  $$\text{WS\_EX\_TOPMOST} \mid \text{WS\_EX\_TRANSPARENT} \mid \text{WS\_EX\_LAYERED}$$
- **DWM Occlusion Queries:** Leverages Desktop Window Manager composition telemetry to detect transparent swapchains rendering between the desktop composition root and the game canvas.

### 21.2 DXGI SwapChain & DirectX Hook Verification
- **`IDXGISwapChain::Present` VTable Inspection:** Audits DirectX 11 / DirectX 12 swapchain virtual method table. Any detour or trampoline redirecting `Present` or `ResizeBuffers` to unbacked memory triggers immediate abort.
- **Overlay Whitelist:** Only cryptographically signed, pre-approved third-party overlays (Discord, Steam, NVIDIA ShadowPlay, OBS) are permitted.
- **Hook Hijacking Detection:** Verifies call chain and integrity of hook module code sections against signed on-disk binaries.

---

## 22. Denuvo Game-Specific Engine Architecture & Anti-Tamper Interaction

Denuvo Anti-Cheat works in tight synergy with Denuvo Anti-Tamper (DAT):

### 22.1 Anti-Tamper Integrity Gates
- The game binary contains hundreds of embedded Denuvo Anti-Tamper triggers interspersed throughout game logic.
- Triggers execute encrypted integrity challenges verifying that:
  - The kernel driver (`denuvo-anti-cheat.sys`) is active and responding on the local WebSocket port.
  - No user-mode hooks or memory patches exist on critical game code sections.
  - Timing intervals between anti-cheat heartbeats stay within strict tolerances.

### 22.2 Bytecode VM Synchronization
- If a cheat patches a game function protected by Denuvo Anti-Tamper, the tampering is detected by the user-mode VM and signaled over the WebSocket to ring-0.
- The ring-0 driver immediately elevates protection, strips handles, and dispatches a telemetry payload to `codefusion.technology`.

---

## 23. Input Heuristics, Trajectory Modeling & Machine Learning Detection

### 23.1 Vector Delta & Angular Distribution Analysis
- Continuously samples view angle changes per tick:
  $$\vec{\Delta} = (\Delta \text{Pitch}, \Delta \text{Yaw})$$
- **Human Biological Baseline:** Human mouse movement exhibits natural acceleration, deceleration, micro-overshoot, and biological tremors.
- **Synthetic Signature Penalties:**
  - **Linear Interpolation:** Straight-line trajectories lacking variance.
  - **Over-Smoothed Curves:** Symmetrical polynomial curves with zero jitter entropy.
  - **Instantaneous Snap:** Single-tick angular shifts exceeding physical human slew rates followed by zero settling time.

### 23.2 Reaction Time & Standard Deviation ($\sigma$) Profiling
- Measures the time delta between an enemy entering line of sight and the first click/shot event:
  - Normal human reaction distribution: $160\text{ms}–240\text{ms}$ with standard deviation $\sigma \approx 25\text{ms}–40\text{ms}$.
  - Machine assistance signature: Consistent sub-120ms response times with sub-5ms standard deviation ($\sigma < 5\text{ms}$) across consecutive rounds.

### 23.3 Packet Arrival Jitter vs Hardware Interrupt Timestamps
- Audits timing intervals between consecutive `MOUSE_INPUT_DATA` packets at `mouclass.sys`.
- Software loops (`SendInput`) produce synthetic periodic timing clusters that diverge from the asynchronous clock drift of physical USB polling.

---

## 24. Boot Lifecycle, Load Order & Self-Integrity

### 24.1 Execution Sequence
```text
1. Windows Boot & OS Kernel Initialization (ntoskrnl.exe)
   │
2. Game Launch Initiated by User
   │
3. Denuvo Anti-Cheat Update Service Start (Denuvo Anti-Cheat Update Service)
   │
4. Kernel Driver Load (denuvo-anti-cheat.sys via Service Manager)
   │
5. Security Cookie & WDF Binding (__security_init_cookie @ 0x14015F000, WdfVersionBind)
   │
6. Device & Symbolic Link Creation (\Device\DAC-DEFAULT-{DeploymentGuid} @ 0x14004A104)
   │
7. In-Kernel Winsock (WSK) WebSocket Server Start (127.0.0.1:<ClientPort> @ NETIO.SYS)
   │
8. Filesystem Minifilter Attachment (FLTMGR.SYS, denuvo-anti-cheat-minifilter-%u)
   │
9. Denuvo VM Execution Engine Activation (Section 9 @ 0x140165000)
   ├── Dynamic API Resolution (MmGetSystemRoutineAddress, 44 calls)
   ├── ObRegisterCallbacks (Process & Thread handle stripping @ 0x1404B903A)
   └── PsSetLoadImageNotifyRoutine & PsSetCreateProcessNotifyRoutineEx
   │
10. Protected Game Process Spawning & Bidirectional WebSocket Attestation
```

### 24.2 Self-Integrity Lock & Section 9 Virtual Machine
1. **Dynamic Minifilter Shield:** Attaches filesystem filter to block unauthorized modification or read access to its own disk image and game binaries.
2. **Encrypted VM Core Execution:** Critical anti-cheat logic resides in Section 9 (Offset `0x103200`, 3.64 MB, Entropy `6.7340`), executed via Denuvo's proprietary VM dispatcher loop.
3. **Dual Verification Pipeline:** The driver verifies the user-mode service via private cryptographic handshake tokens over WebSocket, while the user-mode service attests the driver's signature and integrity via `cng.sys` (`BCryptVerifySignature`).

---

## 25. Hardware Mouse Emulation & Out-of-Band (OOB) Input Vector Analysis

### 25.1 Target Microcontroller Hardware Profiles
- **Arduino Leonardo / Pro Micro (ATmega32U4):** Uses AVR USB Core (`PluggableUSB` / `HID.cpp`).
- **Raspberry Pi Pico / RP2040:** Employs TinyUSB stack for dual CDC (serial) + HID composite profiles.
- **KMBox (B+ / NET / Pro) & Makemcu:** Commercial breakout boards pairing an onboard UART/Ethernet bridge with an MCU running hardware pass-through and coordinate injection firmware.

### 25.2 USB Descriptor & Endpoint Validation Pipeline
1. **VID / PID Fingerprinting:**
   - Default IDs (`VID_2341&PID_8036`, `VID_1B4F&PID_9206`, `VID_1A86&PID_7523` CH340) flagged.
   - Dual interface inspection: devices declaring both **CDC Serial (Class `0x02`)** and **HID Mouse (Class `0x03`)** on the same composite root flagged as automation interfaces.
2. **Report Descriptor Anomalies:**
   - Stock Arduino 44-byte mouse report descriptors lack high-resolution scrolling and vendor usage pages present in authentic gaming mice.
3. **Dual-Mouse Conflict & Arbitration:**
   - If inputs arrive simultaneously from two distinct physical device objects while aiming, Denuvo flags the session for hardware arbitration failure.
4. **Packet Jitter & Polling Intervals:**
   - Authentic 1000Hz gaming mice produce hardware IRQs at $1.000\text{ms} \pm 10\mu\text{s}$. MCU injection via serial/COM produces bursty deltas failing hardware bus consistency checks.

---

## 26. Complete REA Reverse Engineering Ledger & Static Artifact Manifest

Empirical data extracted directly via REA (`inspect_artifact`, `inspect_managed_artifact`, `rea-artifact-graph`) and structural PE32+ disassemblers:

### 26.1 Driver Artifact (`C:\denuvo-anti-cheat.sys`)
* **SHA-256**: `40f3df25f2cefa640802fdb348104427d2b4cbec3ac0f0551393c7964047d502`
* **File Size**: 4,973,192 bytes (~4.74 MB)
* **Architecture**: AMD64 (x86_64, Machine `0x8664`, Subsystem 1 `NATIVE`)
* **Compilation Timestamp**: `1781864941` (`0x6a3519ed`)
* **Characteristics**: `0x22` (`IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE`)
* **DllCharacteristics**: `0x1e0`
* **REA Evidence Ledger IDs**:
  * Root Artifact Inspection: `ev_350f5316ef98fd767cef578baaddeac6d8a07c151612fdff41ae9709b349b57c`
  * Artifact Inventory: `ev_c578a77edbe3e1c073ad3305f7b7773a1b570cddca4a2cf5531c45cc2caeaf89`
  * Managed / CLI Classification: `ev_68007f93491f946a552d97477768099f954d6ebd92391fd7566789fc894b7033` (`not-managed`)
* **Debug RSDS PDB**:
  * Signature: `RSDS`
  * PDB Path: `denuvo-anti-cheat.pdb`
* **Authenticode Digital Signature**:
  * Primary Signer: `Microsoft Windows Hardware Compatibility Publisher` (WHQL driver signature)
  * Intermediate CA: `GlobalSign GCC R45 EV CodeSigning CA 2020` (`gsgccr45evcodesignca2020`)
  * Root CA: `Microsoft Root Certificate Authority 2010` / `GlobalSign Code Signing Root R45`
  * Timestamp CA: `DigiCert Trusted G4 TimeStamping RSA4096 SHA256 2025 CA1`
  * Security Directory Offset: `0x4b9000`, Size: `0x5288`
* **Version Resource Information**:
  * CompanyName: `Denuvo GmbH`
  * ProductName: `Denuvo Anti-Cheat Driver`
  * ProductVersion: `6.15.3.7013`
  * FileVersion: `6.15.3.7013`
  * FileDescription: `Denuvo Anti-Cheat Driver`
  * InternalName: `Denuvo Anti-Cheat Driver`
  * OriginalFilename: `denuvo-anti-cheat.sys`
  * LegalCopyright: `© Denuvo GmbH. All rights reserved.`
  * Comments: `Denuvo Anti-Cheat Driver v6.15`
* **PE Section Table & Entropy Profile**:
  * `[0] .text`: VAddr `0x1000`, VSize `0xc0000`, RawOffset `0x400`, RawSize `0xc0000`, Flags `0x68000020`, Entropy `6.7752`
  * `[1] .rdata`: VAddr `0xc1000`, VSize `0x3e000`, RawOffset `0xc0400`, RawSize `0x3dc00`, Flags `0x48000040`, Entropy `7.2493`
  * `[2] .data`: VAddr `0xff000`, VSize `0x55000`, RawOffset `0xfe000`, RawSize `0x600`, Flags `0xc8000040`, Entropy `2.8745`
  * `[3] .pdata`: VAddr `0x154000`, VSize `0x9000`, RawOffset `0xfe600`, RawSize `0x200`, Flags `0x48000040`, Entropy `0.0000`
  * `[4] PAGE`: VAddr `0x15d000`, VSize `0x1000`, RawOffset `0xfe800`, RawSize `0x600`, Flags `0x60000020`, Entropy `4.7818`
  * `[5] .edata`: VAddr `0x15e000`, VSize `0x1000`, RawOffset `0xfee00`, RawSize `0x200`, Flags `0x40000040`, Entropy `0.0000`
  * `[6] INIT`: VAddr `0x15f000`, VSize `0x2000`, RawOffset `0xff000`, RawSize `0x1400`, Flags `0x62000020`, Entropy `5.1776`
  * `[7] .rsrc`: VAddr `0x161000`, VSize `0x1000`, RawOffset `0x100400`, RawSize `0x600`, Flags `0x42000040`, Entropy `2.9881`
  * `[8] .reloc`: VAddr `0x162000`, VSize `0x3000`, RawOffset `0x100a00`, RawSize `0x2800`, Flags `0x42000040`, Entropy `5.4078`
  * `[9] [unnamed]`: VAddr `0x165000`, VSize `0x3a2fe8`, RawOffset `0x103200`, RawSize `0x3a3000` (3,813,376 bytes = 3.64 MB), Flags `0x68000020`, Entropy `6.7340` (Core Denuvo Obfuscated Virtual Machine)
  * `[10] [unnamed]`: VAddr `0x508000`, VSize `0x17b8`, RawOffset `0x4a6200`, RawSize `0x1800`, Flags `0x48000020`, Entropy `5.5173`
  * `[11] [unnamed]`: VAddr `0x50a000`, VSize `0x49d0`, RawOffset `0x4a7a00`, RawSize `0x4a00`, Flags `0xc8000020`, Entropy `5.5926`
  * `[12] [unnamed]`: VAddr `0x50f000`, VSize `0x8c`, RawOffset `0x4ac400`, RawSize `0x200`, Flags `0x48000020`, Entropy `1.4794`
  * `[13] .rdata`: VAddr `0x510000`, VSize `0xf8`, RawOffset `0x4ac600`, RawSize `0x200`, Flags `0x48000040`, Entropy `1.8780`
  * `[14] .pdata`: VAddr `0x511000`, VSize `0x8fdc`, RawOffset `0x4ac800`, RawSize `0x9000`, Flags `0x48000040`, Entropy `6.1036`
  * `[15] .reloc`: VAddr `0x51a000`, VSize `0x3408`, RawOffset `0x4b5800`, RawSize `0x3600`, Flags `0x42000040`, Entropy `5.4382`
* **Static Import Anchors**:
  * `WDFLDR.SYS`: `WdfVersionBind`, `WdfVersionUnbind`, `WdfVersionBindClass`, `WdfVersionUnbindClass`
  * `NETIO.SYS`: `WskRegister`, `WskCaptureProviderNPI`, `WskDeregister`, `WskReleaseProviderNPI`
  * `cng.sys`: `BCryptVerifySignature`, `BCryptImportKeyPair`, `BCryptCreateHash`, `BCryptHashData`, `BCryptFinishHash`, `BCryptGenRandom`
  * `FLTMGR.SYS`: `FltCreateFile`, `FltReadFile`, `FltQueryInformationFile`, `FltGetFileNameInformation`, `FltClose`
  * `ntoskrnl.exe`: `ObRegisterCallbacks`, `PsSetCreateProcessNotifyRoutineEx`, `PsSetLoadImageNotifyRoutine`, `KeStackAttachProcess`, `IoGetInitialStack`, `PsGetCurrentThreadTeb`, `KeRegisterBugCheckReasonCallback`
* **DriverEntry**:
  * RVA `0x1184` (ImageBase `0x140000000`, VA `0x140001184`)
  * Prologue: `48 89 5c 24 08 57 48 83 ec 20 48 8b da 48 8b f9 e8 6b de 15 00`
  * Dispatches security cookie init (`0x14015F000`) and calls main initialization (`0x140001060`).
