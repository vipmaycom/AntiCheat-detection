# EA AntiCheat (EAAC) — Cheat Detection

What EAAC detects, how, and via what kernel calls. Based on static reverse engineering and Ghidra disassembly of `eaanticheat.sys` (SHA-256: `f6e1c531f104a0cb99cbb1fe04874f87ad378e0f81e12ee147e7965a9d4d6177`, 83.7 MB, PDB: `EAAntiCheat.Driver.pdb`).

---

## Detected Injection Types

| Injection Method | Detected? | How |
|-----------------|-----------|-----|
| LoadLibrary | YES | ETW `IMAGE_LOAD` event + `PsSetLoadImageNotifyRoutine`, dynamic module whitelist/hash validation |
| Manual map (MDL/physical) | YES | CR3 page table audits (checks PTEs where `NX=0` without backing VAD node), stack walking from DPC/ETW handlers |
| Reflective DLL | YES | Working set scanner sweeps `MEM_COMMIT` regions without `SEC_IMAGE` backing, searches for unlinked `MZ`/`PE` headers |
| APC injection | YES | `Circular Kernel Context Logger` ETW session captures thread state transitions; stack walk on APC delivery to unbacked addresses |
| Thread hijack / SetThreadContext | YES | `ObRegisterCallbacks` strips `THREAD_SET_CONTEXT` and `THREAD_SUSPEND_RESUME`; periodic thread `RIP` validation against export tables |
| Process hollowing | YES | `PsSetCreateProcessNotifyRoutineEx` + cross-checks `PEB->ImageBaseAddress` against original section file mapping |
| Module stomping | YES | Minifilter on-disk PE hash vs in-memory `.text`/`.rdata` page hash comparison (`CiValidateFileObject` / `FltReadFile`) |
| DLL sideloading | YES | Image load notify hashes loaded DLLs against EA and Microsoft authenticode catalogs |
| Shellcode in allocated memory | YES | Stack walking via `RtlVirtualUnwind` — traps return addresses resolving outside modules in `PsLoadedModuleList` |
| NtCreateSection-based | YES | Handle stripping blocks cross-process section opening; view mapping audited in target VAD tree |
| Atom bombing | PARTIAL | Memory content scanner catches unbacked payload; execution trapped if worker threads dispatch to dynamic memory |
| ThreadlessInject / PoolParty | YES | Threadpool worker factories and I/O completion ports audited for function pointers resolving to unbacked memory |

---

## Detected Driver Types

| Driver Type | Detected? | How |
|------------|-----------|-----|
| Unsigned driver (NtLoadDriver) | YES | Windows DSE enforced + `SeRegisterImageVerificationCallback` rejects before execution |
| Test-signed driver | YES | BCD element query via `ZwQuerySystemInformation(SystemBootEnvironmentInformation)` + `g_CiOptions` verification |
| Known BYOVD (RTCore64, dbk64, Dell DBUtil, Intel NAL, GIGABYTE) | YES | Encrypted SHA-256 and name blacklist embedded inside virtualized container `.grfn112`, device object directory scans (`\Device\...`) |
| Unknown BYOVD with valid WHQL | PARTIAL | Dynamic behavioral heuristics: physical memory mapping (`\Device\PhysicalMemory`), handle duplication, and dispatch pointer detours |
| Cheat Engine kernel (dbk64.sys) | YES | Device name check (`\Device\DBK64`), export signature check, and certificate blacklist |
| Manual-mapped driver (KDMapper/Capcom) | YES | Kernel pool allocation scan checks executable memory (`NX=0`) in `NonPagedPool`; callback stack unwinding verifies return addresses |
| Driver with dev build path PDB | YES | PE debug directory parsing in loaded drivers; matches against known development and public cheat repository paths |
| Revoked-cert driver | YES | Certificate chain validation against Microsoft CRL and embedded revocation tables |

---

## Detected Hypervisor Techniques

| Technique | Detected? | How |
|----------|-----------|-----|
| CPUID leaf 0x1 bit 31 (HV present) | YES | Direct instruction execution: bit 31 set flags hypervisor presence |
| Custom HV vendor string | YES | `CPUID 0x40000000` read — unknown or synthetic vendor strings flagged |
| CPUID feature filtering | YES | Cross-leaf consistency analysis — architectural feature dependencies checked across leaves |
| MSR hooking (LSTAR, SYSENTER_EIP) | YES | Read-modify-write cycle executed under `CLI`/`STI` barriers — readback mismatch = guest interception |
| EPT page splitting (execute vs read) | PARTIAL | `INVLPG` forces TLB miss → timing discrepancy measured against direct physical memory reads |
| TSC manipulation | YES | `RDTSC`/`RDTSCP` multi-sample delta compared with CPUID calibration and `KUSER_SHARED_DATA` timestamps |
| Hyper-V binary patching (hvix64/hvax64) | YES | Disk vs memory hash comparison of hypervisor and `securekernel.exe` binaries |
| Type-1 HV below Windows | PARTIAL | VM-exit latency penalties measured on high-frequency instruction cycles (`CPUID`, `CR3` reload) |
| VBS/HVCI state change | YES | Direct check via `VslGetSecurePciEnabled` and `ZwQuerySystemInformation(SystemIsolatedUserModeInformation)` |
| HV shared page anomaly | YES | `NtQuerySystemInformation(0xC5)` — non-standard address pattern detection |
| ACPI table manipulation | YES | `HalAcpiGetTableEx` physical queries — checks SLIC, MADT, FADT, and DMAR tables |

