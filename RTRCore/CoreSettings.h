#pragma once
#include <string>
using namespace std;

struct CoreSettings
{
	string Title;     // Title and info of this Mod
	string StratName; // name of the stratmap to run
	string ModName;   // name of the mod
	string BkBitmap;  // launcher background bitmap
	int TurnsPerYear;

	bool BkScript; // do we generate a background script
	bool Windowed;
	bool ShowErr;
	bool LaunchStrat;
	bool DebugAttach;
	bool Inject;
	bool UseCaLog;
	bool CheckBuildings;
	bool CheckImages;
	bool ValidateModels;



	CoreSettings()
	{
		// default values
		Title        = "The RTR Project (c) 2014, The RTR Team";
		StratName	 = "imperial_campaign";
		ModName      = "RTR";
		BkBitmap     = "Launcher.bmp";
		TurnsPerYear = 2; // RTW default is 2

		BkScript    = false;
		Windowed	= true;
		ShowErr		= true;
		LaunchStrat = false;
		DebugAttach = false;
		Inject		= true;
		UseCaLog	= true;
		CheckBuildings = true;
		CheckImages    = false;
		ValidateModels = true;
	}


	void Load();
	void Save();
};