#include "RomePatcher.h"
#include <process_info.h>
#include <log.h>
#include <rpp/file_io.h>

#include "Patches/PatchUtils.h"
#include "Patches/EnableSchiltrom.h"
#include "Patches/EnableBiggerUnitSize.h"
#include "Patches/EnableUnitCanSwim.h"
#include "Patches/EnableUnitCanHorde.h"
#include "Patches/EnableExtendedCamera.h"
#include "Patches/EnableModdedSkyClouds.h"
#include "Patches/ExtendRegionsLimit.h"
#include <ctime>
#include <stdexcept>
#pragma comment(lib, "Version.lib")


RomeExeVersion DetectRomeVersion(process_info& proc, const char* exePath)
{
    // faster and less noisy
    DWORD time = proc.Nt->FileHeader.TimeDateStamp;
    if (time == 1147268826) return RomeALX_1_9;
    if (time == 1410332819) return RomeALX_1_9_1;

    // check RomeTW EXE ProductVersion value
    DWORD dummy;
    DWORD versionSize = GetFileVersionInfoSize(exePath, &dummy);
    std::vector<BYTE> versionInfo; versionInfo.resize(versionSize);
    GetFileVersionInfo(exePath, 0, versionSize, versionInfo.data());
    UINT len;
    VS_FIXEDFILEINFO* ffi;
    VerQueryValue(versionInfo.data() , "\\", reinterpret_cast<LPVOID*>(&ffi) , &len);
    int major = HIWORD(ffi->dwProductVersionMS);
    int minor = LOWORD(ffi->dwProductVersionMS);
    int patch = HIWORD(ffi->dwProductVersionLS);
    int build = LOWORD(ffi->dwProductVersionLS);
    log("%s ProductVersion: %d.%d.%d.%d\n", rpp::file_nameext(exePath).data(), major, minor, patch, build);

    struct KnownVersion { int major, minor, patch; RomeExeVersion ver; };
    std::vector<KnownVersion> knownVersions {
        { 1, 5, 0, RomeTW_1_5    },
        { 1, 5, 1, RomeTW_1_5_1  },
        { 1, 6, 0, RomeBI_1_6    },
        { 1, 6, 1, RomeBI_1_6_1  },
        { 1, 9, 0, RomeALX_1_9   },
        { 1, 9, 1, RomeALX_1_9_1 },
    };

    for (auto& v : knownVersions)
        if (v.major == major && v.minor == minor && v.patch == patch)
            return v.ver;

    throw std::runtime_error{"Failed to detect RTW version!"};
}

RomeExeVersion RunPatcher(const GameEngine& game, const char* exePath, void* exeModule)
{
    process_info proc {exeModule}; // current process info
    log("IMAGE       0x%p\n", proc.Image);
    log("IMAGE_BASE  0x%p\n", proc.ImageBase);
    log("ENTRY POINT 0x%p\n", proc.Nt->OptionalHeader.AddressOfEntryPoint);
    log("SECTIONS    %d\n",   proc.NumSections);

    auto time = time_t(proc.Nt->FileHeader.TimeDateStamp);
    tm ts{}; gmtime_s(&ts, &time);
    char timestr[80]; strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &ts);
    log("TIMESTAMP   (%u)  %s\n", proc.Nt->FileHeader.TimeDateStamp, timestr);

    RomeExeVersion ver = DetectRomeVersion(proc, exePath);
    log("Detected Rome Version: %s\n", to_string(ver));

    log("\nSECTION  .text\n");
    IMAGE_SECTION_HEADER* sec = proc.find_section(".text");
    if (unlocked_section code = proc.map_section(sec))
    {
        log("    VirtAddr  0x%08x\n", proc.ImageBase + sec->VirtualAddress);
        log("    VirtSize  0x%08x\n", sec->SizeOfRawData);
        log("    FPData    0x%08x\n", sec->PointerToRawData);
        log("\n");
        
        EnableSchiltrom(ver);
        EnableBiggerUnitSize(ver);
        EnableUnitCanSwim(ver);
        EnableUnitCanHorde(ver);
        //EnableExtendedCamera(ver);

        // staging atm. so set to false
        if (false)
        {
            ExtendRegionsLimit(code);
        }

        proc.unmap_section(code);
    }

    log("\nSECTION  .data1\n");
    sec = proc.find_section(".data1");
    if (unlocked_section data1 = proc.map_section(sec))
    {
        log("    VirtAddr  0x%08x\n", proc.ImageBase + sec->VirtualAddress);
        log("    VirtSize  0x%08x\n", sec->Misc.VirtualSize);
        log("    FPData    0x%08x\n", sec->PointerToRawData);
        log("\n");
        log("    Ptr       0x%08x\n", data1.Ptr);
        log("    Size      0x%08x\n", data1.Size);

        EnableModdedSkyClouds(data1);

        proc.unmap_section(data1);
    }

    log("=========================\n");
    return ver;
}

