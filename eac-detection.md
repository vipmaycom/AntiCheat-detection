# EAC EOS — Cheat Detection

What EAC detects, how, and via what kernel calls.

---

## Detected Injection Types

| Injection Method | Detected? | How |
|-----------------|-----------|-----|
| LoadLibrary | YES | PsSetLoadImageNotifyRoutine, module blacklist |
| Manual map (MDL/physical) | PARTIAL | Stack walking catches callbacks from unmapped memory |
| Reflective DLL | YES | Memory content scan, no module entry in PsLoadedModuleList |
| APC injection | YES | Thread notify callback, stack walk on APC delivery |
| Thread hijack / SetThreadContext | YES | Handle stripping blocks SetThreadContext, thread start addr validation |
| Process hollowing | YES | Process creation notify, parent relationship check |
| Module stomping | PARTIAL | PE hash mismatch (disk vs memory fingerprint §239-240), section heuristics (§241) |
| DLL sideloading | YES | Image load notify hashes every loaded image |
| Shellcode in allocated memory | YES | Stack walking — return address outside any module |
| NtCreateSection-based | YES | Image load notify fires |
| Atom bombing | PARTIAL | Memory content scan may catch payload |
| ThreadlessInject / PoolParty | PARTIAL | Memory scan if payload touches monitored regions |

---

## Detected Driver Types

| Driver Type | Detected? | How |
|------------|-----------|-----|
| Unsigned driver (NtLoadDriver) | YES | SeRegisterImageVerificationCallback rejects before execution |
| Test-signed driver | YES | BCD testsigning check + cert chain validation |
| Known BYOVD (RTCore64, dbk64, Dell DBUtil, Intel NAL, GIGABYTE) | YES | Encrypted SHA-1 blacklist (42 entries), name blacklist (4,674 comparisons) |
| Unknown BYOVD with valid WHQL | NO | Passes cert chain check, not on blacklist |
| Cheat Engine kernel (dbk64.sys) | YES | Hardcoded in module blacklist |
| Manual-mapped driver | PARTIAL | Stack walk catches calls from unmapped memory, no PsLoadedModuleList entry |
| Driver with dev build path PDB | YES | ~30 encrypted path patterns checked (§34) |
| Revoked-cert driver | YES | Certificate chain validation, CRL check |

---

## Detected Hypervisor Techniques

| Technique | Detected? | How |
|----------|-----------|-----|
| CPUID leaf 0x1 bit 31 (HV present) | YES | Direct check — bit set = hypervisor active |
| Custom HV vendor string | YES | CPUID 0x40000000 read — unknown vendor flagged |
| CPUID feature filtering | YES | Cross-leaf consistency check — missing features = HV filtering |
| MSR hooking (LSTAR, SYSENTER_EIP) | YES | Read-modify-write cycle under CLI/STI — readback mismatch = interception |
| EPT page splitting (execute vs read) | PARTIAL | INVLPG forces TLB miss → fresh page walk reveals split |
| TSC manipulation | YES | RDTSC quad-check with CPUID calibration, WRMSR+RDPMC timing |
| Hyper-V binary patching (hvix64/hvax64) | YES | Disk vs memory hash comparison |
| Type-1 HV below Windows | PARTIAL | MSR probe, RDTSC timing, but no binary to hash |
| VBS/HVCI state change | YES | VslGetSecurePciEnabled check |
| HV shared page anomaly | YES | NtQuerySystemInformation(0xC5) — non-standard address pattern |
| ACPI table manipulation | YES | HalAcpiGetTableEx from 4 sites — checks SLIC, MADT, FADT |

---

## Detected DMA / Firmware Techniques

| Technique | Detected? | How |
|----------|-----------|-----|
| PCILeech default FPGA (Xilinx ID) | YES | PCI config space scan — 512 HalGetBusDataByOffset + port 0xCF8/0xCFC I/O |
| Custom FPGA with spoofed vendor/device ID | NO | Looks like legitimate PCI device |
| Thunderbolt DMA adapter | PARTIAL | PCI device logged, presence noted |
| IOMMU bypass | PARTIAL | VBS state check detects if IOMMU active |
| DMA device with unusual BAR config | YES | PCI bus/device/function topology walk (§215) |
| Modified UEFI firmware | YES | SMBIOS cross-validation with CPUID, ACPI table probes |
| Bootkit (MBR/VBR modification) | YES | TPM PCR mismatch, Secure Boot chain validation |
| UEFI bootkit (firmware DXE) | YES | TPM PCR[0-7] mismatch, firmware table parsing |
| Early-launch driver injection | YES | TPM PCR[11] mismatch |
| BCD test-signing enabled | YES | Registry BCD element check |
| Modified ntoskrnl/securekernel on disk | YES | Disk vs memory image hash |

