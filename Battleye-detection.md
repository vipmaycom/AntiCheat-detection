# BattlEye (BEDaisy) — Cheat Detection & Subsystems Reference

What BattlEye detects, how, and via what kernel calls. Based on static and dynamic reverse engineering of `BEDaisy.sys` (SHA-256: `e7bcf344eefa7a4ebea9b048e376958a5ad711489249033fa480a7ec8b6c018e`, 40.06 MB, PDB: `BEDaisy.pdb`, WHQL Attestation Signed by Microsoft Windows Hardware Compatibility Publisher).

---

## 1. Detected Injection Types

| Injection Method | Detected? | How |
|---|---|---|
| LoadLibrary | YES | `PsSetLoadImageNotifyRoutine` intercepts every module load; validates against system signature catalogs and dynamic module whitelist |
| Manual map (MDL/physical) | YES | Continuous CR3 physical page table audits (traverses PML4 -> PDPT -> PD -> PT); flags executable pages (`NX=0`) unbacked by a file-backed `VAD` node |
| Reflective DLL | YES | Working set memory scanner sweeps `MEM_COMMIT` private allocations lacking `SEC_IMAGE` backing; traps unlinked `MZ`/`PE` headers |
| APC injection | YES | Thread notification routines audit newly delivered APCs; stack unwinding on APC dispatcher targets unbacked executable memory |
| Thread hijack / SetThreadContext | YES | `ObRegisterCallbacks` strips `THREAD_SET_CONTEXT` and `THREAD_SUSPEND_RESUME`; periodic thread `RIP` validation against module bounds |
| Process hollowing | YES | `PsSetCreateProcessNotifyRoutineEx` + cross-checks `PEB->ImageBaseAddress` against original section file mapping during initialization |
| Module stomping | YES | Minifilter on-disk PE hash vs in-memory `.text`/`.rdata` page hash comparison (`CiValidateFileObject` / `FltReadFile`) |
| DLL sideloading | YES | Image load notification callback hashes loaded DLLs against Microsoft authenticode catalogs; flags unverified binaries in game tree |
| Shellcode in allocated memory | YES | Stack walking via `RtlVirtualUnwind` — traps return addresses resolving outside legitimate modules in `PsLoadedModuleList` |
| NtCreateSection-based | YES | Handle stripping blocks cross-process section mapping; view mappings audited in target process `VAD` tree |
| Atom bombing | PARTIAL | Memory content scanner catches unbacked payload in dynamic buffers; execution trapped when worker threads dispatch to dynamic memory |
| ThreadlessInject / PoolParty | YES | Threadpool worker factories and I/O completion port queues audited for function pointers resolving to unbacked memory pages |

---

## 2. Detected Driver Types

| Driver Type | Detected? | How |
|---|---|---|
| Unsigned driver (NtLoadDriver) | YES | Windows DSE enforced + `SeRegisterImageVerificationCallback` intercepts and rejects before execution |
| Test-signed driver | YES | BCD element query via `ZwQuerySystemInformation(SystemBootEnvironmentInformation)` + `g_CiOptions` verification |
| Known BYOVD (RTCore64, dbk64, Dell DBUtil, Intel NAL, GIGABYTE) | YES | Encrypted SHA-256 and driver name blacklists embedded inside VM container `SECTION.6`; Object Manager directory scans (`\Device\...`) |
| Unknown BYOVD with valid WHQL | PARTIAL | Dynamic behavioral heuristics: physical memory mapping (`\Device\PhysicalMemory`), handle duplication to protected PIDs, and dispatch pointer detours |
| Cheat Engine kernel (dbk64.sys) | YES | Device name check (`\Device\DBK64`), export signature check, and certificate blacklist |
| Manual-mapped driver (KDMapper/Capcom) | YES | Kernel pool allocation scan checks executable memory (`NX=0`) in `NonPagedPool`; callback stack unwinding verifies return addresses |
| Driver with dev build path PDB | YES | PE debug directory parsing in loaded drivers; matches against known development and public repository paths |
| Revoked-cert driver | YES | Certificate chain validation against Microsoft CRL and embedded revocation tables |

---

## 3. Detected Hypervisor Techniques

