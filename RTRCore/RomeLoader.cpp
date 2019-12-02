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


static const char secOK[] = "[+] RomeLoader [+]";
static const char secFF[] = "[!] RomeLoader [!]";

// @note This leaks the handles, because I don't care :D
bool CreateSecurityAttributes(SECURITY_ATTRIBUTES& sa)
{
    EXPLICIT_ACCESS ea[2]; ZeroMemory(&ea, sizeof(ea));

    // Create a well-known SID for the Everyone group.
    PSID pEveryoneSID = nullptr;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &pEveryoneSID))
        return false;

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone full access 
    ea[0].grfAccessPermissions = KEY_ALL_ACCESS;
    ea[0].grfAccessMode  = SET_ACCESS;
    ea[0].grfInheritance = NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName   = LPTSTR(pEveryoneSID);

    // Create a SID for the BUILTIN\Administrators group.
    PSID pAdminSID = nullptr;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSID)) 
        return false;

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the Administrators group full access
    ea[1].grfAccessPermissions = KEY_ALL_ACCESS;
    ea[1].grfAccessMode  = SET_ACCESS;
    ea[1].grfInheritance = NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName   = LPTSTR(pAdminSID);
    
    // Create a new ACL that contains the new ACEs.
    PACL pACL = nullptr;
    if (ERROR_SUCCESS != SetEntriesInAcl(2, ea, nullptr, &pACL)) 
        return false;

    // Initialize a security descriptor.
    auto pSD = PSECURITY_DESCRIPTOR(LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH)); 
    if (pSD == nullptr) 
        return false;
 
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) 
        return false;
 
    // Add the ACL to the security descriptor. 
    if (!SetSecurityDescriptorDacl(pSD, true, pACL, false))
        return false;

    // Initialize a security attributes structure.
    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = true;
    return true;
}

namespace core
{
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

    void SetGameEngineParams(const CoreSettings& settings)
    {
        if (memory_map mmap = memory_map::create("RTRGameEngine", sizeof(GameEngineDllParams)))
        {
            map_view view = mmap.create_view();
            GameEngineDllParams params;
            strncpy(params.Title,   settings.Title.c_str(),   sizeof(params.Title));
            strncpy(params.ModName, settings.ModName.c_str(), sizeof(params.ModName));

            memcpy(view, &params, sizeof(params));
        }
        else
        {
            throw std::runtime_error{"Failed to memory map RTRGameEngine DLL parameters"};
        }
    }

    void RomeLoader::Start(const string& cmd, 
                           const string& workingDir,
                           const CoreSettings& settings)
    {
        KillRomeProcesses();

        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi = { nullptr, nullptr };

        SECURITY_ATTRIBUTES sa;
        if (!CreateSecurityAttributes(sa))
        {
            logsec(secFF, "Error: Failed to create global access Security Attributes\n");
        }

        string command = cmd;
        DWORD creationFlags = CREATE_SUSPENDED|CREATE_NEW_CONSOLE;
        if (!CreateProcessA(nullptr, command.data(), &sa, &sa, 0,
                           creationFlags, nullptr, workingDir.c_str(), &si, &pi))
        {
            throw std::runtime_error{fmt::format("Failed to create process {}: {}", command, shadow_getsyserr())};
        }

        // This sends data to GameEngine DllMain
        SetGameEngineParams(settings);

        log("RTW.MainThreadId: %d\n", pi.dwThreadId);

        try
        {
            // try to inject the DLL and Attach the debugger:
            if (settings.DebugAttach)
            {
                logsec(secOK, "VS JIT Attach\n");
                char jitcommand[MAX_PATH];
                sprintf(jitcommand, "vsjitdebugger.exe -p %d", pi.dwProcessId);
                
                logsec(secOK, "Launching %s\n", jitcommand);
                int result = system(jitcommand);
                if (result != S_OK) // nonfatal
                    logsec(secFF, "Debugger Attach Failed: %p\n", result);
            }
            if (settings.Inject)
            {
                if (!rpp::file_exists("GameEngine.dll"))
                    throw std::runtime_error{"Cannot find ./GameEngine.dll"};

                logsec(secOK, "Opening process with PROCESS_ALL_ACCESS privileges\n");
                HANDLE process = pi.hProcess; // shadow_open_process_all_access(pi.dwProcessId);

                // reserve just enough memory for RomeTW-ALX.exe
                logsec(secOK, "Reserving target memory\n");
                remote_dll_injector::reserve_target_memory(process, 0x0269ea9e);

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
