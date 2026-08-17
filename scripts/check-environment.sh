#!/usr/bin/env bash
set -euo pipefail

section() { printf '\n[%s]\n' "$1"; }
try() { "$@" 2>&1 || printf 'UNAVAILABLE (command failed: %s)\n' "$1"; }
have() { command -v "$1" >/dev/null 2>&1; }

section operating-system
sed -n -e 's/^PRETTY_NAME=//p' -e 's/^VERSION_ID=//p' /etc/os-release | tr -d '"'
try uname -srmo

section desktop-session
printf 'desktop=%s\nsession=%s\nwayland_display=%s\nx11_display=%s\n' \
  "${XDG_CURRENT_DESKTOP:-unknown}" "${XDG_SESSION_TYPE:-unknown}" \
  "${WAYLAND_DISPLAY:-unset}" "${DISPLAY:-unset}"

section graphics
if have lspci; then
  lspci -nnk | awk '/VGA compatible controller|3D controller/{print; n=3; next} n>0{print; n--}'
else
  printf 'lspci: NOT FOUND\n'
fi
if have nvidia-smi; then try nvidia-smi --query-gpu=name,driver_version --format=csv,noheader; else printf 'nvidia-smi: NOT FOUND\n'; fi
if have vulkaninfo; then try vulkaninfo --summary; else printf 'vulkaninfo: NOT FOUND\n'; fi

section platform-security
if have mokutil; then try mokutil --sb-state; else printf 'mokutil: NOT FOUND\n'; fi
if have systemd-analyze; then try systemd-analyze has-tpm2; else printf 'systemd-analyze: NOT FOUND\n'; fi
for device in /dev/tpm0 /dev/tpmrm0; do
  if [[ -e "$device" ]]; then printf '%s: present\n' "$device"; else printf '%s: absent or inaccessible\n' "$device"; fi
done

section steam-and-proton
if have steam; then printf 'steam=%s\n' "$(command -v steam)"; else printf 'steam command: NOT FOUND\n'; fi
if have flatpak && flatpak info com.valvesoftware.Steam >/dev/null 2>&1; then printf 'Steam Flatpak: installed\n'; else printf 'Steam Flatpak: not detected\n'; fi
shopt -s nullglob
steam_roots=("${HOME}/.steam/root" "${HOME}/.local/share/Steam" "${HOME}/.var/app/com.valvesoftware.Steam/.local/share/Steam")
found_steam=false
for root in "${steam_roots[@]}"; do
  [[ -d "$root" ]] || continue
  found_steam=true
  printf 'Steam root: %s\n' "$root"
  for version_file in "$root"/steamapps/common/Proton*/version "$root"/compatibilitytools.d/*/version; do
    [[ -f "$version_file" ]] && printf 'Proton: %s: %s\n' "$version_file" "$(head -n 1 "$version_file")"
  done
done
$found_steam || printf 'Standard Steam library paths: not detected or inaccessible\n'

section tools
for tool in wine podman git gcc g++ clang clang++ make cmake meson ninja cargo rustc python3 perl patch; do
  if have "$tool"; then
    version="$($tool --version 2>&1 | head -n 1 || true)"
    printf '%-10s %s (%s)\n' "$tool" "$(command -v "$tool")" "${version:-version unavailable}"
  else
    printf '%-10s NOT FOUND\n' "$tool"
  fi
done

section capacity
try df -hP "${PWD}"
if have free; then try free -h; fi
if have nproc; then printf 'logical_cpus=%s\n' "$(nproc)"; fi

printf '\nReview output before saving or sharing it. No credentials or unrelated files were inspected.\n'
