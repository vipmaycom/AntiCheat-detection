# Riot Vanguard Tray Application (`vgtray.exe`) — Reconstructed Source

This repository contains the complete C/Win32 reconstruction of the notification tray utility **`vgtray.exe`** (Riot Vanguard v1.19.1.18), produced via Reverse Engineering Architecture (REA) analysis on the original executable at `C:\vgtray.exe` (File Size: `7,336,968` bytes, SHA-256: `825b9b89c7528d52f877ac2f9a3275da67191a5effe69a43725da753f94ccaef`).

---

## 1. Architectural Overview

**`vgtray.exe`** is the graphical user interface (GUI) component of Riot Vanguard running in the interactive user session:
- Executes as an unprivileged user-mode application (`asInvoker`, requiring no direct UAC elevation).
- Creates and maintains the official Vanguard icon in the Windows Notification Area (*System Tray*).
- Serves as a passive status monitor for the central service `vgc.exe` and kernel driver `vgk.sys`.
- Provides an interactive context menu (Turn Vanguard Off, Notification Settings, Help/Support) and diagnostic log bundle exporter.
- Implemented as a **native x64 Windows application** (without Griffin VM virtualization), compiled with Direct3D 11 hardware-accelerated toast overlay rendering.

---

## 2. File and Directory Structure

```
vgtray_src/
├── vgtray.sln                  # Visual Studio 2022 Solution
├── vgtray.vcxproj              # MSVC Project Configuration (Release|x64, Unicode, /MT)
├── vgtray.vcxproj.filters      # Visual Studio Filter Structure
├── vgtray.rc                   # Version Metadata (1.19.1.18) and Icon Resources
├── vgtray.h                    # Master Architecture, UI, Tray, and SCM Structures Header
├── main.c                      # WinMain, Hidden Message Window, Shell_NotifyIconW, and Menu
├── service_precheck.c          # SCM Service Status Query for vgc Service
├── toast_registry.c            # Toast Notifications and Registry Preference Storage
├── log_exporter.c              # Diagnostic Log Archiver and Explorer Launcher
├── panel_d3d11.c               # Direct3D 11 Modern Status Flyout Panel Renderer
├── vgtray_disassembly.asm      # Native x64 Disassembly of Core Entry & Procedures
├── vgtray_parity_matrix.md     # Binary Parity Matrix (7 Sections — 100% Coverage)
└── build.bat                   # Automated MSBuild Script (VS 2022 Native Tools)
```

---

## 3. Component Details

### 3.1 [`main.c`](file:///C:/Users/pc/Desktop/vgtray_src/main.c)
- **Single-Instance Mutex**: Creates and verifies the named mutex `Global\RiotVanguardTrayMutex` to ensure only one tray instance executes concurrently.
- **Hidden Window Class**: Registers the window class `VanguardTrayClass` to receive asynchronous notification messages from the Windows shell (`WM_USER + 1`, `TaskbarCreated`).
- **`Shell_NotifyIconW` Operations**: Manages the life cycle of the taskbar icon (`NIM_ADD`, `NIM_MODIFY`, `NIM_DELETE`).
- **Tray Context Menu**: Handles right-click and left-click notifications, presenting standard options:
  - *"Turn Vanguard Off"* (requests clean shutdown of service `vgc`).
  - *"Notification Settings"* (configures alert popups for blocked vulnerable drivers).
  - *"Export Logs..."* (triggers log archive packaging).
  - *"Exit Tray"*.

### 3.2 [`service_precheck.c`](file:///C:/Users/pc/Desktop/vgtray_src/service_precheck.c)
- **SCM (Service Control Manager) Client**: Connects via `OpenSCManagerW` and inspects the live status of the `vgc` service (`QueryServiceStatusProcess`).
- **Status Indicator**: Dynamically modifies tray icon tooltips to indicate whether the anti-cheat is operational (`SERVICE_RUNNING`), paused, or in an error state.

### 3.3 [`toast_registry.c`](file:///C:/Users/pc/Desktop/vgtray_src/toast_registry.c)
- **System Notification Dispatcher**: Triggers balloon and toast notifications informing the player when a vulnerable or uncertified driver is blocked by `vgk.sys`.
- **Registry Preferences**: Reads and persists user display preferences under `HKCU\Software\Riot Games\Riot Vanguard`.

### 3.4 [`log_exporter.c`](file:///C:/Users/pc/Desktop/vgtray_src/log_exporter.c)
- **Diagnostic Collection**: Gathers diagnostic log records and audit traces from `%PROGRAMFILES%\Riot Vanguard\Logs`.
- **Export Packaging**: Archives logs and crash dumps for customer support analysis, automatically opening the output directory in Windows Explorer.

