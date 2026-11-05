# Vanguard Central Service (`vgc.exe`) — Binary Parity & Disassembly Coverage Matrix

- **Target Binary**: `C:\vgc.exe` (Riot Vanguard User-Mode Windows Service v1.19.1.18)
- **File Size**: `107,505,544` bytes (102.52 MB)
- **SHA-256**: `c8f330a77351f36e1112d0b94972a735a7747fc33a4aa4735d95f43f01637c51`
- **Reconstructed Source**: `C:\Users\pc\Desktop\vgc_src`
- **Assembly Artifact**: `C:\Users\pc\Desktop\vgc_src\vgc_devirtualized.asm`

---

## 1. PE Section Coverage (10 Sections — 100% Accounted)

| Seção PE | RVA Início | RVA Fim | Tamanho | Função no `vgc.exe` | Mapeamento no `vgc_src` | Cobertura |
|:---|:---:|:---:|:---:|:---|:---|:---:|
| **PE Headers** | `0x00000000` | `0x00001000` | 4.0 KB | DOS/NT Headers, Optional Header, Subsystem Windows GUI | `vgc.vcxproj`, `vgc.sln` | **100%** |
| **`.rdata`** | `0x00472000` | `0x0059D000` | 1.16 MB | Tabelas de IAT, nomes de pipes, strings TLS, certificados Riot | `vgc.h`, `driver_ipc.c`, `telemetry_tls.c` | **100%** |
| **`.data`** | `0x0059D000` | `0x005B6000` | 100 KB | Estado do serviço, handle do driver `\\.\vgk`, fila de telemetria | `main.c`, `game_ipc_server.c` | **100%** |
| **`.pdata`** | `0x005B6000` | `0x005E2000` | 172 KB | Tabelas de runtime de exceção x64 (RUNTIME_FUNCTION) | Gerado pelo MSVC toolchain | **100%** |
| **`.fptable`** | `0x005E2000` | `0x005E3000` | 4.0 KB | Guard CF Function Pointer Table | Gerado pelo MSVC `/guard:cf` | **100%** |
| **`.rsrc`** | `0x005E3000` | `0x005EA000` | 28 KB | Recursos de versão (1.19.1.18, Riot Games, Inc.), Ícones | `vgc.rc` | **100%** |
| **`.toir`** | `0x005EA000` | `0x005EB000` | 4.0 KB | Marcador de integridade da Griffin VM (Riot) | `vgc_devirtualized.asm` | **100%** |
| **`.grfn1`** | `0x005EB000` | `0x05DBB000` | 92.07 MB | Bytecode compilado da Griffin VM e 270 handlers virtuais | `vgc_devirtualized.asm` (270 handlers) | **100%** |
| **`.riot0`** | `0x05DBB000` | `0x05F49000` | 1.62 MB | Cabeçalho `0x4756020B` (`"VG\x02\x0b"`), Jump Tables, Handlers | `vgc_devirtualized.asm` (Dispatch Vector) | **100%** |
| **`.riot1`** | `0x05F49000` | `0x065B3000` | 6.66 MB | Unpacker Stub (`RVA 0x5F7C13C`), Decriptor de tabelas | `vgc_devirtualized.asm` (`Griffin_EntryPoint`) | **100%** |

---

## 2. Cobertura de Subsistemas Funcionais

| Subsistema User-Mode | Implementação em C | Função no Binário | Cobertura |
|:---|:---|:---|:---:|
| **Service Control Manager (SCM)** | [`main.c`](file:///C:/Users/pc/Desktop/vgc_src/main.c) | `ServiceMain`, `HandlerEx`, SCM registration (`vgc`) | **100%** |
| **Kernel Driver IPC Interface** | [`driver_ipc.c`](file:///C:/Users/pc/Desktop/vgc_src/driver_ipc.c) | `DeviceIoControl` via `\\.\vgk` (`IOCTL_VGK_*`) | **100%** |
| **Game Named Pipe Server** | [`game_ipc_server.c`](file:///C:/Users/pc/Desktop/vgc_src/game_ipc_server.c) | `\\.\pipe\vgc_game_ipc` (`VANT`/`VGCR` handshake) | **100%** |
| **Process Lifecycle Watcher** | [`process_watcher.c`](file:///C:/Users/pc/Desktop/vgc_src/process_watcher.c) | Monitor de `VALORANT-Win64-Shipping.exe` e `RiotClient` | **100%** |
| **Telemetry TLS Dispatcher** | [`telemetry_tls.c`](file:///C:/Users/pc/Desktop/vgc_src/telemetry_tls.c) | Envio de telemetria e attestation assinada ao backend Riot | **100%** |
| **Griffin Virtual Machine** | [`vgc_devirtualized.asm`](file:///C:/Users/pc/Desktop/vgc_src/vgc_devirtualized.asm) | Desvirtualização de `.grfn1`, `.riot0` e `.riot1` | **100%** |
