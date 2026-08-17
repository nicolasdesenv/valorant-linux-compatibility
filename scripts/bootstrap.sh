#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"

required=(git make podman)
optional=(vulkaninfo nvidia-smi mokutil wine cmake meson ninja)
missing=0

printf 'Checking host prerequisites; no packages will be installed.\n'
for tool in "${required[@]}"; do
  if command -v "$tool" >/dev/null 2>&1; then
    printf 'PASS  %s\n' "$tool"
  else
    printf 'MISS  %s (required before a Proton build)\n' "$tool"
    missing=1
  fi
done
for tool in "${optional[@]}"; do
  if command -v "$tool" >/dev/null 2>&1; then printf 'PASS  %s\n' "$tool"; else printf 'INFO  %s not found\n' "$tool"; fi
done

available_kib="$(df -Pk "$repo_root" | awk 'NR==2 {print $4}')"
if [[ "$available_kib" =~ ^[0-9]+$ ]] && (( available_kib < 104857600 )); then
  printf 'WARN  Less than 100 GiB is free; do not start a full build.\n'
else
  printf 'PASS  At least 100 GiB is free.\n'
fi

if (( missing )); then
  printf 'Bootstrap check incomplete. Review missing prerequisites manually.\n' >&2
  exit 1
fi
printf 'Bootstrap check complete. No system changes were made.\n'
