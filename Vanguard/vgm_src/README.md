# Riot Vanguard User-Mode Module (`vgm.exe`) — Reconstructed Source

This repository contains the complete C/Win32 reconstruction of the user-mode anti-tamper module **`vgm.exe`** (Riot Vanguard v1.19.1-18+20261001.143443), produced via Reverse Engineering Architecture (REA) analysis on the original binary at `C:\Program Files\Riot Vanguard\vgm.exe`.

---

## 1. Architectural Overview

**`vgm.exe`** (*Vanguard User-mode Module*) serves as the **interactive user-session scanner and sentinel (Ring 3)** within the Vanguard ecosystem:
- Executes directly inside the **player's interactive desktop session** (`WinSta0`), overcoming the GUI isolation of `vgc.exe` (which runs restricted in Session 0).
- Installs a **Nirvana Syscall Hook** (`ProcessInstrumentationCallback`) to detect and audit direct system calls (*Direct Syscalls* and *Manual Mapping*).
- Performs anti-tamper routines via a **Cooperative Fiber Scheduler (`Fibers`)**, preventing adversaries from selectively freezing individual worker threads.
- Continuously probes CPU hardware debug registers (`DR0..DR7`).
- Features a self-unpacking **Packman / Griffin VM** runtime (~32 MB of obfuscated code) that verifies integrity using **SHA-3 (Keccak-f[1600])** and decrypts its runtime sections into memory on startup.

---

## 2. File and Directory Structure

```
vgm_src/
├── vgm.sln                     # Visual Studio 2022 Solution
├── vgm.vcxproj                 # MSVC Project Configuration (Release|x64, Unicode, /MT)
├── vgm.rc                      # Version Metadata (1.19.1-18+20261001.143443, asInvoker)
├── vgm.h                       # Master Header for Packman, Nirvana, Fibers, and Cryptography
├── main.c                      # WinMain, TLS Callbacks, WindowStation Check, and OEP Transfer
├── packman_loader.c            # Packman Unpacker, Keccak-f[1600], 'Stub' Slab Allocator
├── nirvana_callback.c          # ProcessInstrumentationCallback, Syscall Inspector, DR7 Probe
├── crypto_hash.c               # Inlined Crypto Suite (SHA-1, SHA-256, SHA-512, XXH64, XXH3-128)
├── crash_handler.c             # VGN-4 Fatal Error Filter and Dynamic dbgcore.dll Minidump Writer
└── fiber_ipc_net.c             # Cooperative 4-Fiber Scheduler, Named Pipe IPC, and WS2_32 Engine
```

---

## 3. Component Details

### 3.1 [`main.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgm_src/main.c)
- **`VgmTlsCallback0` & `VgmTlsCallback1`**: Thread Local Storage (TLS) callbacks executed prior to the main entry point, inspecting hardware debug registers immediately upon thread creation (`DLL_PROCESS_ATTACH`, `DLL_THREAD_ATTACH`).
- **`WindowStation` Validation**: Audits the window station (`GetProcessWindowStation` / `GetUserObjectInformationW`) to confirm the module runs in the interactive session.
- **`wWinMain` (Packman Bootstrap `entry` @ `0x1419cf13c`)**: Coordinates container integrity checks against descriptor `LAB_1419cf0c0`. On successful unpack, transfers control to the Original Entry Point (**OEP** at `0x14036785C`).

### 3.2 [`packman_loader.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgm_src/packman_loader.c)
- **Packman Descriptor (`LAB_1419cf0c0`)**: Stores the 32-byte master key, 12-byte master nonce, and relative offsets pointing to section `.riot0`.
- **8-Chunk Section Table**: Defines memory ranges and hashing parameters for packed sections (`.text`, `.rdata`, `.data`, `.pdata`, and `.grfn1`).
- **Keccak-f[1600] (SHA-3) Verification**: Runs 24 rounds of Keccak permutation over the chunk table before memory alteration (`0xB92332F6198F0B67`).
- **`'Stub'` (`0x62757453`) 64 KB Slab Allocator**: Independent slab allocation engine backed by a 512-bit bitmap, keeping anti-cheat allocations isolated from standard CRT heap structures.
- **Masked Syscall Pointers (`XOR 0x1EA0B1`)**: De-obfuscates native `ntdll.dll` pointers at point-of-use.
- **Stream Cipher & RFC 1950/1951 Verification**: Decrypts chunks in-place and validates Adler-32 and CRC-32 checksums before restoring final page protections.

### 3.3 [`nirvana_callback.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgm_src/nirvana_callback.c)
- **ProcessInstrumentationCallback**: Intercepts kernel-to-user-mode return transitions across all threads.
- **Instruction Origin Inspection**: Reads the 2 bytes immediately preceding the return instruction (`R10 - 2`):
  - `0x0F 0x05` : Standard x64 `syscall`.
  - `0xCD 0x2E` : Legacy interrupt gate (`int 2Eh`).
- **Direct Syscall / Unbacked Code Detection**: Compares instruction pointers against the loaded address bounds of `ntdll.dll`. Returns from unbacked or manual-mapped memory are flagged as suspicious.
- **`DR7` Hardware Breakpoint Probing**: Temporarily arms `DR7 = 0x200` via `NtSetContextThread` to detect external hardware breakpoint traps.

### 3.4 [`crypto_hash.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgm_src/crypto_hash.c)
- Zero-dependency implementation of the cryptographic primitives embedded in section `.riot1`:
  - **SHA-1** (FIPS 180-4).
  - **SHA-256** (FIPS 180-4).
  - **SHA-512** (FIPS 180-4).
  - **XXH64** and **XXH3-128** (high-speed non-cryptographic hashing for memory regions).
  - Custom standalone string formatter with `%wZ` (`UNICODE_STRING`) support.

### 3.5 [`crash_handler.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgm_src/crash_handler.c)
- **`VGN-4` Exception Handler**: Intercepts unrecoverable errors and presents the native dialog:
  `"VGN-4 An unrecoverable error has occurred and the process must be terminated!"`.
- **Dynamic Minidump Generation**: Dynamically resolves `dbgcore.dll` / `dbghelp.dll` -> `MiniDumpWriteDump`, generating structured crash dumps (`%s\%s_%llu_%08x.%s`) in the `\Crashes` folder.
- Launches Windows Explorer directly highlighting the dump file for user support submissions.

### 3.6 [`fiber_ipc_net.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgm_src/fiber_ipc_net.c)
- **Cooperative 4-Fiber Scheduler**: Converts the calling thread into a fiber (`ConvertThreadToFiber`) and round-robins across:
  - *Fiber 0*: Hardware debug register verification and self-audit.
  - *Fiber 1*: Signed heartbeat packet generation and Named Pipe dispatch (`\\.\pipe\vgk_vgm_ipc`).
  - *Fiber 2*: Non-blocking asynchronous network polling via `WS2_32.dll`.
  - *Fiber 3*: Periodic memory hash checks of the Packman chunk table.
- **CSPRNG**: Secure pseudo-random number generator utilizing `BCryptGenRandom` with fallback to `SystemFunction036` (`RtlGenRandom`).

---

## 4. Prerequisites

- **Visual Studio 2022** with the **Desktop development with C++** workload.
- **Windows SDK 10.0 or 11.0**.
- C11 / C17 MSVC compiler toolset (`v143`).
