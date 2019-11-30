#pragma once
#include <string>
#include <vector>
#include <rpp/future.h>
#include "CoreSettings.h"

#if !defined(TARGET_ROME) && !defined(TARGET_MEDIEVAL)
    // Default target is Rome Total War. To change this, #define TARGET_MEDIEVAL
    #define TARGET_ROME 1 
#endif

namespace core
{
    using std::string;
    using std::vector;

    enum class GameVersion
    {
        GameNotFound,     // Failed to find a Game instance
        RomeALX,          // Rome TW: Alexander
        RomeBI,           // Rome TW: Barbarian Invasion
        RomeTW,           // Rome TW
        MedievalKingdoms, // Medieval II TW: Kingdoms
    };

    struct GameInfo
    {
        GameVersion Ver;
        string FullPath;
        string ExeName;

        GameInfo() = default;
        GameInfo(GameVersion ver, const string& gameDir, const string& exeName);
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

        // Current RTW game exe info
        GameInfo Game;
        CoreSettings Settings;    // current launch settings

    private:

        std::atomic_bool WorkerRunning { false }; // background worker thread is currently running?

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
         *       The idea is to take the entire load off of the UI thread
         *       and wait until LauncherCore has finished initializing the mod files.
         */
        rpp::cfuture<void> StartAsync();

        /**
         * @note Starts LauncherCore synchronously. This should be called if
         *       StartAsync() has already run once before, caching most of the data.
         */
        void Start();

        /**
         * @note This launches RTW with its current settings - only if the worker
         *       is not currently running.
         * @return TRUE if RTR was successfully launched
         */
        bool LaunchGame();

    private:

        string GetRTWCommandLine() const;

        // @note THROWS on failure
        void LaunchGame(const string& commandLine);

    public:

        GameInfo GetTargetGameInfo() const;

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
        bool IsWorkerRunning() const { return WorkerRunning; }

        /**
         * @brief Validates all map.rwm files and deletes them if any map source files have changed
         */
        void ValidateMapRWM() const;
    };

}
