# Riot Vanguard Tray Application (`vgtray.exe`) — Reconstructed Source

This repository contains the complete C/Win32 reconstruction of the notification tray utility **`vgtray.exe`** (Riot Vanguard v1.19.1-18), produced via Reverse Engineering Architecture (REA) analysis on the original executable at `C:\Program Files\Riot Vanguard\vgtray.exe`.

---

## 1. Architectural Overview

**`vgtray.exe`** is the graphical user interface (GUI) component of Riot Vanguard running in the interactive user session:
- Executes as an unprivileged user-mode application (`asInvoker`, requiring no direct UAC elevation).
- Creates and maintains the official Vanguard icon in the Windows Notification Area (*System Tray*).
- Serves as a passive status monitor for the central service `vgc.exe` and kernel driver `vgk.sys`.
- Provides an interactive context menu (Turn Vanguard Off, Notification Settings, Help/Support) and diagnostic log bundle exporter.

---

## 2. File and Directory Structure

```
vgtray_src/
├── vgtray.sln                  # Visual Studio 2022 Solution
├── vgtray.vcxproj              # MSVC Project Configuration (Release|x64, Unicode, /MT)
├── vgtray.vcxproj.filters      # Visual Studio Filter Structure
├── vgtray.rc                   # Version Metadata (1.19.1-18) and Icon Resources
├── vgtray.h                    # Master Architecture, UI, Tray, and SCM Structures Header
├── main.c                      # WinMain, Hidden Message Window, Shell_NotifyIconW, and Menu
├── service_precheck.c          # SCM Service Status Query for vgc Service
├── toast_registry.c            # Toast Notifications and Registry Preference Storage
├── log_exporter.c              # Diagnostic Log Archiver and Explorer Launcher
└── panel_d3d11.c               # Direct3D 11 Modern Status Flyout Panel Renderer
```

---

## 3. Component Details

### 3.1 [`main.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgtray_src/main.c)
- **Single-Instance Mutex**: Creates and verifies the named mutex `Global\RiotVanguardTrayMutex` to ensure only one tray instance executes concurrently.
- **Hidden Window Class**: Registers the window class `VanguardTrayClass` to receive asynchronous notification messages from the Windows shell (`WM_USER + 1`, `TaskbarCreated`).
- **`Shell_NotifyIconW` Operations**: Manages the life cycle of the taskbar icon (`NIM_ADD`, `NIM_MODIFY`, `NIM_DELETE`).
- **Tray Context Menu**: Handles right-click and left-click notifications, presenting standard options:
  - *"Turn Vanguard Off"* (requests clean shutdown of service `vgc`).
  - *"Notification Settings"* (configures alert popups for blocked vulnerable drivers).
  - *"Export Logs..."* (triggers log archive packaging).
  - *"Exit Tray"*.

### 3.2 [`service_precheck.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgtray_src/service_precheck.c)
- **SCM (Service Control Manager) Client**: Connects via `OpenSCManagerW` and inspects the live status of the `vgc` service (`QueryServiceStatusProcess`).
- **Status Indicator**: Dynamically modifies tray icon tooltips to indicate whether the anti-cheat is operational (`SERVICE_RUNNING`), paused, or in an error state.

### 3.3 [`toast_registry.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgtray_src/toast_registry.c)
- **System Notification Dispatcher**: Triggers balloon and toast notifications informing the player when a vulnerable or uncertified driver is blocked by `vgk.sys`.
- **Registry Preferences**: Reads and persists user display preferences under `HKCU\Software\Riot Games\Riot Vanguard`.

### 3.4 [`log_exporter.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgtray_src/log_exporter.c)
- **Diagnostic Collection**: Gathers diagnostic log records and audit traces from `%PROGRAMFILES%\Riot Vanguard\Logs`.
- **Export Packaging**: Archives logs and crash dumps for customer support analysis, automatically opening the output directory in Windows Explorer.

### 3.5 [`panel_d3d11.c`](file:///C:/Users/joaop/Antigravity-Workspace/vgtray_src/panel_d3d11.c)
- **Direct3D 11 Modern Flyout**: Renders a hardware-accelerated status panel using DirectX 11 / DXGI integrated with Desktop Window Manager (DWM) composition.
- Displays live session state and performance indicators without interfering with active full-screen game rendering.

---

## 4. Prerequisites

- **Visual Studio 2022** with the **Desktop development with C++** workload.
- **Windows SDK 10.0 or 11.0**.
- Direct3D 11 and DXGI runtime libraries (included with Windows SDK).
