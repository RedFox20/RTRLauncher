#pragma once
/**
 * Copyright (c) 2014 - The RTR Project
 */
#include "GameEngine.h"
#include <process_info.h>

struct RomePatcher
{
	const GameEngine& Game;
	RomePatcher(const GameEngine& game);
	~RomePatcher();
	
	/**
	 * Patches all RTW modules. You should run this after GameEngine has been initialized.
	 */
	void RunPatcher(void* exeModule);


	/**
	 * Unlocks more regions
	 */
	void ApplyRegionsPatch(unlocked_section& map);
};