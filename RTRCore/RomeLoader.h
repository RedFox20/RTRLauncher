#pragma once
#include "CoreSettings.h"

namespace core
{
    /**
     * This loads the RomeTW executable process
     * and injects GameEngine.dll along with necessary injector parameters
     */
    void LoadAndInjectRomeProcess(
        const string& executable,
        const string& arguments,
        const string& workingDir,
        const CoreSettings& settings);
}