| Technique | Detected? | How |
|---|---|---|
| CPUID leaf 0x1 bit 31 (HV present) | YES | Direct instruction execution: bit 31 set flags hypervisor presence |
| Custom HV vendor string | YES | `CPUID 0x40000000` read — unknown or synthetic vendor strings flagged (`KVMKVMKVM`, `TCGTCGTCG`, custom strings) |
| CPUID feature filtering | YES | Cross-leaf consistency analysis — architectural feature dependencies checked across leaves |
| MSR hooking (LSTAR, SYSENTER_EIP) | YES | Read-modify-write cycle executed under `CLI`/`STI` barriers — readback mismatch = guest interception |
| EPT page splitting (execute vs read) | PARTIAL | `INVLPG` forces TLB miss → timing discrepancy measured against direct physical memory reads |
| TSC manipulation | YES | `RDTSC`/`RDTSCP` multi-sample delta compared with CPUID calibration and `KUSER_SHARED_DATA` timestamps |
| Hyper-V binary patching (hvix64/hvax64) | PARTIAL | Hypervisor status query via `ZwQuerySystemInformation(SystemIsolatedUserModeInformation)` and VBS state validation |
| Type-1 HV below Windows | PARTIAL | VM-exit latency penalties measured on high-frequency instruction cycles (`CPUID`, `CR3` reload) |
| VBS/HVCI state change | YES | Direct check via `VslGetSecurePciEnabled` and `ZwQuerySystemInformation` |
| HV shared page anomaly | YES | `NtQuerySystemInformation(0xC5)` — non-standard address pattern detection |
| ACPI table manipulation | YES | `HalAcpiGetTableEx` physical queries — checks SLIC, MADT, FADT, and DMAR tables |

---

## 4. Detected DMA / Firmware Techniques

| Technique | Detected? | How |
|---|---|---|
| PCILeech default FPGA (Xilinx ID 10EE) | YES | PCI config space scan (`HalGetBusDataByOffset` + MMIO ECAM) — default VID/PID combinations flagged on boot |
| Custom FPGA with spoofed vendor/device ID | PARTIAL | Bridge topology walking, Subsystem ID validation, class code verification, and abnormal BAR window size detection |
| Thunderbolt DMA adapter | YES | PCIe external hot-plug topology walk checks for unauthorized bus-mastering endpoints |
| IOMMU bypass | PARTIAL | VBS / Kernel DMA Protection check detects if IOMMU is disabled or inactive |
| DMA device with unusual BAR config | YES | Bus/Device/Function iteration cross-checks declared BAR aperture sizes against standard device classes |
| Modified UEFI firmware | YES | SMBIOS table structure validation paired with CPUID leaf matching and ACPI OEM identity verification |
| Bootkit (MBR/VBR modification) | YES | TPM PCR measurements, Secure Boot chain validation, disk boot sector hashing |
| UEFI bootkit (firmware DXE) | YES | TPM PCR[0-7] mismatch, firmware table parsing, TCG2 event log verification |
| Early-launch driver injection | YES | TPM PCR[11] mismatch + early-boot measurement comparison |
| BCD test-signing enabled | YES | Registry BCD element check (`SystemBootEnvironmentInformation`) |
| Modified ntoskrnl/securekernel on disk | YES | On-disk digital signature validation vs in-memory image hashing |

---

## 5. Kernel Calls BattlEye Monitors

Calls BattlEye watches for unauthorized interaction targeting protected game processes and `BEService.exe`:

| Call | Monitoring Method |
|---|---|
| NtOpenProcess | `ObRegisterCallbacks` — strips `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`, `PROCESS_DUP_HANDLE`, `PROCESS_CREATE_THREAD`, `PROCESS_TERMINATE` |
| NtOpenThread | `ObRegisterCallbacks` — strips `THREAD_SET_CONTEXT`, `THREAD_GET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_DIRECT_IMPERSONATION` |
| NtDuplicateObject | `ObRegisterCallbacks` — strips privileges on duplicated handles |
| NtWriteVirtualMemory | Blocked via handle stripping; direct kernel callers intercepted via stack unwinding |
| NtAllocateVirtualMemory | Blocked via handle stripping; kernel allocations audited in target process `VAD` tree |
| NtProtectVirtualMemory | Blocked via handle stripping; PTE sweeps detect unauthorized `RWX` transitions |
| PsCreateSystemThread | Thread start address verified; must reside within legitimate loaded module bounds |
| NtCreateThreadEx | Process notify routines audit newly staged threads in protected process context |
| NtSetContextThread | Blocked via handle stripping; kernel modifications caught during context validation |
| NtLoadDriver / ZwLoadDriver | `SeRegisterImageVerificationCallback` intercepts driver registration pre-execution |
| MmLoadSystemImage | Intercepted via the same driver image verification callback chain |
| MmCopyVirtualMemory | Audited via kernel callback stack inspection; caller must reside in authorized system module |

---

## 6. Kernel Calls BattlEye Makes (Detection Tools)

*Note: In `BEDaisy.sys`, only `RtlPcToFileHeader` is imported in the static IAT. All other kernel routines are resolved dynamically via custom rolling XOR/hash tables to prevent static import analysis.*

