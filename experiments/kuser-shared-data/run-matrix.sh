#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
experiment_dir="$repo_root/experiments/kuser-shared-data"
proton_dir="${KUSER_PROTON_DIR:-${HOME}/.local/share/Steam/steamapps/common/Proton - Experimental}"
steam_root="${HOME}/.local/share/Steam"
run_stamp="$(date +%Y%m%d-%H%M%S)"
run_root="$repo_root/logs/${run_stamp}-kuser-instruction-matrix"

mkdir -p -- "$run_root"
sed -n '1,5p' "$proton_dir/version" > "$run_root/proton-version.txt"
printf 'proton_dir=%s\n' "$proton_dir" > "$run_root/harness-environment.txt"

run_probe()
{
    local probe_name="$1"
    local driver_name="$2"
    local service_name="$3"
    local base_app_id="$4"
    local prefix_dir="$experiment_dir/.state/$run_stamp/matrix-$probe_name"
    local case_dir="$run_root/$probe_name"

    mkdir -p -- "$prefix_dir" "$case_dir"
    chmod 700 "$prefix_dir"
    printf 'probe=%s\ndriver=%s\nservice=%s\nprefix=%s\n' \
        "$probe_name" "$driver_name" "$service_name" "$prefix_dir" > "$case_dir/environment.txt"

    env STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" STEAM_COMPAT_DATA_PATH="$prefix_dir" \
        SteamAppId="$base_app_id" SteamGameId="$base_app_id" PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run cmd.exe /c ver > "$case_dir/prefix-init.txt" 2>&1

    cp -- "$experiment_dir/$driver_name" "$prefix_dir/pfx/drive_c/$driver_name"

    set +e
    env STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" STEAM_COMPAT_DATA_PATH="$prefix_dir" \
        SteamAppId="$((base_app_id + 1))" SteamGameId="$((base_app_id + 1))" \
        PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run sc.exe create "$service_name" \
        type= kernel start= demand binpath= "C:\\$driver_name" > "$case_dir/sc-create.txt" 2>&1
    local create_status=$?

    env STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" STEAM_COMPAT_DATA_PATH="$prefix_dir" \
        SteamAppId="$((base_app_id + 2))" SteamGameId="$((base_app_id + 2))" \
        PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run sc.exe start "$service_name" > "$case_dir/sc-start.txt" 2>&1
    local start_status=$?

    env STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" STEAM_COMPAT_DATA_PATH="$prefix_dir" \
        SteamAppId="$((base_app_id + 3))" SteamGameId="$((base_app_id + 3))" \
        PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run sc.exe query "$service_name" > "$case_dir/sc-query.txt" 2>&1
    local query_status=$?
    set -e

    printf 'create=%d\nstart=%d\nquery=%d\n' \
        "$create_status" "$start_status" "$query_status" > "$case_dir/exit-status.txt"
}

run_probe mov8    ksd-mov8.sys    KsdMov8    292000
run_probe mov16   ksd-mov16.sys   KsdMov16   292010
run_probe mov32   ksd-mov32.sys   KsdMov32   292020
run_probe mov64   ksd-mov64.sys   KsdMov64   292030
run_probe movzx8  ksd-movzx8.sys  KsdMovzx8  292040
run_probe movzx16 ksd-movzx16.sys KsdMovzx16 292050
run_probe or32    ksd-or32.sys     KsdOr32    292060
run_probe xor32   ksd-xor32.sys    KsdXor32   292070
run_probe cmp32   ksd-cmp32.sys    KsdCmp32   292080
run_probe test32  ksd-test32.sys   KsdTest32  292090

printf '%s\n' "$run_root"
