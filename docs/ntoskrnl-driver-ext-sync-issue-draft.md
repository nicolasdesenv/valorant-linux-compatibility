# Issue Draft: Implement Missing `ntoskrnl.exe` Driver Extensions, Work-Item Helpers, Synchronization Primitives, and Export Data Symbols

## Summary

While auditing public Windows kernel APIs against pinned Wine (`81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`) and Proton Experimental (`11.0-20261001`), we identified two distinct compatibility blockers in `dlls/ntoskrnl.exe`:

1. **Unimplemented candidates in `docs/driver-compatibility-candidates.md`:** Specifically Candidate #6 (`KeIpiGenericCall`) and Candidate #7 (`IoAllocateDriverObjectExtension` / `IoGetDriverObjectExtension`), which currently return `STATUS_NOT_IMPLEMENTED` or `0` from explicit stubs in `ntoskrnl.c`.
2. **Missing exports and `@ stub` landmines in `ntoskrnl.exe.spec`:** A static PE import audit of a modern 64-bit Windows kernel driver (`119` imports from `ntoskrnl.exe`) revealed **14 symbols completely absent from Proton 11.0's `ntoskrnl.exe` export table** (causing immediate PE loader failure before `DriverEntry`) and **11 symbols declared as `@ stub`** in `ntoskrnl.exe.spec` (which resolve at load time to `__wine_spec_unimplemented_stub` and raise fatal `EXCEPTION_WINE_STUB` `0x80000100` when invoked, or point to stub thunks instead of memory variables for data exports like `KeNumberProcessors` and `MmHighestUserAddress`).

We also ran an isolated baseline installation of **League of Legends** under Proton Experimental (`logs/20261015-082005-lol-installer-baseline/`) and confirmed an important difference from VALORANT: Riot Client explicitly waives the system reboot check for fresh League of Legends and Teamfight Tactics installations (`restartRequired=false, restartWaivedReason=NEW_INSTALL`), allowing the game content installer to proceed immediately while `vgk.sys` and `vgc.exe` (`1.19.1.18`) are staged in the prefix.

---

## 1. Static PE Import Audit of `ntoskrnl.exe`

Using `llvm-objdump -p` against the staged `ntoskrnl.exe` inside Proton Experimental (`11.0-20261001`) and comparing against pinned upstream Wine (`81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`), we categorized all non-implemented kernel imports required by modern WDM/NT drivers:

### A. Missing from Proton 11.0 `ntoskrnl.exe` Export Table (14 symbols)

If a driver imports any of these symbols and is loaded under Proton 11.0 without an updated `ntoskrnl.exe`, the PE loader fails to resolve the import table before `DriverEntry` can run:

| Symbol | Status in Pinned Wine (`81d78e4f3ea`) | Resolution in Patch `0002` |
|---|---|---|
| `KeAcquireGuardedMutex` | Present in Wine 11.19 (`sync.c`), missing in Proton 11.0 | Exported and backed by `KeAcquireFastMutex` |
| `KeReleaseGuardedMutex` | Present in Wine 11.19 (`sync.c`), missing in Proton 11.0 | Exported and backed by `KeReleaseFastMutex` |
| `KeGetCurrentIrql` | Present in Wine 11.19 (`ntoskrnl.c`), missing in Proton 11.0 | Exported (`PASSIVE_LEVEL` / `0`) |
| `KeIpiGenericCall` | `@ stdcall` stub returning `0` | Calls `BroadcastFunction(Context)` synchronously on the calling processor |
| `IoInitializeWorkItem` | `@ stub` in `ntoskrnl.exe.spec` | Implemented on top of `struct _IO_WORKITEM` |
| `KeQuerySystemTimePrecise` | `@ stub` in `ntoskrnl.exe.spec` | Implemented via `RtlGetSystemTimePrecise()` |
| `ExInitializePushLock` | `@ stub` in `ntoskrnl.exe.spec` | Implemented (`*lock = 0`) |
| `ExAcquirePushLockExclusiveEx` | Missing from `.spec` | Added to `.spec` and implemented via `AcquireSRWLockExclusive` |
| `ExReleasePushLockExclusiveEx` | Missing from `.spec` | Added to `.spec` and implemented via `ReleaseSRWLockExclusive` |
| `ExAllocateCacheAwareRundownProtection` | `@ stub` in `ntoskrnl.exe.spec` | Implemented via `ExAllocatePool` + `ExInitializeRundownProtection` |
| `ExFreeCacheAwareRundownProtection` | `@ stub` in `ntoskrnl.exe.spec` | Implemented via `ExFreePool` |
| `ExAcquireRundownProtectionCacheAwareEx` | `@ stub` in `ntoskrnl.exe.spec` | Implemented via atomic reference counting |
| `ExReleaseRundownProtectionCacheAwareEx` | `@ stub` in `ntoskrnl.exe.spec` | Implemented via atomic reference counting |
| `ExWaitForRundownProtectionReleaseCacheAware` | `@ stub` in `ntoskrnl.exe.spec` | Implemented |

### B. Critical `@ stub` Data and Synchronization Exports in `ntoskrnl.exe.spec`

In `dlls/ntoskrnl.exe/ntoskrnl.exe.spec`, several widely used symbols are declared as `@ stub`:

- **`KeNumberProcessors` and `MmHighestUserAddress`**: Both are **data exports** (variables), not functions. Declaring them as `@ stub` in `.spec` exports a function thunk (`__wine_spec_unimplemented_stub`). When a 64-bit driver dereferences `__imp_KeNumberProcessors` to read the CPU count or reads `__imp_MmHighestUserAddress` to validate user pointers, it reads machine code bytes from the stub thunk instead of valid kernel variables!
  - `KeNumberProcessors` was already defined in `ntoskrnl.c` (`CCHAR KeNumberProcessors = 1;`), so changing its `.spec` entry from `@ stub KeNumberProcessors` to `@ extern KeNumberProcessors` immediately exposes the real variable.
  - For `MmHighestUserAddress`, we defined `void *MmHighestUserAddress = (void *)0x7ffffffeffffULL;` in `ntoskrnl.c` and changed its `.spec` entry to `@ extern MmHighestUserAddress`.
- **Standard Rundown Protection (`ExInitializeRundownProtection`, `ExAcquireRundownProtection`, `ExReleaseRundownProtection`, `ExWaitForRundownProtectionRelease`, etc.)**: Previously `@ stub`, causing an immediate `0x80000100` unimplemented-stub exception when called during driver startup or teardown.

---

## 2. Reproducer and Patch Summary

### Independent Benign Reproducer (`experiments/kuser-shared-data/driver_ext_probe.c`)

Following the repository's strict isolation rules, we wrote a standalone benign kernel driver (`driver_ext_probe.c`) that tests only public Windows kernel APIs in a clean prefix without touching any third-party binaries:

1. Allocates a custom per-driver extension via `IoAllocateDriverObjectExtension`, verifies duplicate client-ID rejection (`STATUS_OBJECT_NAME_COLLISION`), and retrieves the extension pointer via `IoGetDriverObjectExtension`.
2. Queries `IoSizeofWorkItem()`, allocates a buffer, and initializes/uninitializes a work item using `IoInitializeWorkItem` and `IoUninitializeWorkItem`.
3. Invokes `KeIpiGenericCall` with a benign callback and verifies the returned `ULONG_PTR` accumulator and `KeGetCurrentIrql() == PASSIVE_LEVEL`.
4. Queries `KeQuerySystemTimePrecise` and checks that a non-zero timestamp is returned.
5. Exercises `ExInitializeRundownProtection`, `ExAcquireRundownProtection`, `ExReleaseRundownProtection`, `ExWaitForRundownProtectionRelease`, and `ExInitializePushLock` / `ExAcquirePushLockExclusiveEx` / `ExReleasePushLockExclusiveEx`.

### Wine Patch (`patches/wine/0002-ntoskrnl-implement-driver-extensions-work-item-init-.patch`)

Generated cleanly against pinned Wine commit `81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`, modifying:
- `dlls/ntoskrnl.exe/ntoskrnl.c`
- `dlls/ntoskrnl.exe/ntoskrnl.exe.spec`
- `dlls/ntoskrnl.exe/ntoskrnl_private.h`
- `dlls/ntoskrnl.exe/sync.c`
- `include/ddk/wdm.h`

#### Implementation Highlights
- **Driver Object Extensions (`IoAllocateDriverObjectExtension` / `IoGetDriverObjectExtension`):** Added `struct list extensions` to `struct wine_driver` (`ntoskrnl_private.h`), protected by a critical section, storing `(client_id, buffer)` pairs and freeing all registered extensions automatically inside `wine_driver_free` when the driver object is destroyed.
- **Work-Item Helpers (`IoSizeofWorkItem`, `IoInitializeWorkItem`, `IoUninitializeWorkItem`):** Extracted work-item initialization out of `IoAllocateWorkItem` so caller-allocated work items work identically to heap-allocated ones.
- **Rundown Protection & PushLocks (`sync.c`):** Implemented atomic reference counting on `EX_RUNDOWN_REF` (`EX_RUNDOWN_ACTIVE` bit + waiter wakeup) and mapped exclusive push locks (`ExAcquirePushLockExclusiveEx` / `ExReleasePushLockExclusiveEx`) to `SRWLOCK`.

---

## 3. League of Legends Baseline Observation (`logs/20261015-082005-lol-installer-baseline/`)

During our baseline test of the official League of Legends installer (`Install League of Legends tr.exe`) in an isolated Proton Experimental prefix (`.state/lol-prefix`), Riot Client initialized normally, installed Riot Vanguard `1.19.1.18`, and logged the following per-product policy check in `steam-293002.log`:

```text
vanguard: Per-product vanguard check for valorant: restartRequired=true, restartWaivedReason=
vanguard: Per-product vanguard check for league_of_legends: restartRequired=false, restartWaivedReason=NEW_INSTALL
vanguard: Per-product vanguard check for teamfighttactics: restartRequired=false, restartWaivedReason=NEW_INSTALL
```

Unlike VALORANT, which blocks the Play button immediately on first install until a system reboot, League of Legends waives the initial reboot requirement on a fresh install (`NEW_INSTALL`) and allows the full game client (`LeagueClient.exe` / `LeagueClientUx.exe`) to download and launch.

---

## Verification Checklist

- [x] `driver_ext_probe.c` compiles cleanly to a freestanding 64-bit native PE32+ `.sys` driver with `clang --target=x86_64-w64-mingw32 -fuse-ld=lld` and `llvm-dlltool`.
- [x] `patches/wine/0002-ntoskrnl-implement-driver-extensions-work-item-init-.patch` applies cleanly (`git apply --check`) on top of `0001-ntoskrnl-emulate-cmp32-and-test32-reads-on-KUSER_SHA.patch` at Wine commit `81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`.
- [x] No proprietary binaries, session tokens, or anti-cheat bypass mechanisms are included.