| Call | Usage |
|---|---|
| RtlPcToFileHeader | Static anchor; resolves base of owning driver image and identifies module boundaries |
| RtlWalkFrameChain | Stack frame capture during kernel callbacks and timer DPCs |
| RtlVirtualUnwind + RtlLookupFunctionEntry | Exact x64 stack unwinding adhering to `.pdata` exception tables |
| MmCopyVirtualMemory | Safe memory reading of game and external process pages from kernel space |
| MmIsAddressValid | Memory probe validating target addresses before dereferencing |
| PsLookupProcessByProcessId | PID resolution for process tree and parentage validation |
| PsLookupThreadByThreadId | TID resolution for thread context and stack inspection |
| PsGetProcessImageFileName | Process name extraction for blacklist matching |
| SeQueryInformationToken | Queries token privileges, integrity levels, and AppContainer SIDs |
| ObReferenceObjectByName | Named object lookup (`\Driver\...`, `\Device\...`) |
| HalGetBusDataByOffset | Reads PCI bus configuration space across all bus/device/function combinations |
| HalAcpiGetTableEx | Physical ACPI table parsing (MADT, SLIC, FADT, DMAR) |
| IoWMIOpenBlock + IoWMIQueryAllData | Queries SMBIOS and firmware raw data via WMI |
| FltRegisterFilter / FltReadFile | Minifilter filesystem operations; monitors disk reads and PE image loads in game directories |
| ZwQuerySystemInformation(0x05) | Full system process list enumeration |
| ZwQuerySystemInformation(0x0B) | Full loaded module list enumeration (`PsLoadedModuleList`) |
| ZwQuerySystemInformation(0x10) | System-wide open handle table enumeration |
| ZwQuerySystemInformation(0x67) | Physical firmware and SMBIOS table data queries |
| ZwQuerySystemInformation(0xC5) | Hypervisor shared page query |
| KeSetCoalescableTimer / KeSetTimerEx | Background DPC scheduling with pseudo-random millisecond jitter |

---

## 7. Kernel Callbacks Registered

| Callback | API | Scope |
|---|---|---|
| Handle filter | `ObRegisterCallbacks` | Synchronous handle filtering on `PsProcessType` and `PsThreadType`; strips read/write/context privileges targeting the game |
| Process notify | `PsSetCreateProcessNotifyRoutineEx` | System-wide process creation and exit monitoring; verifies parent-child relationships and command lines |
| Thread notify | `PsSetCreateThreadNotifyRoutine` | Intercepts thread creation; validates `StartAddress` against module boundaries in `PsLoadedModuleList` |
| Image load | `PsSetLoadImageNotifyRoutine` | Audits every DLL and driver loaded into user or kernel space before entry point execution |
| Image verification | `SeRegisterImageVerificationCallback` | Intercepts kernel-mode driver verification before execution starts |
| Filesystem minifilter | `FltRegisterFilter` | Inspects file I/O targeting game directories and system executables |
| I/O Control interceptor | `IRP_MJ_DEVICE_CONTROL` | Restricts `\Device\BEDaisy` communication strictly to authenticated `BEService.exe` tokens |

---

## 8. Memory Scanning Methods

| Method | What It Does |
|---|---|
| CR3 physical page table walk | Reads process CR3; walks PML4 -> PDPT -> PD -> PT in physical RAM, bypassing API hooks |
| PTE attribute verification | Scans kernel and process PTEs: flags executable pages (`NX=0`) lacking valid module file backing |
| SIMD content scanner (AVX2/SSE) | Scans memory regions using SSE2/AVX2 instructions searching for known cheat signatures and hook trampolines |
| PE on-disk vs in-memory comparison | Hashes in-memory `.text` sections and compares directly against on-disk signed files via minifilter reads |
| PE section integrity hash | Per-section hash with `.rsrc` and `.reloc` exclusion |
| Import/export table validation | Verifies that IAT/EAT function pointers resolve strictly within the owning module address space |
| VAD tree cross-referencing | Traverses `EPROCESS->VadRoot` AVL tree; cross-checks declared protections with physical PTE bits |
| Spinlock memory watchpoint | Detects concurrent access to monitored internal data structures |

---

## 9. Cheat Tool Detection

| Tool | Detection Method |
|---|---|
| Cheat Engine (dbk64.sys + process) | Process name scan, window title inspection, `\Device\DBK64` driver enumeration, certificate blacklist |
| x64dbg / x32dbg | Process name scan, window class checks, debug register hook monitoring (`DR0`–`DR7`) |
| ReClass.NET | Process name scan, handle table enumeration, memory read pattern tracking |
| Process Hacker / System Informer | Process name scan, driver blacklist (`kprocesshacker.sys`), open handle sweeps |
| IDA Pro / Ghidra | Window title detection, executable path scan, process name enumeration |
| Kernel debugger (WinDbg) | `KdDebuggerEnabled` check, BCD `/debug` element checks, INT 1/3 SEH traps |
| Frida / DBI | RDTSC timing detection, unbacked runtime code generation flags, trampoline detection |
| Custom debug tools | PDB debug directory scanning in PE headers, unbacked thread start address tracking |

---

## 10. Anti-Debug

