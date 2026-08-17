# Observability Design

## Per-run record

```text
logs/YYYYMMDD-HHMMSS-run/
├── environment.txt
├── proton.log
├── wine.log
├── dxvk.log
├── vkd3d-proton.log
├── processes.txt
└── summary.md
```

The collection helper creates the directory, snapshots only allow-listed environment facts, and copies explicitly supplied log files. Generated run directories are ignored by Git.

## Signals

- Proton: `PROTON_LOG=1`, exact Proton name/commit, Steam runtime version, launch exit status
- Wine: selected `WINEDEBUG` channels for errors, DLL loading, process/thread creation, exceptions, services, and targeted API areas; begin narrow and expand only to isolate a failure
- DLL/API behavior: loader messages, module origin, architecture, status/error codes, and minimal reproducer
- Processes/services: parent-child relationships and service-control operations, excluding command lines when they could expose secrets
- Graphics: DXGI adapter selection, D3D feature-level creation, DXVK/VKD3D-Proton versions and logs, Vulkan loader/ICD/device information
- Host: kernel, session type, GPU/driver, Vulkan summary, Secure Boot and TPM availability, disk pressure

## Collection principles

1. Assign one hypothesis or baseline objective to each run.
2. Record immutable versions and the exact non-secret configuration.
3. Reproduce at least twice before changing code.
4. Change one controlled variable at a time.
5. Preserve raw local evidence, then write a sanitized summary suitable for Git.
6. Stop if investigation reaches Vanguard's kernel or trust boundary.

## Data minimization

Never collect Riot credentials, authentication headers, tokens, cookies, chat, browser data, unrelated process environment, full home paths when avoidable, or packet captures by default. Treat Riot/Proton logs as potentially sensitive. Review and redact account identifiers, URLs with query strings, machine identifiers, usernames, access tokens, and absolute personal paths before sharing. Redaction must be reported; do not alter technical error codes or call sequences silently.

## Suggested staged channels

- Baseline: Proton log plus `WINEDEBUG=warn+all,err+all`
- Loader isolation: add `+loaddll,+module`
- Process/service isolation: add `+process,+thread,+service`
- Exception isolation: add `+seh`
- Graphics isolation: component-native DXVK/VKD3D logs and targeted `+dxgi`/graphics channels supported by the selected build

Very broad Wine tracing can generate gigabytes and leak application data. Enable it only for a short, isolated reproduction with an explicit size check.