---

## Detected DMA / Firmware Techniques

| Technique | Detected? | How |
|----------|-----------|-----|
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

## Kernel Calls EAAC Monitors

Calls EAAC watches for unauthorized interaction targeting EA game processes (`EAAntiCheat.GameService.exe` and game binaries):

| Call | Monitoring Method |
|------|------------------|
| NtOpenProcess | `ObRegisterCallbacks` — strips `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`, `PROCESS_DUP_HANDLE`, `PROCESS_CREATE_THREAD`, `PROCESS_TERMINATE` |
| NtOpenThread | `ObRegisterCallbacks` — strips `THREAD_SET_CONTEXT`, `THREAD_GET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_DIRECT_IMPERSONATION` |
| NtDuplicateObject | `ObRegisterCallbacks` — strips privileges on duplicated handles |
| NtWriteVirtualMemory | Blocked via handle stripping; direct kernel callers intercepted via ETW and stack unwinding |
| NtAllocateVirtualMemory | Blocked via handle stripping; kernel allocations audited in target process VAD tree |
| NtProtectVirtualMemory | Blocked via handle stripping; PTE sweeps detect unauthorized `RWX` transitions |
| PsCreateSystemThread | Thread start address verified; must reside within legitimate loaded module bounds |
| NtCreateThreadEx | Process notify routines audit newly staged threads in protected process context |
| NtSetContextThread | Blocked via handle stripping; kernel modifications caught during context validation |
| NtLoadDriver / ZwLoadDriver | `SeRegisterImageVerificationCallback` intercepts driver registration pre-execution |
| MmLoadSystemImage | Intercepted via the same driver image verification callback chain |
| MmCopyVirtualMemory | Audited via kernel callback stack inspection; caller must reside in authorized system module |

---

## Kernel Calls EAAC Makes (Detection Tools)

*Note: In `eaanticheat.sys`, all kernel APIs are resolved dynamically via custom Griffin hash tables to prevent IAT inspection.*

| Call | Usage |
|------|-------|
| RtlWalkFrameChain | Stack frame capture during kernel callbacks and timer DPCs |
| RtlVirtualUnwind + RtlLookupFunctionEntry | Exact x64 stack unwinding adhering to `.pdata` exception tables |
| MmCopyVirtualMemory | Safe memory reading of game and external process pages from kernel space |
| MmCopyMemory | Low-level kernel and physical memory verification |
| PsLookupProcessByProcessId | PID resolution for process tree and parentage validation |
| PsLookupThreadByThreadId | TID resolution for thread context and stack inspection |
| PsGetProcessImageFileName | Process name extraction for blacklist matching |
| SeQueryInformationToken | Queries token privileges, integrity levels, and AppContainer SIDs |
| ObReferenceObjectByName | Named object lookup (`\Driver\...`, `\Device\...`) |
| CiValidateFileObject | Validates authenticode digital signatures against Microsoft and EA catalogs |
| HalGetBusDataByOffset | Reads PCI bus configuration space across all bus/device/function combinations |
| HalAcpiGetTableEx | Physical ACPI table parsing (MADT, SLIC, FADT, DMAR) |
| IoWMIOpenBlock + IoWMIQueryAllData | Queries SMBIOS and firmware raw data via WMI |
| FltRegisterFilter / FltReadFile | Minifilter filesystem operations; monitors disk reads and PE image loads |
| ZwQuerySystemInformation(0x05) | Full system process list enumeration |
| ZwQuerySystemInformation(0x0B) | Full loaded module list enumeration (`PsLoadedModuleList`) |
| ZwQuerySystemInformation(0x10) | System-wide open handle table enumeration |
| ZwQuerySystemInformation(0x67) | Physical firmware and SMBIOS table data queries |
| ZwQuerySystemInformation(0xC5) | Hypervisor shared page query |
| ZwTraceControl | Controls and subscribes to kernel ETW sessions (`Circular Kernel Context Logger`) |
| KeSetCoalescableTimer / KeSetTimerEx | Background DPC scheduling with pseudo-random millisecond jitter |

---

## Kernel Callbacks Registered

| Callback | API | Scope |
|----------|-----|-------|
| Handle filter | `ObRegisterCallbacks` | Synchronous handle filtering on `PsProcessType` and `PsThreadType`; strips read/write/context privileges targeting the game |
| Process notify | `PsSetCreateProcessNotifyRoutineEx` | System-wide process creation and exit monitoring; verifies parent-child relationships and command lines |
| Thread notify | `PsSetCreateThreadNotifyRoutine` | Intercepts thread creation; validates `StartAddress` against module boundaries in `PsLoadedModuleList` |
| Image load | `PsSetLoadImageNotifyRoutine` | Audits every DLL and driver loaded into user or kernel space before entry point execution |
| Image verification | `SeRegisterImageVerificationCallback` | Intercepts kernel-mode driver verification before execution starts |
| Filesystem minifilter | `FltRegisterFilter` | Inspects file I/O system-wide; monitors game directory access, screenshot folders, and external injector binaries |
| ETW Context Logger | `Circular Kernel Context Logger` | Asynchronous real-time kernel logger capturing syscalls, context switches, image loads, and page faults |
| I/O Control interceptor | `IRP_MJ_DEVICE_CONTROL` | Restricts `\Device\EAAntiCheat` communication strictly to authenticated `EAAntiCheat.GameService.exe` tokens |

