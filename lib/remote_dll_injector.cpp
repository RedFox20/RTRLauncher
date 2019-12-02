#include "remote_dll_injector.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <log.h>
#include <shadowlib.h>
#include <rpp/scope_guard.h>
#include <stdexcept>

using pLoadLibraryA = HMODULE (WINAPI*)(LPCSTR);

struct FILE_INJECT
{
    HMODULE LoadedModule = nullptr;
    pLoadLibraryA  fnLoadLibraryA;
    CHAR           LibName[MAX_PATH];
};
#pragma runtime_checks("", off) // disable any runtime checks (crashtastic)
#pragma strict_gs_check(off) // disable security cookie
static DWORD WINAPI FileLoadDll(FILE_INJECT* injector)
{
    //// We don't want to pass LoadLibraryA as the CreateRemoteThread
    //// startup function - that method is easily detectable.
    //// That's why we use this proxy instead
    injector->LoadedModule = injector->fnLoadLibraryA(injector->LibName);
    if (injector->LoadedModule == nullptr)
        return GetLastError(); // loader failed!
    return 0;
}
#pragma runtime_checks("", restore) // restore any specified runtime checks


static const char secOK[] = "[==] Injector [==]";
static const char secFF[] = "[!!] Injector [!!]";
static bool DebugPrivilegeSet = false;

void remote_dll_injector::enable_debug_privilege()
{
    logsec(secOK, "Enabling Debug Privilege\n");
    if (shadow_enable_debug_privilege(GetCurrentProcess()))
    {
        DebugPrivilegeSet = true;
        logsec(secOK, "CURRENT_PROCESS Debug Privilege SET SUCCESS\n");
    }
    else logsec(secFF, "CURRENT_PROCESS Debug Privilege SET FAILED\n");
}

void remote_dll_injector::inject_dllfile(void* targetProcessHandle, const char* filename)
{
    // Set DEBUG privilege ENABLED for the Current process to get sufficient rights
    if (!DebugPrivilegeSet)
        enable_debug_privilege();

    if (shadow_enable_debug_privilege(targetProcessHandle))
        logsec(secOK, "TARGET_PROCESS Debug Privilege SET SUCCESS\n");
    else
        logsec(secFF, "TARGET_PROCESS Debug Privilege SET FAILED\n");

    auto process = HANDLE(targetProcessHandle);
    // allocate memory for the loader code in the remote process
    //  === loader ===
    //  [ global data ]
    //  [ loader code ]
    DWORD codeSize = 4096; // @todo Calculate code size in a reliable way
    PCHAR loader_data = shadow_valloc(process, nullptr, 4096, PAGE_READWRITE);
    PCHAR loader_code = shadow_valloc(process, nullptr, codeSize, PAGE_EXECUTE_READWRITE);
    scope_guard([&] {
        if (loader_data) shadow_vfree(process, loader_data);
        if (loader_code) shadow_vfree(process, loader_code);
    });
    if (!loader_data) throw std::runtime_error{"Unable to allocate memory for loader DATA in remote process"};
    if (!loader_code) throw std::runtime_error{"Unable to allocate memory for loader CODE in remote process"};

    FILE_INJECT FileInject;
    FileInject.fnLoadLibraryA  = shadow_kernel_getproc<pLoadLibraryA>("LoadLibraryA");
    GetFullPathNameA(filename, sizeof(FileInject.LibName), FileInject.LibName, nullptr);

    logsec(secOK, "Allocated loader DATA: %p  CODE: %p\n", loader_data, loader_code);
    logsec(secOK, "Fullpath: %s\n", FileInject.LibName);

    // Write the loader DATA and CODE to target process
    shadow_vwrite(process, loader_data, &FileInject, sizeof(FileInject));
    shadow_vwrite(process, loader_code, FileLoadDll, codeSize);

    /// @note We never call FreeLibrary, because the remote process will be using it from now on
    HANDLE thread = shadow_create_thread(process, LPTHREAD_START_ROUTINE(loader_code), loader_data);

    // don't use WaitForSingleObject(hThread, INFINITE); here! Will deadlock if client DLL 
    // creates new thread during load, so we use a silly loop instead

    // Wait for loader thread to finish execution and grab the results
    ExitStatus status;
    while ((status = shadow_get_thread_status(thread)))
        Sleep(10); // wait for it...

    // Read back the result data
    logsec(secOK, "Reading back loader DATA\n");
    shadow_vread(process, loader_data, &FileInject, sizeof(FileInject));
    shadow_close_handle(thread);

    if (FileInject.LoadedModule == nullptr) // LoadLibrary failed
        throw std::runtime_error{"Remote LoadLibrary failed!"};
}

