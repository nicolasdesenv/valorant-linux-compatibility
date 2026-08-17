#include "ddk/wdm.h"

typedef signed long NTSTATUS;

NTSTATUS WINAPI IoCreateDeviceSecure(DRIVER_OBJECT *, ULONG, UNICODE_STRING *, DEVICE_TYPE,
        ULONG, BOOLEAN, PCUNICODE_STRING, LPCGUID, DEVICE_OBJECT **);

__declspec(dllimport) ULONG __cdecl DbgPrint(const char *, ...);

static NTSTATUS secure_create(DEVICE_OBJECT *device, IRP *irp)
{
    DbgPrint("secure_probe: IRP_MJ_CREATE reached device=%p\n", device);
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

__declspec(dllexport) NTSTATUS DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *registry)
{
    static const WCHAR name_text[] = L"\\Device\\WineSecureProbe";
    static const WCHAR link_text[] = L"\\DosDevices\\WineSecureProbe";
    static const WCHAR sddl_text[] = L"D:P(A;;GA;;;SY)";
    static const GUID class_guid = {0x5e7e8b1a,0x1e3e,0x4d3c,{0x91,0x7d,0x12,0x7a,0x44,0x09,0x6a,0x31}};
    UNICODE_STRING name, link, sddl;
    DEVICE_OBJECT *device = NULL;
    NTSTATUS status;

    (void)registry;
    RtlInitUnicodeString(&name, name_text);
    RtlInitUnicodeString(&link, link_text);
    RtlInitUnicodeString(&sddl, sddl_text);
    status = IoCreateDeviceSecure(driver, 0, &name, FILE_DEVICE_UNKNOWN,
            FILE_DEVICE_SECURE_OPEN, FALSE, &sddl, &class_guid, &device);
    DbgPrint("secure_probe: IoCreateDeviceSecure status=%#lx device=%p\n", status, device);
    if (status != STATUS_SUCCESS) return status;
    device->Flags &= ~DO_DEVICE_INITIALIZING;
    driver->MajorFunction[IRP_MJ_CREATE] = secure_create;
    status = IoCreateSymbolicLink(&link, &name);
    DbgPrint("secure_probe: IoCreateSymbolicLink status=%#lx\n", status);
    return status;
}
