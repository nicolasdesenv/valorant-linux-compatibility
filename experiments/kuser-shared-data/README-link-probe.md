# IoCreateSymbolicLink probe

Independent driver using `IoCreateDevice` and `IoCreateSymbolicLink` with
`\\Device\\WineLinkProbe` and `\\DosDevices\\WineLinkProbe`. The Win32 client
requests `CreateFileW(L"\\\\.\\WineLinkProbe", ...)` in a separate prefix.

Wine calls `NtCreateSymbolicLinkObject` with the supplied NT name. Win32
`\\.\\foo` is translated by ntdll to `\\??\\foo`; Windows normally aliases
`\\DosDevices` to this DOS namespace. SCM load, device creation and symbolic
link creation returned success. The client result was not captured reliably
from the combined command lifetime, so CreateFile/dispatch remain NOT TESTED.
