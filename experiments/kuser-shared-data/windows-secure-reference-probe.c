/* Build/run only on real Windows AMD64. Not a Wine test. */
#include <wdm.h>
#include <wdmsec.h>
extern NTSTATUS WdmlibIoCreateDeviceSecure(PDRIVER_OBJECT, ULONG, PUNICODE_STRING, DEVICE_TYPE, ULONG, BOOLEAN, PCUNICODE_STRING, LPCGUID, PDEVICE_OBJECT *);
static NTSTATUS probe(PDRIVER_OBJECT d, PCUNICODE_STRING s, ULONG tag) {
    UNICODE_STRING n; PDEVICE_OBJECT dev = NULL; NTSTATUS st;
    RtlInitUnicodeString(&n, tag == 1 ? L"\\Device\\WineRefInvalid" : L"\\Device\\WineRefNull");
    st = WdmlibIoCreateDeviceSecure(d, 0, &n, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, s, NULL, &dev);
    DbgPrint("secure-reference tag=%lu status=%08X device=%p\n", tag, st, dev);
    if (dev) IoDeleteDevice(dev);
    return st;
}
NTSTATUS DriverEntry(PDRIVER_OBJECT d, PUNICODE_STRING p) {
    UNICODE_STRING bad; RtlInitUnicodeString(&bad, L"D:P(A;;INVALID;;;SY)");
    UNREFERENCED_PARAMETER(p); probe(d, &bad, 1); probe(d, NULL, 2); return STATUS_SUCCESS;
}
