# valorant-linux-compatibility

Run VALORANT on Linux without a Windows VM, with Vanguard intact.

This repo is the build. It patches Wine so it can load Vanguard's kernel driver, ships the minimal Linux kernel module that supplies what the driver needs from hardware, and documents every dependency in between. The game itself runs through Proton.

It is not a Proton prefix with a launch script. That already exists and gets you as far as the Vanguard trust check.

## What this is, and what it isn't

**Is:** a compatibility layer for Vanguard's driver boundary, plus everything needed to make Wine load `vgk.sys` as a kernel service and answer its IOCTLs.

**Is not:** a Vanguard bypass, a spoof of a trusted environment, an emulator, or a cheat. The official Riot binaries are used unmodified. Nothing is re-signed.

## Scope

The project targets Fedora with a custom Wine build and Proton for game execution. There is no declared boundary: if the work reaches kernel, it reaches kernel.

Earlier revisions of this README said *"testing stops at a Vanguard trust or kernel boundary"* and asked what Riot would need to do. Both were wrong for this work. Riot has not implemented the driver as a Linux module, so the module is part of the project.

The workflow is unchanged and every result still has to survive it:

```
REPRODUCE -> OBSERVE -> ISOLATE -> UNDERSTAND -> IMPLEMENT -> TEST -> DOCUMENT
```

No result is evidence until it is reproducible, logged, and reviewed.

## Architecture

```
┌──────────────────────────────────────────────────┐
│ Proton / game                                   │  DXVK, VKD3D-Proton
├──────────────────────────────────────────────────┤
│ Riot Client + Vanguard user mode                │  winsock, RPC, IPC
├──────────────────────────────────────────────────┤
│ Vanguard driver interface                       │  IOCTLs, device objects
├──────────────────────────────────────────────────┤
│ Wine: service loader, driver path, NT API       │  ← this project
├──────────────────────────────────────────────────┤
│ stubs: ntoskrnl exports vgk.sys imports         │  ← this project
├──────────────────────────────────────────────────┤
│ Linux kernel module: hardware only              │  ← this project
├──────────────────────────────────────────────────┤
│ iommu, msr, cr3, physical memory, tpm           │
└──────────────────────────────────────────────────┘
```

Rules the layering enforces:

- Anything that has a real userspace equivalent stays a stub in `stubs/`. Do not push it to the module.
- The module answers hardware only: page table access, physical mapping, MSR reads and writes, IOMMU, TPM interaction. No game logic, no heuristics.
- Stubs return real behaviour, not plausible values. A stub that lies compiles and hides bugs downstream.

## Environment

| Component | Version | Note |
| --- | --- | --- |
| Fedora | current | host |
| Kernel | build from source | module out-of-tree, patched only where needed |
| Wine | build from fork | `wine/`, rebased per upstream release |
| Proton | current | game execution |
| QEMU | n/a | not used; guest-free |

Hard prerequisites:

- Secure Boot enabled on the host with your own key enrolled in the MOK list, or the module will not load
- TPM 2.0 present and accessible
- VT-d / IOMMU available (still relevant for physical mapping and DMA, no passthrough needed)

The host environment is not modified on install. The module loads, the fork runs, and uninstalling is removing the module and the fork.

## Repository layout

- `wine/`: fork of upstream Wine. Service loader, driver load path, NT API behaviour. Rebased on every upstream release, never merged into a separate tree without a finding to justify it.
- `kernel/`: the Linux kernel module. Hardware surface only.
- `stubs/`: ntoskrnl export table the driver imports. One file per function group, each with a header comment stating the caller, the observed arguments, and what was found by testing.
- `patches/`: each patch names the upstream component it touches and the captured failure it answers.
- `docs/`: environment snapshot, design, requirements, findings, build research, observability, compatibility state.
- `scripts/`: non-root helpers for inspection, build preparation, prefix creation, log collection.
- `tools/`: purpose-built diagnostic utilities, including the IOCTL probe harness.
- `tests/`: reproducible compatibility tests that do not need Riot credentials.
- `logs/`: ignored per-run output. Only sanitized fixtures and summaries are committed.

Empty directories carry `.gitkeep` so a fresh clone keeps the shape.

## Milestones

Each milestone is complete only when its tests pass from a clean checkout.

1. **Loader proof.** `hello.sys` from `tests/` compiles, loads through the modified Wine service loader, and answers an IOCTL. Nothing Vanguard-specific yet.
2. **Driver load.** `vgk.sys` loads and reaches its first unresolved import, with the import named in a log.
3. **Stub coverage.** Every import sites resolved to a real implementation or a documented hardware call. No silent stubs left.
4. **Trust path.** Secure Boot and TPM state visible to the driver in a form it accepts.
5. **Game.** Client, launcher, match, and a full competitive game.
6. **Release.** Installable package, automated environment checks, per-release compatibility matrix.

Do not skip to step 2. Steps 1 and 2 look close and are not.

## Current status

Preparation only. No Riot software installed or run, no Proton build, no dialect patch. See the compatibility matrix.

## Quick start

```
./scripts/check-environment.sh
./scripts/bootstrap.sh
./scripts/create-prefix.sh
./scripts/collect-logs.sh --help
./scripts/build-proton.sh --help
```

These do not install system packages. `build-proton.sh` prints a plan and needs `--execute` for a real network-heavy build. `check-environment.sh` reports Secure Boot, MOK, TPM, and IOMMU state and fails loudly, because each of them silently breaks a later milestone.

## Known risks

- **`vgk.sys` is a moving target.** Riot updates drivers. Exports change. `stubs/` is versioned per driver release and breakage is expected at each one.
- **Attestation.** The driver verifies its environment. Getting that right is the longest single piece of work here and the least predictable.
- **Signature path.** Secure Boot with your own MOK key. Not optional, and testable from milestone 1.
- **Whack-a-mole.** Expect regressions from Riot releases with no notice. The compatibility matrix tracks them per version.

## Contributing

English for code, docs, filenames, comments, and commits. Small commits. Never commit credentials, tokens, cookies, personal data, Riot logs without review, Wine prefixes, or build products.

Every patch cites a captured, reproducible failure. A patch without a failing test or a captured log is a guess and gets sent back.

## License

MIT; see LICENSE. The Wine fork carries upstream LGPL.
