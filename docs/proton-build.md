# Baseline Proton Build Strategy

Research checked on 2026-08-17 against Valve's current [Proton README](https://github.com/ValveSoftware/Proton) and [Steam Runtime documentation](https://github.com/ValveSoftware/steam-runtime). The historical Fedora wiki is not the baseline: it targets Fedora 29-era Docker workflows and recommends disabling SELinux, which is neither necessary nor acceptable here.

## Recommendation

Use an explicitly pinned Valve Proton commit with all submodules, and let Proton's top-level build system run its official Proton SDK image through rootless Podman. Do not manually reproduce the SDK. Keep source, build, and ccache directories inside this repository's ignored local directories (or another user-owned workspace). Preserve SELinux. If bind mounts fail, assess a narrowly scoped build-directory relabel; never relabel a system or home directory wholesale.

Proton 11 and newer run with Steam Linux Runtime 4; Proton 8-10 use Steam Linux Runtime 3 (`sniper`). The checked-out Proton branch and its build scripts determine the correct SDK image. Record the commit and resulting image digest in every build record.

## Upstream repositories/components

- `ValveSoftware/Proton` (orchestrator and compatibility tool), with recursive submodules
- Wine (Proton fork), DXVK, VKD3D-Proton, and other pinned Proton submodules
- Valve Proton SDK OCI image, selected by the build scripts
- Steam Runtime / pressure-vessel at runtime; it is not a separate source checkout needed for the normal build

## Host prerequisites

Current upstream says most work occurs in the SDK container. Required host capabilities are Git (including submodules and Git LFS if required by the selected revision), GNU Make, a functional rootless Podman or Docker setup, network access for source/submodules and OCI images, and sufficient storage. `ccache` is optional. Exact prerequisites must be confirmed with `make help` and `configure.sh --help` at the pinned commit before installation.

Fedora presently has Git, Make, and Podman installed. Rootless Podman must be verified in a normal login session; this managed session cannot write its runtime directory. No missing build packages are installed by this project.

## Reproducible procedure (not executed)

1. Choose and record an upstream tag/branch and immutable commit.
2. Clone `https://github.com/ValveSoftware/Proton.git` with `--recurse-submodules`; verify every submodule revision.
3. Inspect `make help` and `configure.sh --help` at that commit.
4. Verify rootless Podman with a harmless container command and record Podman plus OCI image digest.
5. Prefer the top-level `make install` workflow for a baseline, or configure an out-of-tree build with `--container-engine=podman`, an explicit build name, and optional cache. Use `--relabel-volumes` only after reviewing its SELinux impact and limiting paths.
6. Use conservative parallelism initially because the host has 15 GiB RAM.
7. Produce `redist/` with `make redist` when an inspectable artifact is preferred; `make install` installs the local tool for the current Steam user.

The repository helper prints the intended commands and refuses to clone/build unless `--execute` is supplied.

## Resource estimate

Upstream does not publish a stable size or time guarantee. Plan conservatively for **at least 100 GiB free**, with **150 GiB preferred** for recursive sources, SDK layers, dual-architecture objects, debug symbols, ccache, and rebuilds. A clean build can plausibly take **1-4 hours** on this 12-thread mobile CPU, but network, cooling, RAM pressure, commit, and debug settings dominate. These are planning estimates, not measured results. The current 432 GiB free is sufficient for a controlled baseline.

## Artifact and Steam installation

`make redist` creates a redistributable directory. A local tool belongs under `~/.steam/root/compatibilitytools.d/<tool-name>/` and contains files such as `compatibilitytool.vdf`, `proton`, `proton_dist.tar`, `toolmanifest.vdf`, and `version`. `make install` performs the per-user installation; restart Steam, then select the named compatibility tool. Steam path variants must be resolved first—this session could not locate the user's installation. Installing a local tool is deferred until explicitly approved.

## Expected workstation impact

Source and image downloads are large; compilation is CPU-, RAM-, disk-, and thermally intensive. Rootless Podman stores OCI layers in user storage. A local install modifies the user's Steam compatibility-tools directory. None of these actions are performed in this iteration.
