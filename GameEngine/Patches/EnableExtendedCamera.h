#pragma once
#include "PatchUtils.h"

static void EnableExtendedCamera(RomeExeVersion ver)
{
    if (ver == RomeTW_1_5)
    {
        log("    ENABLE    extended_camera  RomeTW_1_5\n");
        uint_at(0x0150F434)  = 0;      // camera_restriction_set
        uint_at(0x0150F430)  = 0;      //  restrict camera
        float_at(0x0FC3B10) = 100.0f; // max TW
        float_at(0x0FC3BC0) = 100.0f; // max RTS
        float_at(0x0FC3B08) = 1.2f;   // min TW/RTS

        //  prevents TW camera from going below min value
        write_jne(0x090D15B, 0x11);
        write_jne(0x095B45D, 0x11);
        write_jne(0x095B45F, 0x11);
        write_nops(0x090F34F, 5); // prevents RTS camera from overflowing on max value
    }
    else if (ver == RomeTW_1_5_1)
    {
        log("    ENABLE  extended_camera  RomeTW_1_5_1\n");
        uint_at(0x017B1C44)  = 0;      // camera_restriction_set
        uint_at(0x017B1C40)  = 0;      //  restrict camera
        float_at(0x01019DC4) = 100.0f; // max TW
        float_at(0x01019DC8) = 100.0f; // max RTS
        float_at(0x01019DB8) = 1.2f;   // min TW/RTS

        //  prevents TW camera from going below min value
        write_jne(0x095B469, 0x11);
        write_jne(0x095B46B, 0x11);
        write_jne(0x095B46D, 0x11);
    }
}

