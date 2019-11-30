#pragma once
#include "CoreSettings.h"

namespace core
{
    struct RomeLoader
    {
        static void Start(const string& cmd, 
                          const string& workingDir,
                          const CoreSettings& settings);
    };
}