# Candidatas de compatibilidade genérica para drivers Windows

Escopo: APIs públicas que podem ser demonstradas por drivers benignos em
prefixos independentes. Nenhuma candidata abaixo foi derivada de código ou
lógica do Vanguard.

| Prioridade | API/semântica pública | Comportamento esperado | Wine pinado | Teste upstream |
|---|---|---|---|---|
| 1 | `IoCreateDeviceSecure` — criar `DEVICE_OBJECT` com SDDL/ACL | Criar o objeto e aplicar a descrição de segurança solicitada, retornando status NT consistente | Implementação marcada `semi-stub`; criação retorna sucesso, mas o primeiro cliente recebe `ERROR_FILE_NOT_FOUND` para o link simbólico, antes de qualquer decisão ACL | Não há teste focado de ACL/SDDL; testes cobrem `IoCreateDevice` básico em `tests/driver.c` |
| 2 | `IoWMIRegistrationControl` — registrar/remover provider WMI | Registrar o driver WMI, aceitar enable/disable e retornar status documentado | Stub explícito em `dlls/ntoskrnl.exe/ntoskrnl.c` | Nenhum teste WMI específico encontrado em `dlls/ntoskrnl.exe/tests` |
| 3 | `IoInvalidateDeviceRelations` / relações PnP | Notificar PnP de mudanças e produzir relações coerentes | Parcial; `pnp.c` marca relações/tipos não tratados com `FIXME` e `STATUS_NOT_IMPLEMENTED` em caminhos específicos | `driver_pnp.c` testa enumeração e relações básicas, mas não todos os tipos |
| 4 | `IoGetDeviceProperty` e propriedades PnP | Retornar propriedades públicas do dispositivo, tamanho/status corretos | Parcial; `pnp.c` registra propriedades não tratadas e retorna `STATUS_NOT_IMPLEMENTED` para algumas | `driver_pnp.c` e `ntoskrnl.c` cobrem várias propriedades comuns |
| 5 | `MmGetPhysicalAddress` / `MmGetPhysicalMemoryRanges` | Traduzir endereços/relatar memória física de forma consistente | Funções marcadas como stub ou sem suporte completo em `ntoskrnl.c` | Não foi encontrado teste upstream dedicado para memória física |
| 6 | `KeIpiGenericCall` / callbacks de processadores | Executar callback em todos os processadores e retornar o valor agregado | Stub explícito em `ntoskrnl.c` | Nenhum teste dedicado encontrado |
| 7 | `IoAllocateDriverObjectExtension` / extensões de driver | Alocar, reutilizar e liberar extensão associada ao `DRIVER_OBJECT` | Stub explícito em `ntoskrnl.c` | Nenhum teste dedicado encontrado |
| 8 | `IoRegisterPlugPlayNotification` | Registrar callback e receber notificações PnP compatíveis | Implementação parcial/stub; callbacks e filtros têm caminhos não tratados | Testes PnP cobrem notificações de dispositivos, não todos os filtros |

## Candidatas já demonstradas como funcionais

Os testes existentes e os reproducers deste projeto demonstraram suporte
funcional para `DriverEntry`, `ZwLoadDriver`, `IoCreateDevice`, eventos,
threads de sistema, MDLs básicos, `cmp32`/`test32` sobre
`KUSER_SHARED_DATA` e o caminho normal de SCM para um driver benigno.

## Priorização

As prioridades favorecem APIs públicas com status observável por um driver
benigno, implementação explicitamente incompleta no Wine e impacto geral em
drivers de dispositivo. Cada candidata deve ganhar um reproducer isolado e um
teste de regressão antes de qualquer alteração no Wine.

Este documento não autoriza modificar Vanguard, falsificar estado de trust,
contornar anti-cheat ou tentar matchmaking.