---

## Kernel Calls EAC Monitors

Calls EAC watches for unauthorized use (by cheats calling them):

| Call | Monitoring Method |
|------|------------------|
| NtOpenProcess | ObRegisterCallbacks — strips VM_READ/WRITE/OPERATION |
| NtOpenThread | ObRegisterCallbacks — strips SET/GET_CONTEXT, SUSPEND_RESUME |
| NtDuplicateObject | ObRegisterCallbacks — same stripping on duped handles |
| NtWriteVirtualMemory | Handle stripping blocks; stack walking catches kernel calls |
| NtAllocateVirtualMemory | Handle stripping; kernel allocs visible in VAD |
| NtProtectVirtualMemory | Handle stripping; RWX regions flagged by scan |
| PsCreateSystemThread | Thread start address must be in known module |
| NtCreateThreadEx | Process notify catches new threads in game process |
| NtSetContextThread | Handle stripping blocks |
| NtLoadDriver / ZwLoadDriver | SeRegisterImageVerificationCallback fires pre-execution |
| MmLoadSystemImage | Same callback chain |

---

## Kernel Calls EAC Makes (Detection Tools)

| Call | Usage |
|------|-------|
| RtlWalkFrameChain | Stack frame capture (1% sampling in callbacks) |
| RtlVirtualUnwind + RtlLookupFunctionEntry | Return address validation per frame |
| MmCopyVirtualMemory | Read game process memory pages |
| MmCopyMemory | Safe kernel memory reads |
| PsLookupProcessByProcessId | PID resolution (512/scan pass) |
| PsLookupThreadByThreadId | TID resolution (1000+/scan cycle) |
| PsGetProcessImageFileName | Process name for blacklist |
| SeQueryInformationToken | Token privileges, user SID, AppContainer SID |
| ObReferenceObjectByName | Named object lookup (`\Driver\disk`, etc.) |
| CiValidateFileObject | Signed file on disk vs process image |
| HalGetBusDataByOffset | PCI config reads (512) |
| HalAcpiGetTableEx | ACPI table reads (4 sites) |
| IoWMIOpenBlock + IoWMIQueryAllData | WMI/SMBIOS data |
| FltReadFile | File contents via minifilter |
| RtlCompareString | Module name blacklist (4,674 calls) |
| NtQuerySystemInformation(0x05) | Full process list |
| NtQuerySystemInformation(0x0B) | All loaded modules (64 calls) |
| NtQuerySystemInformation(0x10) | All open handles system-wide |
| NtQuerySystemInformation(0x67) | SMBIOS/ACPI firmware tables |
| NtQuerySystemInformation(0xC5) | Hypervisor shared page |

---

## Kernel Callbacks Registered

| Callback | API | Scope |
|----------|-----|-------|
| Handle filter | ObRegisterCallbacks | Every NtOpenProcess/NtOpenThread |
| Process notify | PsSetCreateProcessNotifyRoutine(Ex) | Every process create/exit |
| Thread notify | PsSetCreateThreadNotifyRoutine | Every thread creation |
| Image load | PsSetLoadImageNotifyRoutine | Every DLL/driver load |
| Image verification | SeRegisterImageVerificationCallback | Every driver CI check |
| Filesystem minifilter | FltRegisterFilter | All file I/O system-wide |
| NMI callback | KeRegisterNmiCallback | NMI on all cores |
| Debug print | DbgSetDebugPrintCallback | All debug output |
| WinTrust provider | Custom registration | Signature verification |

---

## Memory Scanning Methods

