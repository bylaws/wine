#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

typedef UINT64 unixlib_handle_t;

extern NTSTATUS (WINAPI *__wine_unix_call_dispatcher)( unixlib_handle_t, unsigned int, void * );

#ifdef __arm64ec__
NTSTATUS __wine_unix_call_arm64ec( unixlib_handle_t handle, unsigned int code, void *args );
NTSTATUS WINAPI __wine_unix_call( unixlib_handle_t handle, unsigned int code, void *args )
{
    return __wine_unix_call_arm64ec( handle, code, args );
}
#else
NTSTATUS WINAPI __wine_unix_call( unixlib_handle_t handle, unsigned int code, void *args )
{
    return __wine_unix_call_dispatcher( handle, code, args );
}
#endif
