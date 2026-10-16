# FACEIT Architecture & Detection Mechanics Reference

Technical analysis and architectural documentation of `FACEIT_AC.sys` and `FACEIT_IOMMU.sys` based on PE32+ header parsing, CodeView RSDS records, export hashing mechanisms, and kernel telemetry analysis.

---

## 1. Driver Identification & Cryptographic Provenance

| Parameter | `FACEIT_AC.sys` | `FACEIT_IOMMU.sys` |
|---|---|---|
| **File Path** | `C:\FACEIT_AC.sys` | `C:\FACEIT_IOMMU.sys` |
| **SHA-256** | `ED41C12543AB438EC2859BDF283BEEC325DB70DA1629C23435081B6227A0FE39` | `0707D97DB1B7A446E4103D05F13FEEDFFDC5F616DCF62D92323BFCFB74A60B57` |
| **File Size** | 108,395,208 bytes (~108.4 MB) | 16,926,920 bytes (~16.9 MB) |
| **PDB Path** | `C:\b\17203081316\driver\x64\Release\FACEIT_AC.pdb` | `C:\b\16891181560\driver\x64\Release\FACEIT_IOMMU.pdb` |
| **RSDS GUID** | `1b5d9657-6836-4179-9b1c-246977d1754c` | `8151c517-f17a-464b-b465-6c4c42bdbf4b` |
| **Digital Signer** | Microsoft Windows Hardware Compatibility Publisher (WHQL) | Microsoft Windows Hardware Compatibility Publisher (WHQL) |
| **CA Issuer** | Microsoft Windows Third Party Component CA 2014 | Microsoft Windows Third Party Component CA 2014 |
| **Architecture** | AMD64 (x86_64 Little-Endian) | AMD64 (x86_64 Little-Endian) |
| **ImageBase** | `0x0000000140000000` | `0x0000000140000000` |
| **SizeOfImage** | `0x06766000` (~108.4 MB) | `0x0102A000` (~17.0 MB) |
| **Entry Point** | `0x0010B000` (Section `INIT`) | `0x0006E000` (Section `INIT`) |
| **Virtualization Core** | Griffin Container (`.grfn1`) | Griffin Container (`.grfn1`) |

---

## 2. Griffin Virtualization Engine Architecture (`.grfn1`)

Both drivers are compiled with the proprietary **Griffin** kernel virtualization framework. Over 98% of the driver payload resides inside encrypted VM containers:

### 2.1 Section Breakdown

#### `FACEIT_AC.sys`
- `.text` (RVA `0x00001000`, Size: 663 KB): Entry stubs, unwinding gateways, and exception handling routines.
- `.rdata` (RVA `0x000A3000`, Size: 396 KB): Runtime descriptors, load configuration tables, and minimal IAT entries.
- `.data` (RVA `0x00104000`, Size: 15 KB): State variables and synchronization locks.
- `.pdata` (RVA `0x00108000`, Size: 10 KB): x64 function table descriptors used by `RtlVirtualUnwind`.
- `INIT` (RVA `0x0010B000`, Size: 790 bytes): Driver initialization entry prologue (`DriverEntry`).
- `.reloc` (RVA `0x0010C000`, Size: 140 bytes): Minimal base relocation directory.
- **`.grfn1` (RVA `0x0010D000`, Size: 107,317,584 bytes — 107.3 MB)**: Monolithic container containing bytecode interpreters, dynamic resolver tables, encrypted detection signatures, and state machines.

#### `FACEIT_IOMMU.sys`
- `.text` (RVA `0x00001000`, Size: 83.5 KB): Gateway functions and HAL/ACPI dispatch logic.
- `.rdata` (RVA `0x00016000`, Size: 341 KB): Read-only metadata and IAT descriptors.
- `.data` (RVA `0x0006A000`, Size: 9.5 KB): Active device mapping tables and physical range records.
- `INIT` (RVA `0x0006E000`, Size: 606 bytes): Bootstrap logic matching `FACEIT_AC.sys` with Griffin signature constant `0x2B992DDFA232B948`.
- **`.grfn1` (RVA `0x00070000`, Size: 16,487,200 bytes — 16.5 MB)**: Virtualized container handling ACPI DMAR/IVRS parsing, VT-d/AMD-Vi page tables, and DMA remapping configuration.

### 2.2 Zero-IAT Dynamic Export Hashing
Traditional static import analysis is prevented by stripping the Import Address Table down to a minimal bootstrap set:
- **`FACEIT_AC.sys`** imports only 20 routines from `ntoskrnl.exe`.
- **`FACEIT_IOMMU.sys`** imports only 15 routines from `ntoskrnl.exe`.
- Both drivers import `MmGetSystemRoutineAddress` and `ZwQuerySystemInformation`. All subsequent Windows executive and HAL APIs are resolved at runtime via internal hash calculations over the `ntoskrnl.exe` and `hal.dll` Export Address Tables (EAT).

---

## 3. Detected Injection & Modification Vectors