| Check | Method |
|---|---|
| INT 1 SEH trap | Driver init gate — debugger consuming interrupt fails driver load |
| INT 3 SEH trap | Embedded breakpoint exceptions checked periodically; active debuggers consume interrupt and fail parity tests |
| RDTSC timing | Back-to-back with randomized loop — single-step/VM overhead detected |
| KdDebuggerEnabled | Direct kernel variable read — ever-set = flagged |
| RDTSC+CPUID quad check | Ratio comparison across CPUID instruction boundaries — hypervisor exit latency detected |
| WRMSR+RDPMC | Performance counter timing channel |
| KUSER_SHARED_DATA timestamps | Read directly from unhookable system memory page (`0x7FFE0000`) |

---

## 11. Communication Channel Detection

| Channel | Detected? | How |
|---|---|---|
| Named device object | YES | `\Device\` and `\DosDevices\` directory enumeration, `IoEnumerateDeviceObjectList` |
| Named section (shared memory) | YES | Handle table scan and Object Manager directory walks |
| Named pipe | YES | `\Device\NamedPipe\` enumeration and handle table audits |
| Unnamed device | PARTIAL | `IoEnumerateDeviceObjectList` sweeps |
| Unnamed section (kernel handle) | PARTIAL | Handle visible if not kernel-only |
| MDL-based shared page | NO | No handle, no section object |
| Physical memory mapping | NO | No handle, no section, no object; driver maps via CR3/PFN |
| WNF state data | PARTIAL | Transport unmonitored (`ExpWnf*`), but receiver thread caught via stack walking if unbacked |
| PCI BAR register signaling | NO | Outside OS object model |
| Direct function call between drivers | YES | Stack frame capture (`RtlVirtualUnwind`) catches calls originating outside module list |

---

## 12. Detection Timing

| Detection | Frequency |
|---|---|
| ObRegisterCallbacks | Synchronous: executes on every process/thread handle operation system-wide |
| Stack walking | Burst sampling in kernel callbacks and periodic timer routines |
| Process scanning | Continuous background cycle |
| Thread enumeration | Continuous background cycle |
| Memory integrity | Periodic: full cycle across monitored modules |
| Module blacklist | Periodic passes over `PsLoadedModuleList` |
| PCI config scan | Boot initialization + periodic polling |
| Filesystem minifilter | Synchronous on every file I/O targeting game directories |
| RDTSC timing | Driver initialization gate + periodic jitter loops |
| Heartbeat verification | High-frequency: bidirectional handshake between usermode `BEService.exe` and `BEDaisy.sys` every 1000ms–3000ms |

---

## 13. BattlEye — HWID Identifiers & Spoofing

Every hardware identifier BattlEye collects, its kernel extraction mechanism, format, and spoofing requirements.

### Collected Identifiers

| Identifier | Source | Format String / Type | Extraction Address / Method | Spoof Method |
|---|---|---|---|---|
| **SMBIOS UUID** | Type 1 System Info | GUID / 128-bit hex | Direct physical RAM map (`0x000F0000`–`0x000FFFFF`) + `ZwQuerySystemInformation(0x67)` | Firmware table hook (`IoWMIOpenBlock` intercept) or BIOS mod |
| **SMBIOS System Serial** | Type 1 System Info | Variable ASCII string | Offset `0x07` in Type 1 structure | Same as UUID — must match manufacturer |
| **SMBIOS Board Serial** | Type 2 Baseboard | Variable ASCII string | Offset `0x07` in Type 2 structure | Firmware table hook / DMI BIOS mod |
| **SMBIOS Enclosure Serial** | Type 3 Enclosure | Variable ASCII string | Offset `0x06` in Type 3 structure | Firmware table hook |
| **Memory Serial** | Type 17 Memory Device | Hex-encoded SPD serial | Offset `0x18` in Type 17 structure | Firmware table hook — change per-DIMM |
| **Processor ID** | Type 4 / CPUID Leaf 1 EAX | 32-bit CPU signature | CPUID instruction + Type 4 offset `0x08` | HV CPUID intercept — must match SMBIOS Type 4 |
| **Processor Brand String** | CPUID 0x80000002-4 | 48-char ASCII string | Direct CPUID instruction | HV CPUID intercept |
| **MBR Disk Signature** | MBR sector 0 offset 0x1B8 | `MBR:%08x` | Read via `IOCTL_DISK_GET_DRIVE_GEOMETRY` | Hook IOCTL or write MBR directly |
| **SCSI LUN** | SCSI inquiry | `:lun=%llx` | SCSI miniport query | Storage miniport filter |
| **Disk Serial** | SCSI inquiry / SMART | 20-char ASCII (byte-swapped) | `IOCTL_STORAGE_QUERY_PROPERTY` + `IOCTL_SCSI_MINIPORT` | Storage miniport filter (`\Driver\stornvme`, `iaStorAC`) |
| **Partition Index / GUID** | Partition table | `Part:%u` / GUID | `IOCTL_DISK_GET_PARTITION_INFO_EX` + NTFS `$Volume` | Partition table metadata mod / reformat |
| **MAC Address** | NDIS permanent | `%02x:%02x:%02x:%02x:%02x:%02x` | Direct NDIS OID `OID_802_3_PERMANENT_ADDRESS` | NDIS LWF driver intercepting permanent address OID |
| **IPv4 Address** | Adapter config | `%u.%u.%u.%u` | Windows network stack | Adapter reconfig / DHCP renewal (unreliable ban ID) |
| **GPU Hardware UUID** | Graphics runtime | Hexadecimal UUID | `dxgkrnl.sys!DxgkQueryAdapterInfo` | Graphics miniport driver hook |
| **TPM Endorsement Key** | TPM Base Services | 256-bit SHA-256 hash | `Tbsip_Submit_Command` (`TPM2_CC_ReadPublic`) | No software spoof on Win11 HVCI — clear fTPM NVRAM or replace chip |
| **TPM PCR Values** | TCG 2.0 event log | PCR[0], PCR[7] | `Tbsip_Submit_Command` (`TPM2_PCR_Read`) | Hardware-sealed — re-sealed by firmware boot |
| **PCI Device IDs** | HalGetBusDataByOffset + MMIO | vendor:device pairs | Iterating bus 0 to 255 | DMA device firmware config space spoof |
| **CPUID Feature Flags** | Leaves 0x1, 0x7 | Bitmask | Direct CPUID execution | HV CPUID passthrough — do not filter |
| **CPUID Vendor String** | Leaf 0x0 | 12-char string | Direct CPUID execution | HV CPUID passthrough |

### SMBIOS Types Parsed

| Type | Name | Key Fields |
|---|---|---|
| 0 | BIOS Information | Vendor, version, release date, ROM size |
| 1 | System Information | Manufacturer, product name, version, **serial**, **UUID** |
| 2 | Baseboard | Manufacturer, product, version, **serial**, asset tag |
| 3 | System Enclosure | Manufacturer, type, version, **serial**, asset tag |
| 4 | Processor | Manufacturer, version, **processor ID**, core count |
| 11 | OEM Strings | All OEM entries |
| 17 | Memory Device | Manufacturer, **serial**, part number, size, locator |
| 127 | End-of-Table | Sentinel |

Collected via direct physical RAM mapping at `0x000F0000`–`0x000FFFFF` (`_SM_`/`_SM3_` anchor search) + `IoWMIOpenBlock(MSSmBios_RawSMBiosTables)` + `ZwQuerySystemInformation(0x67)`.

### CPUID Leaves Collected

| Leaf | Data | Ban Relevance |
|---|---|---|
| 0x00 | Vendor (`GenuineIntel` / `AuthenticAMD`) | Base architecture fingerprint |
| 0x01 | Family/Model/Stepping + features | CPU identity + Hypervisor presence bit (ECX[31]) |
| 0x02-0x0F | Cache, TLB, topology | Hardware topology cross-checked with ACPI MADT table |
| 0x17+ | Extended HW fingerprinting (SOC vendor) | Deep CPU fingerprint |
| 0x40000000 | Hypervisor vendor | Hyper-V, VMware, KVM identification |
| 0x80000002-4 | Brand string ("Intel Core...", "AMD Ryzen...") | CPU name cross-checked with SMBIOS Type 4 |

### PCI Devices Fingerprinted

Full iteration across 256 buses, 32 devices, 8 functions (512 reads via `HalGetBusDataByOffset` and MMIO ECAM space).

**Known flagged devices:**

| Vendor | Device | Hardware Entity |
|---|---|---|
| 0x15AD | 0x0405 | VMware SVGA |
| 0x15AD | 0x0740 | VMware vmxnet3 |
| 0x15AD | 0x07B0 | VMware PVSCSI |
| 0x1AF4 | 0x1000-0x107F | VirtIO (KVM/QEMU) |
| 0x1414 | 0x5353 | Hyper-V video |
| 0x1B36 | * | QEMU PCI bridge |
| 0x10EE | Default | Xilinx default DMA bitstream (PCILeech/Screamer) |

### Ban Enforcement Pipeline

```text
1. Driver samples hardware identifiers (SMBIOS UUID, Baseboard Serial, NVMe Serial, NIC MAC)
2. Collects boot measurements (Secure Boot state, PCR values, HVCI status)
3. Encrypts composite HWID payload with session key schedule inside monolithic VM container (SECTION.6)
4. Dispatches payload to BEService.exe via private IOCTL
5. User-mode BEService transmits payload over TLS to BattlEye master backend
6. Server evaluates hardware hash against global ban database
7. If match found: Server issues ban directive
8. Local driver terminates game process and records security flag
```

### Spoof Priority Checklist

| Priority | Identifier | Why |
|---|---|---|
| CRITICAL | SMBIOS UUID (Type 1) | Primary hardware fingerprint anchor |
| CRITICAL | SMBIOS System Serial (Type 1) | Primary hardware fingerprint anchor |
| CRITICAL | SMBIOS Board Serial (Type 2) | Core motherboard fingerprint |
| HIGH | MBR Disk Signature | Explicitly collected sector signature |
| HIGH | MAC Address (permanent) | Explicitly collected via NDIS OIDs — must spoof before driver loads |
| HIGH | Disk Serial (NVMe / SCSI) | Primary composite storage fingerprint |
| MEDIUM | SMBIOS Memory Serial (Type 17) | Per-DIMM serial in fingerprint |
| MEDIUM | CPUID Processor ID | Must match SMBIOS Type 4 |
| MEDIUM | CPUID Brand String | Cross-validated with SMBIOS Type 4 |
| MEDIUM | GPU Hardware UUID | Queried via dxgkrnl.sys |
| MEDIUM | PCI Device IDs | DMA setups only |
| LOW | IPv4 Address | Changes with DHCP — weak identifier |
| LOW | Partition Index / Volume GUID | Re-created upon filesystem reformat |
| SKIP | TPM Endorsement Key | Hardware-sealed — clear fTPM NVRAM in BIOS or replace chip |

### Cross-Validation Requirements

BattlEye cross-checks hardware data across multiple kernel paths. Inconsistencies trigger integrity flags:

- SMBIOS Type 4 Processor **must match** physical CPUID leaves
- SMBIOS Manufacturer **must be consistent** with ACPI OEM tables
- MAC OUI prefix **must match** a real, registered hardware vendor
- Disk serial **must be consistent** with controller type (NVMe spec vs ATA byte-swapping)
- All identifiers **must change together** to represent an authentic "new machine"
- SMBIOS firmware tables **must pass** internal checksum and tag validation

---

## 14. BattlEye — Ban Traces & Cleaner Reference

Everything BattlEye leaves on the system that requires cleaning after a ban.

### Registry Traces

| Path | What | Clean Method |
|---|---|---|
| `HKLM\SYSTEM\CurrentControlSet\Services\BEDaisy` | Driver service key + detection records | Delete entire key tree |
| `HKLM\SYSTEM\CurrentControlSet\Services\BEService` | User-mode service registration key | Delete entire key tree |
| `HKLM\SOFTWARE\WOW6432Node\BattlEye` | Client installation data + machine telemetry tokens | Delete entire key |
| `HKCU\Software\BattlEye` | User-level cached session tokens and HWID anchors | Delete entire key |
| `HKLM\SOFTWARE\Microsoft\Cryptography` | `MachineGuid` (global OS installation GUID) | Regenerate fresh GUID |
| `HKLM\SYSTEM\CurrentControlSet\Control\IDConfigDB\Hardware Profiles\0001` | `HwProfileGuid` | Regenerate fresh GUID |
| `HKLM\SYSTEM\MountedDevices` | `\??\Volume{GUID}` | System volume GUID associations |

### Filesystem Traces

| Path | What | Clean Method |
|---|---|---|
| `C:\Program Files (x86)\Common Files\BattlEye\` | Core updater, certificates, BEService executable | Delete folder |
| `%LOCALAPPDATA%\BattlEye\` | Cached detection data and local logs | Delete folder |
| `%SystemRoot%\System32\drivers\BEDaisy.sys` | Kernel driver binary | Delete |
| `%SystemRoot%\Prefetch\BEDAISY*.pf` | Windows prefetch execution traces | Delete matching `.pf` files |
| `%SystemRoot%\Prefetch\BESERVICE*.pf` | Windows prefetch execution traces | Delete matching `.pf` files |
| `%SystemRoot%\MEMORY.DMP` | Crash dump — contains bugcheck records if triggered | Delete if present |
| `%SystemRoot%\Minidump\*.dmp` | Mini crash dumps | Delete timestamped dumps |
| `%SystemRoot%\System32\winevt\Logs\System.evtx` | Windows event logs (service install Event ID 7045) | Clear or selectively delete |
| Game install dir | Per-game `BattlEye` subfolder and configs | Delete subfolder |

### Certificate Store Traces

BattlEye reads Windows certificate stores during driver verification:

| Store | Registry Path |
|---|---|
| ROOT | `HKLM\SOFTWARE\Microsoft\SystemCertificates\ROOT\Certificates\` |
| AuthRoot | `HKLM\SOFTWARE\Microsoft\SystemCertificates\AuthRoot\Certificates\` |
| TrustedAppRoot | `HKLM\SOFTWARE\Microsoft\SystemCertificates\TrustedAppRoot\Certificates\` |
| Disallowed | `HKLM\SOFTWARE\Microsoft\SystemCertificates\Disallowed\Certificates\` |

Last-access timestamps on these keys record BattlEye reading revocation lists and catalogs. Forensic indicator.

### Kernel Object Traces

| Object | Path | Lifetime |
|---|---|---|
| Device object | `\Device\BEDaisy` | Deleted on driver unload |
| Symbolic link | `\DosDevices\BEDaisy` | Deleted on driver unload |
| MiniFilter instance | FltMgr altitude registration | Unregistered on teardown |

These are transient — exist only while driver is actively running. No persistent cleanup required.

### Kernel Callback Residue

| Callback | Registration |
|---|---|
| ObRegisterCallbacks | Unregistered on driver unload |
| PsSetCreateProcessNotifyRoutineEx | Unregistered on driver unload |
| PsSetCreateThreadNotifyRoutine | Unregistered on driver unload |
| PsSetLoadImageNotifyRoutine | Unregistered on driver unload |
| SeRegisterImageVerificationCallback | Unregistered on driver unload |
| FltRegisterFilter | Unregistered on driver unload |

All unregistered on clean driver unload. If the driver crashes or the system halts abruptly, reboot clears them from kernel memory.

### Windows Artifacts (OS-Level)

| Artifact | Location | Clean Method |
|---|---|---|
| Prefetch DB | `%SystemRoot%\Prefetch\` | Delete all `BEDAISY*.pf` and `BESERVICE*.pf` files |
| Amcache.hve | `%SystemRoot%\AppCompat\Programs\Amcache.hve` | Delete driver entries (requires offline hive edit or cleaner) |
| ShimCache | `HKLM\SYSTEM\CCS\Control\Session Manager\AppCompatCache` | Flush cache (reboot clears volatile entries) |
| SRUM (System Resource Usage Monitor) | `%SystemRoot%\System32\sru\SRUDB.dat` | Delete or clear execution records |
| BAM/DAM | `HKLM\SYSTEM\CCS\Services\bam\State\UserSettings\*` | Delete execution entries |
| UserAssist | `HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\UserAssist\*` | ROT13-encoded, delete matching GUIDs |
| Jump Lists | `%APPDATA%\Microsoft\Windows\Recent\AutomaticDestinations\` | Delete game launcher entries |
| USN Journal | NTFS volume journal (`$UsnJrnl:$J`) | Reset journal via `fsutil usn deletejournal /d C:` |
| Event Logs (all) | `%SystemRoot%\System32\winevt\Logs\` | Clear System + Application logs |

### Cleaner Execution Order

1. **Uninstall Game + BattlEye:** Run official uninstaller and remove game.
2. **Delete BattlEye Folders:** Delete `C:\Program Files (x86)\Common Files\BattlEye`, game install subfolders, `%LOCALAPPDATA%\BattlEye`.
3. **Clean Registry:** Delete service keys (`BEDaisy`, `BEService`), software keys, and regenerate `MachineGuid` and `HwProfileGuid`.
4. **Clean OS Artifacts:** Clear Prefetch, Amcache (`Amcache.hve`), ShimCache, SRUM, and Windows Event Logs.
5. **Reset NTFS Change Journal:** Execute `fsutil usn deletejournal /d C:`.
6. **Flush Network:** Flush DNS (`ipconfig /flushdns`), reset ARP (`arp -d *`), cycle public IP.
7. **Spoof HWIDs:** Spoof SMBIOS tables (Type 1 UUID/Serial, Type 2 Board), storage serials (NVMe/SCSI), and permanent MAC address before reinstalling.
8. **New Account:** Create fresh, unlinked game account on the clean hardware state.
9. **Reinstall:** Fresh game and BattlEye installation over the sanitized hardware baseline.

---

## 15. Advanced Subsystems: Overlays, Hardware Input & VM Architecture

### 15.1 Direct3D / DXGI & DWM Overlay Detection Architecture
- **Window Extended Style Scanning:** `BEService.exe` audits the system window hierarchy using `GetWindowLongPtr(GWL_EXSTYLE)`. Flags external windows combining:
  $$\text{WS\_EX\_TOPMOST} \mid \text{WS\_EX\_TRANSPARENT} \mid \text{WS\_EX\_LAYERED}$$
- **DXGI SwapChain VTable Integrity:** Audits `Present` (Index 8) and `ResizeBuffers` (Index 13). Any hook redirecting to unbacked memory triggers detection.
- **Overlay DLL Whitelist:** Only approved overlays are permitted (Discord, OBS, Steam, GeForce Experience). Module on-disk hashes are verified to prevent hook hijacking.

### 15.2 Hardware Mouse Emulation & Microcontroller (MCU) Input
- **Target Hardware Profiles:** Arduino Leonardo (ATmega32U4), Raspberry Pi Pico (RP2040 TinyUSB), Teensy, KMBox (B+/NET/Pro).
- **Composite Interface Inspection (CDC Serial + HID):** If a physical USB device declares a CDC Serial Port (Class `0x02`) on the same physical composite root as a Mouse HID interface (Class `0x03`), it is flagged as an automation tool.
- **Dual-Mouse Conflict:** Inspects `mouclass.sys` device object lists. Simultaneous inputs arriving from two distinct `DEVICE_OBJECT` pointers (`\Device\PointerClass0` and `\Device\PointerClass1`) flag arbitration anomalies.
- **Polling Jitter:** Hardware mice produce clock-steady $1.000\text{ms} \pm 10\mu\text{s}$ interrupt deltas. Microcontrollers injecting coordinates over serial connections incur latency clusters ($\Delta t \approx 8\text{ms}$–$16\text{ms}$).

### 15.3 BattlEye VM Architecture (`SECTION.6` Monolithic Container)
- **Monolithic Container:** Houses 39.8 MB of custom bytecode inside an unnamed section (`0x140031000`–`0x14261cfff`).
- **Anti-Disassembly Jump Cascade:** Thousands of chained 5-byte `JMP rel32` thunks designed to exhaust static decompiler call stacks.
- **Zero-IAT Decoupling:** Single static import (`ntoskrnl.exe!RtlPcToFileHeader`). All other kernel APIs resolved dynamically via custom hashes during driver bootstrap.
- **Context Serialization:** Entry dispatcher allocates a `0xB8` byte frame, serializes vector registers `XMM6`-`XMM8`, executes rolling rotation arithmetic (`ROL`, `CMOVO`), and dispatches bytecode execution before returning via indirect register jump (`JMP RBX`).

---

## 16. Heartbeat Protocol & Watchdog Architecture

BattlEye enforces continuous liveness synchronization between the kernel driver (`BEDaisy.sys`), the usermode service (`BEService.exe`), and the dedicated game server to prevent tampering, driver freeze, or debugger pause.

### 16.1 Bidirectional Synchronization Pipeline

```text
Game Client / Engine Loop (BEService.exe)
   │  ▲
   │  │  1. High-frequency IOCTL query (0x1000ms–0x3000ms interval)
   ▼  │  2. Response with encrypted heartbeat frame + challenge nonce
