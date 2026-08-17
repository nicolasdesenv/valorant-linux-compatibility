#include <windows.h>
int main(void) {
 HANDLE tok=0, rtok=0, proc=GetCurrentProcess(), h; DWORD err; char b[256];
 OpenProcessToken(proc,TOKEN_DUPLICATE|TOKEN_QUERY,&tok);
 if(!CreateRestrictedToken(tok,DISABLE_MAX_PRIVILEGE,0,0,0,0,0,0,&rtok)) return 2;
 SetThreadToken(NULL,rtok);
 h=CreateFileW(L"\\\\.\\WineSecureProbe",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL); err=GetLastError();
 wsprintfA(b,"secure_deny: restricted=1 CreateFile=%s err_dec=%lu err_hex=0x%08lX",h==INVALID_HANDLE_VALUE?"FAIL":"PASS",err,err);
 OutputDebugStringA(b); h==INVALID_HANDLE_VALUE?0:CloseHandle(h); RevertToSelf(); CloseHandle(rtok); CloseHandle(tok); return h==INVALID_HANDLE_VALUE?1:0;
}
