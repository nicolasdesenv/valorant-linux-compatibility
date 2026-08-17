# Environment Snapshot

Captured on 2026-08-17. Commands were read-only and scoped to compatibility-relevant system data. The managed inspection session can restrict device and desktop access; limitations are noted instead of being treated as host failures.

| Item | Observed value | Confidence / note |
|---|---|---|
| Distribution | Fedora Linux 44 (COSMIC) | `/etc/os-release` |
| Kernel | `7.1.8-200.fc44.x86_64` | `uname` |
| Architecture | x86_64 | Verified |
| Desktop | KDE Plasma | `XDG_CURRENT_DESKTOP=KDE`; exact Plasma version unavailable in managed session |
| Display session | Wayland (`wayland-0`), Xwayland display `:1` present | Environment variables |
| CPU | Intel Core i5-13420H, 12 logical CPUs | `lscpu`, `nproc` |
| Memory | 15 GiB total, about 10 GiB available during capture | Point-in-time value |
| GPUs | Intel Raptor Lake-P UHD Graphics; NVIDIA AD107M GeForce RTX 4050 Max-Q / Mobile | PCI enumeration |
| NVIDIA driver | 610.57.04 open kernel module; `nvidia`, `nvidia_modeset`, `nvidia_drm`, `nvidia_uvm` loaded | Package, module, and `/proc` data |
| NVIDIA runtime probe | Not verified | `nvidia-smi` could not communicate from the managed session |
| Vulkan packages | Loader 1.4.341; Mesa Vulkan drivers 26.1.6 | RPM database |
| Vulkan devices | Not verified | `vulkaninfo --summary` could not initialize usable display/device paths in the managed session; rerun locally |
| Secure Boot | Enabled | `mokutil --sb-state` |
| TPM 2.0 | Partially detected; firmware/driver not visible, userspace TSS libraries present | `systemd-analyze has-tpm2`; device nodes unavailable in this session. Requires local verification |
| Steam | User reports installed; installation path/version not visible in managed session | No native command, RPM, Flatpak app, or accessible standard library path found |
| Installed Proton | Unknown | Steam library not visible; checker searches standard native and Flatpak paths |
| Wine | Not installed or not on `PATH` | `wine` not found |
| Podman | 5.8.4 installed | Runtime probe blocked because `/run/user/1000/libpod` is read-only in this session |
| Git | 2.55.0 | Verified |
| Toolchain present | GCC/G++ 16.1.1, GNU Make, Python 3.14.6, Perl, patch | Verified |
| Toolchain absent from PATH | Clang/Clang++, CMake, Meson, Ninja, Cargo/Rust | No packages were installed |
| Workspace filesystem | 475 GiB total, 432 GiB available | `df`, point-in-time value |

## Reproduce or refresh

Run `./scripts/check-environment.sh`. It writes a sanitized report to standard output only. Redirect it deliberately if a new snapshot is wanted, and review it before committing.

## Immediate environment concerns

The 15 GiB RAM level is workable but may require conservative build parallelism. Disk capacity is currently ample. Before graphics experiments, verify `nvidia-smi` and `vulkaninfo --summary` from the user's normal Plasma terminal. Do not change Secure Boot or TPM configuration.
