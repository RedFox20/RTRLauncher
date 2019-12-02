#pragma once
/**
 * Copyright (c) 2014 - The RTR Project
 */
#include "GameEngine.h"
#include "RTW/RomeAPI.h"

/**
 * Patches all RTW modules. You should run this after GameEngine has been initialized.
 */
RomeExeVersion RunPatcher(const GameEngine& game, const char* exePath, void* exeModule);

