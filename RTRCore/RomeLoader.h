#pragma once
#include "CoreSettings.h"


struct RomeLoader
{



	static bool Start(char (&errmsg)[512], 
					  const string& cmd, 
					  const string& workingDir,
					  const CoreSettings& settings);

};