---

## Memory Scanning Methods

| Method | What It Does |
|--------|-------------|
| CR3 physical page table walk | Reads process CR3; walks PML4 -> PDPT -> PD -> PT in physical RAM, bypassing API hooks |
| PTE attribute verification | Scans kernel and process PTEs: flags executable pages (`NX=0`) lacking valid module file backing |
| SIMD content scanner (AVX2/SSE) | Scans memory regions using SSE2/AVX2 instructions (32–64 bytes/cycle) searching for known cheat signatures and trampolines |
| PE on-disk vs in-memory comparison | Hashes in-memory `.text` sections and compares directly against on-disk signed files via minifilter reads |
| PE section integrity hash | Per-section hash with `.rsrc` and `.reloc` exclusion |
| Import/export table validation | Verifies that IAT/EAT function pointers resolve strictly within the owning module address space |
| VAD tree cross-referencing | Traverses `EPROCESS->VadRoot` AVL tree; cross-checks declared protections with physical PTE bits |
| Spinlock memory watchpoint | Detects concurrent access to monitored internal data structures |

---

## Cheat Tool Detection

| Tool | Detection Method |
|------|-----------------|
| Cheat Engine (dbk64.sys + process) | Process name scan, window title inspection, `\Device\DBK64` driver enumeration, certificate blacklist |
| x64dbg / x32dbg | Process name scan, window class checks, debug register hook monitoring (`DR0`–`DR7`) |
| ReClass.NET | Process name scan, handle table enumeration, memory read pattern tracking |
| Process Hacker / System Informer | Process name scan, driver blacklist (`kprocesshacker.sys`), open handle sweeps |
| IDA Pro / Ghidra | Window title detection, executable path scan, process name enumeration |
| Kernel debugger (WinDbg) | `KdDebuggerEnabled` check, BCD `/debug` element checks, INT 1/3 SEH traps |
| Frida / DBI | RDTSC timing detection, unbacked runtime code generation flags, trampoline detection |
| Custom debug tools | PDB debug directory scanning in PE headers, unbacked thread start address tracking |

---

## Anti-Debug

| Check | Method |
|-------|--------|
| INT 1 SEH trap | Driver init gate — debugger consuming = driver fails to load |
| INT 3 SEH trap | Embedded breakpoint exceptions checked periodically; active debuggers consume interrupt and fail parity tests |
| RDTSC timing | Back-to-back with randomized loop — single-step/VM overhead detected |
| KdDebuggerEnabled | Direct kernel variable read — ever-set = flagged |
| RDTSC+CPUID quad check | Ratio comparison across CPUID instruction boundaries — hypervisor exit latency detected |
| WRMSR+RDPMC | Performance counter timing channel |
| KUSER_SHARED_DATA timestamps | Read directly from unhookable system memory page (`0x7FFE0000`) |

---

## Communication Channel Detection

| Channel | Detected? | How |
|---------|-----------|-----|
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

## Detection Timing

| Detection | Frequency |
|-----------|-----------|
| ObRegisterCallbacks | Synchronous: executes on every process/thread handle operation system-wide |
| Stack walking | Burst sampling in kernel callbacks and periodic timer routines |
| Process scanning | Continuous background cycle |
| Thread enumeration | Continuous background cycle |
| Memory integrity | Periodic: full cycle across monitored modules |
| Module blacklist | Periodic passes over `PsLoadedModuleList` |
| PCI config scan | Boot initialization + periodic polling (512 device reads) |
| Filesystem minifilter | Synchronous on every file I/O targeting game directories |
| RDTSC timing | Driver initialization gate + periodic jitter loops |
| Heartbeat verification | High-frequency: bidirectional handshake between usermode service and driver every 1000ms–3000ms |


# EA AntiCheat (EAAC) — HWID Identifiers & Spoofing

Every hardware identifier EAAC collects, its kernel extraction mechanism, format, and spoofing requirements.

---

## Collected Identifiers

| Identifier | Source | Format String / Type | Extraction Address / Method | Spoof Method |
|-----------|--------|----------------------|-----------------------------|-------------|
| **SMBIOS UUID** | Type 1 System Info | `%016llx-%016llx` / GUID | Direct physical RAM map (`0x000F0000`–`0x000FFFFF`) + `ZwQuerySystemInformation(0x67)` | Firmware table hook (`IoWMIOpenBlock` intercept) or BIOS mod |
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

---

## SMBIOS Types Parsed

| Type | Name | Key Fields |
|------|------|-----------|
| 0 | BIOS Information | Vendor, version, release date, ROM size |
| 1 | System Information | Manufacturer, product name, version, **serial**, **UUID** |
| 2 | Baseboard | Manufacturer, product, version, **serial**, asset tag |
| 3 | System Enclosure | Manufacturer, type, version, **serial**, asset tag |
| 4 | Processor | Manufacturer, version, **processor ID**, core count |
| 11 | OEM Strings | All OEM entries |
| 17 | Memory Device | Manufacturer, **serial**, part number, size, locator |
| 127 | End-of-Table | Sentinel |

Collected via direct physical RAM mapping at `0x000F0000`–`0x000FFFFF` (`_SM_`/`_SM3_` anchor search) + `IoWMIOpenBlock(MSSmBios_RawSMBiosTables)` + `ZwQuerySystemInformation(0x67)`.

---

## CPUID Leaves Collected

