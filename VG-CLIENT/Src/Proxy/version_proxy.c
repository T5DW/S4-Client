#include "../Client/logger.hpp"
#include "../common.hpp"
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include "../Client/runtime_console.hpp"

#ifdef S4_WITH_COMPONENTS
extern int S4ClientStartComponents(void);
extern void S4ClientStopComponents(void);
#endif

static INIT_ONCE provider_once = INIT_ONCE_STATIC_INIT;
static INIT_ONCE client_once = INIT_ONCE_STATIC_INIT;
static HMODULE provider;
static HANDLE client_thread;
typedef DWORD (WINAPI *GetFileVersionInfoSizeAFn)(LPCSTR, LPDWORD);
static GetFileVersionInfoSizeAFn system_GetFileVersionInfoSizeA;
typedef BOOL (WINAPI *GetFileVersionInfoAFn)(LPCSTR, DWORD, DWORD, LPVOID);
static GetFileVersionInfoAFn system_GetFileVersionInfoA;
typedef DWORD (WINAPI *GetFileVersionInfoSizeExAFn)(DWORD, LPCSTR, LPDWORD);
static GetFileVersionInfoSizeExAFn system_GetFileVersionInfoSizeExA;
typedef BOOL (WINAPI *GetFileVersionInfoExAFn)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
static GetFileVersionInfoExAFn system_GetFileVersionInfoExA;
typedef DWORD (WINAPI *VerFindFileAFn)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, PUINT, LPSTR, PUINT);
static VerFindFileAFn system_VerFindFileA;
typedef DWORD (WINAPI *VerInstallFileAFn)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, PUINT);
static VerInstallFileAFn system_VerInstallFileA;
typedef DWORD (WINAPI *VerLanguageNameAFn)(DWORD, LPSTR, DWORD);
static VerLanguageNameAFn system_VerLanguageNameA;
typedef BOOL (WINAPI *VerQueryValueAFn)(LPCVOID, LPCSTR, LPVOID *, PUINT);
static VerQueryValueAFn system_VerQueryValueA;
typedef DWORD (WINAPI *GetFileVersionInfoSizeWFn)(LPCWSTR, LPDWORD);
static GetFileVersionInfoSizeWFn system_GetFileVersionInfoSizeW;
typedef BOOL (WINAPI *GetFileVersionInfoWFn)(LPCWSTR, DWORD, DWORD, LPVOID);
static GetFileVersionInfoWFn system_GetFileVersionInfoW;
typedef DWORD (WINAPI *GetFileVersionInfoSizeExWFn)(DWORD, LPCWSTR, LPDWORD);
static GetFileVersionInfoSizeExWFn system_GetFileVersionInfoSizeExW;
typedef BOOL (WINAPI *GetFileVersionInfoExWFn)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
static GetFileVersionInfoExWFn system_GetFileVersionInfoExW;
typedef DWORD (WINAPI *VerFindFileWFn)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
static VerFindFileWFn system_VerFindFileW;
typedef DWORD (WINAPI *VerInstallFileWFn)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT);
static VerInstallFileWFn system_VerInstallFileW;
typedef DWORD (WINAPI *VerLanguageNameWFn)(DWORD, LPWSTR, DWORD);
static VerLanguageNameWFn system_VerLanguageNameW;
typedef BOOL (WINAPI *VerQueryValueWFn)(LPCVOID, LPCWSTR, LPVOID *, PUINT);
static VerQueryValueWFn system_VerQueryValueW;

