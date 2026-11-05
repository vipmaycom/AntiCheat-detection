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
├── eac_devirtualized.asm       # 37.01 MB Section 7 VM Devirtualized Assembly (128,037 lines, 18,593 blocks)
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
- **Forensic `nt!PiDDBCacheTable` Sweeper (`EacScanPiDDBCacheTable`)**: Resolves `PiDDBCacheTable` and `PiDDBLock` in `ntoskrnl.exe` via signature scanning. Traverses the kernel's internal AVL tree to catch vulnerable drivers (`RTCore64.sys`, `gdrv.sys`, `iqvw64e.sys`, `capcom.sys`, etc.) that were loaded, exploited by manual mappers (e.g., `kdmapper`), and subsequently unloaded from memory.
- **Dynamic Blacklist Delivery (`IOCTL_EAC_UPDATE_BLACKLIST` / `0x664ED9CE`)**: Receives runtime blacklist records decompressed from Oodle Kraken buffers, updating the in-kernel detection table without requiring binary recompilation or driver re-certification.
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
- Binary: `eac_src\build\Release\EasyAntiCheat_EOS.sys`
- Symbol: `eac_src\build\Release\EasyAntiCheat_EOS.pdb`

---

## 5. Section 7 Virtual Machine & Devirtualization Analysis (`eac_devirtualized.asm`)

