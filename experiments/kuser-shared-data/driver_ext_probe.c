/*
 * Independent benign kernel driver probe for generic ntoskrnl APIs:
 *   - IoAllocateDriverObjectExtension / IoGetDriverObjectExtension
 *   - IoSizeofWorkItem / IoInitializeWorkItem / IoUninitializeWorkItem
 *   - KeIpiGenericCall
 *   - KeQuerySystemTimePrecise
 *   - ExInitializeRundownProtection / ExAcquireRundownProtection /
 *     ExReleaseRundownProtection / ExWaitForRundownProtectionRelease
 *   - ExInitializePushLock / ExAcquirePushLockExclusiveEx /
 *     ExReleasePushLockExclusiveEx
 *
 * This driver contains no Riot or Vanguard logic and executes in an isolated
 * non-Riot prefix.
 */

typedef signed long NTSTATUS;
typedef unsigned char BOOLEAN;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned long long ULONG_PTR;
typedef unsigned long long ULONG64;
typedef void *PVOID;

#define STATUS_SUCCESS                ((NTSTATUS)0x00000000L)
#define STATUS_UNSUCCESSFUL           ((NTSTATUS)0xC0000001L)
#define STATUS_OBJECT_NAME_COLLISION  ((NTSTATUS)0xC0000035L)
#define WINAPI                        __stdcall
#define FASTCALL                      __fastcall

typedef struct _LARGE_INTEGER {
    ULONG64 QuadPart;
} LARGE_INTEGER;

typedef struct _EX_RUNDOWN_REF {
    ULONG_PTR Count;
} EX_RUNDOWN_REF, *PEX_RUNDOWN_REF;

typedef ULONG_PTR (WINAPI *PKIPI_BROADCAST_WORKER)(ULONG_PTR Argument);

__declspec(dllimport) ULONG __cdecl DbgPrint(const char *format, ...);
__declspec(dllimport) NTSTATUS WINAPI IoAllocateDriverObjectExtension(
    PVOID DriverObject, PVOID ClientIdentificationAddress,
    ULONG DriverObjectExtensionSize, PVOID *DriverObjectExtension);
__declspec(dllimport) PVOID WINAPI IoGetDriverObjectExtension(
    PVOID DriverObject, PVOID ClientIdentificationAddress);
__declspec(dllimport) ULONG WINAPI IoSizeofWorkItem(void);
__declspec(dllimport) void WINAPI IoInitializeWorkItem(PVOID IoObject, PVOID IoWorkItem);
__declspec(dllimport) void WINAPI IoUninitializeWorkItem(PVOID IoWorkItem);
__declspec(dllimport) ULONG_PTR WINAPI KeIpiGenericCall(
    PKIPI_BROADCAST_WORKER BroadcastFunction, ULONG_PTR Context);
__declspec(dllimport) void WINAPI KeQuerySystemTimePrecise(LARGE_INTEGER *CurrentTime);
__declspec(dllimport) void FASTCALL ExInitializeRundownProtection(PEX_RUNDOWN_REF RunRef);
__declspec(dllimport) BOOLEAN FASTCALL ExAcquireRundownProtection(PEX_RUNDOWN_REF RunRef);
__declspec(dllimport) void FASTCALL ExReleaseRundownProtection(PEX_RUNDOWN_REF RunRef);
__declspec(dllimport) void FASTCALL ExWaitForRundownProtectionRelease(PEX_RUNDOWN_REF RunRef);
__declspec(dllimport) void WINAPI ExInitializePushLock(ULONG_PTR *PushLock);
__declspec(dllimport) void WINAPI ExAcquirePushLockExclusiveEx(ULONG_PTR *PushLock, ULONG Flags);
__declspec(dllimport) void WINAPI ExReleasePushLockExclusiveEx(ULONG_PTR *PushLock, ULONG Flags);

struct probe_extension {
    ULONG magic;
    ULONG64 timestamp;
};

static ULONG_PTR WINAPI ipi_worker_callback(ULONG_PTR context)
{
    return context ^ 0x55aa55aaULL;
}

