#include <windows.h>

int main(void)
{
    HANDLE handle = CreateFileW(L"\\\\.\\WineSecureProbe", GENERIC_READ, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        char buffer[80];
        wsprintfA(buffer, "secure_client: open=FAIL win32=%lu", GetLastError());
        OutputDebugStringA(buffer);
        return 1;
    }
    OutputDebugStringA("secure_client: open=PASS");
    CloseHandle(handle);
    return 0;
}