| Leaf | Data | Ban Relevance |
|------|------|--------------|
| 0x00 | Vendor (`GenuineIntel` / `AuthenticAMD`) | Base architecture fingerprint |
| 0x01 | Family/Model/Stepping + features | CPU identity + Hypervisor presence bit (ECX[31]) |
| 0x02-0x0F | Cache, TLB, topology | Hardware topology cross-checked with ACPI MADT table |
| 0x17+ | Extended HW fingerprinting (SOC vendor) | Deep CPU fingerprint |
| 0x40000000 | Hypervisor vendor | Hyper-V, VMware, KVM identification |
| 0x80000002-4 | Brand string ("Intel Core...", "AMD Ryzen...") | CPU name cross-checked with SMBIOS Type 4 |

---

## PCI Devices Fingerprinted

Full iteration across 256 buses, 32 devices, 8 functions (512 reads via `HalGetBusDataByOffset` and MMIO ECAM space).

**Known flagged devices:**

| Vendor | Device | Hardware Entity |
|--------|--------|----------------|
| 0x15AD | 0x0405 | VMware SVGA |
| 0x15AD | 0x0740 | VMware vmxnet3 |
| 0x15AD | 0x07B0 | VMware PVSCSI |
| 0x1AF4 | 0x1000-0x107F | VirtIO (KVM/QEMU) |
| 0x1414 | 0x5353 | Hyper-V video |
| 0x1B36 | * | QEMU PCI bridge |
| 0x10EE | Default | Xilinx default DMA bitstream (PCILeech/Screamer) |

---

## Ban Enforcement Function

**Internal EAAC Telemetry Pipeline (`.grfn112`):**

```
1. Driver samples hardware identifiers (SMBIOS UUID, Baseboard Serial, NVMe Serial, NIC MAC)
2. Collects boot measurements (Secure Boot state, PCR[7], HVCI status via VslGetSecurePciEnabled)
3. Encrypts composite HWID payload with session key schedule inside .grfn110 VM
4. Dispatches payload to EAAntiCheat.GameService.exe via ALPC/IOCTL
5. User-mode service transmits payload over TLS to EA backend
6. Server evaluates hardware hash against global ban database
7. If match found: Server issues Banned State Directive (Error 115 / 117)
8. Local driver sets ban flags and terminates protected game process
```

- Server controls check bitmasks per scan wave
- Ban database stored server-side
- Two-stage verification reduces false positives

---

## Spoof Priority Checklist

| Priority | Identifier | Why |
|----------|-----------|-----|
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

---

## Cross-Validation Requirements

EAAC cross-checks hardware data across multiple kernel paths. Inconsistencies trigger integrity flags:

- SMBIOS Type 4 Processor **must match** physical CPUID leaves
- SMBIOS Manufacturer **must be consistent** with ACPI OEM tables
- MAC OUI prefix **must match** a real, registered hardware vendor
- Disk serial **must be consistent** with controller type (NVMe spec vs ATA byte-swapping)
- All identifiers **must change together** to represent an authentic "new machine"
- SMBIOS firmware tables **must pass** internal checksum and tag validation

---

## Spoof Implementation Notes

**Firmware table hook:** Intercept `IoWMIOpenBlock` / `IoWMIQueryAllData` / `ZwQuerySystemInformation(0x67)` at kernel level or modify UEFI NVRAM prior to driver boot. Return modified SMBIOS tables with valid lengths, checksums, and type ordering.

**NDIS MAC spoof:** NDIS lightweight filter driver (LWF). Must intercept `OID_802_3_PERMANENT_ADDRESS` and `OID_GEN_PERMANENT_ADDRESS` — local registry changes (`NetworkAddress`) are bypassed.

**Disk serial spoof:** Storage miniport filter intercepting `IOCTL_STORAGE_QUERY_PROPERTY`, SMART data, and SCSI inquiry responses. Must return consistent serials across all inquiry paths.

**CPUID spoof:** Requires hardware hypervisor. Intercept CPUID exits, return modified leaf data. Ensure leaf 1 family/model matches leaf 0x80000002-4 brand string.


# EA AntiCheat (EAAC) — Ban Traces & Cleaner Reference

Everything EAAC leaves on the system that requires cleaning after a ban.

---

## Registry Traces

| Path | What | Clean Method |
|------|------|-------------|
| `HKLM\SYSTEM\CurrentControlSet\Services\EAAntiCheat` | Driver service key + detection records | Delete entire key tree |
| `HKLM\SYSTEM\CurrentControlSet\Services\EAAntiCheatService` | User-mode service registration key | Delete entire key tree |
| `HKLM\SOFTWARE\Electronic Arts\EA Desktop` | Client installation data + machine telemetry tokens | Delete entire key |
| `HKLM\SOFTWARE\WOW6432Node\Electronic Arts\*` | 32-bit registry mirror for game licenses & install paths | Delete matching keys |
| `HKCU\Software\Electronic Arts\*` | User-level cached session tokens and HWID anchors | Delete entire key |
| `HKLM\SOFTWARE\Microsoft\Cryptography` | `MachineGuid` (global OS installation GUID) | Regenerate fresh GUID |
| `HKLM\SYSTEM\CurrentControlSet\Control\IDConfigDB\Hardware Profiles\0001` | `HwProfileGuid` | Regenerate fresh GUID |
| `HKLM\SYSTEM\MountedDevices` | `\??\Volume{GUID}` | System volume GUID associations |