| Method | What It Does |
|--------|-------------|
| MmCopyVirtualMemory page scan | SHA-256 of 4096-byte pages vs expected hashes |
| SIMD content scanner (SSE2/AVX/AVX2/AVX-512) | 16-64 bytes/cycle, ASCII pattern detection in memory |
| AOB signature scanner (IDA/CE format) | Byte pattern + mask matching against ntoskrnl prologues |
| PE on-disk vs in-memory comparison | Field-by-field PE header fingerprint diff |
| PE section integrity hash | Per-section hash with .rsrc/.reloc exclusion |
| PE section characteristics check | W+X flagging, unusual protection attributes |
| Marker-bounded integrity hash | XOR-decode bounded region → hash → compare expected |
| Import/export table validation | Resolved addresses must be within owning module |
| CRC32 hardware integrity check | CRC32C instruction-based fast integrity |
| Spinlock memory watchpoint | Detects concurrent access to monitored regions |
| CR3 page table walk | Physical memory read bypassing all API monitoring |

---

## Cheat Tool Detection

| Tool | Detection Method |
|------|-----------------|
| Cheat Engine (dbk64.sys + process) | Module blacklist + process name scan |
| x64dbg | Process name scan |
| ReClass.NET | Process name scan |
| Process Hacker / System Informer | Process name scan |
| IDA Pro | Process name scan |
| Kernel debugger (WinDbg) | KdDebuggerEnabled check, INT 1/3 SEH traps |
| Frida / DBI | RDTSC timing detection |
| Custom debug tools | Build path PDB detection, module name matching |

---

## Anti-Debug

| Check | Method |
|-------|--------|
| INT 1 SEH trap | Driver init gate — debugger consuming = driver fails to load |
| INT 3 SEH trap | Same + periodic re-check |
| RDTSC timing | Back-to-back with randomized loop — single-step/VM overhead detected |
| KdDebuggerEnabled | Direct kernel variable read — ever-set = flagged |
| RDTSC+CPUID quad check | Ratio comparison — HV exit cost detected |
| WRMSR+RDPMC | Performance counter timing channel |
| KUSER_SHARED_DATA timestamps | Unhookable timing source |

---

## Communication Channel Detection

| Channel | Detected? | How |
|---------|-----------|-----|
| Named device object | YES | \Device\ directory enumeration |
| Named section (shared memory) | YES | Handle table scan |
| Named pipe | YES | \Device\NamedPipe\ enumeration |
| Unnamed device | PARTIAL | IoEnumerateDeviceObjectList |
| Unnamed section (kernel handle) | PARTIAL | Handle visible if not kernel-only |
| MDL-based shared page | NO | No handle, no section object |
| Physical memory mapping | NO | No handle, no section, no object |
| WNF state data | NO | EAC doesn't scan WNF |
| PCI BAR register signaling | NO | Outside OS object model |
| Direct function call between drivers | YES | Stack walking |

---

## Detection Timing

| Detection | Frequency |
|-----------|-----------|
| Stack walking | 1% per kernel callback |
| ObRegisterCallbacks | Every handle operation system-wide |
| Process scanning | Continuous (512 PIDs/pass) |
| Thread enumeration | Continuous (1000+ TIDs/cycle) |
| Memory integrity | Periodic (dozens of regions/cycle) |
| Module blacklist | 10+ full passes over PsLoadedModuleList |
| PCI config scan | Init + periodic (512 reads) |
| Export table walking | FLTMGR: 10K+, hal: 4.4K, cng: 2.9K, CI: 571 |
| Filesystem minifilter | Every file I/O |
| RDTSC timing | Init only |
| Code self-check | Periodic |
| Cert store scan | ~1000 NtQueryValueKey, ~550 NtOpenKey |


# EAC EOS — HWID Identifiers & Spoofing

Every hardware identifier EAC collects and how to spoof it.

---

## Collected Identifiers

