#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
experiment_dir="$repo_root/experiments/kuser-shared-data"
proton_dir="${HOME}/.local/share/Steam/steamapps/common/Proton - Experimental"
steam_root="${HOME}/.local/share/Steam"
run_stamp="$(date +%Y%m%d-%H%M%S)"
run_root="$repo_root/logs/${run_stamp}-kuser-shared-data"

if [[ ! -x "$proton_dir/proton" ]]; then
    printf 'Official Proton Experimental was not found at %s\n' "$proton_dir" >&2
    exit 1
fi

mkdir -p -- "$run_root"
sed -n '1,5p' "$proton_dir/version" > "$run_root/proton-version.txt"

run_probe()
{
    local probe_name="$1"
    local driver_name="$2"
    local service_name="$3"
    local app_id="$4"
    local prefix_dir="$experiment_dir/.state/$run_stamp/prefix-$probe_name"
    local case_dir="$run_root/$probe_name"
    local init_app_id="$app_id"
    local create_app_id=$((app_id + 1))
    local start_app_id=$((app_id + 2))
    local query_app_id=$((app_id + 3))

    mkdir -p -- "$prefix_dir" "$case_dir"
    chmod 700 "$prefix_dir"

    printf 'probe=%s\ndriver=%s\nservice=%s\nprefix=%s\n' \
        "$probe_name" "$driver_name" "$service_name" "$prefix_dir" > "$case_dir/environment.txt"

    env \
        STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" \
        STEAM_COMPAT_DATA_PATH="$prefix_dir" \
        SteamAppId="$init_app_id" SteamGameId="$init_app_id" \
        PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run cmd.exe /c ver > "$case_dir/prefix-init.txt" 2>&1

    cp -- "$experiment_dir/$driver_name" "$prefix_dir/pfx/drive_c/$driver_name"

    set +e
    env \
        STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" \
        STEAM_COMPAT_DATA_PATH="$prefix_dir" \
        SteamAppId="$create_app_id" SteamGameId="$create_app_id" \
        PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run sc.exe create "$service_name" \
            type= kernel start= demand binpath= "C:\\$driver_name" \
            > "$case_dir/sc-create.txt" 2>&1
    local create_status=$?

    env \
        STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" \
        STEAM_COMPAT_DATA_PATH="$prefix_dir" \
        SteamAppId="$start_app_id" SteamGameId="$start_app_id" \
        PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run sc.exe start "$service_name" \
            > "$case_dir/sc-start.txt" 2>&1
    local start_status=$?

    env \
        STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" \
        STEAM_COMPAT_DATA_PATH="$prefix_dir" \
        SteamAppId="$query_app_id" SteamGameId="$query_app_id" \
        PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run sc.exe query "$service_name" \
            > "$case_dir/sc-query.txt" 2>&1
    local query_status=$?
    set -e

    printf 'create=%d\nstart=%d\nquery=%d\n' \
        "$create_status" "$start_status" "$query_status" > "$case_dir/exit-status.txt"
}

run_probe control kuser-control.sys KUserSharedDataControl 291001
run_probe offset-004 kuser-004.sys KUserSharedData004 291004
run_probe offset-320 kuser-320.sys KUserSharedData320 291320

printf '%s\n' "$run_root"
