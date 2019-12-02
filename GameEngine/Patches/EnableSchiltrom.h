#pragma once
#include "PatchUtils.h"

static void EnableSchiltrom(RomeExeVersion ver)
{
    if (ver == RomeALX_1_9)
    {
        log("    ENABLE    shield_wall  schiltrom  RomeALX_1_9\n");
        /// .text:008D4964
        /// mov  [edi+94h], al ; 6 bytes
        /// mov  [edi+93h], al ; 6 bytes
        write_nops(0x008D4964, 12);
    }
}

