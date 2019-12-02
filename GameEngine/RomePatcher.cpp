#include "RomePatcher.h"
#include <process_info.h>
#include <log.h>

#include "Patches/PatchUtils.h"
#include "Patches/EnableSchiltrom.h"
#include "Patches/EnableBiggerUnitSize.h"
#include "Patches/EnableUnitCanSwim.h"
#include "Patches/EnableUnitCanHorde.h"
#include "Patches/EnableExtendedCamera.h"
#include "Patches/EnableModdedSkyClouds.h"
#include "Patches/ExtendRegionsLimit.h"

RomeExeVersion DetectRomeVersion(process_info& proc)
{
    return RomeALX_1_9;
}

RomeExeVersion RunPatcher(const GameEngine& game, void* exeModule)
{
    log("%s\n", GetCommandLineA());

    process_info proc(exeModule); // current process info
    log("IMAGE      0x%p\n", proc.Image);
    log("IMAGE_BASE 0x%p\n", proc.ImageBase);
    log("NT HDRS    0x%p\n", proc.Dos->e_lfanew);
    log("NT ENTRY   0x%p\n", proc.Nt->OptionalHeader.AddressOfEntryPoint);
    log("SECTIONS   %d\n",   proc.NumSections);

    log("\nSECTION  .text\n");
    RomeExeVersion ver = DetectRomeVersion(proc);

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

