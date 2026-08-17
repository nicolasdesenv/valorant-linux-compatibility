# Compatibility Matrix

Allowed statuses: **NOT TESTED**, **PASS**, **PARTIAL**, **BLOCKED**, **FAILED**.

| Component | Status | Evidence / note |
|---|---|---|
| Proton baseline | PASS | Official Proton Experimental `experimental-11.0-20260805` created an isolated prefix and ran `cmd.exe /c ver` with exit status 0; NVIDIA Vulkan enumeration also passed (`logs/20260817-035712-proton-baseline/`) |
| Pinned Wine kernel KUSER_SHARED_DATA reads | PARTIAL | Unmodified pinned Wine passes `mov`, `movzx`, `or`, and `xor`; read-only `cmp` and `test` are not emulated and fail with unhandled `0xc0000005` (`logs/20260817-043705-kuser-instruction-matrix/`) |
| Experimental Wine KUSER_SHARED_DATA candidate | PASS | Branch `codex/kuser-cmp-test-experimental`, commit `9c75a96c44a`, adds only reproduced `cmp32`/`test32` forms. All ten isolated probes pass; `cmp32` flags `0x85 == 0x85`, `test32` flags `0x4 == 0x4`, with no legacy regression (`logs/20260817-045424-kuser-instruction-matrix/`) |
| Patched Wine benign ZwLoadDriver/SCM path | PASS | Independent control and KUSER-reading drivers both reached `DriverEntry`, returned `STATUS_SUCCESS`, and completed `sc create/start/query` as `0/0/0` in fresh prefixes (`logs/20260817-051546-kuser-zwload-probe/`) |
| Patched Wine `IoCreateDeviceSecure` security semantics | FAILED | Device creation returned success, but the own user-mode client received `ERROR_FILE_NOT_FOUND (2)` for the supposedly created DOS link; expected ACL denial (`ERROR_ACCESS_DENIED`) was not reached (`logs/20260817-052738-kuser-zwload-probe/secure-acl-summary.md`) |
| Riot Client with experimental Wine candidate | PARTIAL | After a real Fedora reboot, the patched Proton copy loaded native `vgk.sys`; Wine's handler continued past both `KUSER_SHARED_DATA` faults at `+0x320` and `+0x004`. `ZwLoadDriver` then failed with `d4494e49` / SCM 317, and Riot remained `restartRequired=true` (`logs/20260817-050820-riot-patched-postreboot/`) |
| Riot installer | PASS | Official Riot-distributed installer completed and installed Riot Client (`logs/20260817-035907-riot-baseline/`) |
| Riot Client startup | PASS | Riot Client services and Electron UI started under Proton |
| Riot Client UI | PASS | Login and post-login UI rendered correctly with remote content |
| Authentication | PASS | User completed authentication manually; no credentials or session material collected |
| Updater | PASS | Riot Client downloaded and completed the reported VALORANT/Vanguard installation |
| VALORANT installation | PASS | User reported installation completion; VALORANT content and Vanguard components are present in the isolated prefix |
| VALORANT bootstrap | BLOCKED | After a confirmed real Fedora reboot, the same prefix still reports `restartRequired=true`; autolaunch is disabled because Vanguard is not running (`logs/20260817-041740-riot-baseline/`) |
| VALORANT process | NOT TESTED | Installed but not launched; Play remains disabled by the restart/Vanguard gate |
| Graphics initialization | NOT TESTED | No game process run |
| Vanguard user-mode | BLOCKED | `vgc.exe` is installed and registered, but no `vgc` process/start attempt was observed after reboot; progression is blocked by the preceding `vgk` failure |
| Vanguard kernel dependency | FAILED | After a confirmed real reboot, Wine naturally attempted system-start `vgk`; native `vgk.sys` raised `0xc0000005`, `ZwLoadDriver` returned `d4494e49`, and service autostart failed with error 317 |
| Online matchmaking | NOT TESTED | No attempt permitted through security circumvention |

Statuses change only with a linked run record or documented boundary finding.
