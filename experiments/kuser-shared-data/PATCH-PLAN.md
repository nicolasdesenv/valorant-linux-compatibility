# Experimental Wine patch plan: KUSER_SHARED_DATA `cmp32` / `test32`

## Provenance and isolation

- Pinned Wine base: `81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`
- Parent Proton base: `0745bfbc4cf4365e8cf048b003990c59def29948`
- Experimental Wine branch: `codex/kuser-cmp-test-experimental`
- Scope: the independent `ksd-cmp32.sys` and `ksd-test32.sys` reproducers only.
- Explicitly excluded: Riot Client, VALORANT, Vanguard, anti-cheat state, and the Riot prefix.

## Source audit

The AMD64 `emulate_instruction()` dispatcher in
`sources/proton/wine/dlls/ntoskrnl.exe/instr.c` recognizes read faults from the
kernel `KUSER_SHARED_DATA` alias and redirects reads to Wine's user mapping at
`0x7ffe0000`. The existing `mov`, `movzx`, `or`, and `xor` cases decode the
ModR/M operand, copy the public shared-data bytes, update a destination register
where applicable, advance `CONTEXT.Rip`, and resume execution.

`store_reg_word()` currently changes register contents for `mov`, `or`, and
`xor`; it does not update `CONTEXT.EFlags`. The latest Wine upstream source
checked on 2026-08-17 has the same dispatcher shape and no `case 0x3b` or
`case 0x85`. No dedicated KUSER_SHARED_DATA instruction-emulator test was found
under `dlls/ntoskrnl.exe/tests` in the pinned tree.

## Patch

### File and function

- `dlls/ntoskrnl.exe/instr.c`
- AMD64 `emulate_instruction()`
- Small local helpers for 32-bit arithmetic/logical flag calculation, if that
  keeps the dispatcher readable and independently testable.

### Opcodes and forms

- `0x3b /r`: `cmp r32, r/m32`, with the faulting read-only memory operand in
  `KUSER_SHARED_DATA` and the first operand in a general register.
- `0x85 /r`: `test r/m32, r32`, with the faulting read-only memory operand in
  `KUSER_SHARED_DATA` and the second operand in a general register.

The implementation will accept only the reproduced 32-bit operand size. It
will not claim support for 16-bit or 64-bit variants in this patch.

### Required semantics

For `cmp r32, r/m32`, calculate a 32-bit subtraction `lhs - rhs`, do not store
the result, advance RIP past prefixes/opcode/ModR/M/addressing bytes, and update:

- `CF`: unsigned borrow (`lhs < rhs`)
- `PF`: even parity of the low result byte
- `AF`: carry/borrow from bit 3
- `ZF`: zero result
- `SF`: result bit 31
- `OF`: signed subtraction overflow

For `test r/m32, r32`, calculate a 32-bit bitwise AND, do not store the result,
advance RIP identically, clear `CF` and `OF`, and update `PF`, `ZF`, and `SF`
from the result. `AF` is architecturally undefined and will not be asserted by
the tests; unrelated flags remain unchanged.

## Tests

1. Extend the independent `cmp32` and `test32` drivers to capture RFLAGS after
   the exact faulting instruction and validate the defined flags, not merely
   continuation of `DriverEntry`.
2. Keep one isolated Proton prefix per driver so one failure cannot contaminate
   another.
3. Confirm generated instructions by disassembly (`3b /r` and `85 /r`).
4. Re-run the complete independent matrix:
   `mov8/16/32/64`, `movzx8/16`, `or32`, `xor32`, `cmp32`, and `test32`.
5. Stop if any previously passing probe regresses.

## Regression risks

- Incorrect ModR/M register selection, especially with REX.R.
- Incorrect 32-bit truncation or accidental register modification.
- Incorrect parity, auxiliary-carry, or signed-overflow calculation.
- Clobbering EFLAGS bits that the instruction does not define.
- Incorrect RIP advancement with prefixes/SIB/displacements.
- Overstating support for operand sizes not covered by the reproducers.
- Reading outside the one-page shared-data mapping due to an incorrect width
  bounds check.

## Minimal build strategy

Build only `dlls/ntoskrnl.exe` in a dedicated Wine build directory configured
from the pinned source. Reuse the installed MinGW compiler for PE test drivers.
A full Proton build is outside scope unless the component cannot be injected
into a completely separate experimental Proton copy; any such expansion must
be justified before execution.