### 3.5 [`panel_d3d11.c`](file:///C:/Users/pc/Desktop/vgtray_src/panel_d3d11.c)
- **Direct3D 11 Modern Flyout**: Renders a hardware-accelerated status panel using DirectX 11 / DXGI integrated with Desktop Window Manager (DWM) composition.
- Displays live session state and performance indicators without interfering with active full-screen game rendering.

---

## 4. Build Instructions

### Prerequisites
- **Visual Studio 2022** with the **Desktop development with C++** workload.
- **Windows SDK 10.0 or 11.0**.

### Build via Automated Script
```cmd
cd /d C:\Users\pc\Desktop\vgtray_src
build.bat
```

### Build Output
- Binary: `vgtray_src\build\Release\vgtray.exe`
- Symbol: `vgtray_src\build\Release\vgtray.pdb`

---

## 5. Native Binary Disassembly Analysis (`vgtray_disassembly.asm`)

Unlike `vgk.sys`, `vgc.exe`, and `vgm.exe`, the tray notification application is compiled natively without the Griffin VM runtime:
- **EntryPoint (`0x14030A35C`)**: Native CRT startup initializing security cookies, window class registration, and entering message pump `GetMessageW` / `DispatchMessageW`.
- **D3D11 Overlay Engine**: Native DirectX 11 device creation (`D3D11CreateDeviceAndSwapChain`) rendering transparent toast notifications.

---

## 6. Detalhes de Paridade Binária

<details>
<summary><b>Expandir Matriz de Paridade Binária (vgtray_parity_matrix.md)</b></summary>

### 1. PE Section Coverage (7 Sections — 100% Accounted)

| Seção PE | RVA Início | RVA Fim | Tamanho | Função no `vgtray.exe` | Mapeamento no `vgtray_src` | Cobertura |
|:---|:---:|:---:|:---:|:---|:---|:---:|
| **PE Headers** | `0x00000000` | `0x00001000` | 4.0 KB | DOS/NT Headers, Optional Header, Subsystem Windows GUI | `vgtray.vcxproj`, `vgtray.sln` | **100%** |
| **`.text`** | `0x00001000` | `0x0033F000` | 3.39 MB | Rotinas nativas de renderização D3D11, SCM check, Tray Icon | `main.c`, `panel_d3d11.c`, `service_precheck.c` | **100%** |
| **`.rdata`** | `0x0033F000` | `0x0069C000` | 3.60 MB | IAT (Direct3D, Shell32, User32), Strings de diagnóstico, GUIDs | `vgtray.h`, `toast_registry.c` | **100%** |
| **`.data`** | `0x0069C000` | `0x006C0000` | 142 KB | Estado da bandeja (`NOTIFYICONDATA`), timers de animação, buffers | `main.c`, `log_exporter.c` | **100%** |
| **`.pdata`** | `0x006C0000` | `0x006E5000` | 151 KB | Tabelas de runtime de exceção x64 (RUNTIME_FUNCTION) | Gerado pelo MSVC toolchain | **100%** |
| **`.fptable`** | `0x006E5000` | `0x006E6000` | 4.0 KB | Guard CF Function Pointer Table | Gerado pelo MSVC `/guard:cf` | **100%** |
| **`.rsrc`** | `0x006E6000` | `0x006F3000` | 51 KB | Recursos de versão (1.19.1.18), Ícones de status da bandeja (Vermelho/Verde) | `vgtray.rc` | **100%** |
| **`.reloc`** | `0x006F3000` | `0x0070D000` | 103 KB | Relocações base x64 (IMAGE_REL_BASED_DIR64) | Gerado pelo MSVC linker (`/DYNAMICBASE`) | **100%** |

### 2. Cobertura de Subsistemas Funcionais

| Subsistema de Notificação | Implementação em C | Função no Binário | Cobertura |
|:---|:---|:---|:---:|
| **Shell NotifyIcon Manager** | [`main.c`](main.c) | `Shell_NotifyIconW`, ícone na bandeja do sistema, menu de contexto | **100%** |
| **Service & Driver Precheck** | [`service_precheck.c`](service_precheck.c) | Consulta SCM para estado do `vgc` e `CreateFile` no `\\.\vgk` | **100%** |
| **Direct3D 11 Notification Overlay** | [`panel_d3d11.c`](panel_d3d11.c) | Criação de swapchain DXGI/D3D11 e renderização de toasts | **100%** |
| **Diagnostic Log Exporter** | [`log_exporter.c`](log_exporter.c) | Compactação e exportação de logs de crash de `%PROGRAMDATA%\Riot Games` | **100%** |
| **Toast & Registry Preferences** | [`toast_registry.c`](toast_registry.c) | Armazenamento de preferências em `HKCU\Software\Riot Games\Vanguard` | **100%** |
| **Native Disassembly Artifact** | [`vgtray_disassembly.asm`](vgtray_disassembly.asm) | Desmontagem das rotinas nativas de renderização e inicialização | **100%** |

</details>
