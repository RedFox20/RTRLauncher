#pragma once
#include <ppltasks.h>
#include <string>
#include <vector>
#include "CoreSettings.h"

#if !defined(TARGET_ROME) && !defined(TARGET_MEDIEVAL)
    // Default target is Rome Total War. To change this, #define TARGET_MEDIEVAL
    #define TARGET_ROME 1 
#endif

using std::string;
using std::vector;


enum GameVersion
{
    GameNotFound, // Failed to find a Game instance
    RomeALX,			// Rome TW: Alexander
    RomeBI,				// Rome TW: Barbarian Invasion
    RomeTW,             // Rome TW
    MedievalKingdoms,	// Medieval II TW: Kingdoms
};






class LauncherCore
{
public:
    string LauncherDir;	// directory of VanillaDir/MOD/
    string DataDir;		// directory of VanillaDir/MOD/data
    string CoreDir;		// directory of VanillaDir/MOD/core
    string GameDir;		// vanilla dir  VanillaDir/

    /// list of core extension directories
    /// @note All extensions must start with "core_"
    vector<string> CoreExtensions;
    

    string       Executable;  // current RomeTW executable
    string       CmdLine;	  // current Cmdline params for RTW
    GameVersion  GameVer;     // current RomeTW version 
    CoreSettings Settings;    // current launch settings

private:

    bool mWorkerRunning; // background worker thread is currently running?

public:

    //// @note This is the main entrypoint for core RTR logic
    //// @note Only the launcher.cfg file is loaded
    //// @warning DO NOT PUT OTHER HEAVY/TIMECONSUMING WORK HERE!
    LauncherCore();

    /**
     * Loads all initial settings data from ./launcher.cfg
     */
    void LoadSettings();

    /**
     * Saves all settings data to ./launcher.cfg
     */
    void SaveSettings();

    /**
     * @note Starts LauncherCore asynchronously (this might take a few seconds)
     *		 The idea is to take the entire load off of the UI thread
     *		 and wait until LauncherCore has finished initializing the mod files.
     */
    concurrency::task<void> StartAsync();

    /**
     * @note Starts LauncherCore synchronously. This should be called if
     *       StartAsync() has already run once before, caching most of the data.
     */
    void Start();

    void DebuggerLoop();

public:

    /**
     * @note This launches RTW with its current settings - only if the worker
     *       is not currently running.
     * @return TRUE if RTR was successfully launched
     */
    bool LaunchGame();

private:
    bool LaunchGame(char (&errmsg)[512]);

public:
    /**
     * @brief	Gets path to Game Executable
     * @param [out]	outExe	Path to the executable
     * @return	Version identifier for RTW or M2TW
     */
    GameVersion GetGamePath(string& outExe) const;


    /**
     * Checks whether the target romeTW version is actually
     * valid for patching or execution.
     * @param [in] romeTW Path to executable to check
     * @return TRUE if the executable is supported, false if not
     */
    bool CheckGameVersion(const string& romeTW) const;


    /**
     * @return true if RTR Core is currently working on something
     * @note You should not post a new task until the current one is finished
     */
    inline bool IsWorkerRunning() const
    {
        return mWorkerRunning;
    }

    /**
     * @brief Validates all map.rwm files and deletes them if any map source files have changed
     */
    void ValidateMapRWM() const;
};