---

## Filesystem Traces

| Path | What | Clean Method |
|------|------|-------------|
| `C:\Program Files\EA\AC\` | EA AntiCheat core binaries, installer, and certificates | Delete folder |
| `C:\ProgramData\Electronic Arts\EA Services\License\` | Cached license files linked to banned HWID | Delete folder contents |
| `%LOCALAPPDATA%\Electronic Arts\EA Desktop\Logs\` | Client logs containing hardware telemetry and ban error codes | Delete folder contents |
| `%LOCALAPPDATA%\Origin\` | Legacy client cache and token files | Delete folder |
| `%PROGRAMDATA%\Origin\` | Legacy shared telemetry and hardware records | Delete folder |
| `%SystemRoot%\System32\drivers\eaanticheat.sys` | Kernel driver binary | Delete |
| `%SystemRoot%\Prefetch\EAANTICHEAT*.pf` | Windows prefetch execution traces | Delete matching `.pf` files |
| `%SystemRoot%\MEMORY.DMP` | Crash dump — contains bugcheck records if triggered | Delete if present |
| `%SystemRoot%\Minidump\*.dmp` | Mini crash dumps | Delete timestamped dumps |
| `%SystemRoot%\System32\winevt\Logs\System.evtx` | Windows event logs (service install Event ID 7045) | Clear or selectively delete |
| Game install dir | Per-game `EAAntiCheat` subfolder and configs | Delete subfolder |

---

## Certificate Store Traces

EAAC reads Windows certificate stores during driver verification:

| Store | Registry Path |
|-------|-------------|
| ROOT | `HKLM\SOFTWARE\Microsoft\SystemCertificates\ROOT\Certificates\` |
| AuthRoot | `HKLM\SOFTWARE\Microsoft\SystemCertificates\AuthRoot\Certificates\` |
| TrustedAppRoot | `HKLM\SOFTWARE\Microsoft\SystemCertificates\TrustedAppRoot\Certificates\` |
| Disallowed | `HKLM\SOFTWARE\Microsoft\SystemCertificates\Disallowed\Certificates\` |

Last-access timestamps on these keys record EAAC reading revocation lists and catalogs. Forensic indicator.

---

## Kernel Object Traces

| Object | Path | Lifetime |
|--------|------|----------|
| Device object | `\Device\EAAntiCheat` | Deleted on driver unload |
| Symbolic link | `\DosDevices\EAAntiCheat` | Deleted on driver unload |
| ETW Session | `Circular Kernel Context Logger` | Detached on driver teardown |
| MiniFilter instance | FltMgr altitude registration (`320000` band) | Unregistered on teardown |

These are transient — exist only while driver is actively running. No persistent cleanup required.

---

## Kernel Callback Residue

| Callback | Registration |
|----------|-------------|
| ObRegisterCallbacks | Unregistered on driver unload |
| PsSetCreateProcessNotifyRoutineEx | Unregistered on driver unload |
| PsSetCreateThreadNotifyRoutine | Unregistered on driver unload |
| PsSetLoadImageNotifyRoutine | Unregistered on driver unload |
| SeRegisterImageVerificationCallback | Unregistered on driver unload |
| FltRegisterFilter | Unregistered on driver unload |
| ETW Context Trace | Unsubscribed on driver unload |

All unregistered on clean driver unload. If the driver crashes or the system halts abruptly, reboot clears them from kernel memory.

---

## TEB Detection Flags

| Trace | Location | Clean |
|-------|----------|-------|
| Per-thread detection flags | `TEB.SystemReserved1` / TLS slots | Cleared per-thread on process termination |
| Detection flag accumulator | Kernel state mirror | Cleared on driver teardown |

Per-session only — does not persist across game reboots.

---

## Server-Side Traces (Can't Clean)

| What | Notes |
|------|-------|
| HWID composite fingerprint | SMBIOS UUID + board serial + disk serial + MAC — stored server-side |
| Detection telemetry | All detection results sent via encrypted TLS protocol to EA backend |
| Ban database entry | Encrypted per-entry; matched against hardware composite hash |
| EA Account linkage | EA account → HWID mapping |
| Game ID + detection profile | Per-game detection history |
| TPM attestation data | PCR values and boot chain measurements sent and stored |
| IP address history | Logged per-session |

Server-side data cannot be cleaned via client software. This is why HWID spoofing is required to present as "new hardware."

---

## Windows Artifacts (OS-Level)

| Artifact | Location | Clean Method |
|----------|----------|-------------|
| Prefetch DB | `%SystemRoot%\Prefetch\` | Delete all `EAANTICHEAT*.pf` files |
| Amcache.hve | `%SystemRoot%\AppCompat\Programs\Amcache.hve` | Delete EAAC driver entries (requires offline hive edit or specialized cleaner) |
| ShimCache | `HKLM\SYSTEM\CCS\Control\Session Manager\AppCompatCache` | Flush cache (reboot clears volatile entries) |
| SRUM (System Resource Usage Monitor) | `%SystemRoot%\System32\sru\SRUDB.dat` | Delete or clear EAAC execution records |
| BAM/DAM | `HKLM\SYSTEM\CCS\Services\bam\State\UserSettings\*` | Delete EA execution entries |
| UserAssist | `HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\UserAssist\*` | ROT13-encoded, delete matching GUIDs |
| Jump Lists | `%APPDATA%\Microsoft\Windows\Recent\AutomaticDestinations\` | Delete EA app launcher entries |
| USN Journal | NTFS volume journal (`$UsnJrnl:$J`) | Reset journal via `fsutil usn deletejournal /d C:` |
| Event Logs (all) | `%SystemRoot%\System32\winevt\Logs\` | Clear System + Application logs |

---

## Network Traces

| Trace | Location |
|-------|----------|
| DNS cache | `ipconfig /flushdns` clears |
| ARP cache | `arp -d *` clears |
| TCP/IP persistent connections | System reboot clears |
| Firewall log | `%SystemRoot%\System32\LogFiles\Firewall\` — delete if logging enabled |
| Router/gateway logs | Cannot clean via client — cycle router for new dynamic IP |

---

## Cleaner Execution Order

1. **Uninstall Game + EAAC:** Run official `EAAntiCheat.Installer.exe` uninstall step and uninstall game.
2. **Delete EAAC Folders:** Delete `C:\Program Files\EA\AC`, `C:\ProgramData\Electronic Arts`, `%LOCALAPPDATA%\Electronic Arts`.
3. **Clean Registry:** Delete service keys (`EAAntiCheat`, `EAAntiCheatService`), software keys, and regenerate `MachineGuid` and `HwProfileGuid`.
4. **Clean OS Artifacts:** Clear Prefetch, Amcache (`Amcache.hve`), ShimCache, SRUM, and Windows Event Logs.
5. **Reset NTFS Change Journal:** Execute `fsutil usn deletejournal /d C:`.
6. **Flush Network:** Flush DNS (`ipconfig /flushdns`), reset ARP (`arp -d *`), cycle public IP.
7. **Spoof HWIDs:** Spoof SMBIOS tables (Type 1 UUID/Serial, Type 2 Board), storage serials (NVMe/SCSI), and permanent MAC address before reinstalling.
8. **New Account:** Create fresh, unlinked EA account on the clean hardware state.
9. **Reinstall:** Fresh game and EAAC installation over the sanitized hardware baseline.

---

## What EAAC Reports to Server on Detection

| Data | Protocol |
|------|----------|
| Detection type code | Encrypted binary payload field |
| Evidence virtual address | Encrypted binary payload field |
| Process name + PID | Encrypted binary payload field |
| Module hashes (SHA-256) | Encrypted binary payload field |
| Stack trace (return addresses) | Encrypted binary payload field |
| HWID composite fingerprint | Encrypted binary payload field |
| Timestamp (`KUSER_SHARED_DATA`) | Encrypted binary payload field |
| Game ID + Title version | Encrypted binary payload field |
| Boot environment info (Secure Boot / HVCI) | Encrypted binary payload field |

All telemetry packets are assembled in the `.grfn110` VM, encrypted with ephemeral session keys, queued to `EAAntiCheat.GameService.exe`, and dispatched over TLS to EA backend servers.

---

## BSOD Artifacts (If BugCheck Triggered)

If kernel integrity violations trigger fatal halts:

| Artifact | Location | Clean |
|----------|----------|-------|
| Full crash dump | `%SystemRoot%\MEMORY.DMP` | Delete |
| Mini dump | `%SystemRoot%\Minidump\*.dmp` | Delete timestamped dump |
| WER report | `%ProgramData%\Microsoft\Windows\WER\` | Delete report folders |
| Event log entry | `System.evtx` — BugCheck event | Clear event log |
| Registry crash count | Various Windows crash reporting keys | Reset values |

Crash dumps contain full register dumps and memory pages proving the execution of unauthorized kernel code. Always purge crash artifacts.


---

# EA AntiCheat (EAAC) — Advanced Subsystems Master Reference

Detailed mapping of EA AntiCheat's graphic overlay auditing, microcontroller input validation, internal Griffin virtualization bytecode execution, launcher error codes, and server-side spatial entity occlusion.

---

## 21. Direct3D 11/12, DXGI & DWM Overlay Detection Architecture

EA AntiCheat employs a multi-tiered inspection pipeline to detect external HUDs, ESP boxes, and menu canvases drawn over the game viewport:

### 21.1 Window Manager & Composition Auditing (DWM)
- **Window Extended Style Scanning:** `EAAntiCheat.GameService.exe` continuously audits the system window hierarchy using `GetWindowLongPtr(GWL_EXSTYLE)`.
- Flags any external window possessing the following composite style combination positioned over the protected game screen bounding box:
  $$\text{WS\_EX\_TOPMOST} \mid \text{WS\_EX\_TRANSPARENT} \mid \text{WS\_EX\_LAYERED}$$
- **DWM Occlusion Queries:** Leverages Desktop Window Manager composition telemetry (`DwmGetCompositionTimingInfo` / DirectComposition APIs) to detect transparent swapchains rendering between the desktop composition root and the game canvas.

### 21.2 DXGI SwapChain & DirectX Hook Verification
- **`IDXGISwapChain::Present` VTable Inspection:** The game client audits its own DirectX swapchain virtual method table. Any detour or trampoline redirecting `Present` (VTable Index 8) or `ResizeBuffers` (VTable Index 13) to unbacked memory triggers immediate abort.
- **Overlay DLL Whitelist:** Only cryptographically signed, pre-approved third-party overlay hooks are permitted to touch the swapchain:
  - EA In-Game Overlay (`EAOverlay.dll` / `OriginOverlay64.dll`)
  - Discord (`DiscordOverlayHook64.dll`)
  - OBS Studio (`graphics-hook64.dll`)
  - NVIDIA GeForce Experience (`nvspcap64.dll`)
  - Steam In-Game Overlay (`GameOverlayRenderer64.dll`)
- **Hook Hijacking Detection:** If a payload injects into a whitelisted host (e.g., Discord) to borrow its overlay rendering pipeline, EAAC verifies the call chain and integrity of the hook module's code section against its signed on-disk binary via minifilter hashing.

---

## 22. Hardware Mouse Emulation & Out-of-Band (OOB) USB Input Analysis

External automation tools attempt to bypass software synthetic input detection (`SendInput`, `mouse_event`, `MouseClassServiceCallback` tampering) by using physical USB microcontrollers (MCU) acting as human interface devices (HID). EAAC deploys hardware validation and bus topology inspection to identify and neutralize these out-of-band input devices:

### 22.1 Target Microcontroller Hardware Profiles
- **Arduino Leonardo / Pro Micro (ATmega32U4):** Uses AVR USB Core (`PluggableUSB` / `HID.cpp`).
- **Raspberry Pi Pico / RP2040:** Employs TinyUSB stack for dual CDC (serial) + HID composite profiles.
- **Teensy 3.x / 4.x (NXP i.MX RT1062):** High-speed USB stack supporting dual serial + mouse interfaces.
- **KMBox (B+ / NET / Pro) & Makemcu:** Commercial breakout boards pairing an onboard UART/Ethernet bridge with an MCU running hardware pass-through and coordinate injection firmware.

### 22.2 USB Descriptor & Endpoint Validation Pipeline
`eaanticheat.sys` queries the USB device stack via USB Request Blocks (`URB`) and internal IOCTLs (`IOCTL_INTERNAL_USB_SUBMIT_URB`):

1. **Standard VID / PID Blacklisting & Fingerprinting:**
   - **Default Bootloader & Hardware IDs:**
     - Arduino Leonardo: `VID_2341&PID_8036` (Bootloader: `VID_2341&PID_0036`)
     - SparkFun Pro Micro: `VID_1B4F&PID_9206`
     - RP2040 TinyUSB: `VID_cafe` (standard test VID) or default Raspberry Pi `VID_2E8A`
     - KMBox / Makemcu default configurations: `VID_1A86&PID_7523` (CH340/CH341 UART bridge paired with MCU)
   - **Composite Interface Inspection (CDC Serial + HID Leaks):**
     - Real gaming mice present strictly as HID interfaces (often with manufacturer-specific feature reports for RGB/DPI).
     - Cheats require a control channel (COM port via UART/CDC) to receive target coordinates from an external PC or secondary user-mode script.
     - If a device declares a **USB Communications Device Class (CDC / Serial COM Port, Class `0x02`)** on the exact same physical USB composite root or parent hub as a mouse HID interface (Class `0x03`), EAAC flags the device as an untrusted automation interface.

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
     Failing to replicate proprietary vendor descriptors (e.g., Logitech `VID_046D` or Razer `VID_1532` 16-bit high-resolution usage pages) results in instant identification.
   - **Serial Number & String Descriptor Discrepancies:** Real mice provide valid Unicode string descriptors for `iManufacturer`, `iProduct`, and `iSerialNumber`. AVR/Pico firmwares often return empty strings or generic descriptors (`"Arduino LLC"`, `"Raspberry Pi"`).

### 22.3 USB Bus Topology & Hardware Interrupt Correlation
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
   │  └── MouseClassServiceCallback (monitored by eaanticheat.sys)
   │
Kernel Input Subsystem (win32kbase.sys Raw Input Thread - RIT)
   │
Target Window (Game Viewport - WM_INPUT)
```

