# IoCreateDeviceSecure — especificação formal pré-patch

Escopo: somente o reproducer benigno `WineSecureProbe`. Nenhum componente Riot,
VALORANT ou Vanguard participa destes testes.

## Convenções

- `STATUS_SUCCESS` = `0x00000000`
- `STATUS_ACCESS_DENIED` = `0xC0000022`
- `ERROR_ACCESS_DENIED` = `5`
- `requested_access` do cliente: `GENERIC_READ`
- SDDL principal: `D:P(A;;GA;;;SY)`
- `IRP_MJ_CREATE` só pode ocorrer depois de uma abertura autorizada.

Cada execução deve registrar: precondition, SDDL, token/contexto sanitizado,
requested access, status NT, erro Win32, chegada/status do IRP e cleanup.

## Matriz

| Caso | Precondition | SDDL | Token/context | Expected Windows semantics | Current Wine result | Expected patched Wine | Cleanup |
|---|---|---|---|---|---|---|---|
| A — autorizado | DriverEntry e link concluídos | `D:P(A;;GA;;;SY)` | Contexto SYSTEM/autorizado demonstrável; se indisponível, não executar | `IoCreateDeviceSecure=SUCCESS`; link `SUCCESS`; `CreateFileW=PASS`; `IRP_MJ_CREATE` reached | NOT TESTED com token SYSTEM legítimo | PASS; handle com granted access coerente; IRP reached | CLOSE/CLEANUP após handle válido |
| B — não autorizado | Mesmo device/link | `D:P(A;;GA;;;SY)` | Cliente restricted não-SYSTEM já criado | `IoCreateDeviceSecure=SUCCESS`; `CreateFileW=FAIL`; NT `STATUS_ACCESS_DENIED`; Win32 `ERROR_ACCESS_DENIED`; IRP não reached | FAIL: `CreateFileW=PASS`, IRP reached | PASS: falha antes do IRP | Nenhum cleanup de file handle |
| C — SDDL inválido | Nome/link não devem existir antes | Entrada inequivocamente inválida, por exemplo `D:P(A;;INVALID;;;SY)` | Contexto normal | WINDOWS REFERENCE REQUIRED para status exato e atomicidade | NOT TESTED | Falha documentada; nenhum device parcial/link residual | Verificar ausência de device/link |
| D — SDDL NULL | Novo nome isolado | `NULL` | Contexto normal | WINDOWS REFERENCE REQUIRED; não inferir política default | NOT TESTED | Reproduzir sem inventar ACL | Verificar descriptor e abertura conforme referência |
| E — regressão | Controle atual | N/A (`IoCreateDevice`) | Cliente normal | Device/link/CreateFile/IRP permanecem funcionais | PASS | PASS | CLOSE/CLEANUP |
| F — lifetime | Device criado; buffer temporário liberável | `D:P(A;;GA;;;SY)` | Repetir A/B após liberar memória client-side | ACL persiste depois do retorno da API e liberação do buffer | NOT TESTED | Mesmo resultado A/B após liberação | CLOSE/CLEANUP por abertura autorizada |
| G1 — primeira abertura | Device estável | SDDL válido | Contexto autorizado ou restrito | Resultado conforme A/B | Parcialmente demonstrado | PASS | Cleanup correspondente |
| G2 — segunda abertura | Após G1, sem recriar device | Mesmo SDDL | Mesmo ou outro contexto | Política e granted access independentes por handle | NOT TESTED | PASS; sem compartilhamento acidental | Fechar ambos separadamente |

## Regras de execução

1. Criar cada caso em prefixo novo, salvo G2/F que explicitamente reutilizam o
   mesmo device para testar persistência.
2. Não iniciar serviços manualmente além do caminho normal do probe.
3. Não obter, fabricar ou elevar para token SYSTEM. Se A não puder ser
   executado com contexto autorizado legítimo, marcar `NOT EXECUTABLE LOCALLY`.
4. Para B, usar o cliente restricted existente e registrar somente propriedades
   necessárias para demonstrar `non-SYSTEM`.
5. Capturar `GetLastError()` imediatamente após `CreateFileW` quando houver
   falha.
6. Verificar ausência de `IRP_MJ_CREATE` em B/C quando a abertura deveria ser
   recusada.
7. Em F, liberar a memória temporária após `IoCreateDeviceSecure` retornar e
   repetir as aberturas.

## Status atual

- **PASS TODAY:** E; controle `IoCreateDevice → symbolic link → CreateFileW → IRP_MJ_CREATE`.
- **FAIL TODAY:** B; token restricted abriu o device e alcançou `IRP_MJ_CREATE`.
- **NOT TESTED:** A com token SYSTEM legítimo, C, D, F e G2.
- **WINDOWS REFERENCE REQUIRED:** semântica exata de SDDL inválido (C) e
  `DefaultSDDLString == NULL` (D), incluindo status e atomicidade.

## Referência Windows (pré-patch)

A documentação Microsoft confirma que `WdmlibIoCreateDeviceSecure`/`IoCreateDeviceSecure`
aplica a segurança derivada de `DefaultSDDLString`, aceita apenas um subconjunto
de SDDL e permite overrides associados a `DeviceClassGuid`. Ela também afirma que
o chamador deve liberar o device se a rotina retornar erro. A documentação não
define o NTSTATUS exato para SDDL sintaticamente inválido, nem define o resultado
de `DefaultSDDLString == NULL`; portanto C e D permanecem `WINDOWS REFERENCE REQUIRED`.

Fonte: Microsoft Learn, WdmlibIoCreateDeviceSecure e Windows Security Model for
Driver Developers.

Foi preparado, mas não executado, `windows-secure-reference-probe.c` para Windows
AMD64 real. Ele testa somente SDDL inválido e `NULL`, registra NTSTATUS e ponteiro
do DEVICE_OBJECT e remove qualquer device criado.

Nenhuma alteração foi feita em `protocol.def`, arquivos gerados, Wine ou Proton.
