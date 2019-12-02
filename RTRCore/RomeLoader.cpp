#include "RomeLoader.h"
#include <log.h>
#include <rpp/file_io.h>
#include <fmt/format.h>
#include <remote_dll_injector.h>
#include "../GameEngine/GameEngineDllParams.h"
#include <memory_map.h>
#include <shadowlib.h>
#include <aclapi.h> // EXPLICIT_ACCESS
#pragma comment(lib, "advapi32.lib")

namespace core
{
    static const char secOK[] = "[+] RomeLoader [+]";
    static const char secFF[] = "[!] RomeLoader [!]";

    void KillRomeProcesses()
    {
        std::vector<ProcessInfo> processes = shadow_get_processes("RomeTW");
        for (const ProcessInfo& pi : processes)
        {
            log("Killing previous RomeTW process: %s %d\n", pi.name.c_str(), pi.pid);
            HANDLE hnd = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, pi.pid);
            TerminateProcess(hnd, 0);
        }
    }

    template<size_t N> void CopyString(char (&dst)[N], const std::string& src)
    {
        strncpy_s(dst, src.c_str(), src.size());
    }

    void LoadAndInjectRomeProcess(
        const string& executable,
        const string& arguments,
        const string& workingDir,
        const CoreSettings& settings)
    {
        KillRomeProcesses();

        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi = { nullptr, nullptr };

        string cmd = executable + " " + arguments;
        DWORD creationFlags = CREATE_SUSPENDED|CREATE_NEW_CONSOLE;
        if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, 0,
                           creationFlags, nullptr, workingDir.c_str(), &si, &pi))
        {
            throw std::runtime_error{fmt::format("Failed to create process {}: {}", cmd, shadow_getsyserr())};
        }

        // This sends data to GameEngine DllMain
        memory_map map = memory_map::create("RTRGameEngine", 16*1024);
        if (!map) throw std::runtime_error{"Failed to memory map RTRGameEngine DLL parameters"};
        GameEngineDllParams params {};
        CopyString(params.Title, settings.Title);
        CopyString(params.ModName, settings.ModName);
        CopyString(params.Executable, executable);
        CopyString(params.Arguments, arguments);
        map.create_view().write_struct(params);

        log("RTW.MainThreadId: %d\n", pi.dwThreadId);

        try
        {
            // try to inject the DLL and Attach the debugger:
            if (settings.DebugAttach)
            {
                logsec(secOK, "VS JIT Attach\n");
                char jit[MAX_PATH];
                snprintf(jit, sizeof(jit), "vsjitdebugger.exe -p %d", pi.dwProcessId);
                
                logsec(secOK, "Launching %s\n", jit);
                int result = system(jit);
                if (result != S_OK) // nonfatal
                    logsec(secFF, "Debugger Attach Failed: %p\n", result);
            }
            if (settings.Inject)
            {
                if (!rpp::file_exists("GameEngine.dll"))
                    throw std::runtime_error{"Cannot find ./GameEngine.dll"};

                logsec(secOK, "Opening process with PROCESS_ALL_ACCESS privileges\n");
                HANDLE process = pi.hProcess; // shadow_open_process_all_access(pi.dwProcessId);

                logsec(secOK, "Injecting .\\GameEngine.dll...\n");
                remote_dll_injector::inject_dllfile(process, "GameEngine.dll");
            }
            logsec(secOK, "Launching the game!\n\n");
            ResumeThread(pi.hThread); // Launch!!! Only useful if injector didn't resume the main thread
        }
        catch (...)
        {
            TerminateProcess(pi.hProcess, 0);
            logsec(secFF, "Error: Failed to inject GameEngine.dll\n");
            throw;
        }
    }
}
