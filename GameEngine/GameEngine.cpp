#include "GameEngine.h"
#include "RomePatcher.h"
#include <shadowlib.h>
#include <log.h>
#include "RTW/RomeAPI.h"
#include <rpp/stack_trace.h>

/*extern*/ GameEngine Game;

void GameEngine::Initialize(void* exeModule)
{
    rpp::register_segfault_tracer(); // WARNING: Don't use `noexcept` with this!

    Version = RunPatcher(Game, exeModule);
    RTW::Initialize(Version);
}

static DWORD WINAPI APIEntryPoint(void* arg)
{
    GameEngine& game = *static_cast<GameEngine*>(arg);
    try
    {
        game.MonitorThread();
    }
    catch (const std::exception& e)
    {
        log("MonitorThread crashed: %s\n", e.what());
        logflush();
    }
    return 0;
}

void GameEngine::LaunchRome()
{
    log("Reinitializing main thread\n");
    DWORD flags = THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION;
    std::vector<DWORD> threadIds = shadow_get_threads();

    if (HANDLE mainThread = shadow_open_thread(threadIds[0], flags))
    {
        shadow_resume_thread(mainThread); // Launch!!!
        shadow_close_handle(mainThread);
        APIThread = shadow_create_thread(APIEntryPoint, this);
    }
}

void GameEngine::MonitorThread()
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
    consolef("RTW.UnitScale = %.2f\n", RTW::UnitScale());

    while (true)
    {
        Sleep(5000);
        //RTW::UnitScale(8.0f);
        consolef("RTW.UnitScale = %.2f\n", RTW::UnitScale());
    }
}
