#pragma once
#include "PatchUtils.h"
#include <process_info.h>

static void EnableModdedSkyClouds(unlocked_section& data)
{
    log("    REPLACE   data/sky/clouds  RTR/data/clouds\n");

    int count = 0;
    char* ptr = data.Ptr;
    while (ptr = data.find(ptr, "data/sky/clouds")) {
        log("        - 0x%x\n", ptr);
        memcpy(ptr, "RTR/data", 8);
        ptr += 16; // skip over the found string
        ++count;
    }
}

