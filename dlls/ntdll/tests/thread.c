/*
 * Unit test suite for ntdll thread functions
 *
 * Copyright 2021 Paul Gofman for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/test.h"

static NTSTATUS (WINAPI *pNtCreateThreadEx)( HANDLE *, ACCESS_MASK, OBJECT_ATTRIBUTES *,
                                             HANDLE, PRTL_THREAD_START_ROUTINE, void *,
                                             ULONG, ULONG_PTR, SIZE_T, SIZE_T, PS_ATTRIBUTE_LIST * );
static NTSTATUS (WINAPI *pNtAllocateVirtualMemoryEx)(HANDLE, PVOID *, SIZE_T *, ULONG, ULONG,
                                                     MEM_EXTENDED_PARAMETER *, ULONG);
static NTSTATUS (WINAPI *pRtlWow64GetProcessMachines)(HANDLE,WORD*,WORD*);

static int * (CDECL *p_errno)(void);

static void init_function_pointers(void)
{
    HMODULE hntdll = GetModuleHandleA( "ntdll.dll" );
#define GET_FUNC(name) p##name = (void *)GetProcAddress( hntdll, #name );
    GET_FUNC( NtCreateThreadEx );
    GET_FUNC( NtAllocateVirtualMemoryEx );
    GET_FUNC( RtlWow64GetProcessMachines );
    GET_FUNC( _errno );
#undef GET_FUNC
}

static void CALLBACK test_NtCreateThreadEx_proc(void *param)
{
}

static void test_dbg_hidden_thread_creation(void)
{
    RTL_USER_PROCESS_PARAMETERS *params;
    PS_CREATE_INFO create_info;
    PS_ATTRIBUTE_LIST ps_attr;
    WCHAR path[MAX_PATH + 4];
    HANDLE process, thread;
    UNICODE_STRING imageW;
    BOOLEAN dbg_hidden;
    NTSTATUS status;

    if (!pNtCreateThreadEx)
    {
        win_skip( "NtCreateThreadEx is not available.\n" );
        return;
    }

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_NtCreateThreadEx_proc,
                                NULL, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    dbg_hidden = 0xcc;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dbg_hidden, sizeof(dbg_hidden), NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    ok( !dbg_hidden, "Got unexpected dbg_hidden %#x.\n", dbg_hidden );

    status = NtResumeThread( thread, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_NtCreateThreadEx_proc,
                                NULL, THREAD_CREATE_FLAGS_CREATE_SUSPENDED | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER,
                                0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    dbg_hidden = 0xcc;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dbg_hidden, sizeof(dbg_hidden), NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    ok( dbg_hidden == 1, "Got unexpected dbg_hidden %#x.\n", dbg_hidden );

    status = NtResumeThread( thread, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );

    lstrcpyW( path, L"\\??\\" );
    GetModuleFileNameW( NULL, path + 4, MAX_PATH );

    RtlInitUnicodeString( &imageW, path );

    memset( &ps_attr, 0, sizeof(ps_attr) );
    ps_attr.Attributes[0].Attribute = PS_ATTRIBUTE_IMAGE_NAME;
    ps_attr.Attributes[0].Size = lstrlenW(path) * sizeof(WCHAR);
    ps_attr.Attributes[0].ValuePtr = path;
    ps_attr.TotalLength = sizeof(ps_attr);

    status = RtlCreateProcessParametersEx( &params, &imageW, NULL, NULL,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL, PROCESS_PARAMS_FLAG_NORMALIZED );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    /* NtCreateUserProcess() may return STATUS_INVALID_PARAMETER with some uninitialized data in create_info. */
    memset( &create_info, 0, sizeof(create_info) );
    create_info.Size = sizeof(create_info);

    status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED
                                  | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status );
    status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    status = NtTerminateProcess( process, 0 );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    CloseHandle( process );
    CloseHandle( thread );
}

struct unique_teb_thread_args
{
    TEB *teb;
    HANDLE running_event;
    HANDLE quit_event;
};

static void CALLBACK test_unique_teb_proc(void *param)
{
    struct unique_teb_thread_args *args = param;
    args->teb = NtCurrentTeb();
    SetEvent( args->running_event );
    WaitForSingleObject( args->quit_event, INFINITE );
}

