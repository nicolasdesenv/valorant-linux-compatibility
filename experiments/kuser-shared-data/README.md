# KUSER_SHARED_DATA Wine Driver Experiment

## Hypothesis

Does the pinned Wine `ntoskrnl` driver environment correctly expose read access
to the AMD64 Windows kernel view of `KUSER_SHARED_DATA`, particularly the
documented fields at offsets `0x004` and `0x320` from
`0xfffff78000000000`?

This is a generic Wine compatibility experiment. It is isolated from Riot
software and does not inspect, modify, emulate, or communicate with Vanguard.

## Windows semantics

Microsoft documents `KUSER_SHARED_DATA` in `ntddk.h`. It is a read-only shared
page through which Windows exposes time, version, processor, and other system
information. On AMD64, kernel code conventionally addresses the page through
`0xfffff78000000000`; the same data has a user-mode view at `0x7ffe0000`.

Authoritative references:

- [Microsoft: KUSER_SHARED_DATA structure](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-kuser_shared_data)
- [Microsoft: !kuser debugger extension](https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-kuser)

The two fields tested here are:

- `+0x004`: `ULONG TickCountMultiplier`.
- `+0x320`: the `KSYSTEM_TIME TickCount` / `ULONG64 TickCountQuad` union.

Both probes perform reads only.

## Pinned Wine implementation

The pinned Wine tree defines the structure and offsets in
`sources/proton/wine/include/ddk/wdm.h`. Wine creates and updates its shared
data object in `server/mapping.c`, `server/fd.c`, `programs/wineboot/wineboot.c`,
and `dlls/ntdll/unix/virtual.c`, mapping its user view at `0x7ffe0000`.

For AMD64 kernel drivers, Wine does not directly map the high kernel virtual
address. `dlls/ntoskrnl.exe/instr.c` installs a vectored exception handler.
When supported read instructions fault on addresses inside
`0xfffff78000000000..0xfffff78000000fff`, the handler copies the requested
bytes from Wine's `0x7ffe0000` page into the emulated destination register.
The supported forms include selected `mov`, `movzx`, `or`, and `xor`
instructions.

## Methodology

Three freestanding PE/COFF AMD64 drivers are built:

1. `kuser-control.sys`: proves `DriverEntry` is reached without accessing the
   shared page.
2. `kuser-004.sys`: reads one `ULONG` at `base + 0x004`.
3. `kuser-320.sys`: reads one `ULONG64` at `base + 0x320`.

Each driver reports entry and successful reads through Wine's existing
`DbgPrint` implementation, returns `STATUS_SUCCESS`, and performs no other
initialization. The drivers are loaded as independent services through Wine's
normal Service Control Manager / `winedevice` / `ZwLoadDriver` path in a new
prefix dedicated to this experiment.

## Build

```bash
make
make inspect
```

The build uses Fedora's MinGW-w64 compiler and binutils. It does not build the
Wine or Proton trees.

## Expected result

On Windows, all three drivers should reach `DriverEntry`; both documented
fields should be readable. Under the pinned Wine implementation, simple `mov`
loads should be handled by the `ntoskrnl.exe/instr.c` exception emulation and
produce `PASS` messages.

## Observed result

Run: `logs/20260817-043125-kuser-shared-data/`

All three drivers were registered as independent demand-start kernel services
in independent, non-Riot prefixes. Every `sc create`, `sc start`, and `sc query`
invocation returned exit status zero.

### Control

`kuser-control.sys` reached `DriverEntry` and printed `control PASS`.

### Offset `+0x004`

The compiled access is a 32-bit `mov (%rax), %eax` from
`0xfffff78000000004`. The unmapped high address first produced an internal
`0xc0000005` read exception. Wine's `ntoskrnl` vectored handler returned
`EXCEPTION_CONTINUE_EXECUTION`, and the driver continued:

```text
kuser_probe: DriverEntry reached; offset=0x4 width=32
kuser_probe: read32 offset=0x4 value=0x1000000 PASS
```

The value matches the pinned Wine initialization
`TickCountMultiplier = 1 << 24`.

### Offset `+0x320`

The compiled access is a 64-bit `mov (%rax), %rax` from
`0xfffff78000000320`. Wine handled the internal read exception and the driver
continued:

```text
kuser_probe: DriverEntry reached; offset=0x320 width=64
kuser_probe: read64 offset=0x320 value=0xf2494 PASS
```

The tick value is time-dependent; the evidence required here is successful
read and continuation, not a fixed value.

## Result and classification

- `DriverEntry`: PASS
- `+0x004`: PASS
- `+0x320`: PASS
- Independent reproduction of the terminal Vanguard access violation: NO
- Classification: **NOT REPRODUCED GENERICALLY**

The generic hypothesis that Wine cannot expose either documented field to a
normal driver using simple `mov` reads is rejected. Wine does not directly map
the AMD64 kernel virtual page, but its instruction-specific exception emulation
correctly services both tested reads. This does not establish compatibility for
other instruction forms, accesses, kernel APIs, or trust/integrity behavior,
and it does not explain or change the Vanguard result.