1. **Dual-Mouse Conflict & Focus Arbitration:**
   - When a hardware cheat is plugged in, the OS has **two** active mouse devices registered under `\Device\PointerClass0` and `\Device\PointerClass1`:
     - Device A: The player's actual physical hand mouse.
     - Device B: The Arduino / KMBox / Makemcu injection hardware.
   - EAAC inspects `mouclass.sys` device object lists. If inputs arrive simultaneously from two distinct physical device objects (`DEVICE_OBJECT`) while the player is aiming and shooting, the dual-source anomaly flags the session for hardware arbitration failure.

2. **Packet Inter-Arrival Timing & Polling Jitter Analysis:**
   - Hardware USB host controllers generate hardware interrupts (IRQs) tied strictly to bus clock cycles:
     - USB 1000Hz polling produces an interrupt packet every $1.000\text{ms} \pm 10\mu\text{s}$ (hardware clock drift).
   - In contrast, MCU coordinate injection over serial/COM port (e.g., Python script reading screen $\rightarrow$ writing to COM port $\rightarrow$ Arduino executing `Mouse.move()`):
     - Incurs serialization delays, UART baud-rate latency, and firmware queuing.
     - Produces bursty inter-packet deltas ($\Delta t$ clustering around $16\text{ms}$, $8\text{ms}$, or non-uniform spikes) while claiming to be a continuous hardware mouse.

