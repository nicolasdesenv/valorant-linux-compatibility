# VALORANT Linux Compatibility Research

`valorant-linux-compatibility` is a reproducible engineering investigation into how far the Windows Riot Client and VALORANT binaries can run on Fedora through Wine and Proton. It is a research repository, not a claim that VALORANT currently works on Linux.

The long-term question is: **What would Riot Games need to implement or support officially on Linux while preserving Riot Vanguard's security guarantees?**

## Safety and research boundary

This project does not attempt to bypass Vanguard, weaken anti-cheat, spoof a trusted environment, fake TPM or Secure Boot state, impersonate kernel components, provide cheats, or access Riot services through security circumvention. Testing stops at a Vanguard trust or kernel boundary. Findings beyond that boundary are limited to documentation and legitimate compatibility-layer analysis.

No result is considered evidence until it is reproducible, logged, and reviewed. The workflow is:

```text
REPRODUCE -> OBSERVE -> ISOLATE -> UNDERSTAND -> IMPLEMENT -> TEST -> DOCUMENT
```

## Research areas

- Riot Client, installer, updater, UI, and process/service compatibility
- Wine and Proton compatibility, including Win32 and NT API behavior
- DirectX-to-Vulkan translation through DXVK and VKD3D-Proton
- Process creation, service interactions, exceptions, and DLL loading
- The Vanguard user-mode and kernel dependency boundary
- Secure Boot and TPM implications for an officially supported design

## Repository layout

- `docs/`: environment snapshot, design, requirements, findings, build research, observability, and compatibility state
- `scripts/`: non-root, defensive helpers for inspection, build preparation, prefixes, and log collection
- `tools/`: future purpose-built diagnostic utilities
- `tests/`: reproducible compatibility tests that do not depend on Riot credentials
- `patches/`: observed-problem patches, separated by upstream component
- `logs/`: ignored per-run output; only sanitized fixtures or summaries may be committed intentionally

The baseline layout is extended with `docs/proton-build.md` and `docs/observability.md` because build provenance and diagnostic design are first-class research artifacts. Empty working directories contain `.gitkeep` files so a fresh clone preserves the intended structure.

## Current status

Preparation only. No Riot software has been installed or run, Proton has not been built, and no compatibility patch exists. See [the compatibility matrix](docs/compatibility-matrix.md).

## Quick start

```bash
./scripts/check-environment.sh
./scripts/bootstrap.sh
./scripts/create-prefix.sh
./scripts/collect-logs.sh --help
./scripts/build-proton.sh --help
```

These commands do not install system packages. `build-proton.sh` prints a plan by default; an actual network-heavy build requires the explicit `--execute` option.

## Contributing

Use English for code, documentation, filenames, comments, and commits. Keep commits small. Never commit credentials, tokens, cookies, personal data, Riot logs without review, Wine prefixes, or generated build products. Every patch must cite a captured, reproducible failure.

## License

MIT; see [LICENSE](LICENSE).