static void test_unique_teb(void)
{
    HANDLE threads[2], running_events[2];
    struct unique_teb_thread_args args1, args2;
    NTSTATUS status;

    if (!pNtCreateThreadEx)
    {
        win_skip( "NtCreateThreadEx is not available.\n" );
        return;
    }

    args1.running_event = running_events[0] = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( args1.running_event != NULL, "CreateEventW failed %lu.\n", GetLastError() );

    args2.running_event = running_events[1] = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( args2.running_event != NULL, "CreateEventW failed %lu.\n", GetLastError() );

    args1.quit_event = args2.quit_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    ok( args1.quit_event != NULL, "CreateEventW failed %lu.\n", GetLastError() );

    status = pNtCreateThreadEx( &threads[0], THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_unique_teb_proc,
                                &args1, 0, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    status = pNtCreateThreadEx( &threads[1], THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_unique_teb_proc,
                                &args2, 0, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    WaitForMultipleObjects( 2, running_events, TRUE, INFINITE );
    SetEvent( args1.quit_event );

    WaitForMultipleObjects( 2, threads, TRUE, INFINITE );
    CloseHandle( threads[0] );
    CloseHandle( threads[1] );
    CloseHandle( args1.running_event );
    CloseHandle( args2.running_event );
    CloseHandle( args1.quit_event );

    ok( NtCurrentTeb() != args1.teb, "Multiple threads have TEB %p.\n", args1.teb );
    ok( NtCurrentTeb() != args2.teb, "Multiple threads have TEB %p.\n", args2.teb );
    ok( args1.teb != args2.teb, "Multiple threads have TEB %p.\n", args1.teb );
}

static void test_errno(void)
{
    int val;

    if (!p_errno)
    {
        win_skip( "_errno not available\n" );
        return;
    }
    ok( NtCurrentTeb()->Peb->TlsBitmap->Buffer[0] & (1 << 16), "TLS entry 16 not allocated\n" );
    *p_errno() = 0xdead;
    val = PtrToLong( TlsGetValue( 16 ));
    ok( val == 0xdead, "wrong value %x\n", val );
    *p_errno() = 0xbeef;
    val = PtrToLong( TlsGetValue( 16 ));
    ok( val == 0xbeef, "wrong value %x\n", val );
}

static void test_NtCreateUserProcess(void)
{
    RTL_USER_PROCESS_PARAMETERS *params;
    PS_CREATE_INFO create_info;
    PS_ATTRIBUTE_LIST ps_attr;
    WCHAR path[MAX_PATH + 4];
    HANDLE process, thread;
    UNICODE_STRING imageW;
    NTSTATUS status;

    lstrcpyW( path, L"\\??\\" );
    GetModuleFileNameW( NULL, path + 4, MAX_PATH );

    RtlInitUnicodeString( &imageW, path );

    memset( &ps_attr, 0, sizeof(ps_attr) );
    ps_attr.Attributes[0].Attribute = PS_ATTRIBUTE_IMAGE_NAME;
    ps_attr.Attributes[0].Size = lstrlenW(path) * sizeof(WCHAR);
    ps_attr.Attributes[0].ValuePtr = path;
    ps_attr.TotalLength = sizeof(ps_attr);

    status = RtlCreateProcessParametersEx( &params, &imageW, NULL, NULL,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL, PROCESS_PARAMS_FLAG_NORMALIZED );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    memset( &create_info, 0, sizeof(create_info) );
    create_info.Size = sizeof(create_info);
    status = NtCreateUserProcess( &process, &thread, PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, SYNCHRONIZE,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    status = NtTerminateProcess( process, 0 );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    CloseHandle( process );
    CloseHandle( thread );
}

static void extract_resource(const char *name, const char *type, const char *path)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileA(path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", path, GetLastError());

    res = FindResourceA(NULL, name, type);
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

BOOL seen_tls_thread_attach;
BOOL seen_tls_thread_detach;

void WINAPI tls_callback(HANDLE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_THREAD_ATTACH:
            seen_tls_thread_attach = TRUE;
            break;
        case DLL_THREAD_DETACH:
            seen_tls_thread_detach = TRUE;
            break;
    }
}

#define _CRTALLOC(x) __attribute__ ((section (x), used))

__attribute__((used)) ULONG _tls_index = 0;

_CRTALLOC(".tls") char *_tls_start = NULL;
_CRTALLOC(".tls$ZZZ") char *_tls_end = NULL;

_CRTALLOC(".CRT$XLA") PIMAGE_TLS_CALLBACK __xl_a = 0;

_CRTALLOC(".CRT$XLC") PIMAGE_TLS_CALLBACK __xl_c = tls_callback;
_CRTALLOC(".CRT$XLZ") PIMAGE_TLS_CALLBACK __xl_z = 0;

__attribute__((used)) const IMAGE_TLS_DIRECTORY _tls_used = {
  (ULONG_PTR) &_tls_start, (ULONG_PTR) &_tls_end,
  (ULONG_PTR) &_tls_index, (ULONG_PTR) (&__xl_a+1),
  (ULONG) 0, (ULONG) 0
};

struct skip_thread_attach_args
{
    BOOL teb_flag;
    PVOID teb_tls_pointer;
    PVOID teb_fls_slots;
};

static void CALLBACK test_skip_thread_attach_proc(void *param)
{
    struct skip_thread_attach_args *args = param;
    args->teb_flag = NtCurrentTeb()->SkipThreadAttach;
    args->teb_tls_pointer = NtCurrentTeb()->ThreadLocalStoragePointer;
    args->teb_fls_slots = NtCurrentTeb()->FlsSlots;

}

static void test_skip_thread_attach(void)
{
    BOOL *seen_thread_attach, *seen_thread_detach;
    struct skip_thread_attach_args args;
    HANDLE thread;
    NTSTATUS status;
    char path_dll_local[MAX_PATH + 11];
    char path_tmp[MAX_PATH];
    HMODULE module = NULL;

    if (!pNtCreateThreadEx)
    {
        win_skip( "NtCreateThreadEx is not available.\n" );
        return;
    }


    GetTempPathA(sizeof(path_tmp), path_tmp);

    sprintf(path_dll_local, "%s%s", path_tmp, "testdll.dll");
    extract_resource("testdll.dll", "TESTDLL", path_dll_local);

    printf("%s\n",path_dll_local);
    module = LoadLibraryA(path_dll_local);
    if (!module) {
        trace("Could not load testdll.\n");
        return;
    }
    seen_thread_attach = (BOOL *)GetProcAddress(module, "seen_thread_attach");
    seen_thread_detach = (BOOL *)GetProcAddress(module, "seen_thread_detach");
    seen_tls_thread_attach = FALSE;
    seen_tls_thread_detach = FALSE;

    ok( !*seen_thread_attach, "Unexpected\n" );
    ok( !*seen_thread_detach, "Unexpected\n" );


    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_skip_thread_attach_proc,
                                &args, THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    WaitForSingleObject( thread, INFINITE );

    CloseHandle( thread );

    ok( !*seen_thread_attach, "Unexpected\n" );
    ok( !*seen_thread_detach, "Unexpected\n" );
    ok( !seen_tls_thread_attach, "Unexpected\n" );
    ok( !seen_tls_thread_attach, "Unexpected\n" );
    ok( args.teb_flag, "Unexpected\n" );
    ok( !args.teb_tls_pointer, "Unexpected\n" );
    ok( !args.teb_fls_slots, "Unexpected\n" );

    FreeLibrary(module);
    DeleteFileA(path_dll_local);
}

struct test_skip_load_init_args
{
    USHORT teb_same_teb_flags;
};

static ULONG patched_code[] =
{
    0xd282fdc2, /* mov x2, #0x17ee */
    0x78626a41, /* ldrh w1, [x18, x2]   (NtCurrentTeb()->SameTebFlags)*/
    0x79000001, /* strh w1, [x0]    (args->teb_same_teb_flags)*/
    0xd65f03c0, /* ret */
};

static void test_arm64_skip_loader_init(void)
{
    struct test_skip_load_init_args args;
    HANDLE thread;
    NTSTATUS status;
    void *code_mem = NULL;
#ifdef __x86_64__
    MEM_EXTENDED_PARAMETER param = { 0 };
    SIZE_T code_size = 0x10000;

    param.Type = MemExtendedParameterAttributeFlags;
    param.ULong64 = MEM_EXTENDED_PARAMETER_EC_CODE;
    if (!pNtAllocateVirtualMemoryEx ||
        pNtAllocateVirtualMemoryEx( GetCurrentProcess(), &code_mem, &code_size, MEM_RESERVE | MEM_COMMIT,
                                    PAGE_EXECUTE_READWRITE, &param, 1 ))
    {
        trace("NtAllocateVirtualMemoryEx failed\n");
        return;

    }
#else
    code_mem = VirtualAlloc(NULL, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!code_mem) {
        trace("VirtualAlloc failed\n");
        return;
    }
#endif
    if (!pNtCreateThreadEx)
    {
        win_skip( "NtCreateThreadEx is not available.\n" );
        return;
    }

    memcpy(code_mem, patched_code, sizeof(patched_code));

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), (PRTL_THREAD_START_ROUTINE)code_mem,
                                &args, THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH | THREAD_CREATE_FLAGS_SKIP_LOADER_INIT, 0, 0, 0, NULL );

    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    WaitForSingleObject( thread, INFINITE );

    ok( (args.teb_same_teb_flags & 0x4008) == 0x4008, "wrong value %x\n", args.teb_same_teb_flags );

    CloseHandle( thread );
}

START_TEST(thread)
{
    init_function_pointers();

    test_dbg_hidden_thread_creation();
    test_unique_teb();
    test_errno();
    test_NtCreateUserProcess();
    test_skip_thread_attach();

    if (pRtlWow64GetProcessMachines)
    {
        USHORT current, native;
        NTSTATUS status = pRtlWow64GetProcessMachines( GetCurrentProcess(), &current, &native );
        if (!status && native == IMAGE_FILE_MACHINE_ARM64)
        {
            trace( "Running arm64 tests.\n" );
            test_arm64_skip_loader_init();
        }
    }
}
