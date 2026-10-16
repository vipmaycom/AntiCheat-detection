# Vanguard (VGK) — Kernel Architecture, Detection & Telemetry Master Reference

Comprehensive mapping of Riot Vanguard (`vgk.sys` / `vgc.exe`) kernel instrumentation, memory scanning, execution profiling, hardware identification, ban enforcement, and forensic trace footprint. Structured in direct parity with the EAC EOS technical reference baseline.

---

## 1. Detected Injection Types

| Injection Method | Detected? | How / Kernel Mechanism |
|---|---|---|
| `LoadLibrary` | **YES** | `PsSetLoadImageNotifyRoutine`, dynamic module whitelist/blacklist, signature verification via `CiValidateFileObject`. |
| Manual map (MDL / physical allocation) | **YES** | Periodic NMI stack walking (`KeRegisterNmiCallback`) interrupts threads executing in unmapped memory. CR3 page table walker checks for `NX=0` with no VAD entry. |
| Reflective DLL | **YES** | Heuristic scanner checks memory allocations (`MEM_COMMIT`) lacking `SEC_IMAGE` backing; flags PE headers (`MZ`/`PE`) and unlinked sections. |
| APC injection | **YES** | `PsSetCreateThreadNotifyRoutine` and kernel APC dispatch hooks monitor alertable thread states; NMI stack walk catches APC delivery to unbacked addresses. |
| Thread hijack / `SetThreadContext` | **YES** | `ObRegisterCallbacks` strips `THREAD_SET_CONTEXT`, `THREAD_SUSPEND_RESUME`, and `THREAD_DIRECT_IMPERSONATION`; periodic DPC stack checks monitor active thread registers. |
| Process hollowing | **YES** | `PsSetCreateProcessNotifyRoutineEx` intercepts process creation; cross-checks file section mapping against `PEB->ImageBaseAddress`. |
| Module stomping | **YES** | Compares on-disk signed binary page hashes against in-memory mapped pages (`CiValidateFileObject`); flags modified code sections and export tables. |
| DLL sideloading | **YES** | `PsSetLoadImageNotifyRoutine` hashes every loaded library against known Riot and Microsoft authenticode catalogs. |
| Shellcode in allocated memory | **YES** | NMI stack walking traps active instruction pointers (`RIP`) resolving outside legitimate module bounds in `PsLoadedModuleList`. |
| `NtCreateSection`-based mapping | **YES** | Handle filtering and cross-process section audits flag view mappings into the protected game instance. |
| Atom bombing | **PARTIAL** | Memory content scan detects unbacked payload; callback execution caught if target thread unwinds to dynamic memory. |
| ThreadlessInject / PoolParty | **YES** | Worker Factory and Threadpool completion ports audited for dispatch pointers resolving to unbacked addresses. |

---

## 2. Detected Driver Types

| Driver Type | Detected? | How |
|---|---|---|
| Unsigned driver (`NtLoadDriver`) | **YES** | `SERVICE_BOOT_START` initialization + Secure Boot & TPM 2.0 enforcement; `SeRegisterImageVerificationCallback` rejects pre-execution. |
| Test-signed driver (`TESTSIGNING`) | **YES** | Direct BCD element query via `ZwQuerySystemInformation(SystemBootEnvironmentInformation)` + kernel code integrity flags. |
| Known BYOVD (RTCore64, dbk64, Dell DBUtil, Gdrv, Capcom, AsrDrv) | **YES** | Encrypted driver certificate and SHA-256 blacklist; monitors pool allocations and device interface GUIDs. |
| Unknown BYOVD with valid WHQL | **PARTIAL** | Monitored dynamically: physical memory mapping operations, cross-process handle creation, and dispatch table detours trigger heuristic alerts. |
| Cheat Engine kernel (`dbk64.sys`) | **YES** | Hardcoded certificate and module name blacklist; active object directory scanning. |
| Manual-mapped driver (KDMapper / Capcom) | **YES** | High-frequency NMI stack frame inspection (interrupts all cores, checks `RIP` against `PsLoadedModuleList`); page table audits flag `NX=0` on pool memory. |
| Driver with dev build path PDB | **YES** | Kernel memory scanner checks driver PE headers for suspicious debug paths and linker signatures. |
| Revoked-cert driver | **YES** | Real-time CRL validation against Microsoft Code Integrity revocation lists. |

---

## 3. Detected Hypervisor Techniques

| Technique | Detected? | How |
|---|---|---|
| `CPUID.1:ECX[31]` (HV present) | **YES** | Direct instruction execution: bit 31 set flags hypervisor presence. |
| Custom HV vendor string | **YES** | `CPUID 0x40000000` query: unknown or synthetic vendor strings flagged. |
| CPUID feature filtering | **YES** | Cross-leaf consistency checks: missing architectural features paired with synthetic leaves trigger anomalies. |
| MSR hooking (`LSTAR`, `SYSENTER_EIP`, `GS_BASE`) | **YES** | Direct read-modify-write cycle across CLI/STI barriers: readback mismatch indicates guest virtualization trap. |
| EPT page splitting (execute vs read divergence) | **YES** | Forced `INVLPG` execution timing checks combined with physical memory cross-reads; VM-exit latency measured. |
| TSC manipulation | **YES** | `RDTSC`/`RDTSCP` multi-sample delta compared against hardware ACPI Power Management Timer (`PMT`) and APIC timer. |
| Hyper-V / VBS binary patching | **YES** | Disk vs memory hash comparison of `securekernel.exe` and hypervisor binaries. |
| Type-1 / Nested HV below Windows | **YES / PARTIAL** | Windows 11 mandates VBS/HVCI; nested hypervisors induce VM-exit latency penalties caught during boot calibration. |
| VBS / HVCI state change | **YES** | Direct verification via `VslGetSecurePciEnabled` and `ZwQuerySystemInformation(SystemIsolatedUserModeInformation)`. |
| CR4 / CR0 register masking | **YES** | Verification of reserved control register bits; attempts to shadow `CR4.VMXE` flag timing discrepancies. |
| ACPI table manipulation | **YES** | Physical table reads via `HalAcpiGetTableEx` checking MADT, SLIC, FADT, and DMAR tables. |

---

## 4. Detected DMA / Firmware Techniques

