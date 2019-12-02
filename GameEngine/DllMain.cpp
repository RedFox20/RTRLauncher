#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tlhelp32.h>
#include <log.h>
#include "GameEngine.h"
#include "GameEngineDllParams.h"
#include <shadowlib.h>
#include <memory_map.h>

constexpr float APIVersion = 1.31f;

void onProcessAttach(HINSTANCE hinstDLL)
{
    console_size(90, 1000, 90, 40);
    logger_init("RTR\\rome.log");

    char name[MAX_PATH] = "";
    BOOL isMemInject = !GetModuleFileNameA(hinstDLL, name, MAX_PATH);
    if (isMemInject) log("[*!*]        Memory Inject         [*!*]\n");
    else             log("[*!*]         File Inject          [*!*]\n");
    log(  "[*!*] Welcome to GameEngine %.2f [*!*]\n", APIVersion);

    if (memory_map mmap = memory_map::open("RTRGameEngine"))
    {
        map_view view = mmap.create_view();
        auto* params = view.get<GameEngineDllParams>();
    }

    log(  "[*!*]     Initializing Patch     [*!*]\n");
    Game.Initialize(PVOID(0x00400000)); // initialize game engine and patch rome

    log("\n[*!*]       Launching Game       [*!*]\n");
    Game.LaunchRome();
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
                logflush();
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