| Identifier | Source | Format String | Address | Spoof Method |
|-----------|--------|--------------|---------|-------------|
| **SMBIOS UUID** | Type 1 System Info | `%016llx-%016llx` | 0x140300AEA | Firmware table hook (IoWMIOpenBlock intercept) or BIOS mod |
| **SMBIOS System Serial** | Type 1 System Info | part of 128-bit ID | 0x140300B30 | Same as UUID — must match manufacturer |
| **SMBIOS Board Serial** | Type 2 Baseboard | part of 128-bit ID | — | Firmware table hook |
| **SMBIOS Enclosure Serial** | Type 3 Enclosure | part of fingerprint | — | Firmware table hook |
| **Memory Serial** | Type 17 Memory Device | part of fingerprint | — | Firmware table hook — change per-DIMM |
| **Processor ID** | Type 4 / CPUID Leaf 1 EAX | part of fingerprint | — | HV CPUID intercept — must match SMBIOS Type 4 |
| **Processor Brand String** | CPUID 0x80000002-4 | 48-char string | — | HV CPUID intercept |
| **MBR Disk Signature** | MBR sector 0 offset 0x1B8 | `MBR:%08x` | 0x140300B1E | Hook IOCTL_DISK_GET_DRIVE_GEOMETRY or write MBR directly |
| **SCSI LUN** | SCSI inquiry | `:lun=%llx` | 0x140300B0A | Storage miniport filter — intercept inquiry response |
| **Disk Serial** | SCSI inquiry / SMART | part of fingerprint | — | Storage miniport filter |
| **Partition Index** | Partition table | `Part:%u` | 0x140300BFC | Partition table metadata mod |
| **MAC Address** | NDIS permanent | `%02x:%02x:%02x:%02x:%02x:%02x` | 0x140300BC0 | NDIS LWF driver intercepting OID_802_3_PERMANENT_ADDRESS |
| **IPv4 Address** | Adapter config | `%u.%u.%u.%u` | 0x140300C18 | Adapter reconfig — unreliable ban ID (DHCP changes) |
| **Optical Drive** | Device enum | `CD:%u` | 0x140300C0C | Not critical |
| **TPM Endorsement Key** | Tbsi_Context_Create → Tbsip_Submit_Command | hash | — | No viable software spoof — skip TPM or no TPM |
| **TPM PCR Values** | TCG 2.0 event log | PCR[0-14] | — | Hardware-sealed — can't modify without re-seal |
| **PCI Device IDs** | HalGetBusDataByOffset + port 0xCF8/0xCFC | vendor:device pairs | — | DMA device firmware config space spoof |
| **CPUID Feature Flags** | Leaves 0x1, 0x7 | bitmask | DAT_14030a240 | HV CPUID passthrough — don't filter |
| **CPUID Vendor String** | Leaf 0x0 | 12-char | — | HV CPUID passthrough |

---

## SMBIOS Types Parsed

| Type | Name | Key Fields |
|------|------|-----------|
| 0 | BIOS Information | Vendor, version, release date, ROM size |
| 1 | System Information | Manufacturer, product, version, **serial**, **UUID** |
| 2 | Baseboard | Manufacturer, product, version, **serial**, asset tag |
| 3 | System Enclosure | Manufacturer, type, version, **serial**, asset tag |
| 4 | Processor | Manufacturer, version, **processor ID**, voltage, speed |
| 11 | OEM Strings | All OEM entries |
| 17 | Memory Device | Manufacturer, **serial**, part number, size, speed, locator |
| 127 | End-of-Table | Sentinel |

Collected via `IoWMIOpenBlock(MSSmBios_RawSMBiosTables)` + `IoWMIQueryAllData` + `NtQuerySystemInformation(0x67)`.

EAC validates "CsIc"/"CrIc" tag markers in firmware tables (§186) — invalid/missing tags = spoof detected.

---

## CPUID Leaves Collected

| Leaf | Data | Ban Relevance |
|------|------|--------------|
| 0x00 | Vendor (GenuineIntel / AuthenticAMD) | CPU vendor fingerprint |
| 0x01 | Family/Model/Stepping + features | CPU identity |
| 0x02-0x0F | Cache, TLB, topology | Hardware topology |
| 0x17+ | Extended HW fingerprinting (SOC vendor) | Deep fingerprint |
| 0x40000000 | Hypervisor vendor | HV detection |
| 0x80000002-4 | Brand string ("Intel Core i9-...") | CPU name |

---

## PCI Devices Fingerprinted

512 reads across all bus/device/function combos. Per device: vendor ID, device ID, subsystem vendor, subsystem ID, class code, revision.

**Known flagged devices:**

| Vendor | Device | What |
|--------|--------|------|
| 0x15AD | 0x0405 | VMware SVGA |
| 0x15AD | 0x0740 | VMware vmxnet3 |
| 0x15AD | 0x07B0 | VMware PVSCSI |
| 0x1AF4 | 0x1000-0x107F | VirtIO (KVM/QEMU) |
| 0x1414 | 0x5353 | Hyper-V video |
| 0x1B36 | * | QEMU |
| Xilinx default | — | PCILeech/Screamer FPGA |

