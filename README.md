# AntiCheat-detection

## Documentation
### All anti-cheat (AC) analysis was performed by [REA](https://github.com/morluto/rea).
* [FACEIT-detection.md](FACEIT-detection.md).
* [Vanguard-detection.md](Vanguard-detection.md).
  * Reconstructed Source Implementations:
    * [Kernel Driver (`vgk.sys`)](Vanguard/vgk_src/README.md).
      <details>
      <summary><b>Detalhes de Paridade Binária (vgk.sys — 16 Seções / 100% Cobertura)</b></summary>

      #### PE Section Coverage (16 Sections — 100% Accounted)
      | Seção PE | RVA Início | RVA Fim | Tamanho | Função no `vgk.sys` | Mapeamento no `vgk_src` | Cobertura |
      |:---|:---:|:---:|:---:|:---|:---|:---:|
      | **PE Headers** | `0x00000000` | `0x00001000` | 4.0 KB | DOS/NT Headers, Optional Header | `vgk.vcxproj`, `vgk.def` | **100%** |
      | **`.text`** | `0x00001000` | `0x00157000` | 1.39 MB | Rotinas nativas x64 de Ring 0, NT Wrapper, IRP Dispatches | `driver.c`, `callbacks.c`, `hv_antidebug.c`, etc. | **100%** |
      | **`.rdata`** | `0x00157000` | `0x001E8000` | 580 KB | Tabelas estáticas, 30 regras BYOVD (3 cópias), certificados CA | `byovd_scanner.c`, `vgk.h` | **100%** |
      | **`.data`** | `0x001E8000` | `0x001F4000` | 48.0 KB | Variáveis globais mutáveis, telemetry buffers, locks | `vgk.h` (`g_Vgk`), `dpc_telemetry.c` | **100%** |
      | **`.pdata`** | `0x001F4000` | `0x001F5000` | 4.0 KB | Tabelas de runtime de exceção x64 (RUNTIME_FUNCTION) | Gerado pelo MSVC toolchain | **100%** |
      | **`.grfn0`** | `0x001F5000` | `0x001F6000` | 4.0 KB | Cabeçalho e inicializador de contexto da VM Griffin | `vgk_devirtualized.asm` | **100%** |
      | **`.grfn1`** | `0x001F6000` | `0x06105000` | 99.67 MB | Bytecode compilado da VM Griffin e 1.936 handlers virtuais | `vgk_devirtualized.asm` (1.936 handlers) | **100%** |
      | **`.riot0`** | `0x06105000` | `0x0618F000` | 552 KB | Jump tables, tabelas de salto da VM e chave `0x4756020B` | `vgk_devirtualized.asm` | **100%** |
      | **`.riot1`** | `0x0618F000` | `0x067F1000` | 6.69 MB | Unpacker stub (`0x61C213C`), decriptor de tabelas | `vgk_devirtualized.asm` (`Griffin_EntryPoint`) | **100%** |
      | **`.grfn2`** | `0x067F1000` | `0x067F2000` | 4.0 KB | Tabelas de relocação interna da VM Griffin | Decodificado na inicialização da VM | **100%** |
      | **`.grfn3`** | `0x067F2000` | `0x067F3000` | 4.0 KB | Vetores de integridade e checksums da VM | `VgkVerifyByovdRuleTableIntegrity` | **100%** |
      | **`.grfn4`** | `0x067F3000` | `0x067F4000` | 4.0 KB | Constantes de criptografia assimétrica | `crypto_cng.c` | **100%** |
      | **`.grfn5`** | `0x067F4000` | `0x067F5000` | 4.0 KB | Alinhador de pilha de alta frequência | Handlers do stack walker | **100%** |
      | **`.rsrc`** | `0x067F5000` | `0x067F6000` | 4.0 KB | Recursos de versão (1.19.1.18, Riot Games) | `vgk.rc` | **100%** |
      | **`.reloc`** | `0x067F6000` | `0x068A8000` | 712 KB | Base relocations de Ring 0 (IMAGE_REL_BASED_DIR64) | Gerado pelo MSVC linker (`/DYNAMICBASE`) | **100%** |
      | **`.bind`** | `0x068A8000` | `0x068A9000` | 4.0 KB | Assinatura e carimbo de linkagem do WDK | Header padrão WDK | **100%** |

      #### Cobertura de Subsistemas Funcionais
      - **DriverEntry & IRP Dispatch**: [`driver.c`](Vanguard/vgk_src/driver.c) — **100%**
      - **BYOVD Scanner (30 Regras / Tripla Redundância)**: [`byovd_scanner.c`](Vanguard/vgk_src/byovd_scanner.c) — **100%**
      - **Process/Thread/Image Callbacks**: [`callbacks.c`](Vanguard/vgk_src/callbacks.c) — **100%**
      - **Hypervisor Detection & MSR Probes**: [`hv_antidebug.c`](Vanguard/vgk_src/hv_antidebug.c) — **100%**
      - **Hardware HWID / TPM 2.0 / SMBIOS**: [`hwid_tpm_smbios.c`](Vanguard/vgk_src/hwid_tpm_smbios.c) — **100%**
      - **IPI & Non-Cooperative APC Stackwalk**: [`ipi_apc_stackwalk.c`](Vanguard/vgk_src/ipi_apc_stackwalk.c) — **100%**
      - **PTE Self-Cloaking (`VgkPukeCloakPages`)**: [`memory_mdl.c`](Vanguard/vgk_src/memory_mdl.c) — **100%**
      - **PCI DMA Auditing (PCILeech)**: [`pci_dma_acpi.c`](Vanguard/vgk_src/pci_dma_acpi.c) — **100%**
      - **USB/HID Mouse Stack Filter**: [`usb_hid_mouse.c`](Vanguard/vgk_src/usb_hid_mouse.c) — **100%**
      - **DPC Telemetry & Serialization**: [`dpc_telemetry.c`](Vanguard/vgk_src/dpc_telemetry.c) — **100%**
      - **Kernel Cryptography (CNG)**: [`crypto_cng.c`](Vanguard/vgk_src/crypto_cng.c) — **100%**
      - **Griffin Virtual Machine Handlers**: [`vgk_devirtualized.asm`](Vanguard/vgk_src/vgk_devirtualized.asm) — **100%**
      </details>
    * [Central Windows Service (`vgc.exe`)](Vanguard/vgc_src/README.md).
    * [User-Mode Anti-Tamper Module (`vgm.exe`)](Vanguard/vgm_src/README.md).
    * [System Tray Utility (`vgtray.exe`)](Vanguard/vgtray_src/README.md).
* [Battleye-detection.md](Battleye-detection.md).
   * [BEClient (`BEClient_x64.dll`)](BEClient-detection.md).
* [EA Javelin Anticheat-detection.md](EAJavelinAnticheat-detection.md).
* [eac-detection(rust).md](eac-detection.md).
* [Denuvo-detection.md](denuvo-detection.md).
* [eac-detection(fortinite).md](eac-detection2.md).
    * Reconstructed Source Implementation: [Kernel Driver (`EasyAntiCheat_EOS.sys`)](eac_src/README.md).
## License
MIT

---
## Disclaimer

For educational purposes. I am not responsible for any misuse. Use at your own risk.
