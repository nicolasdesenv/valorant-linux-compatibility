#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
source_dir="$repo_root/sources/proton"
build_name="valorant-linux-baseline"
ref="master"
execute=false

usage() {
  printf 'Usage: %s [--ref TAG_OR_COMMIT] [--build-name NAME] [--execute]\n' "${0##*/}"
  printf 'Without --execute, prints the preparation plan. --execute clones and runs the network- and resource-intensive upstream build.\n'
}

while (( $# )); do
  case "$1" in
    --ref) [[ $# -ge 2 ]] || { printf 'ERROR: --ref needs a value\n' >&2; exit 2; }; ref="$2"; shift 2 ;;
    --build-name) [[ $# -ge 2 ]] || { printf 'ERROR: --build-name needs a value\n' >&2; exit 2; }; build_name="$2"; shift 2 ;;
    --execute) execute=true; shift ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'ERROR: unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
  esac
done

[[ "$build_name" =~ ^[A-Za-z0-9._-]+$ ]] || { printf 'ERROR: unsafe build name\n' >&2; exit 2; }
[[ "$ref" != -* && "$ref" != *$'\n'* ]] || { printf 'ERROR: unsafe ref\n' >&2; exit 2; }

printf 'Proton source: %s\nRequested ref: %s\nBuild name: %s\n' "$source_dir" "$ref" "$build_name"
printf 'Strategy: upstream recursive checkout; upstream Makefile; rootless Podman SDK.\n'
printf 'Expected impact: large downloads, OCI storage, >=100 GiB working space, sustained CPU/RAM use.\n'

$execute || { printf 'DRY RUN: nothing cloned or built. Re-run with --execute only after approval.\n'; exit 0; }

for tool in git make podman; do command -v "$tool" >/dev/null 2>&1 || { printf 'ERROR: %s is required\n' "$tool" >&2; exit 1; }; done
[[ ! -e "$source_dir" ]] || { printf 'ERROR: source path already exists: %s\n' "$source_dir" >&2; exit 1; }
available_kib="$(df -Pk "$repo_root" | awk 'NR==2 {print $4}')"
[[ "$available_kib" =~ ^[0-9]+$ ]] && (( available_kib >= 104857600 )) || { printf 'ERROR: at least 100 GiB free is required\n' >&2; exit 1; }
podman info >/dev/null
mkdir -p -- "$repo_root/sources"
git clone --recurse-submodules https://github.com/ValveSoftware/Proton.git "$source_dir"
git -C "$source_dir" checkout "$ref"
git -C "$source_dir" submodule update --init --recursive
printf 'Pinned Proton commit: %s\n' "$(git -C "$source_dir" rev-parse HEAD)"
make -C "$source_dir" build_name="$build_name" install
