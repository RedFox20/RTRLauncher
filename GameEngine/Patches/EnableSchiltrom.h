#pragma once
#include "PatchUtils.h"

static void EnableSchiltrom(RomeExeVersion ver)
{
    log("    ENABLE    shield_wall  schiltrom\n");

    uint32_t address;
    switch (ver)
    {
        default: ;
        case RomeTW_1_5:    address = 0x008D4964; break;
        case RomeTW_1_5_1:  address = 0x008D4964; break;
        case RomeBI_1_6:    address = 0x008D4964; break;
        case RomeBI_1_6_1:  address = 0x008D4964; break;
        case RomeALX_1_9:   address = 0x008D4964; break;
        case RomeALX_1_9_1: address = 0x008D4964; break;
    }

    /// .text:008D4964
    /// mov  [edi+94h], al ; 6 bytes
    /// mov  [edi+93h], al ; 6 bytes
    write_nops(address, 12);
}

