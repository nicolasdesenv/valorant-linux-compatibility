#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
logs_root="$repo_root/logs"

usage() { printf 'Usage: %s [--copy NAME=PATH]...\nCreates a per-run directory and copies only explicitly named logs.\nAllowed names: proton, wine, dxvk, vkd3d-proton, processes\n' "${0##*/}"; }
declare -a copies=()
while (( $# )); do
  case "$1" in
    --copy) [[ $# -ge 2 ]] || { printf 'ERROR: --copy needs NAME=PATH\n' >&2; exit 2; }; copies+=("$2"); shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'ERROR: unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
  esac
done

timestamp="$(date -u +%Y%m%d-%H%M%S)"
run_dir="$logs_root/${timestamp}-run"
counter=0
while [[ -e "$run_dir" ]]; do counter=$((counter + 1)); run_dir="$logs_root/${timestamp}-${counter}-run"; done
mkdir -p -- "$run_dir"
"$repo_root/scripts/check-environment.sh" >"$run_dir/environment.txt"

for spec in "${copies[@]}"; do
  name="${spec%%=*}"; source_path="${spec#*=}"
  [[ "$spec" == *=* ]] || { printf 'ERROR: invalid copy specification: %s\n' "$spec" >&2; exit 2; }
  case "$name" in proton|wine|dxvk|vkd3d-proton|processes) ;; *) printf 'ERROR: invalid log name: %s\n' "$name" >&2; exit 2 ;; esac
  [[ -f "$source_path" && ! -L "$source_path" ]] || { printf 'ERROR: source is not a regular non-symlink file: %s\n' "$source_path" >&2; exit 1; }
  cp -- "$source_path" "$run_dir/$name.log"
done

cat >"$run_dir/summary.md" <<'SUMMARY'
# Run Summary

- Objective:
- Proton version/commit:
- Configuration (no secrets):
- Expected:
- Observed:
- Exit status:
- Repetition count:
- Security-boundary assessment:
- Sensitive-data review completed: NO
- Next isolating test:
SUMMARY
chmod -R go-rwx "$run_dir"
printf 'Created run directory: %s\nReview every file for secrets before sharing.\n' "$run_dir"
