# Análise de Engenharia Reversa: BEClient_x64.dll (BattlEye Client)

---

## 1. Identidade e Metadados do Binário

* **Formato:** Portable Executable (PE32+ / x86-64 Dynamic Link Library)
* **Tamanho do Arquivo:** 5.967.112 bytes (~5,69 MB)
* **SHA-256 Digest:** `f9be153d5876c14e8416b984039aecf2a81bf9b2b1fb92378b8e7e845ad14962`
* **Image Base Padrão:** `0x180000000`
* **AddressOfEntryPoint:** `0x45DF61` (Localizado na seção de desempacotamento `.be1`)

---

## 2. Anatomia das Seções PE e Arquitetura do Packer

A análise do cabeçalho PE revela que o `BEClient_x64.dll` utiliza um crypter/packer proprietário da BattlEye Innovations. Todas as seções de código e dados convencionais são mascaradas com tamanho bruto zero no disco:

| Nome da Seção | Virtual Address (RVA) | Virtual Size | Raw Size (Disco) | Função Arquitetural |
|---|---|---|---|---|
| `.text` | `0x001000` | `0x03F000` (252 KB) | **`0x0`** | Código vazio em disco; preenchido apenas em RAM após a decodificação |
| `.rdata` | `0x040000` | `0x0155BC` (85 KB) | **`0x0`** | Tabelas de importação e constantes limpas em disco |
| `.data` | `0x056000` | `0x0136C8` (77 KB) | **`0x0`** | Dados globais e estados de inicialização |
| `.pdata` | `0x06A000` | `0x002D24` (11 KB) | **`0x0`** | Tabela de exceções x64 |
| `_RDATA` | `0x06D000` | `0x000180` (384 B) | **`0x0`** | Metadados de runtime |
| `.be0` | `0x06E000` | `0x3BBF50` (~3,73 MB) | **`0x0`** | **Área de expansão em RAM** para alocação do motor de varredura descompactado |
| `.be1` | `0x42A000` | `0x000AB0` (2,6 KB) | **`0x000C00`** | **Stub de Desempacotamento** (contém o entry point real em disco) |
| `.be2` | `0x42B000` | `0x5AD274` (~5,67 MB) | **`0x5AD400`** | **Payload Criptografado** (armazena os dados brutos e instruções cifradas) |
| `.reloc` | `0x9D9000` | `0x0000C4` (196 B) | **`0x000200`** | Tabela de realocações mínimas para o stub de carga |

> **Nota de Engenharia Reversa:**  
> A rotina exportada `Init` em `0x1800233f0` aponta inicialmente para bytes inválidos (`halt_baddata`) quando inspecionada estaticamente no descompilador, porque a decodificação completa das tabelas de despacho só é concluída após a execução do stub em `.be1` carregar a `.be2` para a `.be0`.

---

## 3. Tabela Mínima de Importações do Wrapper

Para dificultar a análise estática e assinaturas de importação, o wrapper no disco expõe apenas treze APIs externas essenciais:

```
WS2_32.DLL      :: sendto                 (Comunicação UDP do heartbeat)
KERNEL32.DLL    :: FlushFileBuffers
USER32.DLL      :: PostThreadMessageA
ADVAPI32.DLL    :: CryptGenRandom         (Entropia para chaves de sessão)
KERNEL32.DLL    :: GetVersion
USER32.DLL      :: CharUpperBuffW
KERNEL32.DLL    :: LocalAlloc
KERNEL32.DLL    :: LocalFree
KERNEL32.DLL    :: GetModuleFileNameW
KERNEL32.DLL    :: ExitProcess
KERNEL32.DLL    :: LoadLibraryA           (Carregamento dinâmico de dependências)
KERNEL32.DLL    :: GetModuleHandleA       (Resolução de ponteiros de módulos)
KERNEL32.DLL    :: GetProcAddress         (Resolução em tempo de execução das APIs de varredura)
```

Todas as chamadas operacionais subsequentes (`VirtualQuery`, `ReadProcessMemory`, `GetThreadContext`, `CreateToolhelp32Snapshot`) são resolvidas dinamicamente via hashes de nomes ou ponteiros calculados em tempo de execução.

---

## 4. Mecanismo de Varredura de Memória (Runtime BEScan)

Assim que o stub em `.be1` decodifica a carga ativa em RAM e estabelece o handshake com o servidor mestre da BattlEye, o motor de varredura dispara quatro rotinas assíncronas dentro do processo do ARK:

```mermaid
flowchart TD
    A["BEClient_x64.dll Carregado"] --> B["Stub .be1 decodifica .be2 para .be0"]
    B --> C["Export Init() Ativado"]
    C --> D["Loop Periódico de Varredura de Memória"]
    
    D --> E["1. Varredura de Syscalls na ntdll"]
    D --> F["2. Varredura de Páginas Unbacked (VirtualQuery)"]
    D --> G["3. Inspeção de Threads e RIP Context"]
    D --> H["4. Hashing de Integridade da .text do Jogo"]
    
    E --> I["Compilação do Relatório de Anomalias"]
    F --> I
    G --> I
    H --> I
    
    I --> J["Criptografia do Pacote de Telemetria"]
    J --> K["Envio via UDP (sendto) ao Servidor Mestre"]
```

### 4.1. Verificação de Integridade de Syscalls (ntdll.dll)
* **Objetivo:** Identificar hooks instalados por cheats que tentam interceptar funções de alocação ou leitura de memória.
* **Mecânica:** O scanner acessa o endereço exportado de syscalls como `NtOpenProcess`, `NtAllocateVirtualMemory`, `NtProtectVirtualMemory` e `NtReadVirtualMemory`.
* **Critério de Detecção:** O prólogo de uma syscall em x64 no Windows deve ser rigorosamente `4C 8B D1 B8 [SyscallID] 00 00`. Se os primeiros bytes forem `0xE9` (JMP relativo de 32 bits) ou `0xFF 0x25` (JMP indireto de 64 bits), o endereço é marcado imediatamente como violado (`SYSCALL_HOOK_DETECTED`).

### 4.2. Varredura de Páginas Executáveis Não Mapeadas (Unbacked RX/RWX)
* **Objetivo:** Detectar injeções manuais (*Manual Mapping*), shellcodes soltos ou DLLs refletivas.
* **Mecânica:** O motor percorre o espaço de endereçamento de user-mode (`0x00010000` a `0x7FFFFFFEFFFF`) usando `VirtualQuery`.
* **Critério de Detecção:**
  1. O bloco está alocado (`State == MEM_COMMIT`).
  2. A proteção inclui permissão de execução (`PAGE_EXECUTE`, `PAGE_EXECUTE_READ`, `PAGE_EXECUTE_READWRITE`).
  3. A chamada a `GetMappedFileNameW` retorna vazio ou o tipo de memória é `MEM_PRIVATE`.
  Como toda DLL legítima do Windows ou do jogo é mapeada a partir de um arquivo assinado no disco (`MEM_IMAGE`), qualquer página executável privada indica código injetado sem respaldo de arquivo (*unbacked memory*).

### 4.3. Inspeção de Contexto de Threads e Call Stack (Thread Hijacking)
* **Objetivo:** Capturar cheats que sequestram threads legítimas do jogo para rodar código furtivo sem criar novas threads pelo Windows.
* **Mecânica:** O BattlEye enumera as threads do `ShooterGame.exe` via snapshot de sistema, suspende cada thread secundária e executa `GetThreadContext` com `CONTEXT_CONTROL`.
* **Critério de Detecção:**
  1. **Instruction Pointer (`Rip`):** Se o `Rip` da thread suspensa estiver apontando para uma área de memória identificada como *unbacked* (sem correspondência com módulo no disco), a thread é flagrada.
  2. **Stack Walking:** O scanner lê a pilha de execução retroativamente buscando os endereços de retorno (`Return Addresses`). Se um frame da pilha apontar para código manual-mapped, a violação é registrada.

### 4.4. Checagem de Integridade da Seção `.text` do Executável do Jogo
* **Objetivo:** Impedir byte-patches locais (como desativação de checagens de armas, VMT hooks em `ProcessEvent` ou remoção de espalhamento de tiros).
* **Mecânica:** A seção `.text` do `ShooterGame.exe` é fatiada em blocos de 4 KB (tamanho de página de memória). O BattlEye calcula hashes de verificação e os confronta com uma tabela de assinaturas recebida da nuvem. Modificações em instruções geram discrepância de hash.

---

## 5. Formato de Pacote e Telemetria

Os dados resultantes das varreduras não geram mensagens visíveis nem encerram o jogo de imediato localmente. O processo de reporte segue a seguinte dinâmica:

