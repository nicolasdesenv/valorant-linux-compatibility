#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
experiment_dir="$repo_root/experiments/kuser-shared-data"
proton_dir="${KUSER_PROTON_DIR:-$repo_root/.state/Proton-KUSER-Experimental}"
steam_root="${HOME}/.local/share/Steam"
run_stamp="$(date +%Y%m%d-%H%M%S)"
run_root="$repo_root/logs/${run_stamp}-kuser-zwload-probe"
mkdir -p "$run_root"
printf 'proton=%s\nscope=independent ZwLoadDriver/SCM probe\n' "$proton_dir" > "$run_root/environment.txt"

run_case()
{
    local name="$1" driver="$2" service="$3" appid="$4"
    local prefix="$experiment_dir/.state/$run_stamp/$name" case_dir="$run_root/$name"
    mkdir -p "$prefix" "$case_dir"; chmod 700 "$prefix"
    env STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" STEAM_COMPAT_DATA_PATH="$prefix" \
        SteamAppId="$appid" SteamGameId="$appid" PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run cmd.exe /c ver > "$case_dir/init.txt" 2>&1
    cp "$experiment_dir/$driver" "$prefix/pfx/drive_c/$driver"
    if [[ "$name" == secure ]]; then cp "$experiment_dir/secure-client.exe" "$prefix/pfx/drive_c/secure-client.exe"; fi
    if [[ "$name" == link ]]; then cp "$experiment_dir/link-client.exe" "$prefix/pfx/drive_c/link-client.exe"; fi
    set +e
    env STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" STEAM_COMPAT_DATA_PATH="$prefix" \
        SteamAppId="$((appid + 1))" SteamGameId="$((appid + 1))" PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run sc.exe create "$service" type= kernel start= demand binpath= "C:\\$driver" > "$case_dir/create.txt" 2>&1
    create=$?
    if [[ "$name" == link || "$name" == secure ]]; then
        env STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" STEAM_COMPAT_DATA_PATH="$prefix" \
            SteamAppId="$((appid + 2))" SteamGameId="$((appid + 2))" PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
            "$proton_dir/proton" run cmd.exe /c "sc start $service & C:\\$([[ $name == link ]] && echo link-client.exe || echo secure-client.exe)" > "$case_dir/start.txt" 2>&1
    else
        env STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" STEAM_COMPAT_DATA_PATH="$prefix" \
            SteamAppId="$((appid + 2))" SteamGameId="$((appid + 2))" PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
            "$proton_dir/proton" run sc.exe start "$service" > "$case_dir/start.txt" 2>&1
    fi
    start=$?
    env STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root" STEAM_COMPAT_DATA_PATH="$prefix" \
        SteamAppId="$((appid + 3))" SteamGameId="$((appid + 3))" PROTON_LOG=1 PROTON_LOG_DIR="$case_dir" \
        "$proton_dir/proton" run sc.exe query "$service" > "$case_dir/query.txt" 2>&1
    query=$?
    if [[ "$name" == link || "$name" == secure ]]; then
        client=combined
    else
        client=not-tested
    fi
    set -e
    printf 'create=%d\nstart=%d\nquery=%d\nclient=%s\n' "$create" "$start" "$query" "$client" > "$case_dir/status.txt"
}

run_case control zwload-control.sys ZwloadControl 293000
run_case kuser zwload-kuser.sys ZwloadKuser 293010
run_case secure secure-probe.sys SecureProbe 293020
run_case link link-probe.sys LinkProbe 293030
printf '%s\n' "$run_root"
