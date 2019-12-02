#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tlhelp32.h>
#include <log.h>
#include <algorithm>
#include "GameEngine.h"
#include "RomeAPI.h"
#include "MemoryModule.h"
#include <memory_map.h>
#include <shadowlib.h>
using namespace std;


#define APIVersion 1.31f

static HANDLE APIThread = nullptr;


DWORD WINAPI APIEntryPoint(void* arg)
{
    HWND win = nullptr;
    log("[==]  StartMonitorThread  [==]\n");
    log("[==] Waiting for UILoaded [==]\n");
    while (shadow_get_threadcount() < 14)
    {
        if (!win && (win = FindWindowA("Rome: Total War - Alexander", nullptr)))
        {
            log("[==]    Set window text   [==]\n");
            SetWindowTextA(win, "Rome: Total War (RTR GameEngine Attached)");
        }
        Sleep(50);
    }

    log("[==]      UI Loaded       [==]\n");

    consolef("NumThreads %d\n", shadow_get_threadcount());
    RTW::UnitScale(8.0f);
    consolef("RTW.UnitScale = %.2f\n", RTW::UnitScale());

    while (true)
    {
        Sleep(1000);
        //consolef("RTW.UnitScale = %.2f\n", RTW::UnitScale());
    }
    return 0;
}


BOOL ReinitMainThread(ExeEntryProc newEntryPoint = nullptr)
{
    consolef("Reinitializing main thread\n");
    DWORD flags = THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION;
    vector<DWORD> threadIds = shadow_get_threads();

    if (HANDLE mainThread = shadow_open_thread(threadIds[0], flags))
    {
        if (newEntryPoint)
        {
            shadow_set_threadip(mainThread, newEntryPoint);
        }
        shadow_resume_thread(mainThread); // Launch!!!
        shadow_close_handle(mainThread);

        APIThread = shadow_create_thread(APIEntryPoint, nullptr);
        return TRUE;
    }
    return FALSE;
}

void onProcessAttach(HINSTANCE hinstDLL)
{
    console_size(90, 1000, 90, 40);
    logger_init("RTR\\rome.log");

    char name[MAX_PATH] = "";
    BOOL isMemInject = !GetModuleFileNameA(hinstDLL, name, MAX_PATH);
    if (isMemInject) log("[*!*]       Memory Inject        [*!*]\n");
    else             log("[*!*]        File Inject         [*!*]\n");
    log("[*!*] Welcome to GameEngine %.2f [*!*]\n", APIVersion);
    
    log("[*!*]      Injecting Patch       [*!*]\n");

    ExeEntryProc entryProc = nullptr;
    //if (memory_map mmap = memory_map::open("RTRGameEngine"))
    //{
    //    map_view view = mmap.create_view();

    //    // unmap the old exe
    //    log("DLL Location 0x%p\n", hinstDLL);
    //    consolef("======================================\n");
    //    shadow_vprint(PVOID(0x00400000), 0x0269ea9e);
    //    consolef("======================================\n");
    //    consolef("Scanning for modules in range:\n");
    //    shadow_scanmodules(PVOID(0x00400000), 0x0269ea9e);
    //    consolef("======================================\n");
    //    consolef("Unmapping range 0x%p - 0x%p\n", 0x00400000, 0x00400000 + 0x0269ea9e);
    //    shadow_unmap(PVOID(0x00400000), 0x0269ea9e);
    //    consolef("======================================\n");
    //    shadow_vprint(PVOID(0x00400000), 0x0269ea9e);
    //    log("======================================\n");
    //    if (PMEMORYMODULE module = MemoryLoadLibrary(view, PVOID(0x00400000)))
    //    {
    //        log("Module info at 0x%p\n", module);
    //        log("Module data at 0x%p\n", module->codeBase);
    //        entryProc = module->exeEntry;
    //    }
    //    else
    //    {
    //        log("Module load failed\n");
    //    }
    //    shadow_vprint((void*)0x00400000, 0x0269ea9e);
    //}

    log("\n[*!*]     Initializing Game     [*!*]\n");
    //Game.Initialize(PVOID(0x00400000)); // initialize game engine and patch rome
    //ReinitMainThread(entryProc);
}

/**
 * @note DllMain is only called once -- by the DLL injector / loader
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID reserved)
{
    /**
     * @warning During memory DLL injection, only DLL_PROCESS_ATTACH is ever invoked
     * @warning CODE OUTSIDE DLL_PROCESS_ATTACH IS NEVER EXECUTED
     */
    switch (fdwReason)
    {
        case DLL_PROCESS_DETACH: // we don't have anything to un-init
            break;
        case DLL_PROCESS_ATTACH:
        {
            try
            {
                onProcessAttach(hinstDLL);
                //// @warning We must exit the loader thread, since we sometimes overwrite the next DLL
                //// @warning in memory, leading to NT loader segfault when it calls the next DllMain
                //ExitThread(TRUE); 
                return TRUE; // success
            }
            catch (const std::exception& e)
            {
                log("DLL_PROCESS_ATTACH failed: %s\n", e.what());
                return FALSE; // :(
            }
        }
        case DLL_THREAD_ATTACH: // a new thread is created in this process
            break;
        case DLL_THREAD_DETACH: // a thread dies
            break;
    }
    return TRUE; // success
}