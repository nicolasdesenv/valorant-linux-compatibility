# Riot Requirements and Official-Support Questions

This document frames questions for legitimate official Linux support. It does not propose bypasses.

## Known policy constraints for this research

- Preserve Vanguard's security guarantees and Riot's authority over trust decisions.
- Preserve real Secure Boot and TPM state; never spoof or reconfigure either.
- Do not emulate, impersonate, patch out, or disable Vanguard components.
- Do not enter matchmaking through an environment Riot does not authorize.

## Questions requiring authoritative evidence

- Which Riot Client Win32/NT APIs and service assumptions are incompatible with Wine?
- Which graphics API path and Vulkan feature/driver baseline would Riot support?
- Could Riot provide a native, Riot-signed Linux security architecture with comparable guarantees?
- What kernel, module-signing, attestation, distribution, update, telemetry, and incident-response model would Riot require?
- How would Wayland, sandboxing, namespaces, immutable systems, and heterogeneous Linux kernels affect the threat model?
- Which Secure Boot and TPM measurements would be meaningful and privacy-preserving on supported Linux distributions?

Answers must come from reproducible observations or Riot/platform documentation. They must not be reverse-engineered into a mechanism for deceiving production trust systems.