---

## 23. Griffin Virtual Machine Architecture (`.grfn`) & Dynamic Export Hashing Engine

Ghidra disassembly of `eaanticheat.sys` confirmed the deployment of Electronic Arts' proprietary **Griffin** virtualization and anti-tamper container:

### 23.1 Section Layout & Role Matrix
| Section | Virtual Address Span | Size | Structural Role in Kernel |
|---|---|---|---|
| `.text` | `0xfffff804692e1000`–`0xfffff80469866729` | 5.5 MB | Gateway thunks, trampoline matrix, and basic API dispatchers |
| `.grfn1` | `0xfffff80469949000`–`0xfffff80469949aab` | 2.7 KB | VM entry point stub and CPU context serialization (`RSP`, `RFLAGS`, GPRs) |
| `.grfn2` | `0xfffff8046994a000`–`0xfffff80469e91b57` | 5.4 MB | VM working memory, rolling encryption keys, and context structures |
| `.grfn110` | `0xfffff8046a94a000`–`0xfffff8046b949fc9` | 16.0 MB | Primary VM bytecode execution engine; handles detection heuristics |
| `.grfn12` | `0xfffff8046b94a000`–`0xfffff8046bd582c0` | 4.0 MB | Integrity verification core; audits `.text` and kernel code caves |
| `.grfn112` | `0xfffff8046c94a000`–`0xfffff8046e2bd1f7` | 26.2 MB | Monolithic detection payload, signature databases, and HWID logic |

