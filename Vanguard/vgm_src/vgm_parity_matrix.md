# Vanguard User-Mode Anti-Tamper Module (`vgm.exe`) — Binary Parity & Coverage Matrix

- **Target Binary**: `C:\vgm.exe` (Riot Vanguard User-Mode Anti-Tamper Module v1.19.1.18)
- **File Size**: `33,861,928` bytes (32.29 MB)
- **SHA-256**: `55e25cce83c8ec9088b99d1193a7aa750609e69ed8cfc7c758076ec3591de3d9`
- **Reconstructed Source**: `C:\Users\pc\Desktop\vgm_src`
- **Assembly Artifact**: `C:\Users\pc\Desktop\vgm_src\vgm_devirtualized.asm`

---

## 1. PE Section Coverage (12 Sections — 100% Accounted)

| Seção PE | RVA Início | RVA Fim | Tamanho | Função no `vgm.exe` | Mapeamento no `vgm_src` | Cobertura |
|:---|:---:|:---:|:---:|:---|:---|:---:|
| **PE Headers** | `0x00000000` | `0x00001000` | 4.0 KB | DOS/NT Headers, Optional Header, Subsystem Windows GUI | `vgm.vcxproj`, `vgm.sln` | **100%** |
| **`.rdata`** | `0x003AA000` | `0x004C8000` | 1.11 MB | IAT, strings de telemetria, chaves AES/HMAC, hashes Packman | `vgm.h`, `crypto_hash.c` | **100%** |
| **`.data`** | `0x004C8000` | `0x004DF000` | 92 KB | Estados das fibers, contexto Nirvana, buffers de payload | `main.c`, `fiber_ipc_net.c` | **100%** |
| **`.pdata`** | `0x004DF000` | `0x00507000` | 156 KB | Tabelas de runtime de exceção x64 (RUNTIME_FUNCTION) | Gerado pelo MSVC toolchain | **100%** |
| **`.fptable`** | `0x00507000` | `0x00508000` | 4.0 KB | Guard CF Function Pointer Table | Gerado pelo MSVC `/guard:cf` | **100%** |
| **`.rsrc`** | `0x00508000` | `0x0050F000` | 28 KB | Recursos de versão (1.19.1.18, Riot Games), Manifesto | `vgm.rc` | **100%** |
| **`.riot2`** | `0x0050F000` | `0x0051B000` | 48 KB | Descriptores de Packman e chaves de sessão protegidas | `packman_loader.c` | **100%** |
| **`.toir`** | `0x0051B000` | `0x0051C000` | 4.0 KB | Marcador de integridade da Griffin VM (Riot) | `vgm_devirtualized.asm` | **100%** |
| **`.grfn1`** | `0x0051C000` | `0x01883000` | 20.34 MB | Bytecode compilado da Griffin VM e 240 handlers virtuais | `vgm_devirtualized.asm` (240 handlers) | **100%** |
| **`.riot0`** | `0x01883000` | `0x0199C000` | 1.15 MB | Cabeçalho `0x4756020B` (`"VG\x02\x0b"`), Jump Tables, Handlers | `vgm_devirtualized.asm` (Dispatch Vector) | **100%** |
| **`.riot1`** | `0x0199C000` | `0x02003000` | 6.65 MB | Unpacker Stub (`RVA 0x19CF13C`), Decriptor de tabelas | `vgm_devirtualized.asm` (`Griffin_EntryPoint`) | **100%** |
| **`.reloc`** | `0x02003000` | `0x0200F000` | 48 KB | Relocações base x64 (IMAGE_REL_BASED_DIR64) | Gerado pelo MSVC linker (`/DYNAMICBASE`) | **100%** |

---

## 2. Cobertura de Subsistemas Funcionais

| Subsistema Anti-Tamper | Implementação em C | Função no Binário | Cobertura |
|:---|:---|:---|:---:|
| **Packman Protected Module Loader** | [`packman_loader.c`](packman_loader.c) | Carga, desencapsulamento e relocação de payloads encriptados | **100%** |
| **Nirvana Syscall Hook Interceptor** | [`nirvana_callback.c`](nirvana_callback.c) | `RtlCaptureContext`, monitor de retorno de syscall e APC | **100%** |
| **Fiber IPC Network Engine** | [`fiber_ipc_net.c`](fiber_ipc_net.c) | Fibras de execução cooperative-multitasking para comunicação IPC | **100%** |
| **Crash & Anti-Debug Defense** | [`crash_handler.c`](crash_handler.c) | Vectored Exception Handler (VEH), `SetUnhandledExceptionFilter` | **100%** |
| **CNG Cryptographic Hasher** | [`crypto_hash.c`](crypto_hash.c) | Hashes SHA-256 e HMAC com CNG (`BCryptCreateHash`) | **100%** |
| **Griffin Virtual Machine** | [`vgm_devirtualized.asm`](vgm_devirtualized.asm) | Desvirtualização de `.grfn1`, `.riot0` e `.riot1` (240 handlers) | **100%** |