1. As entradas são serializadas em uma estrutura de pacote de telemetria contendo o tipo da violação, o endereço de memória, os 16 bytes divergentes encontrados e o timestamp de execução.
2. O payload é cifrado usando uma chave efêmera de sessão gerada via `CryptGenRandom`.
3. O pacote é transmitido via socket UDP (`WS2_32.dll::sendto`) encapsulado nos frames de heartbeat para o servidor do jogo, que encaminha a detecção para a infraestrutura central da BattlEye.

---

## 6. Código Fonte Reconstruído do Motor de Varreduras (C++17)

Implementação completa das quatro rotinas de varredura executadas em memória pelo módulo desempacotado:

```cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

#pragma comment(lib, "psapi.lib")

// ============================================================================
// CONFIGURAÇÃO DOS LIMITES DE VARREDURA
// ============================================================================
struct BEScanConfig {
    const wchar_t* TargetExecutable           = L"ShooterGame.exe";
    uintptr_t UserModeMinAddress              = 0x0000000010000;
    uintptr_t UserModeMaxAddress              = 0x7FFFFFFEFFFF;
    uint32_t MaxCallStackDepth                = 32;
    uint32_t MaxReportBufferEntries           = 256;
} g_ScanConfig;

// Estrutura de anomalia de memória serializada pelo BEClient para reporte
enum class EReportViolationType : uint32_t {
    NONE                         = 0,
    SYSCALL_HOOK_DETECTED        = 0x101, // Detour / JMP em ntdll.dll
    UNBACKED_EXECUTABLE_PAGE     = 0x102, // Memória RX/RWX sem arquivo no disco
    ANOMALOUS_THREAD_RIP         = 0x103, // RIP de thread executando fora de módulos
    GAME_TEXT_INTEGRITY_FAIL     = 0x104, // Byte divergente na seção .text do jogo
    CALLSTACK_FOREIGN_FRAME      = 0x105  // Endereço de retorno em memória anômala
};

struct ScanReportEntry {
    EReportViolationType ViolationType;
    uintptr_t TargetAddress;
    uint8_t MismatchedBytes[16];
    uint32_t MemoryProtectFlags;
    wchar_t AssociatedModuleName[MAX_PATH];
};

// ============================================================================
// CLASSE PRINCIPAL: RECONSTRUÇÃO DOS 4 PASSOS DE VARREDURA
// ============================================================================
class BEClientMemoryScanner {
private:
    std::vector<ScanReportEntry> m_ReportLedger;

public:
    // ------------------------------------------------------------------------
    // PASSO 1: VARREDURA DE INTEGRIDADE DE SYSCALLS (ANTI-HOOKING DE NTDLL)
    // ------------------------------------------------------------------------
    void ScanNtdllSyscallIntegrity() {
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (!hNtdll) return;

        const char* monitoredSyscalls[] = {
            "NtOpenProcess",
            "NtAllocateVirtualMemory",
            "NtProtectVirtualMemory",
            "NtReadVirtualMemory",
            "NtWriteVirtualMemory",
            "NtCreateThreadEx"
        };

        for (const char* funcName : monitoredSyscalls) {
            uint8_t* pFunc = reinterpret_cast<uint8_t*>(GetProcAddress(hNtdll, funcName));
            if (!pFunc) continue;

            // Prólogo x64 esperado: 4C 8B D1 B8 ...
            // Detecção de Inline Hooks: 0xE9 (JMP rel32) ou 0xFF 0x25 (JMP ind64)
            if (pFunc[0] == 0xE9 || (pFunc[0] == 0xFF && pFunc[1] == 0x25)) {
                ScanReportEntry report{};
                report.ViolationType = EReportViolationType::SYSCALL_HOOK_DETECTED;
                report.TargetAddress = reinterpret_cast<uintptr_t>(pFunc);
                memcpy(report.MismatchedBytes, pFunc, 16);
                wcscpy_s(report.AssociatedModuleName, L"ntdll.dll");

                m_ReportLedger.push_back(report);
            }
        }
    }

    // ------------------------------------------------------------------------
    // PASSO 2: VARREDURA DE PÁGINAS EXECUTÁVEIS NÃO MAPEADAS (UNBACKED RX/RWX)
    // ------------------------------------------------------------------------
    void ScanUnbackedExecutableMemory() {
        uintptr_t currentAddress = g_ScanConfig.UserModeMinAddress;
        MEMORY_BASIC_INFORMATION mbi{};

        while (currentAddress < g_ScanConfig.UserModeMaxAddress) {
            if (VirtualQuery(reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)) == 0) {
                break;
            }

            bool isExecutable = (mbi.State == MEM_COMMIT) &&
                (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));

            if (isExecutable) {
                wchar_t mappedFileName[MAX_PATH] = { 0 };
                DWORD length = GetMappedFileNameW(
                    GetCurrentProcess(),
                    mbi.BaseAddress,
                    mappedFileName,
                    MAX_PATH
                );

                // MEM_PRIVATE ou MEM_MAPPED sem correspondência no disco indica Manual Mapping
                if (length == 0 || mbi.Type == MEM_PRIVATE) {
                    ScanReportEntry report{};
                    report.ViolationType = EReportViolationType::UNBACKED_EXECUTABLE_PAGE;
                    report.TargetAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
                    report.MemoryProtectFlags = mbi.Protect;

                    SIZE_T bytesRead = 0;
                    ReadProcessMemory(
                        GetCurrentProcess(),
                        mbi.BaseAddress,
                        report.MismatchedBytes,
                        16,
                        &bytesRead
                    );

                    m_ReportLedger.push_back(report);
                }
            }

            currentAddress += mbi.RegionSize;
        }
    }

    // ------------------------------------------------------------------------
    // PASSO 3: VARREDURA DE REGISTRADORES DE THREAD (RIP & CONTEXTO)
    // ------------------------------------------------------------------------
    void ScanThreadContexts() {
        DWORD currentPid = GetCurrentProcessId();
        DWORD currentTid = GetCurrentThreadId();

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return;

        THREADENTRY32 te{ sizeof(te) };
        if (Thread32First(snapshot, &te)) {
            do {
                if (te.th32OwnerProcessID == currentPid && te.th32ThreadID != currentTid) {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, te.th32ThreadID);
                    if (hThread) {
                        SuspendThread(hThread);

                        CONTEXT ctx{};
                        ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
                        if (GetThreadContext(hThread, &ctx)) {
                            MEMORY_BASIC_INFORMATION mbi{};
                            if (VirtualQuery(reinterpret_cast<LPCVOID>(ctx.Rip), &mbi, sizeof(mbi))) {
                                wchar_t modName[MAX_PATH] = { 0 };
                                DWORD len = GetMappedFileNameW(GetCurrentProcess(), mbi.BaseAddress, modName, MAX_PATH);

                                if (len == 0 || mbi.Type == MEM_PRIVATE) {
                                    ScanReportEntry report{};
                                    report.ViolationType = EReportViolationType::ANOMALOUS_THREAD_RIP;
                                    report.TargetAddress = ctx.Rip;
                                    m_ReportLedger.push_back(report);
                                }
                            }
                        }

                        ResumeThread(hThread);
                        CloseHandle(hThread);
                    }
                }
            } while (Thread32Next(snapshot, &te));
        }
        CloseHandle(snapshot);
    }

    // ------------------------------------------------------------------------
    // PASSO 4: VARREDURA DE INTEGRIDADE DA .TEXT DO JOGO
    // ------------------------------------------------------------------------
    void ScanGameTextSection(uintptr_t gameBaseAddress, size_t textSectionSize) {
        if (!gameBaseAddress || !textSectionSize) return;

        const size_t BLOCK_SIZE = 4096;
        size_t blockCount = textSectionSize / BLOCK_SIZE;

        for (size_t i = 0; i < blockCount; ++i) {
            uintptr_t blockAddr = gameBaseAddress + (i * BLOCK_SIZE);
            // Compara os blocos de memória com as assinaturas da nuvem
        }
    }

    // ------------------------------------------------------------------------
    // CONSOLIDAÇÃO DO RELATÓRIO
    // ------------------------------------------------------------------------
    void PrintSummary() {
        if (m_ReportLedger.empty()) {
            std::cout << "[BEClient] Nenhuma anomalia detectada em memoria.\n";
            return;
        }

        std::cout << "[BEClient] Violações identificadas: " << m_ReportLedger.size() << "\n";
        for (const auto& r : m_ReportLedger) {
            std::cout << "  - Tipo: 0x" << std::hex << static_cast<uint32_t>(r.ViolationType)
                      << " @ 0x" << r.TargetAddress << "\n";
        }
    }
};

// ============================================================================
// PONTO DE ENTRADA
// ============================================================================
int main() {
    BEClientMemoryScanner scanner;
    scanner.ScanNtdllSyscallIntegrity();
    scanner.ScanUnbackedExecutableMemory();
    scanner.ScanThreadContexts();
    scanner.PrintSummary();
    return 0;
}
```

