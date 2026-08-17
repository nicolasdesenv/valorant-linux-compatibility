#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
prefix="$repo_root/.state/prefix"

if [[ -e "$prefix" ]]; then
  printf 'Prefix already exists: %s\n' "$prefix"
  exit 0
fi
mkdir -p -- "$prefix"
chmod 700 "$prefix"
printf 'Created isolated empty prefix directory: %s\n' "$prefix"
printf 'No Wine or Proton process was started.\n'
