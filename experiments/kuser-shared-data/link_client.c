#include <windows.h>
int main(void) {
    HANDLE h = CreateFileW(L"\\\\.\\WineLinkProbe", GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    char b[512];
    DWORD err = GetLastError();
    HANDLE out = CreateFileA("link-client-result.txt", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    DWORD n;
    if (h == INVALID_HANDLE_VALUE) { LPWSTR msg = 0; FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, err, 0, (LPWSTR)&msg, 0, 0); wsprintfA(b, "link_client: CreateFile=FAIL err_dec=%lu err_hex=0x%08lX", err, err); if (msg) LocalFree(msg); }
    else { lstrcpyA(b, "link_client: CreateFile=PASS"); CloseHandle(h); }
    OutputDebugStringA(b); if (out != INVALID_HANDLE_VALUE) { WriteFile(out, b, lstrlenA(b), &n, 0); CloseHandle(out); }
    return h == INVALID_HANDLE_VALUE;
}