| Vector | Monitored / Detected? | Technical Implementation Mechanism |
|---|---|---|
| **LoadLibrary Injection** | YES | Trapped via `PsSetLoadImageNotifyRoutine`; module hashes cross-referenced with pre-authenticated module signatures. |
| **Manual Mapping (MDL / Physical)** | YES | Physical CR3 page table audits; scanning PTE entries where execution is enabled (`NX=0`) without a backing `SEC_IMAGE` VAD node. |
| **Reflective DLL Injection** | YES | Memory sweeps auditing `MEM_COMMIT` working set allocations for unlinked PE/MZ header signatures. |
| **APC Injection** | YES | Asynchronous Procedure Call delivery monitored; stack walking during kernel callback traps execution inside unbacked regions. |
| **Thread Context Hijacking** | YES | `ObRegisterCallbacks` strips `THREAD_SET_CONTEXT` and `THREAD_SUSPEND_RESUME`; RIP address validated against verified module bounds. |
| **Process Hollowing** | YES | Process creation callback audits initial image section address versus runtime PEB base pointer. |
| **Module Stomping** | YES | Periodic hash comparison of memory `.text` sections against disk binaries read via filesystem filters. |
| **Shellcode in Allocated Memory** | YES | Kernel stack unwinding via `RtlVirtualUnwind` traps call chains returning to dynamic heap/pool memory. |
| **Threadless Injection / PoolParty** | YES | Verification of thread pool worker execution addresses and I/O completion port callback targets. |

---

## 4. Kernel Driver & BYOVD Detection

| Driver Vector | Detected? | Implementation Mechanism |
|---|---|---|
| **Unsigned Drivers (DSE Bypass)** | YES | Enforced via Windows Driver Signature Enforcement (DSE) and `SeRegisterImageVerificationCallback`. |
| **Test-Signed Drivers** | YES | Verification of BCD execution parameters (`SystemBootEnvironmentInformation`) and `g_CiOptions` integrity. |
| **Known BYOVD Vulnerable Drivers** | YES | Internal encrypted hash database within `.grfn1` (matching CVE-associated drivers such as `RTCore64`, `dbk64`, `DBUtil`, `NAL`). |
| **Cheat Engine Kernel Driver (`dbk64.sys`)** | YES | Explicit device name lookup (`\Device\DBK64`), export signature parsing, and certificate blacklist. |
| **Kernel Pool Manual Mapping** | YES | Traversal of `NonPagedPool` allocations searching for unbacked executable memory blocks; callback stack unwinding. |
| **Revoked Certificate Drivers** | YES | Digital certificate validation against Microsoft Certificate Revocation Lists (CRL). |

---

## 5. Hypervisor & Virtualization Detection

| Technique | Detected? | Implementation Mechanism |
|---|---|---|
| **CPUID Leaf 0x1 Bit 31** | YES | Hardware execution: Hypervisor Present bit verified. |
| **Synthetic Hypervisor Signatures** | YES | CPUID `0x40000000` query verifying vendor strings. |
| **MSR Hooking (`LSTAR`, `SYSENTER`)** | YES | Direct `__readmsr` validation under interrupt-disabled execution barriers (`cli`/`sti`). |
| **Extended Page Table (EPT) Splitting** | PARTIAL | Memory translation latency checks; `INVLPG` execution coupled with physical readback comparison. |
| **TSC Timing Desynchronization** | YES | `RDTSC`/`RDTSCP` multi-sample delta measured against hardware timer references and `KUSER_SHARED_DATA`. |
| **Virtualization-Based Security (VBS)** | YES | Direct query of HVCI security state via `ZwQuerySystemInformation(SystemIsolatedUserModeInformation)`. |

---

## 6. Hardware DMA & IOMMU Architecture (`FACEIT_IOMMU.sys`)

A distinct structural design feature of this deployment is the inclusion of a standalone, specialized kernel driver (`FACEIT_IOMMU.sys`, 16.9 MB) devoted exclusively to hardware DMA isolation and translation table auditing:

```
                      +-------------------+
                      |   PCIe Bus Root   |
                      +---------+---------+
                                |
               +----------------+----------------+
               |                                 |
               v                                 v
      +-----------------+               +-----------------+
      | GPU / Legitimate|               | External DMA /  |
      |   Bus Master    |               | Rogue Endpoints |
      +--------+--------+               +--------+--------+
               |                                 |
               +----------------+----------------+
                                |
                                v
                +-------------------------------+
                |   IOMMU Remapping Engine      |
                |   (Intel VT-d / AMD-Vi)       |
                |   Audited by FACEIT_IOMMU.sys |
                +---------------+---------------+
                                |
                   [ Validated DMAR / IVRS ]
                                |
               +----------------+----------------+
               |                                 |
               v                                 v
      +-----------------+               +-----------------+
      | Process Memory  |               |  ACCESS DENIED  |
      |   (Authorized)  |               | (Hardware Fault)|
      +-----------------+               +-----------------+
```

### 6.1 Subsystem Responsibilities of `FACEIT_IOMMU.sys`
1. **Physical ACPI Table Parsing:**
   - Maps physical memory ranges directly to read ACPI **DMAR** (Intel Virtualization Technology for Directed I/O) and **IVRS** (AMD I/O Virtualization) tables.
   - Audits DMA Remapping Hardware Units (DRHD) and Reserved Memory Region Reporting (RMRR) descriptors.
2. **DMA Remapping Table Verification:**
   - Evaluates IOMMU second-level page tables to confirm device isolation.
   - Detects attempts to bypass memory protection by checking Bus Master enable states and PCIe Requester IDs.
3. **PCI Topology Sweeping:**
   - Iterates PCIe configuration space across all bus, device, and function ranges (using `HalGetBusDataByOffset` and MMIO ECAM) to verify device descriptors and BAR aperture sizes.