---

## Ban Enforcement Function

**FUN_140149b2d** (§38):

```
1. Read live hardware value
2. Server sends check_bitmask — controls which IDs to check
3. Decrypt expected banned value (per-entry PRNG XOR key schedule)
4. Case-insensitive compare (& 0xDF mask) — changing case won't evade
5. Match → BAN_FLAG set
6. Two-stage verification (primary match → secondary lookup → confirm)
7. Zero decrypted buffers after compare (anti-forensics)
```

- Server controls which checks run per scan wave
- Ban database encrypted — can't dump statically
- Two-stage reduces false positives

---

## Spoof Priority Checklist

| Priority | Identifier | Why |
|----------|-----------|-----|
| CRITICAL | SMBIOS UUID (Type 1) | Primary hardware fingerprint component |
| CRITICAL | SMBIOS System Serial (Type 1) | Primary hardware fingerprint component |
| CRITICAL | SMBIOS Board Serial (Type 2) | Primary hardware fingerprint component |
| HIGH | MBR Disk Signature | Explicitly collected with format string |
| HIGH | MAC Address (permanent) | Explicitly collected — must spoof BEFORE EAC reads |
| HIGH | Disk Serial (SCSI) | Part of composite fingerprint |
| MEDIUM | SMBIOS Memory Serial (Type 17) | Per-DIMM serial in fingerprint |
| MEDIUM | CPUID Processor ID | Must match SMBIOS Type 4 |
| MEDIUM | CPUID Brand String | Cross-validated with SMBIOS |
| MEDIUM | PCI Device IDs | DMA setups only |
| LOW | IPv4 Address | Changes with DHCP — weak identifier |
| LOW | Partition Index | Minor component |
| SKIP | TPM Endorsement Key | No viable software spoof — disable TPM or don't have one |

---

## Cross-Validation Requirements

EAC cross-checks identifiers. Changing one without others = inconsistency flag:

- SMBIOS Type 4 Processor **must match** CPUID data
- SMBIOS Manufacturer **must be consistent** with ACPI OEM tables
- MAC OUI prefix **must match** a real vendor
- Disk serial **must be consistent** with controller type
- All identifiers **must change together** to look like "new hardware"
- SMBIOS firmware tables **must pass** CsIc/CrIc tag validation

---

## Spoof Implementation Notes

**Firmware table hook:** Intercept `IoWMIOpenBlock` / `IoWMIQueryAllData` / `NtQuerySystemInformation(0x67)` at kernel level. Return modified SMBIOS tables with new serials/UUIDs. Must maintain valid SMBIOS structure (correct lengths, checksums, type ordering).

**NDIS MAC spoof:** NDIS lightweight filter driver. Must intercept `OID_802_3_PERMANENT_ADDRESS` — not just current address. Install before EAC loads or the original MAC is already collected.

**Disk serial spoof:** Storage miniport filter intercepting SCSI inquiry responses. Must return consistent serial across multiple query paths (IOCTL_STORAGE_QUERY_PROPERTY, SMART, SCSI inquiry).

**CPUID spoof:** Requires hypervisor. Intercept CPUID exits, return modified leaf data. Ensure all related leaves are consistent (leaf 1 family/model must match leaf 0x80000002-4 brand string).


# EAC EOS — Ban Traces & Cleaner Reference

Everything EAC leaves on the system that needs cleaning after a ban.

---

## Registry Traces

| Path | What | Clean Method |
|------|------|-------------|
| `HKLM\SYSTEM\CCS\Services\EasyAntiCheat_EOS` | Driver service key + detection records | Delete entire key tree |
| `HKLM\SYSTEM\CCS\Services\EasyAntiCheat_EOS\*` | Per-process detection result values | Delete values |
| `HKLM\SYSTEM\CCS\Services\EasyAntiCheat` (non-EOS) | Legacy EAC service key | Delete if present |
| `HKLM\SYSTEM\CCS\Enum\*` (driver entries) | PnP driver enumeration history | Delete EAC-related entries |
| `HKLM\SOFTWARE\WOW6432Node\EasyAntiCheat` | EAC config/install data | Delete entire key |
| `HKLM\SYSTEM\CCS\Control\WMI\Security` | WMI access audit trail | Check for EAC GUIDs |
| `HKCU\SOFTWARE\EpicGames\*` | Game launcher state, ban state cached | Delete or reset |

