#pragma once
/**
 * Copyright (c) 2014 - The RTR Project
 */
#include "RTW/RomeAPI.h"

struct GameEngine
{
    RomeExeVersion Version;
    void* APIThread = nullptr;

    void Initialize(void* exeModule);

    /**
     * Starts RomeTW main thread
     */
    void LaunchRome();

    /**
     * Background thread which monitors the RomeTW process
     */
    void MonitorThread();
};

extern GameEngine Game;

