#include "GameEngine.h"
#include "RomePatcher.h"

/*extern*/ GameEngine Game;

void GameEngine::Initialize(void* exeModule)
{
    RomePatcher{*this}.RunPatcher(exeModule);
}