__declspec(dllexport)
NTSTATUS WINAPI DriverEntry(PVOID driver_object, PVOID registry_path)
{
    struct probe_extension *ext = (struct probe_extension *)0;
    struct probe_extension *dup = (struct probe_extension *)0;
    struct probe_extension *looked_up = (struct probe_extension *)0;
    UCHAR work_item_buf[64];
    LARGE_INTEGER now;
    EX_RUNDOWN_REF rundown;
    ULONG_PTR push_lock = 0;
    ULONG_PTR ipi_result;
    ULONG work_item_size;
    NTSTATUS status;

    (void)registry_path;

    /* 1. Driver object extension allocation, lookup, and collision check */
    status = IoAllocateDriverObjectExtension(
        driver_object, (PVOID)&ipi_worker_callback, sizeof(*ext), (PVOID *)&ext);
    if (status != STATUS_SUCCESS || !ext) {
        DbgPrint("driver_ext_probe: IoAllocateDriverObjectExtension failed status=%#lx\n", status);
        return STATUS_UNSUCCESSFUL;
    }
    ext->magic = 0xdecafbadUL;

    looked_up = (struct probe_extension *)IoGetDriverObjectExtension(
        driver_object, (PVOID)&ipi_worker_callback);
    if (looked_up != ext || looked_up->magic != 0xdecafbadUL) {
        DbgPrint("driver_ext_probe: IoGetDriverObjectExtension mismatch got=%p\n", looked_up);
        return STATUS_UNSUCCESSFUL;
    }

    status = IoAllocateDriverObjectExtension(
        driver_object, (PVOID)&ipi_worker_callback, sizeof(*dup), (PVOID *)&dup);
    if (status != STATUS_OBJECT_NAME_COLLISION) {
        DbgPrint("driver_ext_probe: expected collision, got=%#lx\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    /* 2. Caller-allocated work item lifecycle */
    work_item_size = IoSizeofWorkItem();
    if (work_item_size == 0 || work_item_size > sizeof(work_item_buf)) {
        DbgPrint("driver_ext_probe: unexpected IoSizeofWorkItem=%lu\n", work_item_size);
        return STATUS_UNSUCCESSFUL;
    }
    IoInitializeWorkItem(driver_object, work_item_buf);
    IoUninitializeWorkItem(work_item_buf);

    /* 3. Synchronous IPI callback */
    ipi_result = KeIpiGenericCall(ipi_worker_callback, 0x12345678ULL);
    if (ipi_result != (0x12345678ULL ^ 0x55aa55aaULL)) {
        DbgPrint("driver_ext_probe: KeIpiGenericCall unexpected result=%#llx\n", ipi_result);
        return STATUS_UNSUCCESSFUL;
    }

    /* 4. Precise system time query */
    now.QuadPart = 0;
    KeQuerySystemTimePrecise(&now);
    if (now.QuadPart == 0) {
        DbgPrint("driver_ext_probe: KeQuerySystemTimePrecise returned zero\n");
        return STATUS_UNSUCCESSFUL;
    }
    ext->timestamp = now.QuadPart;

    /* 5. Rundown protection lifecycle */
    ExInitializeRundownProtection(&rundown);
    if (!ExAcquireRundownProtection(&rundown)) {
        DbgPrint("driver_ext_probe: ExAcquireRundownProtection failed before rundown\n");
        return STATUS_UNSUCCESSFUL;
    }
    ExReleaseRundownProtection(&rundown);
    ExWaitForRundownProtectionRelease(&rundown);
    if (ExAcquireRundownProtection(&rundown)) {
        DbgPrint("driver_ext_probe: ExAcquireRundownProtection succeeded after rundown\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* 6. PushLock lifecycle */
    ExInitializePushLock(&push_lock);
    ExAcquirePushLockExclusiveEx(&push_lock, 0);
    ExReleasePushLockExclusiveEx(&push_lock, 0);

    DbgPrint("driver_ext_probe: all checks PASS (time=%#llx)\n", ext->timestamp);
    return STATUS_SUCCESS;
}