## Relevant source locations

- `sources/proton/wine/include/ddk/wdm.h` — public structure layout used by Wine.
- `sources/proton/wine/server/mapping.c:create_user_data_mapping` — shared object.
- `sources/proton/wine/server/fd.c:set_user_shared_data_time` — time updates.
- `sources/proton/wine/programs/wineboot/wineboot.c:create_user_shared_data` — initialization.
- `sources/proton/wine/dlls/ntdll/unix/virtual.c:virtual_map_user_shared_data` — user mapping.
- `sources/proton/wine/dlls/ntoskrnl.exe/instr.c:emulate_instruction` — AMD64 kernel-address read emulation.
- `sources/proton/wine/dlls/ntoskrnl.exe/instr.c:vectored_handler` — exception dispatch.

## Build provenance

Built with Fedora MinGW-w64 GCC 16.1.1 and binutils 2.46.0. The three drivers
are freestanding NT-native PE32+ images importing only `DbgPrint` from
`ntoskrnl.exe`; no Proton or Wine tree was built or modified.

## Instruction matrix experiment

Run: `logs/20260817-043705-kuser-instruction-matrix/`

### Prediction from the pinned source

Before execution, `dlls/ntoskrnl.exe/instr.c:emulate_instruction` was inspected.
Its AMD64 dispatcher explicitly handles these memory-source opcodes for the
high KUSER_SHARED_DATA address:

| Instruction family | Opcode | Predicted |
|---|---:|---|
| `mov r8, r/m8` | `8a` | emulated |
| `mov r16/32/64, r/m` | `66 8b` / `8b` / `48 8b` | emulated |
| `movzx r32, r/m8` | `0f b6` | emulated |
| `movzx r32, r/m16` | `0f b7` | emulated |
| `or r32, r/m32` | `0b` | emulated |
| `xor r32, r/m32` | `33` | emulated |
| `cmp r32, r/m32` | `3b` | not handled |
| `test r/m32, r32` | `85` | not handled |

The prediction concerns read handling and continuation. The matrix does not
claim to validate every arithmetic flag side effect for `or` or `xor`.

### Public fields selected

- `ProductTypeIsValid`, offset `+0x268`, 8 bits.
- `NativeProcessorArchitecture`, offset `+0x26a`, 16 bits.
- `TickCountMultiplier`, offset `+0x004`, 32 bits.
- `TickCountQuad`, offset `+0x320`, 64 bits.

All operations use the fields only as read operands. Each probe has its own
driver, service, and Proton prefix.

### Disassembly confirmation

The generated instructions were confirmed before execution:

```text
mov8:    8a 00       mov    (%rax),%al
mov16:   66 8b 00    mov    (%rax),%ax
mov32:   8b 00       mov    (%rax),%eax
mov64:   48 8b 00    mov    (%rax),%rax
movzx8:  0f b6 00    movzbl (%rax),%eax
movzx16: 0f b7 00    movzwl (%rax),%eax
or32:    0b 02       or     (%rdx),%eax
xor32:   33 02       xor    (%rdx),%eax
cmp32:   3b 02       cmp    (%rdx),%eax
test32:  85 02       test   %eax,(%rdx)
```

### Results

| instruction | width | offset | internal fault | emulated | DriverEntry continued | PASS/FAIL |
|---|---:|---:|---|---|---|---|
| `mov` | 8 | `0x268` | yes, `0xc0000005` | yes | yes | PASS |
| `mov` | 16 | `0x26a` | yes, `0xc0000005` | yes | yes | PASS |
| `mov` | 32 | `0x004` | yes, `0xc0000005` | yes | yes | PASS |
| `mov` | 64 | `0x320` | yes, `0xc0000005` | yes | yes | PASS |
| `movzx` | 8→32 | `0x268` | yes, `0xc0000005` | yes | yes | PASS |
| `movzx` | 16→32 | `0x26a` | yes, `0xc0000005` | yes | yes | PASS |
| `or` | 32 | `0x004` | yes, `0xc0000005` | yes | yes | PASS |
| `xor` | 32 | `0x004` | yes, `0xc0000005` | yes | yes | PASS |
| `cmp` | 32 | `0x004` | yes, `0xc0000005` | no | no | FAIL |
| `test` | 32 | `0x004` | yes, `0xc0000005` | no | no | FAIL |

For successful cases, the vectored handler returned `0xffffffff`
(`EXCEPTION_CONTINUE_EXECUTION`) and the driver printed its final PASS line.
For `cmp` and `test`, it returned `0` (`EXCEPTION_CONTINUE_SEARCH`), Wine
reported an unhandled read page fault, and `sc start` failed (host exit 29;
Wine SCM error 1053).

### Generic compatibility gap