| Technique | Detected? | How |
|---|---|---|
| PCILeech default FPGA (Xilinx VID/PID) | **YES** | PCI configuration space scan (`HalGetBusDataByOffset` + MMIO ECAM `0xE0000000+`): default IDs (`10EE`) flagged immediately. |
| Custom FPGA with spoofed VID/PID | **PARTIAL** | Bridge topology walking, Subsystem ID validation, class code verification, and abnormal Base Address Register (BAR) window size detection. |
| Thunderbolt DMA adapter | **YES** | PCI bus hierarchy inspection: external hot-plug PCIe bridges audited for DMA capabilities. |
| IOMMU / VT-d / AMD-Vi bypass | **YES** | IOMMU activation enforced on Windows 11; DMA remapping blocks unmapped host memory access. |
| DMA device with unusual BAR config | **YES** | Bus/Device/Function iteration audits BAR allocation sizes against valid hardware database. |
| Modified UEFI firmware | **YES** | TPM PCR[0] measurement verification + SMBIOS cross-validation against ACPI tables. |
| Bootkit (MBR / VBR modification) | **YES** | TPM PCR measurements, Secure Boot chain validation, disk boot sector hashing. |
| UEFI bootkit (Firmware DXE driver) | **YES** | TPM PCR[0-7] measurement verification, TCG2 event log parsing (`TCG2_EVENT_LOG`). |
| Early-launch driver injection | **YES** | TPM PCR[11] verification + early boot measurement comparison. |
| BCD test-signing enabled | **YES** | Registry BCD element check (`SystemBootEnvironmentInformation`). |
| Modified `ntoskrnl` / `securekernel` on disk | **YES** | On-disk digital signature validation vs in-memory image hashing. |

---

## 5. Kernel Calls Vanguard Monitors

Calls Vanguard monitors for unauthorized interaction with protected processes (`VALORANT.exe` / `vgk.sys`):

| Call | Monitoring Method |
|---|---|
| `NtOpenProcess` | `ObRegisterCallbacks` — strips `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`, `PROCESS_DUP_HANDLE`, `PROCESS_CREATE_THREAD`, `PROCESS_TERMINATE`. |
| `NtOpenThread` | `ObRegisterCallbacks` — strips `THREAD_SET_CONTEXT`, `THREAD_GET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_DIRECT_IMPERSONATION`. |
| `NtDuplicateObject` | `ObRegisterCallbacks` — enforces handle rights stripping on duplicated descriptors. |
| `NtWriteVirtualMemory` | Blocked via handle stripping; direct kernel callers intercepted via NMI stack walking. |
| `NtAllocateVirtualMemory` | Blocked via handle stripping; kernel-mode allocations visible in VAD audits. |
| `NtProtectVirtualMemory` | Blocked via handle stripping; PTE walker flags unexpected `RWX` transitions. |
| `PsCreateSystemThread` | Thread start address verified; must reside within legitimate loaded module bounds. |
| `NtCreateThreadEx` | Process notify routines intercept new thread staging within protected instance. |
| `NtSetContextThread` | Blocked via handle stripping; kernel modifications caught during DPC thread context validation. |
| `NtLoadDriver` / `ZwLoadDriver` | `SeRegisterImageVerificationCallback` validates signatures pre-execution; blocks blacklisted certificates. |
| `MmCopyVirtualMemory` | Audited via NMI stack trace inspection; caller address must reside in authorized system module. |

---

## 6. Kernel Calls Vanguard Makes (Detection Tools)

| Routine | Purpose / Detection Functionality |
|---|---|
| `KeRegisterNmiCallback` | Registers high-IRQL callback (`HIGH_LEVEL`) for core-wide thread stack unwinding. |
| `HalSendNMI` | Dispatches physical Non-Maskable Interrupts across all logical cores for execution profiling. |
| `RtlVirtualUnwind` + `RtlLookupFunctionEntry` | Performs precise x64 stack unwinding adhering to `.pdata` exception tables. |
| `MmCopyVirtualMemory` | Safely reads and verifies user-mode process pages from kernel space. |
| `MmCopyMemory` | Low-level physical and kernel memory validation. |
| `PsLookupProcessByProcessId` | PID resolution for entity tracking. |
| `PsLookupThreadByThreadId` | TID resolution for stack and context auditing. |
| `CiValidateFileObject` | Validates authenticode digital signatures against Microsoft WHQL and Riot catalogs. |
| `HalGetBusDataByOffset` | Reads PCI bus configuration space across all bus/device/function combinations. |
| `HalAcpiGetTableEx` | Physical ACPI table parsing (MADT, SLIC, FADT, DMAR, RSMB). |
| `ZwQuerySystemInformation(0x05)` | Complete process list enumeration. |
| `ZwQuerySystemInformation(0x0B)` | Full loaded module enumeration (`PsLoadedModuleList`). |
| `ZwQuerySystemInformation(0x10)` | System-wide open handle table enumeration. |
| `ZwQuerySystemInformation(0x67)` | Physical firmware and SMBIOS table data. |
| `KeSetCoalescableTimer` / `KeSetTimerEx` | High-frequency kernel DPC scheduling with millisecond jitter for background auditing. |
| `IoEnumerateDeviceObjectList` | Walks kernel device lists to detect unauthorized `DEVICE_OBJECT` endpoints. |
| `Tbsip_Submit_Command` | Submits raw TPM 2.0 command bytes to query Endorsement Keys and PCR banks. |

---

## 7. Kernel Callbacks Registered

| Callback | API Registration | Scope & Operational Target |
|---|---|---|
| Handle Filter | `ObRegisterCallbacks` | Intercepts every handle creation/duplication targeting `VALORANT.exe` and protected game threads. |
| Process Notify | `PsSetCreateProcessNotifyRoutineEx` | Intercepts process creation/termination system-wide; audits image path, parent PID, and command line. |
| Thread Notify | `PsSetCreateThreadNotifyRoutine` | Intercepts every thread creation; validates `StartAddress` against module boundaries. |
| Image Load Notify | `PsSetLoadImageNotifyRoutine` | Audits every DLL and driver image loaded into user or kernel space before entry execution. |
| Driver Image Verification | `SeRegisterImageVerificationCallback` | Intercepts kernel-mode driver verification before execution starts. |
| NMI Callback | `KeRegisterNmiCallback` | Executes high-frequency interruption cycles across all cores to audit active registers and stack return addresses. |
| I/O Control Interceptor | Dispatch Table (`IRP_MJ_DEVICE_CONTROL`) | Restricts `\Device\vgk` exclusively to authenticated `vgc.exe` user-mode process tokens. |

---

## 8. Memory Scanning Methods

| Method | Architecture & Operational Details |
|---|---|
| CR3 Page Directory Walking | Reads target process CR3 register; manually walks PML4 -> PDPT -> PD -> PT in physical RAM, bypassing software API hooks. |
| System PTE Attribute Verification | Audits kernel Page Table Entries: flags any page where `NoExecute (NX) == 0` that falls outside legitimate `.text` sections of signed modules. |
| High-Frequency NMI Stack Profiling | Generates periodic NMIs across all cores; captures `RIP`, `RSP`, `RBP`; unwinds frames using `.pdata`; flags return addresses in unbacked memory. |
| VAD Tree Cross-Referencing | Traverses `EPROCESS->VadRoot` AVL tree; cross-checks `VadFlags` with physical PTE protections to detect hidden memory regions (MDL allocations). |
| SIMD Content Scanner (AVX2/SSE) | Scans kernel pools and committed process memory at 32-64 bytes/cycle searching for known cheat signatures, strings, and hook trampolines. |
| PE On-Disk vs In-Memory Comparison | Hashes `.text` sections of critical drivers (`ntoskrnl.exe`, `hal.dll`, graphics drivers) and compares directly against on-disk signed files. |
| Code Cave Detection | Cross-references PE section characteristics; flags executable execution occurring inside `.data` or `.rdata` sections of signed modules. |
| Spinlock Memory Watchpoints | Detects concurrent access to protected data structures by external threads. |

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
| Custom Debug / Mapping Tools | PDB build path scanning in PE headers, `PoolTag` audits, unbacked thread start address tracking. |

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

