#include "GameEngine.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <rpp/strview.h>
#include "RomePatcher.h"

/*extern*/ GameEngine Game;

void GameEngine::Initialize(void* exeModule)
{
	RomePatcher(*this).RunPatcher(exeModule);
}