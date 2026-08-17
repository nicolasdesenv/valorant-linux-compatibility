# Research Architecture

## Layers under study

```text
Riot installer / Riot Client / VALORANT Windows processes
                         |
                  Win32 and NT APIs
                         |
                 Proton / Wine user space
                  |                  |
        DXVK (D3D9-11)       VKD3D-Proton (D3D12)
                  \                  /
                       Vulkan
                         |
          Fedora graphics stack and NVIDIA driver

Vanguard service/user mode ---- STOP ---- Windows kernel driver/trust boundary
```

The host, containerized build environment, Steam runtime, compatibility tool, isolated Wine prefix, and per-run evidence are separate artifacts. A run record must identify each artifact precisely enough to reproduce the observation.

## Evidence policy

An observation should include the component version or commit, command/configuration, sanitized log, expected behavior, actual behavior, and repeat count. A proposed patch must link to an observation and a minimal reproducer where practical. No patch may simulate or suppress a security decision.

## Boundary policy

User-mode API and service behavior may be observed. When progress requires a Windows kernel driver, Vanguard impersonation, trust spoofing, Secure Boot/TPM falsification, anti-cheat disabling, or unauthorized service access, execution stops and the boundary is documented.
