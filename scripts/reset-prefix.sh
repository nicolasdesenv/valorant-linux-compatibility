#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
state_root="$repo_root/.state"
prefix="$state_root/prefix"
archive_root="$state_root/prefix-archive"

usage() { printf 'Usage: %s --archive\nMoves the isolated prefix to a timestamped local archive; it does not delete it.\n' "${0##*/}"; }
[[ "${1:-}" == "--archive" && $# -eq 1 ]] || { usage; exit 2; }
[[ -d "$prefix" && ! -L "$prefix" ]] || { printf 'ERROR: isolated prefix does not exist or is unsafe: %s\n' "$prefix" >&2; exit 1; }
case "$prefix" in "$state_root"/*) ;; *) printf 'ERROR: prefix escaped state root\n' >&2; exit 1 ;; esac
mkdir -p -- "$archive_root"
timestamp="$(date -u +%Y%m%d-%H%M%S)"
destination="$archive_root/prefix-$timestamp"
[[ ! -e "$destination" ]] || { printf 'ERROR: archive destination exists\n' >&2; exit 1; }
mv -- "$prefix" "$destination"
printf 'Archived prefix to %s\nThis is recoverable and remains ignored by Git.\n' "$destination"
