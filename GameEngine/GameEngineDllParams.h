#pragma once

// Keep this in plain C, no C++ types
struct GameEngineDllParams
{
    char Title[256]; // Mod Title, eg ""
    char ModName[256]; // Mod Name, eg "RTR"
    char Executable[512]; // full exe path, eg "C:/RTW/RomeTW-ALX.exe"
    char Arguments[512];  // exe arguments
};

