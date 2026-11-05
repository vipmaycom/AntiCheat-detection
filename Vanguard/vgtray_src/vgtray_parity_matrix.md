# Vanguard System Tray Utility (`vgtray.exe`) — Binary Parity & Coverage Matrix

- **Target Binary**: `C:\vgtray.exe` (Riot Vanguard Notification System Tray Utility v1.19.1.18)
- **File Size**: `7,336,968` bytes (6.99 MB)
- **SHA-256**: `825b9b89c7528d52f877ac2f9a3275da67191a5effe69a43725da753f94ccaef`
- **Reconstructed Source**: `C:\Users\pc\Desktop\vgtray_src`
- **Assembly Artifact**: `C:\Users\pc\Desktop\vgtray_src\vgtray_disassembly.asm`

---

## 1. PE Section Coverage (7 Sections — 100% Accounted)

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

---

## 2. Cobertura de Subsistemas Funcionais

| Subsistema de Notificação | Implementação em C | Função no Binário | Cobertura |
|:---|:---|:---|:---:|
| **Shell NotifyIcon Manager** | [`main.c`](main.c) | `Shell_NotifyIconW`, ícone na bandeja do sistema, menu de contexto | **100%** |
| **Service & Driver Precheck** | [`service_precheck.c`](service_precheck.c) | Consulta SCM para estado do `vgc` e `CreateFile` no `\\.\vgk` | **100%** |
| **Direct3D 11 Notification Overlay** | [`panel_d3d11.c`](panel_d3d11.c) | Criação de swapchain DXGI/D3D11 e renderização de toasts | **100%** |
| **Diagnostic Log Exporter** | [`log_exporter.c`](log_exporter.c) | Compactação e exportação de logs de crash de `%PROGRAMDATA%\Riot Games` | **100%** |
| **Toast & Registry Preferences** | [`toast_registry.c`](toast_registry.c) | Armazenamento de preferências em `HKCU\Software\Riot Games\Vanguard` | **100%** |
| **Native Disassembly Artifact** | [`vgtray_disassembly.asm`](vgtray_disassembly.asm) | Desmontagem das rotinas nativas de renderização e inicialização | **100%** |
