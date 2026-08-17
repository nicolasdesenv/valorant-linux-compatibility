#include "ddk/wdm.h"
typedef signed long NTSTATUS;
__declspec(dllimport) NTSTATUS WINAPI IoCreateSymbolicLink(UNICODE_STRING*, UNICODE_STRING*);
__declspec(dllimport) void WINAPI RtlInitUnicodeString(UNICODE_STRING*, const WCHAR*);
__declspec(dllimport) ULONG __cdecl DbgPrint(const char*, ...);
static NTSTATUS create_dispatch(DEVICE_OBJECT *device, IRP *irp) {
    DbgPrint("link_probe: IRP_MJ_CREATE BEGIN device=%p\\n", device);
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IofCompleteRequest(irp, 0); DbgPrint("link_probe: IRP_MJ_CREATE status=%08lx\\n", STATUS_SUCCESS);
    return STATUS_SUCCESS;
}
static NTSTATUS other_dispatch(DEVICE_OBJECT *device, IRP *irp) {
    ULONG code = irp->Tail.Overlay.CurrentStackLocation->MajorFunction;
    DbgPrint("link_probe: %s device=%p\\n", code == 0x12 ? "IRP_MJ_CLEANUP" : "IRP_MJ_CLOSE", device);
    irp->IoStatus.Status = STATUS_SUCCESS; IofCompleteRequest(irp, 0); return STATUS_SUCCESS;
}
NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path) {
    static WCHAR n[] = L"\\Device\\WineLinkProbe";
    static WCHAR l[] = L"\\DosDevices\\WineLinkProbe";
    UNICODE_STRING name, link;
    DEVICE_OBJECT *device = 0;
    NTSTATUS status;
    (void)path; DbgPrint("link_probe: DriverEntry BEGIN\\n");
    RtlInitUnicodeString(&name, n); RtlInitUnicodeString(&link, l);
    status = IoCreateDevice(driver, 0, &name, 0x22, 0x100, 0, &device);
    DbgPrint("link_probe: IoCreateDevice status=%08lx device=%p name=%ls link=%ls\\n", status, device, n, l);
    if (status != STATUS_SUCCESS) { DbgPrint("link_probe: DriverEntry END status=%08lx\\n", status); return status; }
    device->Flags &= ~0x80;
    driver->MajorFunction[0] = create_dispatch;
    driver->MajorFunction[0x12] = other_dispatch; driver->MajorFunction[0x02] = other_dispatch;
    status = IoCreateSymbolicLink(&link, &name);
    DbgPrint("link_probe: IoCreateSymbolicLink status=%08lx nt_link=%ls nt_target=%ls\\n", status, l, n);
    DbgPrint("link_probe: DriverEntry END status=%08lx\\n", status); return status;
}