---

## Filesystem Traces

| Path | What | Clean Method |
|------|------|-------------|
| `C:\Program Files (x86)\EasyAntiCheat\` | EAC install dir, certs, configs | Delete folder |
| `C:\Program Files (x86)\EasyAntiCheat_EOS\` | EOS variant install | Delete folder |
| `%APPDATA%\EasyAntiCheat\` | User-level EAC data | Delete folder |
| `%LOCALAPPDATA%\EasyAntiCheat\` | Cached detection data, logs | Delete folder |
| `%SystemRoot%\System32\drivers\EasyAntiCheat*.sys` | Kernel driver on disk | Delete |
| `%SystemRoot%\Prefetch\EASYANTICHEAT*.pf` | Prefetch trace (records load history) | Delete matching .pf files |
| `%SystemRoot%\MEMORY.DMP` | Crash dump — contains 0xEAC bugcheck if triggered | Delete if contains EAC bugcheck |
| `%SystemRoot%\Minidump\*.dmp` | Mini crash dumps | Delete EAC-dated dumps |
| `%SystemRoot%\System32\winevt\Logs\System.evtx` | Event log — driver load/unload/crash events | Clear or selectively delete |
| Game install dir | Per-game EAC folder (e.g. `FortniteGame\Binaries\Win64\EasyAntiCheat\`) | Delete EAC subfolder |
| `%ProgramData%\EasyAntiCheat\` | Shared EAC data | Delete folder |

---

## Certificate Store Traces

EAC reads 4 cert stores and leaves access timestamps:

| Store | Registry Path |
|-------|-------------|
| ROOT | `HKLM\SOFTWARE\Microsoft\SystemCertificates\ROOT\Certificates\` |
| AuthRoot | `HKLM\SOFTWARE\Microsoft\SystemCertificates\AuthRoot\Certificates\` |
| TrustedAppRoot | `HKLM\SOFTWARE\Microsoft\SystemCertificates\TrustedAppRoot\Certificates\` |
| FlightRoot | `HKLM\SOFTWARE\Microsoft\SystemCertificates\FlightRoot\Certificates\` |

~1000 NtQueryValueKey + ~550 NtOpenKey calls per scan. Last-access timestamps on these keys can show EAC was reading them. Not critical for ban evasion but forensic indicator.

---

## Kernel Object Traces

| Object | Path | Lifetime |
|--------|------|----------|
| Device object | `\Device\EasyAntiCheat_EOS` | Deleted on driver unload |
| Symbolic link | `\DosDevices\EasyAntiCheat_EOS` | Deleted on driver unload |
| Shared sections | `\EasyAntiCheat_EOS\*` | Closed on teardown |
| MiniFilter instance | FltMgr altitude registration | Unregistered on teardown |

These are transient — exist only while driver is loaded. No cleanup needed.

---

## Kernel Callback Residue

| Callback | Registration |
|----------|-------------|
| ObRegisterCallbacks | Unregistered on teardown (§32) |
| PsSetCreateProcessNotifyRoutine(Ex) | Unregistered on teardown |
| PsSetCreateThreadNotifyRoutine | Unregistered on teardown |
| PsSetLoadImageNotifyRoutine | Unregistered on teardown |
| SeRegisterImageVerificationCallback | Unregistered on teardown |
| FltRegisterFilter | Unregistered on teardown |
| KeRegisterNmiCallback | Unregistered on teardown |
| DbgSetDebugPrintCallback | Unregistered on teardown |
| WinTrust provider | Custom deregistration |

All unregistered on clean driver unload. If driver crashes (0xEAC bugcheck), callbacks may not deregister cleanly — reboot clears them.

---

## TEB Detection Flags

| Trace | Location | Clean |
|-------|----------|-------|
| Per-thread detection flags | `TEB.SystemReserved1[0xf]` | Cleared per-thread on teardown |
| Detection flag accumulator | Kernel state mirror | Cleared on teardown |

If EAC crashes, TEB flags may persist until thread exit. Not a ban trace — per-session only.

---

## Server-Side Traces (Can't Clean)

| What | Notes |
|------|-------|
| HWID fingerprint | SMBIOS UUID + board serial + disk serial + MAC — stored server-side |
| Detection telemetry | All detection results sent via TLV protocol to *.easy.ac |
| Ban database entry | Encrypted per-entry, case-insensitive compare (& 0xDF) |
| EOS account linkage | Epic account → HWID mapping |
| Game ID + detection profile | Per-game detection history |
| TPM attestation data | PCR values, boot chain measurement — sent and stored |
| IP address history | Logged per-session |

You can't clean server-side data. This is why HWID spoofing is required — you need to present as "new hardware."

---

## Windows Artifacts (OS-Level)

| Artifact | Location | Clean Method |
|----------|----------|-------------|
| Prefetch/Superfetch DB | `%SystemRoot%\Prefetch\` | Delete all EAC .pf files |
| Amcache.hve | `%SystemRoot%\AppCompat\Programs\Amcache.hve` | Delete EAC driver entries (requires offline edit) |
| ShimCache | `HKLM\SYSTEM\CCS\Control\Session Manager\AppCompatCache` | Flush cache (reboot clears volatile entries) |
| SRUM (System Resource Usage Monitor) | `%SystemRoot%\System32\sru\SRUDB.dat` | Delete or clear EAC entries |
| BAM/DAM (Background Activity Monitor) | `HKLM\SYSTEM\CCS\Services\bam\State\UserSettings\*` | Delete EAC execution entries |
| UserAssist | `HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\UserAssist\*` | ROT13-encoded, delete matching GUIDs |
| Jump Lists | `%APPDATA%\Microsoft\Windows\Recent\AutomaticDestinations\` | Delete game/EAC launcher entries |
| USN Journal | NTFS volume journal | Can't selectively clean — full format or journal reset |
| Event Logs (all) | `%SystemRoot%\System32\winevt\Logs\` | Clear System + Application + Security logs (or selective) |

---

## Network Traces

| Trace | Location |
|-------|----------|
| DNS cache | `ipconfig /flushdns` clears |
| ARP cache | `arp -d *` clears |
| TCP/IP persistent connections | Reboot clears |
| Firewall log | `%SystemRoot%\System32\LogFiles\Firewall\` — delete if logging enabled |
| Router/gateway logs | Can't clean — ISP/router level |

---

## Cleaner Execution Order

1. **Uninstall game + EAC** (removes service, driver, game files)
2. **Delete EAC folders** (Program Files, AppData, ProgramData)
3. **Clean registry** (service keys, cert store timestamps, BAM/DAM, ShimCache, UserAssist)
4. **Clean OS artifacts** (Prefetch, Amcache, SRUM, Event Logs, Jump Lists)
5. **Flush network** (DNS, ARP)
6. **Spoof HWIDs** (all identifiers from `2-hwid-identifiers.md` — must be done before reinstall)
7. **New account** (fresh Epic/Steam account — old account is flagged server-side)
8. **Reinstall** (fresh game + EAC install on "new" hardware)

---

## What EAC Reports to Server on Detection

| Data | Protocol |
|------|----------|
| Detection type code | TLV field |
| Evidence virtual address | TLV field |
| Process name + PID | TLV field |
| Module hashes (SHA-256) | TLV field |
| Stack trace (return addresses) | TLV field |
| HWID composite fingerprint | TLV field |
| Timestamp (KUSER_SHARED_DATA) | TLV field |
| Game ID | TLV field |
| Boot environment info | TLV field |

All encrypted with PRNG stream cipher, compressed with Oodle, sent over TLS to EAC backend. Threshold-based — not every detection fires immediately; accumulates to significance threshold (§189).

---

## BSOD Artifacts (If __fastfail Triggered)

174 INT 0x29 sites with bugcheck 0xEAC. If triggered:

| Artifact | Location | Clean |
|----------|----------|-------|
| Full crash dump | `%SystemRoot%\MEMORY.DMP` | Delete |
| Mini dump | `%SystemRoot%\Minidump\*.dmp` | Delete timestamped dump |
| WER report | `%ProgramData%\Microsoft\Windows\WER\` | Delete report folders |
| Event log entry | System.evtx — BugCheck event | Clear event log |
| Registry crash count | Various crash reporting keys | Reset values |

Crash dump contains bugcheck code 0xEAC — forensic proof EAC detected something. Always delete crash artifacts.

Information from https://www.unknowncheats.me/forum/anti-cheat-research/774584-eac-rust-reversal-update-detection-vectors-hwid-identifiers-traces-misc.html