| Channel | Detected? | Vanguard Monitoring Mechanism |
|---|---|---|
| Named Device Object (`\Device\...`) + `CreateFile` | **YES** | Object Manager directory walk (`\Device\` & `\DosDevices\`), `IoEnumerateDeviceObjectList`, and global handle sweeps (`ZwQuerySystemInformation(0x10)`). |
| `MajorFunction` Dispatch Table Hooking (`beep.sys`, `null.sys`) | **YES** | Dispatch Table Integrity Audits: periodic sweeps of `DriverObject->MajorFunction[IRP_MJ_*]` verifying pointers reside within `.text` module boundaries. |
| Subsystem Dispatch Pointer Hooks (`win32k.sys`, `dxgkrnl.sys`) | **YES** | Function pointer hashing and boundary checking across graphics dispatch tables. |
| Kernel MDL Mapping (`MmMapLockedPagesSpecifyCache` with `UserMode`) | **YES** | Paging Hierarchy (CR3) vs VAD Tree Cross-Check: PTE has `User/Supervisor = 1` without a corresponding `VAD` node in `EPROCESS->VadRoot`. |
| Named Shared Memory (`\BaseNamedObjects\...`) | **YES** | System-wide Object Manager scans and handle table iteration. |
| Usermode-Allocated Shared Memory (Physical Translation / CR3) | **NO** | Zero kernel object footprint; memory allocated via `VirtualAlloc` in external process (valid VAD node); driver reads/writes via physical PFN mapping. |
| Data-Only Buffer / Queue in Non-Executable Kernel Memory | **NO** | Raw data read/write in non-executable buffers (`NX=1`) without modifying code sections (`.text`) or dispatch pointers leaves no hook signature. |
| Windows Named Pipes (`\Device\NamedPipe\...`) | **YES** | `\Device\NamedPipe\` directory enumeration and process handle table audits. |
| WNF (Windows Notification Facility) State Data | **PARTIAL** | Transport is unmonitored (`ExpWnf*`), but callback routines and reader threads inside protected processes caught via NMI stack walking. |
| Local Network Sockets / Loopback (`127.0.0.1`) | **YES / HEURISTIC** | Winsock transport layer inspection, TDI/WFP kernel filters, and ETW network provider telemetry. |
| Synthetic Input Injection (`SendInput` / Software `mouclass` Packets) | **YES** | Input subsystem interception: audits mouse packets at `\Driver\MouClass` and HID layers; flags packets lacking physical hardware bus interrupt timestamps. |
| Hardware Microcontroller Out-of-Band (KMBox / Arduino / Pico HID) | **NO** | Real hardware bus endpoints via physical USB host controller; presents authentic USB composite HID descriptors and physical bus interrupt frames. |

---

## 12. DPC (Deferred Procedure Call) & Kernel Timer Auditing

While NMIs (`HIGH_LEVEL` / IRQL 31/32) handle instantaneous execution profiling, Vanguard uses DPCs (`KDPC`) running at `DISPATCH_LEVEL` (IRQL 2) to perform heavy, continuous background memory analysis and system integrity verification.

### 12.1 Scheduling & Execution Pipeline

| Mechanism | Kernel API / Interface | Operational Behavior |
|---|---|---|
| **Periodic Timers with Jitter** | `KeSetCoalescableTimer`, `KeSetTimerEx` | Schedules DPC routines at pseudo-random intervals with millisecond jitter to prevent payloads from predicting execution windows or synchronizing state toggling. |
| **Multi-Core Distribution** | `KeSetTargetProcessorDpc` | Cycles execution across all logical CPU cores, ensuring page table walks and system checks are distributed without causing localized thread starvation. |
| **Work Item Deferral** | `IoQueueWorkItem`, `ExQueueWorkItem` | When an audit routine requires file I/O or blocking operations that cannot execute at `DISPATCH_LEVEL`, the DPC delegates tasks to system worker threads at `PASSIVE_LEVEL`. |

### 12.2 Audit Operations Conducted by DPCs

1. **System Page Table (PTE) Sweeps:**
   - Iterates physical page table structures (PML4 -> PDPT -> PD -> PT) in calibrated batches (typically 512 entries per DPC tick).
   - Extracts Page Table Entries where `Present == 1` and `NoExecute (NX) == 0`.
   - Cross-references virtual addresses with `PsLoadedModuleList`. Any executable kernel page unbacked by a signed `.text` section triggers a security violation.
2. **Dispatch Table Integrity Verification:**
   - Sweeps `DriverObject->MajorFunction[IRP_MJ_*]` pointers across critical Windows subsystem drivers (`ntoskrnl.exe`, `hal.dll`, `beep.sys`, `null.sys`, `mouclass.sys`).
   - Verifies that target addresses strictly reside within the original signed binary boundaries on disk.
3. **Handle Table Iteration:**
   - Scans system-wide handle tables to identify processes holding open handles to unregistered `DEVICE_OBJECT` endpoints.
4. **Heartbeat & Telemetry Generation:**
   - Calculates rolling integrity hashes over monitored memory regions.
   - Encrypts and queues heartbeat payloads for consumption by `vgc.exe` via ALPC/IOCTL.

### 12.3 Anti-Tampering & Evasion Countermeasures

| Evasion Concept | Failure Vector / Detection Outcome | System Reaction |
|---|---|---|
| **Canceling Vanguard Timers (`KeCancelTimer`)** | **Heartbeat Desynchronization:** If the DPC timer fails to report back to `vgc.exe` within the expected time window, the user-mode watchdog flags a communication drop. | Immediate game client termination (`VAN 128` / `VAN -81`) and account security flag. |
| **Hooking `KDPC->DeferredRoutine`** | **Driver Code & Object Integrity:** The `vgk.sys` driver object, code pages, and timer structures are checksummed against baseline memory states. | Kernel integrity flag, immediate driver abort. |
| **Tampering with CPU DPC Queues (`KPRCB->DpcData`)** | **Microsoft Kernel Patch Protection (PatchGuard):** The processor control block and DPC list heads are critical kernel structures continuously audited by KPP. | BugCheck `0x109 (CRITICAL_STRUCTURE_CORRUPTION)`. |
| **Blocking DPC Execution via IRQL (`KeRaiseIrql`)** | **Hardware Watchdog Violation:** Keeping a CPU core at `DISPATCH_LEVEL` or higher indefinitely to starve Vanguard DPCs triggers the system timer watchdog. | BugCheck `0x133 (DPC_WATCHDOG_VIOLATION)`. |

---

## 13. Detection Timing & Frequencies

| Detection Mechanism | Execution Frequency & Cadence |
|---|---|
| **NMI Stack Profiling** | Burst sampling across all logical cores via `HalSendNMI`; randomized intervals (100ms–2000ms jitter). |
| **`ObRegisterCallbacks`** | Synchronous: executes on every process/thread handle operation system-wide. |
| **CR3 Physical Memory Sweeps** | Continuous background DPC loops: batched in chunks of 512 PTEs per tick. |
| **Kernel Dispatch Table Auditing** | Periodic: full cycle across major subsystem drivers every 5–15 seconds. |
| **Handle Table Scanning** | Periodic: walks system handle tables (`ZwQuerySystemInformation(0x10)`) every 10–30 seconds. |
| **PCI Configuration Space Walks** | Boot initialization + periodic polling (512 device reads). |
| **Heartbeat Token Verification** | High-frequency: bidirectional handshake between `vgc.exe` and `vgk.sys` every 1000ms–3000ms. |
| **TPM / PCR Attestation** | Boot-time validation + session start attestation. |
| **Hardware Interrupt Timestamp Auditing** | Real-time: evaluates input packets continuously on arrival at `mouclass`. |

---

## 14. Vanguard (VGK) — HWID Identifiers & Spoofing

Comprehensive mapping of every hardware identifier collected by Riot Vanguard (`vgk.sys` / `vgc.exe`), the internal kernel extraction mechanism, data format, and hardware/spoofing constraints:

| Identifier | Kernel Source / Extraction Mechanism | Format String / Representation | Detection Seam & Spoof Constraint |
|---|---|---|---|
| **TPM 2.0 Endorsement Key (EK Certificate / Hash)** | TBS Kernel API (`Tbsip_Submit_Command`) + direct TPM command `TPM2_CC_ReadPublic` (Index `0x81010001` / `0x81010002`). | 256-bit SHA-256 Public Key Hash / X.509 Certificate | **Hardware-Sealed Silicon:** Baked directly into physical TPM 2.0 cryptoprocessor / AMD fTPM / Intel PTT. Cannot be modified via software hooks under Windows 11 HVCI without physical motherboard replacement or total fTPM NVRAM clear. |
| **TPM Platform Configuration Registers (PCRs)** | TCG2 Event Log parsing (`ZwQuerySystemInformation(0x67)`) + `TPM2_PCR_Read` for PCR[0], PCR[2], PCR[7], PCR[11]. | 24-byte SHA-1 / 32-byte SHA-256 digests per bank | **Cryptographic Chain:** Re-calculated dynamically by firmware during system boot. Any discrepancy between PCR[7] (Secure Boot state) and boot logs triggers driver load refusal. |
| **SMBIOS UUID** | Direct physical memory mapping of SMBIOS tables (`0x000F0000`–`0x000FFFFF`) + `HalAcpiGetTableEx('RSMB')`. | `%08X-%04X-%04X-%02X%02X-%012llX` (UUID) | Bypasses user-mode WMI hooks. Requires physical memory table re-writing or UEFI NVRAM flashing before `vgk.sys` boot initialization. |
| **SMBIOS System Serial (Type 1)** | Parsed from System Information Table (Type 1, Offset `0x07`). | Variable length ASCII string | Must match manufacturer format conventions (e.g., ASUS, MSI, Gigabyte string structures); randomized strings trigger entropy anomalies. |
| **SMBIOS Baseboard Serial (Type 2)** | Parsed from Baseboard Information Table (Type 2, Offset `0x07`). | Variable length ASCII string | Physical memory table rewrite / DMI BIOS mod required before driver boot. |
| **SMBIOS Chassis Serial (Type 3)** | Parsed from System Enclosure Table (Type 3, Offset `0x06`). | Variable length ASCII string | Physical SMBIOS table rewrite required. |
| **SMBIOS Memory Serial Numbers (Type 17)** | Parsed per-DIMM slot from Physical Memory Table (Type 17, Offset `0x18`). | Hex-encoded SPD serial per slot | Monitored per physical RAM channel; mismatched slot count vs CPU memory controller topology triggers flag. |
| **Storage Drive Serial Numbers (NVMe / SSD)** | Dispatches direct I/O request packets: `IOCTL_STORAGE_QUERY_PROPERTY` (`StorageDeviceProperty`), `IOCTL_SCSI_MINIPORT` (`SMART_RCV_DRIVE_DATA`), and ATA Pass-Through. | 20-character ASCII string (Byte-swapped for ATA) | Miniport driver intercept required (`\Driver\stornvme`, `\Driver\iaStorAC`). Disk filters in user-mode are ignored. |
| **Disk Volume & Partition GUIDs** | Queries `IOCTL_DISK_GET_PARTITION_INFO_EX` and filesystem volume serial via NTFS file records (`$Volume`). | Partition UUID `%08X-...` + Volume ID `%08X` | Partition table metadata re-format / partition recreating required. |
| **GPU Hardware UUID & PCIe Serial** | Queries `dxgkrnl.sys!DxgkQueryAdapterInfo` (`DXGK_QUERYADAPTERINFO`) + proprietary NVIDIA/AMD bus miniport queries. | Hexadecimal UUID string + PCIe device serial | Extracted via kernel graphics runtime; user-mode registry GPU ID overrides are bypassed. |
| **Monitor Display EDID Serial** | Direct I2C/DDC bus extraction via graphics miniport + `WmiGetMonitorRawEEdidV1Block`. | Encoded 128-byte EDID block (Offset `0x0C`–`0x11` + Serial Descriptor `0x36`) | Queried from hardware EEPROM inside physical monitor. Software registry EDID overrides are ignored. |
| **Network Interface Card (NIC) MAC Address** | Direct NDIS miniport OID requests: `OID_802_3_PERMANENT_ADDRESS` and `OID_GEN_PERMANENT_ADDRESS`. | `%02X:%02X:%02X:%02X:%02X:%02X` | Bypasses local adapter registry spoofing (`NetworkAddress`). Requires NDIS Light-Weight Filter (LWF) intercept or permanent EEPROM re-flashing. |
| **Processor ID & Brand String** | `CPUID` Leaf `0x1:EAX` (Processor ID) + Leaves `0x80000002`–`0x80000004` (48-character Brand String). | 32-bit CPU signature + 48-byte ASCII string | Read directly by CPU instruction. Can only be altered via hardware hypervisor interception (which triggers timing checks). |
| **PCI Device Topology & Bridge Map** | `HalGetBusDataByOffset` iterating bus 0 to 255 + MMIO ECAM space (`0xE0000000+`). | Hierarchical Vendor:Device ID pairs + BAR sizes | Complete device tree fingerprint; unassigned bridges or altered vendor/device pairs flag abnormal hardware configurations. |

### 14.1 Deep-Dive: TPM 2.0 Endorsement Key (EK) & Silicon Attestation Architecture

Why software spoofers cannot bypass Vanguard's TPM 2.0 hardware verification:

1. **Extraction Path (TBS Command Pipeline):**
   - Vanguard does not access raw physical MMIO registers (`0xFED40000` / CRB) directly, which would cause hardware bus arbitration collisions and bugchecks under Windows 11 VBS/HVCI.
   - Instead, `vgk.sys` interfaces with the Windows TPM Base Services (`tbs.sys`) via `Tbsip_Submit_Command`, dispatching raw TCG binary command byte streams directly to the physical cryptoprocessor silicon.
2. **Target Command Codes & NVRAM Indices:**
   - **`TPM2_CC_ReadPublic (0x00000173)`:** Queries the public component of the primary Endorsement Key at persistent handle `0x81010001` (RSA 2048) or `0x81010002` (ECC NIST P-256).
   - **`TPM2_CC_NV_Read (0x0000014E)`:** Reads the manufacturer-signed X.509 Endorsement Certificate stored at standard NVRAM index `0x01C00002` (RSA) or `0x01C0000A` (ECC).
3. **Cryptographic Root of Trust Validation:**
   - The public EK certificate returned by the chip is cryptographically signed by the hardware manufacturer's Certificate Authority (e.g., AMD Root CA, Intel PTT CA, Infineon, Nuvoton, STMicroelectronics).
   - Fabricating a random public key fails: Riot's backend verifies that the certificate chain resolves back to a legitimate, unrevoked hardware silicon vendor root.
4. **Asymmetric Attestation Challenges (Quotes):**
   - The private portion of the Endorsement Key **never leaves the internal silicon boundary** of the TPM chip.
   - When the Riot server issues an attestation challenge containing an ephemeral cryptographic nonce, the physical TPM chip signs the measurement quote using its internal private key. A software hook cannot forge this signature without knowledge of the hardware private key.

---

## 15. SMBIOS Types Parsed by Vanguard

| Type | Name | Key Monitored Fields |
|---|---|---|
| **0** | BIOS Information | BIOS Vendor, BIOS Version, Release Date, ROM Size, System BIOS Major/Minor Release. |
| **1** | System Information | Manufacturer, Product Name, Version, System Serial Number, System UUID, Wake-up Type. |
| **2** | Baseboard (Motherboard) | Manufacturer, Product, Version, Board Serial Number, Asset Tag, Chassis Location. |
| **3** | System Enclosure / Chassis | Manufacturer, Type, Version, Enclosure Serial Number, Asset Tag Number. |
| **4** | Processor Information | Socket Designation, Processor Family, Manufacturer, Processor ID, Version, Core Count. |
| **17** | Memory Device (per-DIMM) | Device Locator, Bank Locator, Manufacturer, Serial Number, Asset Tag, Part Number, Configured Clock Speed. |
| **127** | End-of-Table | Sentinel structure validating structural termination. |

*Extraction route:* Direct physical RAM map at `0x000F0000`–`0x000FFFFF` (`_SM_` / `_SM3_` anchor search) + `HalAcpiGetTableEx('RSMB')`. Bypasses user-mode WMI hooks.

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

Full iteration across 256 buses, 32 devices, 8 functions (512 reads via `HalGetBusDataByOffset` and MMIO ECAM space):

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

Vanguard's ban execution and client-server enforcement workflow:

1. **Hardware Telemetry Assembly:**
   - Driver samples TPM EK public hash, SMBIOS UUID, Baseboard Serial, NVMe serials, and NIC MAC addresses.
   - Collects boot state measurements: Secure Boot state, PCR[7], HVCI status (`VslGetSecurePciEnabled`).
2. **Encrypted Telemetry Dispatch:**
   - Assembles composite HWID payload; encrypts with ephemeral session keys; queues payload to `vgc.exe` via ALPC/IOCTL.
   - `vgc.exe` dispatches payload over TLS to Riot backend servers.
3. **Server-Side Evaluation & Check Bitmask:**
   - Backend evaluates hardware hash against the global ban database.
   - If a match is detected:
     - Server issues a **Banned State Directive** back to client.
     - Sets local driver ban flags.
     - Triggers user-mode client termination (`VAN 152` / `VAL 5`).
4. **Heartbeat Desynchronization Traps:**
   - If the user hooks or freezes the reporting loop, `vgc.exe` fails to provide the required challenge response to `vgk.sys`.
   - Driver triggers client shutdown with errors `VAN 128` or `VAN -81`.

---

## 19. Spoof Priority Checklist & Cross-Validation Requirements

### Priority Checklist:
| Priority | Hardware Identifier | Architectural Justification |
|---|---|---|
| **CRITICAL** | **TPM 2.0 Endorsement Key (EK)** | Primary hardware-root identifier on Windows 11. Bypassing requires fTPM NVRAM clear or motherboard replacement. |
| **CRITICAL** | **SMBIOS UUID & System Serial (Type 1)** | Foundational composite fingerprint anchor. |
| **CRITICAL** | **SMBIOS Baseboard Serial (Type 2)** | Core motherboard fingerprint; must match manufacturer serial format. |
| **HIGH** | **Storage Drive Serial Numbers (NVMe)** | Queried directly via miniport IRPs (`\Driver\stornvme`); must spoof SCSI inquiry and SMART data. |
| **HIGH** | **NIC Permanent MAC Address** | Queried via NDIS OIDs (`OID_802_3_PERMANENT_ADDRESS`); local registry changes are ignored. |
| **HIGH** | **Monitor Display EDID Serial** | Queried from hardware monitor EEPROM; must be intercepted in graphics miniport. |
| **MEDIUM** | **GPU UUID / PCIe Serial** | Queried via `dxgkrnl.sys`; requires driver-level spoofing. |
| **MEDIUM** | **SMBIOS Memory Serials (Type 17)** | Per-DIMM serials cross-checked against channel topology. |
| **LOW** | **Disk Volume / Partition GUIDs** | Re-created upon filesystem reformat / volume recreation. |
| **DO NOT FILTER** | **CPUID Features & Brand String** | Must match SMBIOS Type 4; synthetic filtering introduces hypervisor timing anomalies. |

### Cross-Validation Requirements:
Vanguard cross-validates hardware data across multiple independent kernel paths. Inconsistencies flag the system:
- **SMBIOS Type 4 vs CPUID:** Processor brand string, family, model, and stepping must match physical CPUID leaves.
- **SMBIOS vs ACPI:** SMBIOS manufacturer strings must be consistent with ACPI OEM Table IDs.
- **NIC OUI vs Manufacturer:** The first 3 bytes of the MAC address (OUI) must resolve to a valid registered hardware vendor.
- **Storage Controller Consistency:** NVMe serial format must conform to NVMe specification (no ASCII padding anomalies).
- **PCI Topology:** Device class codes and BAR sizes must match physical chipsets.

---

## 20. Vanguard (VGK) — Ban Traces & Forensic Cleaner Reference

When a ban is issued, Vanguard and the Riot Client establish persistent markers across filesystem journals, cryptographic registries, and system caches to ensure hardware bans persist across re-installations:

### 20.1 Registry Artifacts & Persistent Keys

| Registry Path | Key / Value Audited | Purpose & Persistence Mechanism |
|---|---|---|
| `HKLM\SOFTWARE\Riot Games\Riot Client` | `MachineId`, `InstallLocation`, `TelemetryId` | Core client identity tokens. Cross-referenced against cloud ban database on client startup. |
| `HKLM\SOFTWARE\Microsoft\Cryptography` | `MachineGuid` | Windows global cryptographic installation GUID. Captured at install and hashed into HWID payload. |
| `HKLM\SYSTEM\CurrentControlSet\Control\IDConfigDB\Hardware Profiles\0001` | `HwProfileGuid` | Hardware profile identifier generated by Windows HAL during installation. |
| `HKLM\SYSTEM\MountedDevices` | `\??\Volume{GUID}` | System volume GUID associations and drive signatures. |
| `HKCU\Software\Riot Games` | `SessionData`, `TrackingId` | User-mode state tracking and telemetry tokens. |
| `HKLM\SYSTEM\CurrentControlSet\Services\vgk` | `Start`, `Type`, `FailureActions` | Driver service registration state, install timestamp, and failure counters. |

### 20.2 Filesystem Traces & Persistent Cache Blobs

| Directory / File Path | Artifact Content & Telemetry Role |
|---|---|
| `C:\ProgramData\Riot Games\` | Persistent machine configuration (`machine.cfg`), telemetry caches, and riot client private tokens. |
| `C:\Users\<User>\AppData\Local\Riot Games\` | CEF (Chromium Embedded Framework) web storage, crash dumps, and local installation metadata. |
| `C:\Users\<User>\AppData\Local\VALORANT\Saved\` | Config files, crash report context blobs (`CrashContext.runtime-xml`), and encrypted telemetry cookies. |
| `C:\Program Files\Riot Vanguard\` | Runtime binary directory; contains log files (`vgk.sys` logs) and persistent session signatures. |

### 20.3 Low-Level NTFS & Windows Forensic Artifacts

1. **NTFS Change Journal (`$UsnJrnl:$J`):**
   - The NTFS Change Journal logs every file creation, deletion, and truncation on the drive.
   - Merely deleting Riot folders leaves explicit delete records in `$UsnJrnl`. Vanguard's scanner inspects the journal to verify if security or trace files were recently purged.
2. **Master File Table ($MFT) Record History:**
   - Deleted file records remain in unallocated MFT record slots until overwritten.
3. **AppCompatCache / ShimCache (`HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\AppCompatCache`):**
   - Windows kernel cache recording every executable run on the system, including path, binary size, and last execution timestamp.
4. **Amcache (`C:\Windows\appcompat\Programs\Amcache.hve`):**
   - Stores cryptographic SHA-1 hashes of executables, PE header metadata, and compilation timestamps for all executed binaries and loaded drivers.
5. **System Event Log Records (`winevt\Logs\System.evtx` & `Application.evtx`):**
   - Event IDs `7045` (New service installed), `7000` (Service failed to start), and `TPM-WMI` event records (`Event ID 1024` for TPM provisioning).

### 20.4 Network Traces

| Trace | Cache / Storage Location | Invalidation / Flush Method |
|---|---|---|
| DNS Cache | Windows DNS Client Resolver Cache | `ipconfig /flushdns` |
| ARP Table | Kernel ARP Cache (`arp -a`) | `arp -d *` |
| Persistent TCP Connections | Windows Network Stack | System reboot |
| DHCP Lease History | Registry Lease Records | DHCP release / renewal or static IP reassignment |

### 20.5 Cleaner Execution Order

To fully sanitize a machine following a hardware ban:
1. **Uninstall Game & Services:** Uninstall VALORANT and Riot Vanguard via official uninstaller (unregisters `vgk` and `vgc` services).
2. **Purge Filesystem Traces:** Delete all directories under `ProgramData\Riot Games`, `AppData\Local\Riot Games`, `AppData\Local\VALORANT`, and `Program Files\Riot Vanguard`.
3. **Clean System Registries:** Delete `HKLM\SOFTWARE\Riot Games`, `HKCU\Software\Riot Games`, and reset `MachineGuid` / `HwProfileGuid`.
4. **Purge Windows Forensic Artifacts:** Flush ShimCache, clear Amcache (`Amcache.hve`), clear Prefetch (`Windows\Prefetch\`), and clear Windows Event Logs (`System.evtx`, `Application.evtx`).
5. **Reset NTFS Change Journal:** Reset or truncate `$UsnJrnl` via `fsutil usn deletejournal /d C:`.
6. **Flush Network Caches:** Flush DNS cache, reset ARP table, obtain new DHCP lease / public IP.
7. **Hardware & Firmware Re-attestation:**
   - Clear and reset TPM 2.0 state (fTPM clear in BIOS) to generate fresh endorsement key hierarchy.
   - Re-flash / spoof SMBIOS tables (UUID, Baseboard Serial, System Serial).
   - Spoof NVMe drive serials at storage miniport level.
   - Spoof NIC permanent MAC address.
   - Re-format drive partitions to regenerate Volume GUIDs.
8. **Fresh Account & Installation:** Install game on authenticated, unlinked account on the clean hardware state.

### 20.6 BSOD / BugCheck Artifacts

When Vanguard encounters fatal kernel integrity violations or anti-tamper failures, it triggers system crashes or fatal error halts:

| Error / BugCheck | Cause / Trigger Mechanism | Forensic Artifact Generated |
|---|---|---|
| **`VAN 128`** | Heartbeat desynchronization between `vgc.exe` and `vgk.sys`; DPC timer starved or driver frozen. | Process exit code + Riot Client telemetry log. |
| **`VAN 152` / `VAL 5`** | Hardware ban directive received from Riot server. | Client ban state cached in registry and cloud database. |
| **`BugCheck 0x109`** | `CRITICAL_STRUCTURE_CORRUPTION` triggered by Microsoft PatchGuard if DPC queues, IDT, or MSRs are hooked. | `%SystemRoot%\MEMORY.DMP` (Full memory dump). |
| **`BugCheck 0x133`** | `DPC_WATCHDOG_VIOLATION` triggered if CPU core is held at elevated IRQL to block Vanguard DPCs. | `%SystemRoot%\Minidump\*.dmp` (Mini crash dump). |
| **`BugCheck 0x7F`** | `DOUBLE_FAULT` triggered if unbacked code incurs a page fault while executing at `HIGH_LEVEL` in NMI. | `%SystemRoot%\MEMORY.DMP`. |

*Cleanup Requirement:* Always purge `%SystemRoot%\MEMORY.DMP` and `%SystemRoot%\Minidump\*.dmp` after any crash, as crash dumps contain full register state dumps, stack frames, and unbacked memory pages proving the execution of unauthorized kernel code.

---

## 21. Direct3D 11, DXGI & DWM Overlay Detection Architecture

Vanguard employs a multi-tiered inspection pipeline to detect external HUDs, ESP boxes, and menu canvases drawn over the game viewport:

### 21.1 Window Manager & Composition Auditing (DWM)
- **Window Extended Style Scanning:** `vgc.exe` continuously audits the system window hierarchy using `GetWindowLongPtr(GWL_EXSTYLE)`.
- Flags any external window possessing the following composite style combination positioned over `VALORANT.exe`'s screen bounding box:
  $$\text{WS\_EX\_TOPMOST} \mid \text{WS\_EX\_TRANSPARENT} \mid \text{WS\_EX\_LAYERED}$$
- **DWM Occlusion Queries:** Leverages Desktop Window Manager composition telemetry (`DwmGetCompositionTimingInfo` / DirectComposition APIs) to detect transparent swapchains rendering between the desktop composition root and the game canvas.

### 21.2 DXGI SwapChain & DirectX Hook Verification
- **`IDXGISwapChain::Present` VTable Inspection:** The game client audits its own DirectX 11 swapchain virtual method table. Any detour or trampoline redirecting `Present` (VTable Index 8) or `ResizeBuffers` (VTable Index 13) to unbacked memory triggers immediate abort.
- **Overlay DLL Whitelist:** Only cryptographically signed, pre-approved third-party overlay hooks are permitted to touch the swapchain:
  - Discord (`DiscordOverlayHook64.dll`)
  - OBS Studio (`graphics-hook64.dll`)
  - NVIDIA GeForce Experience (`nvspcap64.dll`)
  - Steam In-Game Overlay (`GameOverlayRenderer64.dll`)
- **Hook Hijacking Detection:** If a payload injects into a whitelisted host (e.g., Discord) to borrow its overlay rendering pipeline, Vanguard verifies the call chain and integrity of the hook module's code section against its signed on-disk binary.

---

## 22. Server-Side Fog of War & Network Entity Culling (PVS)

To render client-side memory scraping (including hardware DMA reading) ineffective against distant enemies, Riot implements server-authoritative spatial occlusion:

### 22.1 Potentially Visible Set (PVS) Raycasting
- The dedicated game server executes real-time 3D visibility raycasts between every player's eye coordinates and all other entities on the map.
- If an enemy player is completely occluded behind walls, solid geometry, or outside auditory perception range:
  - **The server completely halts transmission of the enemy's spatial coordinate packets.**
  - Enemy coordinate fields $(X, Y, Z)$, velocity vectors, and entity states **do not exist in client RAM**.

### 22.2 The Occlusion Envelope & De-Cloaking Thresholds
- To compensate for network round-trip time (RTT / Ping) and prevent "pop-in," the server maintains a dynamic predictive envelope:
  - As an enemy approaches an angle, corner, or threshold where they could become visible within $\Delta t = \text{Ping} + \text{InterpolationBuffer}$, the server begins transmitting position data.
  - Only when this spatial envelope is broken do enemy player pawns appear in client process memory, bounding ESP visualization exclusively to immediately peekable ranges.

---

## 23. Input Heuristics, Trajectory Modeling & Machine Learning Detection

Beyond driver and hardware validation, Riot deploys server-side and client-side behavioral modeling to detect algorithmic aiming aids:

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
- Software loops (`while(true) SendInput(...)`) create synthetic periodic timing clusters ($1.000\text{ms}, 2.000\text{ms}$) that diverge from the asynchronous clock drift of physical USB polling (e.g., 1000Hz polling rate micro-variance).

---

## 24. Boot Lifecycle (`SERVICE_BOOT_START`), Early-Load Order & Self-Integrity

The architectural necessity of Vanguard operating as a boot-start service:

### 24.1 Execution Sequence
```text
1. UEFI Firmware Initialization (Secure Boot & TPM PCR measurements)
   │
2. Windows Boot Manager (bootmgr)
   │
3. Windows OS Kernel (ntoskrnl.exe)
   │
4. ELAM (Early Launch Anti-Malware) Driver
   │
5. SERVICE_BOOT_START Drivers:
   ├── Critical Hardware & Storage Drivers (stornvme.sys)
   └── Riot Vanguard (vgk.sys)  <── Hooks registered BEFORE third-party drivers
   │
6. System Service Initialization (smss.exe, csrss.exe, winlogon.exe)
   │
7. Normal Service Drivers (SERVICE_SYSTEM_START / SERVICE_AUTO_START)
   │
8. User-Mode Desktop & Riot Vanguard Service (vgc.exe)
```

### 24.2 Strategic Advantages of Early Boot
1. **Preemptive Callback Registration:** `vgk.sys` registers `SeRegisterImageVerificationCallback` and `PsSetLoadImageNotifyRoutine` before any vulnerable third-party driver can load, ensuring all subsequent kernel modules pass through Vanguard's certificate filter.
2. **Clean Baseline Memory Snapshot:** Samples system page tables and kernel dispatch tables before any user-mode code runs, creating an uncorrupted reference baseline for later integrity checks.
3. **Self-Integrity Lock:** `vgk.sys` calculates hashes of its own `.text` and `.rdata` sections against its signed on-disk image. Any modification to `vgk.sys` on disk prevents driver initialization, halting game access until repaired.

---

## 25. Hardware Mouse Emulation & Out-of-Band (OOB) Input Vector Analysis

External cheats attempt to bypass software synthetic input detection (`SendInput`, `mouse_event`, `MouseClassServiceCallback` tampering) by using physical USB microcontrollers (MCU) acting as human interface devices (HID). Vanguard deploys an end-to-end hardware validation and bus topology inspection pipeline to identify and neutralize these out-of-band input devices:

### 25.1 Target Microcontroller Hardware Profiles
Cheats commonly deploy the following platforms to inject mouse coordinates over USB:
- **Arduino Leonardo / Pro Micro (ATmega32U4):** Uses AVR USB Core (`PluggableUSB` / `HID.cpp`).
- **Raspberry Pi Pico / RP2040:** Employs TinyUSB stack for dual CDC (serial) + HID composite profiles.
- **Teensy 3.x / 4.x (NXP i.MX RT1062):** High-speed USB stack supporting dual serial + mouse interfaces.
- **KMBox (B+ / NET / Pro) & Makemcu:** Commercial breakout boards pairing an onboard UART/Ethernet bridge with an MCU running hardware pass-through and coordinate injection firmware.

---

### 25.2 USB Descriptor & Endpoint Validation Pipeline
`vgk.sys` queries the USB device stack via USB Request Blocks (`URB`) and internal IOCTLs (`IOCTL_INTERNAL_USB_SUBMIT_URB`):

1. **Standard VID / PID Blacklisting & Fingerprinting:**
   - **Default Bootloader & Hardware IDs:**
     - Arduino Leonardo: `VID_2341&PID_8036` (Bootloader: `VID_2341&PID_0036`)
     - SparkFun Pro Micro: `VID_1B4F&PID_9206`
     - RP2040 TinyUSB: `VID_cafe` (standard test VID) or default Raspberry Pi `VID_2E8A`
     - KMBox / Makemcu default configurations: `VID_1A86&PID_7523` (CH340/CH341 UART bridge paired with MCU)
   - **Composite Interface Inspection (CDC Serial + HID Leaks):**
     - Real gaming mice present strictly as HID interfaces (often with manufacturer-specific feature reports for RGB/DPI).
     - Cheats require a control channel (COM port via UART/CDC) to receive target coordinates from an external PC or secondary user-mode script.
     - If a device declares a **USB Communications Device Class (CDC / Serial COM Port, Class `0x02`)** on the exact same physical USB composite root or parent hub as a mouse HID interface (Class `0x03`), Vanguard flags the device as an untrusted automation interface.

2. **USB Report Descriptor & Feature Anomalies:**
   - **Descriptor Byte-Pattern Checks:** Stock Arduino/TinyUSB report descriptors have static, well-known byte arrays:
     ```c
     // Stock Arduino ATmega32U4 Mouse Report Descriptor (44 bytes)
     0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00, 
     0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 
     0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05, 
     0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81, 
     0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06, 0xC0, 0xC0
     ```
     Even if VID/PID are spoofed to match Logitech (`VID_046D`) or Razer (`VID_1532`), failing to replicate the proprietary multi-page report descriptors (e.g., high-resolution scrolling, 16-bit X/Y coordinate fields, vendor usage pages) results in instant identification.
   - **Serial Number & String Descriptor Discrepancies:**
     - Real mice provide valid Unicode string descriptors for `iManufacturer`, `iProduct`, and `iSerialNumber`.
     - AVR/Pico firmwares often return empty strings, generic strings (`"Arduino LLC"`, `"Raspberry Pi"`), or static serials (`"HID Mouse"`).

---

### 25.3 USB Bus Topology & Hardware Interrupt Correlation
Vanguard does **not** emulate `mouhid.sys`; it cross-audits the real hardware input chain from the root controller down to `mouclass.sys`:

```text
Physical USB Root Hub (PCIe xHCI Host Controller - HalGetBusDataByOffset)
   │
USB Hub Driver (usbhub.sys / usbccgp.sys)
   │  ├── Intercepts URB_FUNCTION_SELECT_CONFIGURATION
   │  └── Validates USB Physical Device Object (PDO)
   │
HID Miniport (mouhid.sys)
   │  ├── Translates incoming USB interrupt transfers to MOUSE_INPUT_DATA
   │  └── Emits IRP_MJ_READ completion routines
   │
Mouse Class Driver (mouclass.sys)
   │  └── MouseClassServiceCallback (monitored by vgk.sys)
   │
Kernel Input Subsystem (win32kbase.sys Raw Input Thread - RIT)
   │
Target Window (VALORANT.exe - WM_INPUT)
```

1. **Direct USB Bus Topology Walk:**
   - `vgk.sys` enumerates `\Driver\usbhub` and `\Driver\usbccgp` device nodes.
   - Evaluates port tiering, hub depth, and transfer speeds:
     - Real high-performance gaming mice run at USB Full-Speed (12 Mbps) or High-Speed (480 Mbps) with 1000Hz–8000Hz bInterval values (1ms or microframe sub-millisecond intervals).
     - Low-end microcontrollers (e.g., bit-banged ATtiny or stock ATmega32U4) operate at USB Low-Speed (1.5 Mbps) or fixed 10ms intervals, failing high-refresh gaming profile checks.

2. **Dual-Mouse Conflict & Focus Arbitration:**
   - When a hardware cheat is plugged in, the OS has **two** active mouse devices registered under `\Device\PointerClass0` and `\Device\PointerClass1`:
     - Device A: The player's actual physical hand mouse.
     - Device B: The Arduino / KMBox / Makemcu injection hardware.
   - Vanguard inspects `mouclass.sys` device object lists. If inputs arrive simultaneously from two distinct physical device objects (`DEVICE_OBJECT`) while the player is aiming and shooting:
     - The physical mouse produces continuous micro-tremors and natural jitter.
     - The MCU injects sudden coordinate bursts on a separate `DEVICE_OBJECT` with zero sensor baseline.
     - This dual-source anomaly flags the session for hardware arbitration failure.

3. **Packet Inter-Arrival Timing & Polling Jitter Analysis:**
   - Hardware USB host controllers generate hardware interrupts (IRQs) tied strictly to bus clock cycles:
     - USB 1000Hz polling produces an interrupt packet every $1.000\text{ms} \pm 10\mu\text{s}$ (hardware clock drift).
   - In contrast, MCU coordinate injection over serial/COM port (e.g., Python script reading screen $\rightarrow$ writing to COM port $\rightarrow$ Arduino executing `Mouse.move()`):
     - Incurs Python execution latency, UART baud-rate serialization delay, and firmware buffer queuing.
     - Produces erratic, bursty inter-packet deltas ($\Delta t$ clustering around $16\text{ms}$, $8\text{ms}$, or non-uniform spikes) while claiming to be a continuous hardware mouse.

---

### 25.4 Hardware Spoofing & Countermeasure Matrix

| MCU Platform / Technique | Vanguard Detection Vector | Viable Spoofing Requirement |
|---|---|---|
| **Stock Arduino Leonardo** | Default `VID_2341&PID_8036`, generic Arduino report descriptor, CDC serial port exposed alongside HID. | Re-flash bootloader, spoof VID/PID, replace report descriptor, remove CDC serial code, receive coordinates via SPI/I2C. |
| **KMBox (B+ / NET)** | Dual COM port interface, default CH340 USB bridge IDs, predictable command packet framing. | KMBox Net isolates COM port to separate Ethernet interface, but network packet timing correlation and mouse pass-through descriptor anomalies still expose it. |
| **Raspberry Pi Pico (RP2040)** | TinyUSB default descriptors, USB CDC ACM interface presence, missing vendor-specific gaming endpoints. | Write custom bare-metal C USB stack, strip TinyUSB headers, match target gaming mouse HID descriptor byte-for-byte. |
| **Hardware Pass-Through (Host $\rightarrow$ MCU $\rightarrow$ PC)** | MCU intercepts real mouse and passes data through with deltas added. | Hardware latency adds measurable delay ($\Delta t > 2\text{ms}$) to USB interrupt frames; fails USB microframe timing audits if MCU processing is slow. |
| **Synthetic `MouseClassServiceCallback` Call** | Memory call-site is unbacked or lacks associated hardware `DEVICE_OBJECT` in `mouclass.sys`. | None — completely blocked by Vanguard kernel callback and caller-module verification. |
| **`SendInput` / `mouse_event`** | Flagged with `LLMHF_INJECTED` bit in `win32kbase.sys`; filtered out directly by Vanguard input hook. | None — user-mode injection is completely dead against Valorant. |