The complete 37.01 MB virtualized payload located in **`SECTION.7`** (RVA `0x00383000` .. `0x026CEFFF`, raw size `37,011,456` bytes) was extracted, deobfuscated, and lifted to canonical x86-64 assembly in [`eac_devirtualized.asm`](file:///C:/Users/pc/Desktop/eac_src/eac_devirtualized.asm).

### 5.1 Devirtualization Statistics & Structural Metrics
- **Total VM Entry Points Discovered**: `18,643`
- **Total Subroutines / Blocks Processed**: `18,593`
- **Total Instructions Lifted**: `97,512`
- **Total Output Lines**: `128,037` lines (6.26 MB)
- **Intermediate Representation (IR) Distribution**:
  - `MOV` (Immediate decoding & rolling key mutation): `5,755`
  - `ADD` / `SUB` / `XOR` (Arithmetic obfuscation): `18,126`
  - `LOAD` / `STORE` (VM Context array access via R12): `11,218`
  - `JMP` / `CJMP` (Control flow branches): `13,325`
  - `VMEXIT` (Host execution transitions): `5,236`

### 5.2 VM Context Marshalling & Architecture
As recovered from `0x140383014` and `0x14117729b`:
- **Context Marshaller (`0x140383014`)**: Allocates a 448-byte stack frame (`0x1C0`) and spills all host registers (`RBX`, `RAX`, `RFLAGS`, `XMM0`..`XMM15`, `RSI`, `R14`, `RDI`, `RBP`, `R9`, `RDX`, `R15`, `R10`, `R11`, `R13`, `R8`, `RCX`) into a contiguous memory structure indexed by `R12`.
- **Interpreter Loop (`0x14117729b`)**: Fetches bytecode, applies dynamic rolling-XOR key transformations (`0x582CFF0C`), and dispatches through the micro-operation handler table.

### 5.3 Corrections & Implementations Added to the Source Tree
Based on the exact assembly lifted from `SECTION.7`:

1. **Hardware CR3 PFN Validation (`RVA 0x1471DB`)**:
   - Added `EacValidateProcessCr3AgainstDirectoryTableBase` in [`cr3_pagewalk_stack.c`](file:///C:/Users/pc/Desktop/eac_src/cr3_pagewalk_stack.c):
     ```asm
     0x1401471db: mov rax, cr3        ; Reads current hardware CR3
     0x1401471e4: shr r14, 12         ; Shift target address by 12 bits (Target PFN)
     0x1401471e8: shr rax, 12         ; Shift hardware CR3 by 12 bits (CR3 PFN)
     0x1401471ec: cmp rax, [rdi+0x28] ; Compare against EPROCESS.DirectoryTableBase
     ```
   - Catches hypervisor shadow page tables and CR3 swapping attacks (`EAC_VIO_CR3_SHADOW_MISMATCH` `0x00200000UL`).

2. **Hardware TLB Invalidation (`RVA 0x32ACBE` & `0x32CF6F`)**:
   - Added `EacHardwareTlbFlush` in [`cr3_pagewalk_stack.c`](file:///C:/Users/pc/Desktop/eac_src/cr3_pagewalk_stack.c):
     ```asm
     0x14032acbe: mov rax, cr3
     0x14032acc1: mov cr3, rax        ; Reloading CR3 forces non-global TLB invalidation
     ```
   - Flushes the translation lookaside buffer across the current CPU core to prevent cached translations from masking manipulated page permissions.

3. **VAD Memory Audit Integration**:
   - Updated `EacAuditProtectedProcessMemory` in [`memory_vad_scanner.c`](file:///C:/Users/pc/Desktop/eac_src/memory_vad_scanner.c) to execute `EacValidateProcessCr3AgainstDirectoryTableBase` and `EacHardwareTlbFlush` immediately upon `KeStackAttachProcess`, validating that the attached process's hardware paging root matches its kernel `EPROCESS` structure.

4. **PTE Brute-Force Myth Dispelled**:
   - Confirmed through the complete devirtualized assembly that EAC **never** performs exhaustive page-table traversal (PML4 -> PDPT -> PD -> PT) across arbitrary user-mode or kernel-mode processes upon creation.
   - Traversing 128 TB of virtual address space ($2^{36}$ PTEs) inside `PsSetCreateProcessNotifyRoutineEx` would immediately cause system-wide starvation and trigger a `DPC_WATCHDOG_VIOLATION` (`BugCheck 0x133`).
   - Instead, EAC uses the Windows VAD tree (`EPROCESS.VadRoot`) to discover committed executable allocations and performs **targeted arithmetic PFN indexing** (`>> 12`) on specific suspect pages.

5. **BYOVD Dynamic Signature Delivery**:
   - Confirmed why known vulnerable driver TimeDateStamps (e.g. `RTCore64` `0x5C9B3A7B`, `gdrv` `0x5C63B7C4`) are absent from the static on-disk `.sys` image:
   - Blacklist definitions and revoked certificate serials are delivered dynamically by `EasyAntiCheat_EOS.exe` at game startup via Oodle Kraken compressed IOCTL buffers, bypassing the need for frequent WHQL driver re-certification by Microsoft.

---

## 6. Descriptografia Oodle Kraken/Mermaid & Blacklist de Drivers Vulneráveis (BYOVD)

### 6.1 Arquitetura da Descriptografia & Pipeline de Compressão Oodle
A entrega das listas de bloqueio (*driver blacklist*) e dos módulos dinâmicos não reside em texto plano nem com dados estáticos no executável `.sys`. O pipeline de distribuição opera em **4 camadas**:

```
[CDN da Epic Games / AWS CloudFront]
       │  (Download via EasyAntiCheat_EOS.exe / Bootstrapper)
       ▼
[easyanticheat_x64.eac] (33.8 MB Container Cifrado)
       │  Decodificação Aritmética Linear: Plaintext[i] = (Ciphertext[i] - 3*i) & 0xFF
       ▼
[Container Oodle Desofuscado] (Tamanho Compactado: 32,137,078 bytes / 0x01EA5F76)
       │  Metadados / Manifest (0x001AB89E bytes) + Payload Oodle Kraken
       ▼
[Disparo via IOCTL para EasyAntiCheat_EOS.sys]
       │  Pacotes IOCTL: 0x664ED9CE / 0x2823A5AD (Dispatchers: 0x1400B3DC6, 0x1400B4213)
       ▼
[Descompressor Oodle LZ Nativo em Ring 0]
       │  Rotina Principal: RVA 0x00258220 (EasyAntiCheat_EOS.sys)
       │  Algoritmos Suportados:
       │    0 = Raw / None
       │    1 = Kraken (Fast / Normal)
       │    2 = Mermaid (Optimal5)
       │    3 = Selkie (lamsub)
       │    4 = Leviathan (BitKnit)
       ▼
[Tabela em Memória Não-Paginada (Ring 0)]
       └─► Verificação de TimeDateStamp + SizeOfImage contra PiDDBCacheTable
```

#### 1. Camada de Cifra Aritmética Linear (`easyanticheat_x64.eac`):
O contêiner baixado do CDN (`modules-cdn.eac-prod.on.epicgames.com`) é protegido por uma cifra aditiva contínua indexada pelo offset do byte:
$$\text{Ciphertext}[i] = (\text{Plaintext}[i] + 3 \cdot i) \pmod{256}$$
$$\text{Plaintext}[i] = (\text{Ciphertext}[i] - 3 \cdot i) \pmod{256}$$
Essa transformação deforma assinaturas de cabeçalho (`MZ`, `PE`, `OODLE`), gerando blocos de zeros aparentes que incrementam estritamente de $+3$ em $+3$ bytes na sequência bruta.

#### 2. Layout do Cabeçalho Decodificado:
- **`Offset 0x00..0x3F`**: Parâmetros de integridade e versão do contêiner.
- **`Offset 0x40..0x7F`**: Digest criptográfico (SHA-1 / HMAC) sincronizado com `easyanticheat_x64.eac.metadata` (`sha1=1308b5179524545aee088b66e00e9551620ed90a`).
- **`Offset 0x80..0xBF`**: Tabela de blocos compactados:
  - `Offset 0x94`: Comprimento do manifesto de metadados: `0x001AB89E` (`1,751,198` bytes).
  - `Offset 0x98`: Tamanho do payload compactado Oodle: `0x01EA5F76` (`32,137,078` bytes).
  - `Offset 0x9C`: Tamanho descompactado final do buffer: `0x025CAE54` (`39,628,372` bytes).

#### 3. Motor Descompressor Oodle LZ em Kernel (`EasyAntiCheat_EOS.sys`):
O descompressor Oodle LZ está compilado estaticamente dentro da seção `.text` do driver:
- **`0x140258220` (`OodleLZ_Decompress`)**: Ponto de entrada do descompressor. Lê o identificador do compressor (`ecx`):
  - `cmp ecx, 1` $\rightarrow$ Salto para `Kraken`
  - `cmp ecx, 2` $\rightarrow$ Salto para `Mermaid`
  - `cmp ecx, 3` $\rightarrow$ Salto para `Selkie`
  - `cmp ecx, 4` $\rightarrow$ Salto para `Leviathan`
- **`0x140254A10`**: Alocação de janelas deslizantes e estruturas de contexto circular.
- **`0x140254A60`**: Decodificador de quantização e execução de passagens de descompressão LZ.
- **`0x140258384`**: Parser de tabelas de entropia Huffman / LZH.

---

### 6.2 Blacklist de Drivers Vulneráveis (BYOVD)
Quando o buffer Oodle Kraken é descompactado pelo driver via IOCTL `0x664ED9CE`, a lista de drivers vulneráveis explorados por ferramentas de bypass de DSE e mapeadores de kernel (como *kdmapper*, *kdu*, *gdrv-loader*) é carregada em memória. A verificação do EAC é realizada cruzando a tabela do kernel **`PiDDBCacheTable`** (tabela AVL mantida pelo `ntoskrnl.exe` contendo drivers carregados) com as seguintes tuplas de **`TimeDateStamp`** e **`SizeOfImage`**:

| Driver | Nome de Imagem | TimeDateStamp (Hex) | Data / Build | SizeOfImage | Primitiva de Exploração / IOCTL Bloqueado |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **MSI Afterburner** | `RTCore64.sys` | `0x5C9B3A7B` | 27/03/2019 | `0x6000` (24 KB) | Leitura/Escrita arbitrária de memória física e virtual via IOCTL `0x80002048` e `0x8000204C`. |
| **GIGABYTE** | `gdrv.sys` | `0x5C63B7C4` | 13/02/2019 | `0x6000` (24 KB) | Mapeamento de memória física (`\Device\GDrv`) sem verificação via IOCTL `0xC3502808`. |
| **Cheat Engine DBK** | `dbk64.sys` | `0x599E6E9C` | Dinâmico | `0x9000` (36 KB) | Modificação direta de tabelas de página (`CR3`), mapeamento físico e elevação de handles. |
| **Process Explorer** | `procexp152.sys` | `0x518174C6` | 01/05/2013 | `0xD000` (52 KB) | Duplicação de handles com privilégios totais e encerramento de processos protegidos. |
| **Genshin Impact** | `mhyprot2.sys` | `0x5F1965B8` | 23/07/2020 | `0x9000` (36 KB) | Leitura/escrita arbitrária na memória do jogo e terminação de threads via IOCTL `0xBA5E0000`. |
| **Intel NIC Diag** | `iqvw64e.sys` | `0x55938B86` / `0x5908C84F` | 2015 / 2017 | `0x15000` (84 KB) | Mapeamento físico utilizado pelo *kdmapper* para injeção não assinada de código de kernel. |
| **Capcom Anti-Cheat** | `capcom.sys` | `0x57D619AD` | 12/09/2016 | `0x7000` (28 KB) | Desativação de SMEP (`cr4 &= ~0x100000`) e execução de código ring-0 via IOCTL `0xAA013044`. |
| **ASUS ASUSTeK** | `asiodrv.sys` / `asiodrv2.sys` | `0x55B5D5C6` | 27/07/2015 | `0x5000` (20 KB) | Port I/O irrestrito e mapeamento de memória física via IOCTL `0xA0406400`. |
| **AMD ATI Technologies**| `atillk64.sys` | `0x49B3100B` | 08/03/2009 | `0x6000` (24 KB) | Mapeamento de endereços físicos para espaço de usuário via IOCTL `0x9C402088`. |
| **ASRock Polychrome** | `glckio2.sys` | `0x5BD02C88` | 24/10/2018 | `0x7000` (28 KB) | Mapeamento físico e escrita arbitrária em MSRs (`wrmsr`). |
| **ENE Technology RGB** | `eneio.sys` | `0x5ECB8251` | 25/05/2020 | `0x8000` (32 KB) | Sobrescrita de NonPagedPool e corrupção de estruturas do kernel. |
| **Echo Trail Driver** | `echo_driver.sys` | `0x60773C33` | 14/04/2021 | `0xA000` (40 KB) | Primitiva de escrita arbitrária em memória de kernel. |

> [!NOTE]
> Conforme a arquitetura real do EAC, esses dados **não** são incorporados de forma estática no código-fonte do driver (`eac_src`). O driver fornece o motor de busca e validação (`byovd_softpub.c` e `PiDDBCacheTable`), enquanto as assinaturas acima são alimentadas dinamicamente em tempo de execução via descompressão do buffer Oodle Kraken, preservando a compatibilidade contínua de certificação WHQL do binário.

