# Windows handoff

## Objetivo atual

Continuar a pesquisa de compatibilidade de drivers Windows no Wine em um
Windows AMD64 real, sem executar bypasses de anti-cheat. O objetivo imediato é
obter a referência Windows para `IoCreateDeviceSecure` nos casos de SDDL
inválido e `DefaultSDDLString == NULL`.

## Estado dos experimentos

- Fedora 44, KDE Wayland, RTX 4050, Secure Boot e TPM verificados.
- Riot Client e autenticação funcionaram no Proton Experimental.
- VALORANT instalou, mas o restart gate permaneceu após reboot.
- `vgk.sys` carregou e falhou na inicialização; `vgc` não iniciou.
- Nenhum bypass, patch de Vanguard ou alteração de Secure Boot foi usado.

## KUSER_SHARED_DATA

O reproducer independente demonstrou falhas genéricas de acesso em `cmp32` e
`test32` nos offsets associados a `KUSER_SHARED_DATA`. Foi criada uma correção
experimental no checkout Proton/Wine separado:

- base: `81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`
- commit experimental: `9c75a96c44ac340da38566686eb1b49285b612e4`

O patch implementa emulação genérica de `cmp r32,r/m32` e `test r/m32,r32`,
incluindo RIP e EFLAGS. A matriz independente passou para mov/movzx/or/xor/cmp/test.

## IoCreateDeviceSecure

Reproducer benigno confirmado:

```text
D:P(A;;GA;;;SY)
restricted non-SYSTEM token
expected ACCESS_DENIED
observed CreateFileW PASS
IRP_MJ_CREATE reached
```

O Wine registra `IoCreateDeviceSecure` como `semi-stub`; o SDDL não é
transportado para o Wine server. O controle `IoCreateDevice` comum passa.

## Casos A–G

- A — SDDL válido/autorizado: ainda não executado com contexto SYSTEM legítimo.
- B — SDDL válido/restricted: FAIL reproduzido.
- C — SDDL inválido: requer referência Windows real.
- D — `DefaultSDDLString == NULL`: requer referência Windows real.
- E — regressão `IoCreateDevice`: PASS.
- F — lifetime do descriptor: não testado.
- G — múltiplas aberturas/handles: não testado completamente.

## Probe Windows

O código está em:

`experiments/kuser-shared-data/windows-secure-reference-probe.c`

Ele testa somente `IoCreateDeviceSecure` com SDDL inválido e com `NULL`,
registrando NTSTATUS, ponteiro `DEVICE_OBJECT` e cleanup.

## Próxima sessão Codex no Windows

1. Compilar `windows-secure-reference-probe.c` com WDK/Wdmsec.lib.
2. Executar em Windows AMD64 real, em ambiente de teste controlado.
3. Registrar somente NTSTATUS, `DeviceObject == NULL/non-NULL` e cleanup.
4. Atualizar `IOCREATEDEVICESECURE-TEST-SPEC.md` com resultados observados.
5. Não alterar o Wine/Proton até que C e D estejam determinados.

É proibido desabilitar ou alterar Secure Boot, TPM, SELinux, assinatura de
drivers ou mecanismos de segurança. Não usar bypass, spoofing ou modificação
de Riot/Vanguard.
