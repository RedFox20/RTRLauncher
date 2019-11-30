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

static HANDLE APIThread = NULL;


DWORD WINAPI APIEntryPoint(void* arg)
{
	HWND win = NULL;
	log("[==]  StartMonitorThread  [==]\n");
	log("[==] Waiting for UILoaded [==]\n");
	while (shadow_get_threadcount() < 14)
	{
		if (!win && (win = FindWindowA("Rome: Total War - Alexander", NULL)))
		{
			log("[==]    Set window text   [==]\n");
			SetWindowTextA(win, "Rome: Total War");
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


BOOL ReinitMainThread(void* newEntryPoint)
{
	DWORD flags = THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION;
	vector<DWORD> threadIds;
	shadow_get_threads(threadIds);

	if (HANDLE mainThread = shadow_open_thread(threadIds[0], flags))
	{
		shadow_set_threadip(mainThread, newEntryPoint);
		shadow_resume_thread(mainThread); // Launch!!!
		shadow_close_handle(mainThread);

		APIThread = shadow_create_thread(APIEntryPoint, 0);
		return TRUE;
	}
	return FALSE;
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
			logger_init("RTR\\rome.log");
			console_size(90, 1000, 90, 40);

			char name[MAX_PATH] = "";
			BOOL isMemInject = !GetModuleFileNameA(hinstDLL, name, MAX_PATH);
			if (isMemInject) log("[*!*]       Memory Inject        [*!*]\n");
			else             log("[*!*]        File Inject         [*!*]\n");
			log("[*!*] Welcome to GameEngine %.2f [*!*]\n", APIVersion);
			
			log("[*!*]      Injecting Patch       [*!*]\n");
			PMEMORYMODULE module = NULL;
			{
				memory_map mmap = memory_map::open("RTRGameEngine");
				map_view view = mmap.create_view();

				// unmap the old exe
				log("DLL Location 0x%p\n", hinstDLL);
				consolef("======================================\n");
				shadow_vprint(PVOID(0x00400000), 0x0269ea9e);
				consolef("======================================\n");
				consolef("Scanning for modules in range:\n");
				shadow_scanmodules(PVOID(0x00400000), 0x0269ea9e);
				consolef("======================================\n");
				consolef("Unmapping range 0x%p - 0x%p\n", 0x00400000, 0x00400000 + 0x0269ea9e);
				shadow_unmap(PVOID(0x00400000), 0x0269ea9e);
				consolef("======================================\n");
				shadow_vprint(PVOID(0x00400000), 0x0269ea9e);
				log("======================================\n");
				if (module = MemoryLoadLibrary(view, PVOID(0x00400000)))
				{
					log("Module info at 0x%p\n", module);
					log("Module data at 0x%p\n", module->codeBase);
				}
				else
				{
					log("Module load failed\n");
				}
				shadow_vprint((void*)0x00400000, 0x0269ea9e);
			}

			log("\n[*!*]     Initializing Game     [*!*]\n");
			Game.Initialize(PVOID(0x00400000)); // initialize game engine and patch rome
			ReinitMainThread(module->exeEntry);

			//// @warning We must exit the loader thread, since we sometimes overwrite the next DLL
			//// @warning in memory, leading to NT loader segfault when it calls the next DllMain
			//ExitThread(TRUE); 
			return TRUE; // success
		}
		case DLL_THREAD_ATTACH: // a new thread is created in this process
			break;
		case DLL_THREAD_DETACH: // a thread dies
			break;
	}
	return TRUE; // success
}