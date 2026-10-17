# Riot Vanguard Central Service (`vgc.exe`) — Reconstructed Source

This repository contains the complete C/Win32 reconstruction of the central service **`vgc.exe`** (Riot Vanguard v1.19.1-18), produced via Reverse Engineering Architecture (REA) analysis on the original executable at `C:\Program Files\Riot Vanguard\vgc.exe`.

---

## 1. Architectural Overview

**`vgc.exe`** runs as a **Windows Service** registered in the Service Control Manager (SCM) under the service name `vgc` (`SERVICE_WIN32_OWN_PROCESS`, configured for manual startup `SERVICE_DEMAND_START`).

It functions as the **central orchestrator** of the Vanguard anti-cheat ecosystem:
- Executes under the **`NT AUTHORITY\SYSTEM`** account in **Session 0**.
- Serves as the sole authorized user-mode communication bridge to the kernel-mode driver **`vgk.sys`**.
- Hosts the secure local IPC server (Named Pipe) to coordinate game client initialization and user-mode monitoring modules (`vgm.exe`).
- Monitors game processes (`VALORANT.exe`, `ShooterGame-Win64-Shipping.exe`) and manages outbound encrypted telemetry to Riot Cloud infrastructure via TLS/WinHTTP.

---

## 2. File and Directory Structure

```
vgc_src/
├── vgc.sln                     # Visual Studio 2022 Solution
├── vgc.vcxproj                 # MSVC Project Configuration (Release|x64, Unicode, /MT)
├── vgc.vcxproj.filters         # Visual Studio Filter Structure
├── vgc.rc                      # Version Metadata (1.19.1-18, Riot Vanguard Service)
├── vgc.h                       # Master Architecture, IPC, and Service Structures Header
├── main.c                      # ServiceMain, ServiceHandlerEx, CLI, and Lifecycle Management
├── driver_ipc.c                # Bidirectional Kernel IPC (\\.\vgk, CreateFile / DeviceIoControl)
├── game_ipc_server.c           # Secure Named Pipe Server (\\.\pipe\vgc_ipc_pipe)
├── process_watcher.c           # Process Lifecycle and Game Binary Integrity Auditing
└── telemetry_tls.c             # Outbound WinHTTP / TLS Client for Riot Cloud Telemetry
```

---

## 3. Component Details

### 3.1 [`main.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgc_src/main.c)
- **`wWinMain` / `wmain`**: Executable entry point. Supports SCM service execution via `StartServiceCtrlDispatcherW` or interactive CLI modes (`-console`, `-install`, `-uninstall`).
- **`VgcServiceMain`**: Primary service entry routine registered with the SCM. Transitions through `SERVICE_START_PENDING`, `SERVICE_RUNNING`, and `SERVICE_STOP_PENDING`.
- **`VgcServiceHandler`**: Processes SCM control messages (`SERVICE_CONTROL_STOP`, `SERVICE_CONTROL_SHUTDOWN`, `SERVICE_CONTROL_INTERROGATE`).
- **Worker Thread**: Initializes and coordinates all active background subsystems (driver IPC, game IPC server, process watcher, and telemetry engine).

### 3.2 [`driver_ipc.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgc_src/driver_ipc.c)
- **Kernel Device Handle**: Acquires an exclusive handle to the kernel driver via `CreateFileW(L"\\\\.\\vgk", ...)`.
- **Cryptographic Handshake & Key Exchange**: Conducts mutual session authentication with `vgk.sys` through `DeviceIoControl`, transmitting session tokens and retrieving kernel integrity state reports.
- **Heartbeat Verification**: Performs periodic pings to verify the driver remains loaded and untampered in kernel space.

### 3.3 [`game_ipc_server.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgc_src/game_ipc_server.c)
- **Secure Named Pipe Server**: Establishes and listens on `\\.\pipe\vgc_ipc_pipe` using restrictive discretionary access control lists (DACLs) restricted to verified game processes and callers.
- **Message Protocol**:
  - Enforces structured message frames with magic bytes, opcode IDs, payload length, and HMAC authentication.
  - Receives match session startup and shutdown notifications.
  - Synchronizes active session credentials between the game client and the kernel driver.

### 3.4 [`process_watcher.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgc_src/process_watcher.c)
- **Process Enumeration & Monitoring**: Watches the active Windows process tree for official game client executables.
- **Digital Signature Auditing**: Validates code-signing certificates and cryptographic hashes before authorizing competitive match entry.
- **Abnormal Termination Detection**: Identifies unexpected client crashes or process terminations to cleanly disarm kernel session states.

### 3.5 [`telemetry_tls.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgc_src/telemetry_tls.c)
- **WinHTTP / TLS Client**: Establishes secure HTTPS connections to official Riot Games telemetry endpoints.
- **Payload Packaging**: Serializes diagnostic metrics and kernel integrity logs into compressed, digitally signed payload packages.
- **Certificate Pinning**: Enforces remote endpoint certificate validation to prevent local network Man-in-the-Middle (MitM) inspection.

---

## 4. Prerequisites

- **Visual Studio 2022** with the **Desktop development with C++** workload.
- **Windows SDK 10.0 or 11.0**.
- C11 / C17 MSVC compiler toolset (`v143`).