### 23.2 Zero-IAT Dynamic Export Hashing
- `eaanticheat.sys` contains **zero plain text imports** in its Import Address Table (IAT).
- All kernel routines (`ObRegisterCallbacks`, `PsSetCreateProcessNotifyRoutineEx`, `ZwQuerySystemInformation`, `MmCopyVirtualMemory`) are resolved dynamically at driver initialization:
  ```c
  // Structural model of Griffin Dynamic API Resolver
  PVOID GriffinResolveKernelExport(ULONG TargetApiHash) {
      PVOID NtosBase = GetNtoskrnlBase();
      PIMAGE_EXPORT_DIRECTORY ExportDir = GetExportDirectory(NtosBase);
      PDWORD Names = (PDWORD)((PBYTE)NtosBase + ExportDir->AddressOfNames);
      PWORD Ordinals = (PWORD)((PBYTE)NtosBase + ExportDir->AddressOfNameOrdinals);
      PDWORD Functions = (PDWORD)((PBYTE)NtosBase + ExportDir->AddressOfFunctions);
      
      for (DWORD i = 0; i < ExportDir->NumberOfNames; i++) {
          LPCSTR FunctionName = (LPCSTR)((PBYTE)NtosBase + Names[i]);
          if (CalculateGriffinHash(FunctionName) == TargetApiHash) {
              return (PVOID)((PBYTE)NtosBase + Functions[Ordinals[i]]);
          }
      }
      return NULL;
  }
  ```
- This prevents static PE analyzers from identifying the driver's monitoring capabilities by inspecting import tables.

---

## 24. EAAC Error Codes & Client BugCheck Ledger

When EA AntiCheat triggers a defensive shutdown or ban directive, specific numerical error codes are surfaced through `EAAntiCheat.GameServiceLauncher.exe`:

| Error Code | Kernel / Usermode Root Cause | System Response & State |
|---|---|---|
| **Error 105** | **Heartbeat Desynchronization:** Communication drop between usermode `EAAntiCheat.GameService.exe` and `eaanticheat.sys`. | Immediate game termination; prevents playing with driver suspended. |
| **Error 107** | **Driver Load Abort:** DSE violation, `TESTSIGNING` enabled, or Secure Boot not configured properly. | Driver fails initialization gate; game launch blocked before window opens. |
| **Error 109** | **Game File Integrity Mismatch:** Game executable or critical DLLs modified on disk (section hash divergence). | Launch aborted; user prompted to repair game installation. |
| **Error 115** | **Active Kernel Memory Violation:** Modification detected in kernel dispatch tables, unbacked executable code, or CR3 walk anomaly. | Instant process kill; security flag recorded in local registry and sent to EA backend. |
| **Error 117** | **Blacklisted Driver / BYOVD Detected:** Known vulnerable driver (e.g., `RTCore64.sys`, `dbk64.sys`) loaded in system memory. | Driver unload demanded; launch blocked until system reboot. |
| **Error 120** | **Anti-Debug SEH Trap Fired:** Active kernel debugger (WinDbg) or hardware breakpoint register (`DR0`–`DR7`) set. | Silent detection report queued; launcher halts with generic security fault. |

---

## 25. Server-Side Fog of War & Network Entity Culling (PVS)

To render client-side memory scraping (including hardware DMA reading) ineffective against distant enemies in competitive titles (Apex Legends, Battlefield):

### 25.1 Potentially Visible Set (PVS) Raycasting
- The dedicated game server executes real-time 3D visibility raycasts between every player's eye coordinates and all other entities on the map.
- If an enemy player is completely occluded behind walls, solid geometry, or outside auditory perception range:
  - **The server completely halts transmission of the enemy's spatial coordinate packets.**
  - Enemy coordinate fields $(X, Y, Z)$, velocity vectors, and entity states **do not exist in client RAM**.

### 25.2 The Occlusion Envelope & De-Cloaking Thresholds
- To compensate for network round-trip time (RTT / Ping) and prevent "pop-in," the server maintains a dynamic predictive envelope:
  - As an enemy approaches an angle, corner, or threshold where they could become visible within $\Delta t = \text{Ping} + \text{InterpolationBuffer}$, the server begins transmitting position data.
  - Only when this spatial envelope is broken do enemy player pawns appear in client process memory, bounding ESP visualization exclusively to immediately peekable ranges.