static BOOL CALLBACK LoadSystemVersion(PINIT_ONCE once, PVOID parameter, PVOID *context)
{
    wchar_t path[32768];
    const wchar_t suffix[] = L"\\version.dll";
    UINT length;
    (void)once; (void)parameter; (void)context;
    length = GetSystemDirectoryW(path, ARRAYSIZE(path));
    if (!length || length >= ARRAYSIZE(path) ||
        length + ARRAYSIZE(suffix) > ARRAYSIZE(path)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    memcpy(path + length, suffix, sizeof(suffix));
     
    provider = LoadLibraryExW(path, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!provider) return FALSE;
    system_GetFileVersionInfoSizeA = (GetFileVersionInfoSizeAFn)GetProcAddress(provider, "GetFileVersionInfoSizeA");
    if (!system_GetFileVersionInfoSizeA) goto missing_export;
    system_GetFileVersionInfoA = (GetFileVersionInfoAFn)GetProcAddress(provider, "GetFileVersionInfoA");
    if (!system_GetFileVersionInfoA) goto missing_export;
    system_GetFileVersionInfoSizeExA = (GetFileVersionInfoSizeExAFn)GetProcAddress(provider, "GetFileVersionInfoSizeExA");
    if (!system_GetFileVersionInfoSizeExA) goto missing_export;
    system_GetFileVersionInfoExA = (GetFileVersionInfoExAFn)GetProcAddress(provider, "GetFileVersionInfoExA");
    if (!system_GetFileVersionInfoExA) goto missing_export;
    system_VerFindFileA = (VerFindFileAFn)GetProcAddress(provider, "VerFindFileA");
    if (!system_VerFindFileA) goto missing_export;
    system_VerInstallFileA = (VerInstallFileAFn)GetProcAddress(provider, "VerInstallFileA");
    if (!system_VerInstallFileA) goto missing_export;
    system_VerLanguageNameA = (VerLanguageNameAFn)GetProcAddress(provider, "VerLanguageNameA");
    if (!system_VerLanguageNameA) goto missing_export;
    system_VerQueryValueA = (VerQueryValueAFn)GetProcAddress(provider, "VerQueryValueA");
    if (!system_VerQueryValueA) goto missing_export;
    system_GetFileVersionInfoSizeW = (GetFileVersionInfoSizeWFn)GetProcAddress(provider, "GetFileVersionInfoSizeW");
    if (!system_GetFileVersionInfoSizeW) goto missing_export;
    system_GetFileVersionInfoW = (GetFileVersionInfoWFn)GetProcAddress(provider, "GetFileVersionInfoW");
    if (!system_GetFileVersionInfoW) goto missing_export;
    system_GetFileVersionInfoSizeExW = (GetFileVersionInfoSizeExWFn)GetProcAddress(provider, "GetFileVersionInfoSizeExW");
    if (!system_GetFileVersionInfoSizeExW) goto missing_export;
    system_GetFileVersionInfoExW = (GetFileVersionInfoExWFn)GetProcAddress(provider, "GetFileVersionInfoExW");
    if (!system_GetFileVersionInfoExW) goto missing_export;
    system_VerFindFileW = (VerFindFileWFn)GetProcAddress(provider, "VerFindFileW");
    if (!system_VerFindFileW) goto missing_export;
    system_VerInstallFileW = (VerInstallFileWFn)GetProcAddress(provider, "VerInstallFileW");
    if (!system_VerInstallFileW) goto missing_export;
    system_VerLanguageNameW = (VerLanguageNameWFn)GetProcAddress(provider, "VerLanguageNameW");
    if (!system_VerLanguageNameW) goto missing_export;
    system_VerQueryValueW = (VerQueryValueWFn)GetProcAddress(provider, "VerQueryValueW");
    if (!system_VerQueryValueW) goto missing_export;
     
    return TRUE;
missing_export:
    FreeLibrary(provider);
    provider = NULL;
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static DWORD WINAPI StartClient(PVOID parameter)
{
    DWORD result = 0;
    if (!S4ClientOpenConsole()) {
        S4Log("Console initialization failed.\n");
        result = 1;
    } else {
         
         
        S4Log("Starting hooks and automatic auth in 20 seconds...\n");
        fflush(stdout);
        Sleep(20000);
#ifdef S4_WITH_COMPONENTS
        if (!S4ClientStartComponents()) {
            S4Log("Component initialization failed.\n");
            result = 2;
        }
#endif
    }
     
    FreeLibraryAndExitThread((HMODULE)parameter, result);
    return result;
}

static BOOL CALLBACK RequestClient(PINIT_ONCE once, PVOID parameter, PVOID *context)
{
    HMODULE self = NULL;
    (void)once; (void)parameter; (void)context;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           (LPCWSTR)(ULONG_PTR)&StartClient, &self)) return FALSE;
    client_thread = CreateThread(NULL, 0, StartClient, self, 0, NULL);
    if (!client_thread) { FreeLibrary(self); return FALSE; }
    return TRUE;
}

static BOOL EnsureForwarding(void)
{
    if (!InitOnceExecuteOnce(&provider_once, LoadSystemVersion, NULL, NULL)) return FALSE;
     
    {
        DWORD saved_error = GetLastError();
        (void)InitOnceExecuteOnce(&client_once, RequestClient, NULL, NULL);
        SetLastError(saved_error);
    }
    return TRUE;
}

 
BOOL WINAPI VersionProxyInitialize(FARPROC size, FARPROC info, FARPROC query)
{
    if (!size || !info || !query || !EnsureForwarding()) return FALSE;
    return size == (FARPROC)system_GetFileVersionInfoSizeW &&
           info == (FARPROC)system_GetFileVersionInfoW &&
           query == (FARPROC)system_VerQueryValueW;
}

void WINAPI VersionProxyShutdown(void)
{
    if (client_thread) {
        WaitForSingleObject(client_thread, INFINITE);
        CloseHandle(client_thread);
        client_thread = NULL;
    }
#ifdef S4_WITH_COMPONENTS
    S4ClientStopComponents();
#else
    S4ClientCloseConsole();
#endif
}