---

## 7. Monitored Kernel System Calls

Direct interception and policy enforcement on user-mode system routines targeting protected processes:

| System Routine | Enforcement Mechanism | Effect |
|---|---|---|
| `NtOpenProcess` | `ObRegisterCallbacks` | Access mask stripped of `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`, `PROCESS_DUP_HANDLE`. |
| `NtOpenThread` | `ObRegisterCallbacks` | Access mask stripped of `THREAD_SET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_DIRECT_IMPERSONATION`. |
| `NtDuplicateObject` | `ObRegisterCallbacks` | Prevents privilege escalation on duplicated handle structures. |
| `NtAllocateVirtualMemory` | Memory Management Audit | Non-backed executable allocations flagged in VAD tree. |
| `NtProtectVirtualMemory` | Page Table Sweeper | Detection of transitions to executable states (`PAGE_EXECUTE_READWRITE`). |
| `NtLoadDriver` | `SeRegisterImageVerificationCallback` | Image execution authorization checked prior to driver initialization. |

---

## 8. Registered Kernel Callbacks

| Callback Type | Registration Routine | Scope & Execution Point |
|---|---|---|
| **Process Filtering** | `ObRegisterCallbacks` | Synchronous evaluation on process handle creation and duplication. |
| **Thread Filtering** | `ObRegisterCallbacks` | Synchronous evaluation on thread handle creation and manipulation. |
| **Process Lifecycle** | `PsSetCreateProcessNotifyRoutineEx` | Intercepts process creation and teardown; validates image paths and PEB data. |
| **Thread Lifecycle** | `PsSetCreateThreadNotifyRoutine` | Audits thread initialization; validates start addresses against registered modules. |
| **Image Loading** | `PsSetLoadImageNotifyRoutine` | Evaluates all user-mode DLLs and kernel drivers prior to entry point execution. |
| **Image Verification** | `SeRegisterImageVerificationCallback` | Intercepts code-signing verification in the kernel image loading path. |
| **Filesystem Minifilter**| `FltRegisterFilter` | System-wide filesystem I/O monitoring; inspects file handles targeting monitored paths. |

---

## 9. Communication Channel Detection

Analysis of IPC (Inter-Process Communication), kernel-to-user transports, and cross-driver communication vectors monitored by `FACEIT_AC.sys` and `FACEIT_IOMMU.sys`:

