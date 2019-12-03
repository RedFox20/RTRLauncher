#pragma once
#include "PatchUtils.h"

static void EnableExtendedCamera(RomeExeVersion ver)
{
    if (ver == RomeTW_1_5)
    {
        log("    ENABLE    extended_camera  RomeTW_1_5\n");
        uint_at(0x0150F434)  = 0;      // camera_restriction_set
        uint_at(0x0150F430)  = 0;      //  restrict camera
        float_at(0x0FC3B10) = 100.0f; // max camera_restriction_set height
        float_at(0x0FC3BC0) = 100.0f; // max TW/RTS
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
        float_at(0x01019DC4) = 100.0f; // max restriction_set height
        float_at(0x01019DC8) = 100.0f; // max TW/RTS
        float_at(0x01019DB8) = 1.2f;   // min TW/RTS

        //  prevents TW camera from going below min value
        write_jne(0x095B469, 0x11);
        write_jne(0x095B46B, 0x11);
        write_jne(0x095B46D, 0x11);
    }
    else if (ver == RomeALX_1_9)
    {
        uint_at(0x1528F94)  = 0;    // camera_restriction_set
        uint_at(0x1528F90)  = 0;    // restrict camera
        float_at(0x00FDACD0) = 100.0f;   // max camera_restriction_set height
        float_at(0x00FDAD80) = 100.0f;   // max RTS    
        float_at(0x00FDACCC) = 100.0f;   // max TW
        float_at(0x00FDACC8) = 0.0f;     // min RTS/TW
        float_at(0x00FDACA4) = 0.0f;     // force min RTS for those not using free look/shift to zoom
      
        //  prevents TW camera from going below min value
        write_jne(0x00913933, 0x11);
        write_jne(0x00913935, 0x11);
        write_jne(0x00913937, 0x11);
        write_nops(0x00915B27, 5); //prevents RTS camera from overflowing on max value
    }
}