DWORD WINAPI S4_GetFileVersionInfoSizeA(LPCSTR filename, LPDWORD handle)
{
    if (!EnsureForwarding()) return 0;
    return system_GetFileVersionInfoSizeA(filename, handle);
}

BOOL WINAPI S4_GetFileVersionInfoA(LPCSTR filename, DWORD handle, DWORD length, LPVOID data)
{
    if (!EnsureForwarding()) return 0;
    return system_GetFileVersionInfoA(filename, handle, length, data);
}

DWORD WINAPI S4_GetFileVersionInfoSizeExA(DWORD flags, LPCSTR filename, LPDWORD handle)
{
    if (!EnsureForwarding()) return 0;
    return system_GetFileVersionInfoSizeExA(flags, filename, handle);
}

BOOL WINAPI S4_GetFileVersionInfoExA(DWORD flags, LPCSTR filename, DWORD handle, DWORD length, LPVOID data)
{
    if (!EnsureForwarding()) return 0;
    return system_GetFileVersionInfoExA(flags, filename, handle, length, data);
}

DWORD WINAPI S4_VerFindFileA(DWORD flags, LPSTR filename, LPSTR windows_dir, LPSTR app_dir, LPSTR current_dir, PUINT current_length, LPSTR destination_dir, PUINT destination_length)
{
    if (!EnsureForwarding()) return 0;
    return system_VerFindFileA(flags, filename, windows_dir, app_dir, current_dir, current_length, destination_dir, destination_length);
}

DWORD WINAPI S4_VerInstallFileA(DWORD flags, LPSTR source_file, LPSTR destination_file, LPSTR source_dir, LPSTR destination_dir, LPSTR current_dir, LPSTR temp_file, PUINT temp_length)
{
    if (!EnsureForwarding()) return 0;
    return system_VerInstallFileA(flags, source_file, destination_file, source_dir, destination_dir, current_dir, temp_file, temp_length);
}

DWORD WINAPI S4_VerLanguageNameA(DWORD language, LPSTR buffer, DWORD length)
{
    if (!EnsureForwarding()) return 0;
    return system_VerLanguageNameA(language, buffer, length);
}

BOOL WINAPI S4_VerQueryValueA(LPCVOID block, LPCSTR subblock, LPVOID * buffer, PUINT length)
{
    if (!EnsureForwarding()) return 0;
    return system_VerQueryValueA(block, subblock, buffer, length);
}

DWORD WINAPI S4_GetFileVersionInfoSizeW(LPCWSTR filename, LPDWORD handle)
{
    if (!EnsureForwarding()) return 0;
    return system_GetFileVersionInfoSizeW(filename, handle);
}

BOOL WINAPI S4_GetFileVersionInfoW(LPCWSTR filename, DWORD handle, DWORD length, LPVOID data)
{
    if (!EnsureForwarding()) return 0;
    return system_GetFileVersionInfoW(filename, handle, length, data);
}

DWORD WINAPI S4_GetFileVersionInfoSizeExW(DWORD flags, LPCWSTR filename, LPDWORD handle)
{
    if (!EnsureForwarding()) return 0;
    return system_GetFileVersionInfoSizeExW(flags, filename, handle);
}

BOOL WINAPI S4_GetFileVersionInfoExW(DWORD flags, LPCWSTR filename, DWORD handle, DWORD length, LPVOID data)
{
    if (!EnsureForwarding()) return 0;
    return system_GetFileVersionInfoExW(flags, filename, handle, length, data);
}

DWORD WINAPI S4_VerFindFileW(DWORD flags, LPWSTR filename, LPWSTR windows_dir, LPWSTR app_dir, LPWSTR current_dir, PUINT current_length, LPWSTR destination_dir, PUINT destination_length)
{
    if (!EnsureForwarding()) return 0;
    return system_VerFindFileW(flags, filename, windows_dir, app_dir, current_dir, current_length, destination_dir, destination_length);
}

DWORD WINAPI S4_VerInstallFileW(DWORD flags, LPWSTR source_file, LPWSTR destination_file, LPWSTR source_dir, LPWSTR destination_dir, LPWSTR current_dir, LPWSTR temp_file, PUINT temp_length)
{
    if (!EnsureForwarding()) return 0;
    return system_VerInstallFileW(flags, source_file, destination_file, source_dir, destination_dir, current_dir, temp_file, temp_length);
}

DWORD WINAPI S4_VerLanguageNameW(DWORD language, LPWSTR buffer, DWORD length)
{
    if (!EnsureForwarding()) return 0;
    return system_VerLanguageNameW(language, buffer, length);
}

BOOL WINAPI S4_VerQueryValueW(LPCVOID block, LPCWSTR subblock, LPVOID * buffer, PUINT length)
{
    if (!EnsureForwarding()) return 0;
    return system_VerQueryValueW(block, subblock, buffer, length);
}

