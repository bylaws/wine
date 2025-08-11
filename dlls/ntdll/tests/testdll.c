#if 0
#pragma makedep testdll
#endif

#include <windows.h>

BOOL seen_thread_attach;
BOOL seen_thread_detach;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_THREAD_ATTACH:
            seen_thread_attach = TRUE;
            break;
        case DLL_THREAD_DETACH:
            seen_thread_detach = TRUE;
            break;
    }

    return TRUE;
}