Kernel Driver (BEDaisy.sys Dispatcher)
   │  ▲
   │  │  3. DPC timer fires (KeSetCoalescableTimer / KeSetTimerEx)
   ▼  │  4. Kernel state verification (watchdog counters updated)
Dedicated Game Server (BattlEye Master Network Thread)
```

1. **Kernel DPC Scheduling:**
   - Background monitoring routines are scheduled via Deferred Procedure Calls (DPCs) using `KeSetCoalescableTimer` or `KeSetTimerEx`.
   - Pseudo-random millisecond jitter ($\Delta t \in [1000\text{ms}, 3000\text{ms}]$) is added to every timer interval to prevent deterministic thread prediction or hook synchronization.
2. **Heartbeat Frame Composition:**
   - Every heartbeat packet contains:
     - Monotonically increasing 32-bit packet sequence counter.
     - Ephemeral session challenge nonce supplied by the server.
     - High-resolution timestamp sampled directly from `KUSER_SHARED_DATA` (`0x7FFE0014` / `InterruptTime`).
     - Kernel integrity accumulator bits (reporting active callback state, hook verification status, and page table audit passes).
     - Rolling session XOR key derived during initial authentication.

### 16.2 Watchdog Verification & Timeout Eviction

To defeat techniques that suspend or freeze the security thread (`SuspendThread`, thread hijacking, or hardware breakpoint stalls):

| Condition | Threshold Window | Enforcement Action |
|---|---|---|
| **Missed Usermode Heartbeat** | > 3 consecutive cycles (~6000ms–9000ms) | Driver flags heartbeat desynchronization; notifies game client via IOCTL failure and signals process exit. |
| **Thread Suspension Stall** | Clock delta discrepancy between CPU TSC and `KUSER_SHARED_DATA` > $500\text{ms}$ | Identifies thread freeze; logs watchdog anomaly flag and invalidates the session challenge token. |
| **Kernel DPC Starvation** | DPC queue execution latency > $10000\text{ms}$ | Triggers defensive termination of the game process context. |
| **Network Heartbeat Drop** | Client fails to exchange UDP heartbeat packets with game server within $30\text{s}$ | Server-side disconnect directive issued (`BattlEye: Client not responding`); player evicted from session. |

### 16.3 Anti-Replay & Sequence Integrity
- Heartbeat packets are valid strictly for a single round-trip transmission.
- Replaying previously captured packets fails because the server validates:
  $$\text{Timestamp}_{\text{current}} - \text{Timestamp}_{\text{frame}} \le \text{MaxAllowedLatency}$$
  $$\text{SequenceID}_{\text{current}} = \text{SequenceID}_{\text{previous}} + 1$$
- Any reuse of a sequence counter, duplicate nonce, or out-of-order frame triggers immediate session invalidation.