`cmp` and `test` are ordinary read-only instruction forms that work against a
mapped, readable Windows KUSER_SHARED_DATA page. Wine's high-address model is
instruction-emulated rather than mapped, and the pinned dispatcher has no
cases for opcodes `0x3b` or `0x85`. Consequently, semantically valid reads fail
according to instruction selection.

The smallest reproducer is `ksd-cmp32.sys`: after one `DbgPrint` proving entry,
it executes `cmpl (%rdx), %eax` against documented
`TickCountMultiplier` at `0xfffff78000000004`. The handler declines the
instruction and `DriverEntry` cannot continue. `ksd-test32.sys` independently
confirms the same coverage boundary.

Classification: **GENERAL WINE COMPATIBILITY GAP** in the instruction-specific
KUSER_SHARED_DATA emulation. This finding remains independent of Vanguard and
does not establish which behavior caused the Riot driver failure.

## Experimental generic Wine fix

An isolated fix was developed on Wine branch
`codex/kuser-cmp-test-experimental`, from base
`81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`. The implementation commit is
`9c75a96c44ac340da38566686eb1b49285b612e4` and changes only the AMD64
dispatcher in `dlls/ntoskrnl.exe/instr.c`.

The added cases are deliberately limited to the independently reproduced
32-bit forms:

- `0x3b /r`, `cmp r32, r/m32`: read the four-byte shared-data operand, compute
  subtraction flags without storing the result, update `CF/PF/AF/ZF/SF/OF`,
  and advance RIP.
- `0x85 /r`, `test r/m32, r32`: read the four-byte shared-data operand, compute
  the logical AND without storing it, clear `CF/OF`, update `PF/ZF/SF`, preserve
  architecturally unrelated flags, and advance RIP. `AF` is undefined and is
  not tested.

The latest upstream Wine `master` inspected on 2026-08-17 still had no cases
for either opcode. The pinned test directory also had no focused test for this
KUSER_SHARED_DATA instruction-emulation path.

### Flag-aware validation

The independent drivers now capture RFLAGS immediately after the generated
instruction. Disassembly reconfirmed `3b 02` followed directly by
`pushf; pop`, and `85 02` followed directly by `pushf; pop`. Before `test`, the
driver deliberately sets both CF and OF, so the result also verifies that the
emulated instruction clears them.

Run: `logs/20260817-045424-kuser-instruction-matrix/`

| instruction | width | offset | internal fault | emulated | DriverEntry continued | PASS/FAIL |
|---|---:|---:|---|---|---|---|
| `mov` | 8 | `0x268` | yes, `0xc0000005` | yes | yes | PASS |
| `mov` | 16 | `0x26a` | yes, `0xc0000005` | yes | yes | PASS |
| `mov` | 32 | `0x004` | yes, `0xc0000005` | yes | yes | PASS |
| `mov` | 64 | `0x320` | yes, `0xc0000005` | yes | yes | PASS |
| `movzx` | 8→32 | `0x268` | yes, `0xc0000005` | yes | yes | PASS |
| `movzx` | 16→32 | `0x26a` | yes, `0xc0000005` | yes | yes | PASS |
| `or` | 32 | `0x004` | yes, `0xc0000005` | yes | yes | PASS |
| `xor` | 32 | `0x004` | yes, `0xc0000005` | yes | yes | PASS |
| `cmp` | 32 | `0x004` | yes, `0xc0000005` | yes | yes | PASS |
| `test` | 32 | `0x004` | yes, `0xc0000005` | yes | yes | PASS |

For `cmp32`, the defined-flags mask was `0x8d5`; observed and expected flags
were both `0x85` (`CF|PF|SF`). For `test32`, the defined-flags mask was `0x8c5`;
observed and expected flags were both `0x4` (`PF`), proving in this vector that
`CF`, `OF`, `ZF`, and `SF` were clear. All eight prior PASS results remained
PASS.

Only `dlls/ntoskrnl.exe` was built. The first component build took 7.6 seconds,
the build directory used 109 MiB, and the resulting PE module was 1.93 MiB.
Validation used a separate copy of Proton and fresh per-probe prefixes; the
official Proton installation and all Riot-related prefixes were untouched.

Result: **candidate generic Wine fix**. It still needs an upstream-style Wine
test and maintainer review before it can be proposed upstream.

## Independent ZwLoadDriver/SCM control

The patched Proton was tested with a separate benign driver in
`zwload_probe.c`. A control driver returned `STATUS_SUCCESS` without KUSER
accesses; a second driver executed the patched `cmp32`/`test32` reads and then
returned `STATUS_SUCCESS`. Both completed `sc create/start/query` with
`0/0/0` in fresh prefixes (`logs/20260817-051546-kuser-zwload-probe/`).

This demonstrates that the patched KUSER reads are compatible with the normal
Wine `ZwLoadDriver`/SCM path for a benign driver. It does not reproduce
Vanguard's later `d4494e49` failure and does not justify changing Vanguard or
the loader based on that opaque status.