| Communication Channel | Detected? | Inspection Mechanism & Technical Implementation |
|---|---|---|
| **Named Device Object** | YES | Object Manager directory traversal (`\Device\`, `\DosDevices\`) via `ZwOpenDirectoryObject` / `ZwQueryDirectoryObject` and `IoEnumerateDeviceObjectList`. |
| **Named Section (Shared Memory)** | YES | Auditing of `\BaseNamedObjects\` and system-wide handle table sweeps (`ZwQuerySystemInformation(0x10)`). Detects unbacked cross-process section mappings. |
| **Named Pipe** | YES | Directory enumeration under `\Device\NamedPipe\`; audits client process credentials and endpoint handles. |
| **Unnamed Device Object** | PARTIAL | Traversal of `DRIVER_OBJECT->DeviceObject` linked lists across loaded system drivers; detects hidden or unlinked device extensions. |
| **Unnamed Section (Kernel Handle)** | PARTIAL | Process handle tables audited; kernel-only handles (`OBJ_KERNEL_HANDLE`) avoid user handle sweeps but fail VAD backing checks when mapped into game space. |
| **MDL-Based Shared Memory** | YES | Target process VAD tree audit (`EPROCESS->VadRoot`). Any page mapped via `MmMapLockedPagesSpecifyCache` without a backing file object is flagged as an unbacked shared memory region. |
| **Usermode-Allocated Physical Translation (CR3 / PFN Mapping)** | PARTIAL (Transport: NO / Driver: YES) | Zero kernel object footprint in user space; memory allocated via `VirtualAlloc` in an external process (valid VAD node). The transport itself creates no handles or sections, but the resident entity performing physical PFN reads/writes is trapped by kernel pool scans (`NX=0` in `NonPagedPool`), stack unwinding, or hardware IOMMU isolation if attempted via PCIe DMA. |
| **Physical Memory Mapping (`\Device\PhysicalMemory`)** | YES | Access to `\Device\PhysicalMemory` is stripped via `ObRegisterCallbacks`. Direct CR3 page table modifications are trapped by physical PTE walks. |
| **Windows Notification Facility (WNF)** | PARTIAL | WNF state names are not globally hooked, but subscriber threads receiving state changes are caught via stack unwinding if execution dispatches to unbacked memory. |
| **PCIe BAR MMIO Signaling** | YES | **Enforced by `FACEIT_IOMMU.sys`**: Unmapped MMIO ranges assigned to rogue bus masters generate translation faults; device BAR configurations are cross-referenced with standard device schemas. |
| **Direct Cross-Driver Call** | YES | Stack unwinding (`RtlVirtualUnwind`) during kernel callbacks and timer DPCs traps call frames originating outside modules registered in `PsLoadedModuleList`. |
| **ALPC Ports** | YES | System-wide handle enumeration; communication ports connected to the game process must belong to authenticated Windows subsystems or the official FACEIT client. |

---

## 10. Anti-Debug & Hardware Breakpoint Auditing

| Mechanism | Target / Vector | Detection Implementation |
|---|---|---|
| **INT 1 SEH Trap** | Hardware Debugger / Single Step | Executed during driver initialization gate; if an external debugger handles the exception, the driver halts execution. |
| **INT 3 SEH Trap** | Software Breakpoints | Embedded breakpoint probes executed periodically; active debuggers consuming the interrupt fail driver state parity checks. |
| **Hardware Breakpoints (`DR0`–`DR7`)** | Thread Debug Registers | Thread contexts sampled via kernel DPC; any active debug registers (`DR0`, `DR1`, `DR2`, `DR3`) or enabled control bits in `DR7` are flagged immediately. |
| **`KdDebuggerEnabled` & `KdPitchDebugger`** | Windows Kernel Debugging | Direct kernel variable reads in `ntoskrnl.exe`. If kernel debugging is enabled via BCD (`/DEBUG`), the driver rejects initialization. |
| **RDTSC + CPUID Jitter Loops** | Hypervisor & Stepping Latency | Multi-sample timestamp deltas evaluated back-to-back across instruction boundaries; detects VM-Exit overhead and instruction single-stepping. |
| **`KUSER_SHARED_DATA` Timestamps** | Unhookable Clock Baseline | Direct reads from the read-only page `0x7FFE0000` (`SystemTime`, `InterruptTime`) to verify hardware clock continuity against TSC drift. |

---

## 11. Analysis Tool & Debugger Fingerprinting

| Target Tool | Detection Method in FACEIT AC |
|---|---|
| **Cheat Engine (`dbk64.sys`)** | Driver blacklist within `.grfn1`, window class enumeration, device object lookup (`\Device\DBK64`), and signature scanning. |
| **x64dbg / x32dbg** | Process executable image name scans, window titles, hardware breakpoint register traps (`DR0`–`DR7`). |
| **ReClass.NET** | Process name sweeps, handle table audits searching for open handles targeting protected game memory. |
| **Process Hacker / System Informer** | Driver blacklist (`kprocesshacker.sys`), open handle scans, process image naming heuristics. |
| **Kernel Debugger (WinDbg)** | `KdDebuggerEnabled` check, BCD environment verification, SEH debug traps. |
| **Frida / Dynamic Binary Instrumentation** | RDTSC timing detection, runtime trampoline and code cave sweeps, unbacked executable allocation flags. |

---

## 12. Detection Frequency & Polling Matrix

| Detection Subsystem | Execution Model | Frequency / Timing |
|---|---|---|
| **`ObRegisterCallbacks`** | Synchronous | Real-time: fires on every process and thread handle creation or duplication system-wide. |
| **Stack Walking (`RtlVirtualUnwind`)**| Intermittent / Sampled | Burst sampling during kernel callbacks and high-priority timer DPCs. |
| **CR3 Physical Page Walks** | Periodic Background Loop | Continuous cycles auditing target process page table entries and VAD consistency. |
| **Process & Thread Enumeration** | Continuous Loop | System-wide background sweeps over process structures and thread lists. |
| **Filesystem Minifilter** | Synchronous | Real-time: intercepts all filesystem I/O targeting game directories and system executables. |
| **PCIe Bus Configuration Sweeps** | Initialization + Periodic | Initialized at system boot by `FACEIT_IOMMU.sys` and verified periodically during game sessions. |
| **Heartbeat Verification** | Bidirectional Handshake | Continuous: cryptographic handshake exchanged between user-mode client service and `FACEIT_AC.sys` every 1000ms–2500ms. |

---

## 13. Hardware Fingerprinting Inventory

Telemetry collected to identify host environments:

| Identifier | Source / Interface | Extraction Method |
|---|---|---|
| **SMBIOS System UUID** | SMBIOS Type 1 | Physical memory mapping (`0x000F0000`–`0x000FFFFF`) + `ZwQuerySystemInformation(0x67)`. |
| **Motherboard Serial** | SMBIOS Type 2 | Direct structure offset parsing from SMBIOS table. |
| **Chassis Serial** | SMBIOS Type 3 | Structure parsing from SMBIOS table. |
| **Storage Serials** | NVMe / SATA Miniport | Direct dispatch of `IOCTL_STORAGE_QUERY_PROPERTY` to storage device stacks. |
| **Permanent MAC Address**| NDIS Driver Interface | Direct query of `OID_802_3_PERMANENT_ADDRESS` to network miniports. |
| **TPM 2.0 State** | TPM Base Services | Public key and PCR measurement queries via `Tbsip_Submit_Command`. |
| **Processor Identity** | CPUID Architecture | Leaf `0x1` and extended brand strings from leaves `0x80000002` through `0x80000004`. |
| **PCI Topology Hash** | PCIe Bus Hierarchy | Consolidated hash derived from bus/device/function enumerations and BAR descriptors. |

---

## 14. Deep Architectural Analysis of `FACEIT_IOMMU.sys`

`FACEIT_IOMMU.sys` represents a specialized hardware security layer operating directly at the interface between the operating system, chipset firmware, and the PCI Express root complex.

### 14.1 Physical ACPI Table & Topology Mapping
The driver executes low-level parsing of system firmware tables via physical memory translation:
- **Intel VT-d (`DMAR` Table):**
  - **DRHD (DMA Remapping Hardware Unit Definition):** Identifies every base address of the remapping hardware registers mapped in MMIO space. Verifies flags for `INCLUDE_PCI_ALL` to ensure no devices bypass the remapping units.
  - **RMRR (Reserved Memory Region Reporting):** Evaluates reserved memory ranges assigned to legacy devices (e.g., USB controllers or integrated graphics). Audits whether unauthorized addresses are being shielded under reserved regions.
  - **ATSR (Root Port ATS Capability Reporting):** Audits Address Translation Services (ATS) to detect unauthorized device-side translation caching.
- **AMD-Vi (`IVRS` Table):**
  - **IVHD (I/O Virtualization Hardware Definition):** Parses the physical MMIO aperture of AMD IOMMU hardware, device table base addresses, and event log buffers.
  - **Device Table Entries (DTE):** Inspects whether every peripheral endpoint has valid domain IDs, translation valid bits (`V=1`), and legitimate page table pointers.

### 14.2 Second-Level Address Translation (SLAT) & IOMMU Page Tables
`FACEIT_IOMMU.sys` verifies that IOMMU page tables correlate strictly with Windows Kernel DMA Protection rules:
- **Device Domains:** Enforces that unauthenticated PCIe devices (e.g., secondary slots, Thunderbolt, hot-plug bridges) are isolated into dedicated protection domains where DMA read/write access to system physical RAM is completely blocked.
- **Fault Trapping:** Intercepts IOMMU translation faults generated when an unauthorized endpoint issues a read or write request to memory pages outside its permitted scatter-gather list.

### 14.3 PCIe Configuration Space & Rogue Device Profiling
The driver conducts systematic bus walks across all 256 PCIe buses:
- Evaluates declared **Base Address Registers (BARs)** against standard device class specifications.
- Flags abnormal BAR aperture sizes, unexpected memory spaces, and anomalous command register configurations (e.g., Bus Master Enable bits forced active on unconfigured endpoints).
- Verifies PCIe Capability Pointers, Subsystem Vendor IDs, and Link State registers against known hardware vendor schemas to identify synthetic FPGA device identities.

---

## 15. Deep Architectural Analysis of `FACEIT_AC.sys`

`FACEIT_AC.sys` is the monolithic core anti-tamper and security enforcement engine (108.4 MB). 

### 15.1 The Monolithic Griffin Virtual Machine (`.grfn1`)
Over 107 MB of `FACEIT_AC.sys` is compiled into custom Griffin virtualization bytecode:
- **Bytecode Interpretation Core:** Code execution is shifted into a proprietary virtual machine containing randomized opcode handlers, dynamic register allocation, and multi-tier rolling encryption keys.
- **State Machine Integrity:** The VM maintains rolling state accumulators. Any deviation in expected execution state, missing hardware features, or unexpected thread returns results in state poisoning, triggering a silent telemetry flag or controlled driver unload.
- **Anti-Emulation & Timing Gates:** The VM frequently measures CPU instruction cycles across sensitive blocks using `RDTSC` and `RDTSCP` calibrated against standard processor frequency references. Execution latency typical of binary instrumentation frameworks (Frida, DBI, custom emulators) triggers validation failure.

### 15.2 Dynamic Kernel API Hashing Pipeline
`FACEIT_AC.sys` maintains zero plain-text references to Windows kernel routines:
- **Import Anchoring:** Relies strictly on `MmGetSystemRoutineAddress` and `ZwQuerySystemInformation` as base exports.
- **Dynamic Resolution:** At initialization, the Griffin core locates the base address of `ntoskrnl.exe`, `hal.dll`, and `fltmgr.sys` via internal module table traversals.
- **EAT Hash Traversal:** It computes custom cryptographic hashes over every function name in the target module's Export Address Table (EAT) and matches against its internal table of required routine hashes.
- **Inline Trampolines:** Resolved API function addresses are stored within the protected VM working space and accessed exclusively through ephemeral gateway stubs.

---

## 16. Memory Integrity, CR3 Physical Translation & Page Auditing

`FACEIT_AC.sys` bypasses standard operating system memory APIs by interacting directly with processor memory management structures:

### 16.1 Direct CR3 Page Table Walking
- **Paging Hierarchy Traversal:** High-priority kernel DPCs read the processor CR3 control register and physically walk the four-level paging structure:
  $$\text{CR3} \longrightarrow \text{PML4E} \longrightarrow \text{PDPTE} \longrightarrow \text{PDE} \longrightarrow \text{PTE}$$
- **Hardware Attribute Validation:** Scans page table entries (PTEs) for anomalies:
  - Validates the `No-Execute (NX)` bit (Bit 63). Any page where `NX=0` (executable) is cross-checked against the backing VAD.
  - Checks for inconsistencies between user/supervisor bits, write permissions, and dirty/accessed flags.
- **Direct Physical RAM Inspection:** Reads physical memory blocks directly, bypassing user-mode and kernel-mode page hooks (`VirtualQuery`, `ReadProcessMemory`, or software hooks).

### 16.2 Virtual Address Descriptor (VAD) Tree Cross-Validation
- Traverses the target process `EPROCESS->VadRoot` AVL tree.
- Cross-references every declared memory range against the corresponding physical page table entries:
  - Flags any executable allocation that is not backed by a valid `FILE_OBJECT` referencing an authenticated binary on disk.
  - Flags memory regions with mismatched protection states (e.g., page marked `PAGE_READONLY` in VAD but possessing write attributes in physical PTEs).

### 16.3 Section Hash Comparison (Disk vs Memory)
- Utilizes the kernel filesystem minifilter interface to read the original executable binary and signed DLLs directly from disk into an isolated buffer.
- Performs comparative SHA-256 and CRC32C hashing on `.text` and executable code sections:
  $$\Delta = \text{Hash}(\text{Disk Section}) \oplus \text{Hash}(\text{Memory Section})$$
- Excludes legitimate relocation blocks and runtime binding imports. Any divergence in actual code bytes indicates unauthorized patching or detours.

### 16.4 Anonymous Private Memory (`VirtualAlloc`) & Dual-Mapped PFN Shared Memory
- **VAD Classification (`VadNone` vs `VadImageMap`):** Memory allocated via `VirtualAlloc` is classified in the VAD tree as private anonymous memory (`VadNone`, backed by system paging file). Any executable transitions (`PAGE_EXECUTE_READWRITE`) in `VadNone` spaces are trapped by the physical PTE scanner.
- **Cross-Process PFN Collisions (Physical Dual-Mapping):** When a kernel driver creates a shared memory bridge by mapping the same physical Page Frame Numbers (PFNs) into both an external process and the protected game (via `MmMapLockedPagesSpecifyCache` or direct PTE fabrication):
  - `FACEIT_AC.sys` walks the CR3 page tables of both the game and monitored external processes.
  - Traverses the kernel physical page database (`MmPfnDatabase`).
  - Identifies PFN collision anomalies where distinct `EPROCESS` contexts point to identical physical PFN addresses without an authorized Windows shared Section Object (`ControlArea`) linking them.
- **Dangling / Ghost PTE Auditing:** If a driver inserts a PTE mapping directly into the hardware page tables while bypassing the Windows Memory Manager (leaving the process VAD tree untouched):
  - The CR3 page table walker finds a Present PTE (`Valid=1`).
  - The VAD cross-reference discovers no corresponding node in `EPROCESS->VadRoot`.
  - The discrepancy between hardware page tables and the executive VAD tree is flagged as a fatal memory manipulation fault.

---

## 17. Input Pipeline & Hardware Device Validation

To ensure fairness, `FACEIT_AC.sys` audits the complete input chain from physical USB hardware down to the Windows subsystem:

### 17.1 Mouse Device Object & Class Callback Auditing
- **`mouclass.sys` Inspection:** Audits the `DEVICE_OBJECT` chain linked under `\Device\PointerClass*`.
- **`MouseClassServiceCallback` Integrity:** Ensures the service callback address points directly within the bounds of `mouclass.sys` as listed in `PsLoadedModuleList`. Any inline detour or redirection to dynamic memory is trapped immediately.
- **Dual-Device Arbitration:** Detects concurrent mouse input packets arriving from multiple active device objects while the game viewport has focus, identifying auxiliary hardware injection devices operating alongside the player's physical mouse.

### 17.2 USB Descriptor & Endpoint Verification
- Queries the USB bus stack via Internal IOCTLs (`IOCTL_INTERNAL_USB_SUBMIT_URB`) to inspect device configuration descriptors.
- Identifies microcontroller profiles (ATmega32U4, RP2040, STM32, CH340/341 serial bridges) presenting composite interfaces (e.g., simultaneous CDC serial communication ports alongside HID interfaces).
- Audits report descriptors, vendor usage pages, and USB polling interval consistency ($1.000\text{ms} \pm \text{jitter}$ on 1000Hz hardware).

---

## 18. Hypervisor, Virtualization & Secure Boot Enforcement Standards

`FACEIT_AC.sys` enforces strict system configuration requirements before permitting game execution:

### 18.1 Hardware Virtualization Auditing
- Validates that CPUID virtualization leaves return consistent data across all physical cores.
- Executes `RDTSC` timing comparisons across instruction execution boundaries to measure latency introduced by VM-Exit handlers.
- Queries `SystemIsolatedUserModeInformation` to verify the operational state of Virtualization-Based Security (VBS) and Hypervisor-Protected Code Integrity (HVCI).

### 18.2 Secure Boot & Platform Integrity
- Queries TPM 2.0 Base Services (`Tbsip_Submit_Command`) to inspect Platform Configuration Registers (PCRs), specifically:
  - **PCR[0]:** Core firmware code and platform initialization measurements.
  - **PCR[7]:** Secure Boot policy, PK/KEK/db/dbx platform certificates.
- Verifies that BCD options have `TESTSIGNING` disabled, debugging disabled (`/DEBUG`), and Driver Signature Enforcement fully operational.

---

## 19. FACEIT Ban Traces & Forensic Artifacts Reference

Comprehensive mapping of forensic artifacts, driver objects, registry residues, and server-side telemetry generated by `FACEIT_AC.sys`, `FACEIT_IOMMU.sys`, and the user-mode client during execution and violation enforcement.

### 19.1 Registry Traces

| Registry Path | Description / Key Role | Persistence |
|---|---|---|
| `HKLM\SYSTEM\CurrentControlSet\Services\FACEIT` | Core driver service key for `FACEIT_AC.sys` (`ImagePath`, start type, error control). | Persistent |
| `HKLM\SYSTEM\CurrentControlSet\Services\FACEIT_IOMMU` | Hardware IOMMU driver service configuration key (`FACEIT_IOMMU.sys`). | Persistent |
| `HKLM\SOFTWARE\FACEIT` | Client installation settings, machine metadata, and client state flags. | Persistent |
| `HKLM\SOFTWARE\WOW6432Node\FACEIT` | 32-bit compatibility registry hive for client components. | Persistent |
| `HKCU\Software\FACEIT` | Current user cached session tokens, user IDs, and client preferences. | Persistent |
| `HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid` | Global OS installation GUID queried as a machine anchor. | Persistent |
| `HKLM\SYSTEM\CurrentControlSet\Control\IDConfigDB\Hardware Profiles\0001\HwProfileGuid` | Hardware profile GUID cross-checked with hardware telemetry. | Persistent |

### 19.2 Filesystem Traces

| Path | Contents / Forensic Nature |
|---|---|
| `C:\Program Files\FACEIT AC\` | Anti-cheat binary directory (`FACEITClient.exe`, service launchers, certificates). |
| `%LOCALAPPDATA%\FACEIT\` | Electron client runtimes, Squirrel update logs (`Squirrel-ProcessStart.log`), local packages, and update caches. |
| `%APPDATA%\FACEIT\` | User application state, session databases (`declarative_performance_observer.db`), IndexedDB, and diagnostic logs (`logs\main.log`). |
| `%SystemRoot%\System32\drivers\FACEIT_AC.sys` | Kernel-mode driver image (108.4 MB Griffin container). |
| `%SystemRoot%\System32\drivers\FACEIT_IOMMU.sys` | Hardware DMA / IOMMU isolation driver image (16.9 MB). |
| `%SystemRoot%\Prefetch\FACEIT*.pf` | Windows Prefetch execution history containing execution timestamps and loaded DLL maps. |
| `%SystemRoot%\MEMORY.DMP` | Full kernel crash dump generated if driver initiates a BugCheck on fatal tampering. |
| `%SystemRoot%\Minidump\*.dmp` | Triage minidumps recording exception context and register state. |

### 19.3 Kernel Object Traces

| Kernel Object | Object Manager Path | Lifetime |
|---|---|---|
| **AC Device Object** | `\Device\FACEIT` | Transient (Created at `DriverEntry`, destroyed on unload). |
| **AC Symbolic Link** | `\DosDevices\FACEIT` | Transient (Used for user-mode IOCTL communication). |
| **IOMMU Device Object** | `\Device\FACEIT_IOMMU` | Transient (Created during hardware translation initialization). |
| **IOMMU Symbolic Link**| `\DosDevices\FACEIT_IOMMU` | Transient (Administrative interface for DMA isolation status). |
| **Protected Section** | `\BaseNamedObjects\FACEIT_*` | Transient (Shared memory buffers for IPC with service). |

### 19.4 Certificate Store Auditing Residue
During driver verification and authenticode validation of background modules, `FACEIT_AC.sys` queries Windows system certificate stores:
- `HKLM\SOFTWARE\Microsoft\SystemCertificates\ROOT\Certificates\`
- `HKLM\SOFTWARE\Microsoft\SystemCertificates\AuthRoot\Certificates\`
- `HKLM\SOFTWARE\Microsoft\SystemCertificates\Disallowed\Certificates\`
Last-access timestamps on these keys record certificate revocation queries and catalog audits performed during driver startup.

### 19.5 Windows Operating System Artifacts (OS-Level)

| OS Subsystem | Artifact Location | Information Recorded |
|---|---|---|
| **ShimCache (AppCompatCache)** | `HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\AppCompatCache` | Binary execution history, full file path, size, and last modified timestamp. |
| **Amcache.hve** | `%SystemRoot%\AppCompat\Programs\Amcache.hve` | SHA-1 hashes of executed binaries, PE header fields, compile timestamps, and uninstalls. |
| **BAM / DAM** | `HKLM\SYSTEM\CurrentControlSet\Services\bam\State\UserSettings\*` | Background Activity Monitor logging first and last execution timestamps per user SID. |
| **Windows Event Logs** | `%SystemRoot%\System32\winevt\Logs\System.evtx` | Event ID 7045 (New Service Installed) logging `FACEIT` and `FACEIT_IOMMU` registrations. |
| **USN Journal** | NTFS Change Journal (`$UsnJrnl:$J`) | File creation, modification, and deletion records on NTFS volumes. |

### 19.6 Server-Side Telemetry & Enforcement Pipeline
Detection evidence and hardware identifiers are not evaluated purely locally; they are packaged into the secure telemetry stream:

```text
  [ Driver Telemetry Collection ]
  - SMBIOS UUID / Serial
  - Storage Serial / MAC
  - CR3 Walk Mismatch Records
  - TPM PCR[0], PCR[7] Readings
  - IOMMU Translation Fault Data
                 │
                 ▼
  [ Griffin Container Processing (.grfn1) ]
  - Payload serialized inside encrypted VM memory
  - Ephemeral session encryption key applied
                 │
                 ▼
  [ IOCTL Dispatch to Service ]
  - Transferred to user-mode service via \Device\FACEIT
                 │
                 ▼
  [ TLS Transport to FACEIT Backend ]
  - Encrypted transmission over TLS 1.3
  - Server evaluates hardware composite against global database
                 │
                 ▼
  [ Enforcement Directive ]
  - Server issues immediate or delayed account/hardware enforcement directive
  - Local client locks session and terminates game connection
```

### 19.7 Crash Dump & BugCheck Artifacts
When unrecoverable kernel tampering (e.g., unauthorized hook on dispatch tables, CR3 corruption, or driver code-patching) is detected, the kernel initiates a fatal BugCheck:
- Crash dumps (`%SystemRoot%\MEMORY.DMP`) contain full CPU register states (`RAX`, `RBX`, `RIP`, `RSP`, `CR3`, `CR4`), active page table structures, and loaded module lists.
- Windows Error Reporting (WER) logs the event under `%ProgramData%\Microsoft\Windows\WER\ReportQueue\` and records event entries in `System.evtx`.

---

## 20. Direct3D 11/12, DXGI & DWM Graphic Overlay Auditing

`FACEIT_AC.sys` and the associated client application deploy strict graphic pipeline verification to eliminate unauthorized external HUDs, ESP rendering, and transparent drawing canvases:

### 20.1 Desktop Window Manager (DWM) Extended Style Auditing
- The user-mode client continuously audits the Windows hierarchy via `GetWindowLongPtr(GWL_EXSTYLE)`.
- Identifies any window occupying coordinates over the game screen possessing composite transparent/click-through attributes:
  $$\text{WS\_EX\_TOPMOST} \mid \text{WS\_EX\_TRANSPARENT} \mid \text{WS\_EX\_LAYERED}$$
- DirectComposition and DWM occlusion telemetry are polled to identify transparent composition layers positioned directly between the game render swapchain and the desktop surface.

### 20.2 DXGI SwapChain Hook Auditing
- **VTable Integrity:** The virtual method table of `IDXGISwapChain` is continuously validated. Redirections of `IDXGISwapChain::Present` (VTable Index 8) or `ResizeBuffers` (VTable Index 13) to unbacked dynamic memory trigger immediate termination.
- **Signed Overlay Whitelist:** Only pre-approved, cryptographically signed third-party overlay binaries are permitted:
  - Steam In-Game Overlay (`GameOverlayRenderer64.dll`)
  - Discord Overlay (`DiscordOverlayHook64.dll`)
  - NVIDIA GeForce Experience (`nvspcap64.dll`)
- Code sections of authorized overlay DLLs are continuously hashed against their authenticated disk counterparts via filesystem minifilter reads to prevent overlay hijacking.

---

## 21. Server-Side Spatial Entity Occlusion (Fog of War / Anti-ESP)

In competitive titles hosted on the platform (such as Counter-Strike 2), client-side memory protection is augmented by custom server-side entity culling:

### 21.1 Real-Time Raycasted Potentially Visible Set (PVS)
- The dedicated game server calculates real-time 3D visibility vectors between all active players using map BSP, mesh geometry, and auditory perception radiuses.
- When an opponent is fully occluded behind solid structures:
  - **The server completely suppresses transmission of the opponent's spatial coordinate packets.**
  - World coordinate floats $(X, Y, Z)$, bone matrices, and velocity vectors **do not exist in the client machine's physical RAM**.
- Memory scanners (including hardware PCIe DMA cards) reading client memory encounter strictly null or stale entity structures for occluded players.

### 21.2 Dynamic Network De-Cloaking Envelope
- To eliminate player model "pop-in" caused by network latency, the server maintains an adaptive predictive boundary:
  $$\Delta t = \text{Ping} + \text{InterpolationTicks}$$
- Enemy coordinate streams are re-engaged precisely when an entity approaches a threshold where their model could become visible within $\Delta t$, restricting memory scraping exclusively to immediately peekable angles.

---

## 22. Client Error Codes & Integrity Violation Ledger

When `FACEIT_AC.sys` or the launcher halts execution, specific error directives are surfaced to the user:

| Error / Directive | Root Cause & System State |
|---|---|
| **"Secure Boot is required"** | BCD element query or TPM PCR[7] verification indicates Secure Boot is disabled in UEFI firmware. |
| **"TPM 2.0 is required"** | TPM Base Services (`Tbsip_Submit_Command`) query failed; hardware security chip missing or disabled. |
| **"The system has been started in test mode"** | `TESTSIGNING` or `/DEBUG` flag enabled in Windows Boot Configuration Data (BCD). |
| **"Blocked vulnerable driver: <name>"** | Loaded kernel driver matches the encrypted SHA-256 / certificate blacklist within `.grfn1`. |
| **"Service communication failed"** | Heartbeat desynchronization between user-mode client and `\Device\FACEIT` driver. |
| **"Forbidden driver signature"** | Kernel-mode driver loaded without valid Microsoft WHQL authenticode certificate. |

---

## 23. Kernel-Usermode Heartbeat & IPC Architecture (`\Device\FACEIT`)

The communication pipeline between the user-mode client and the kernel enforcement drivers:

### 23.1 Restricted Device Object Dispatch
- The device object `\Device\FACEIT` accepts `IRP_MJ_DEVICE_CONTROL` requests strictly from authenticated client processes.
- Handle requests targeting `\Device\FACEIT` are filtered via `SeQueryInformationToken` to confirm caller integrity level and process token signatures.

### 23.2 Cryptographic Heartbeat & Keep-Alive Tokens
- The user-mode client and `FACEIT_AC.sys` exchange encrypted heartbeat packets via IOCTL every **1000ms to 2500ms**.
- Each heartbeat carries a monotonic counter combined with ephemeral session keys negotiated inside the Griffin VM (`.grfn1`).
- If communication drops or responses arrive with abnormal timing deltas ($\Delta t > 3000\text{ms}$), the driver enters a defensive state, detaches from game memory, and terminates game execution.
