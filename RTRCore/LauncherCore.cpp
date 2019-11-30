#include "LauncherCore.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <log.h>
#include "RtwScript.h"
#include "RomeLoader.h"
#include <fnv.h>
#include <rpp/file_io.h>


    LauncherCore::LauncherCore() : mWorkerRunning(false)
    {
        logger_init("launcher.log"); // yeah we're gonna need this for debugz
        log(" === Welcome to Rome Launcher ===\n");

        // initialize main directories
        // LauncherDir = "C:/Games/RTRGame/RTR/"
        LauncherDir = rpp::working_dir();
        rpp::normalize(LauncherDir, '/');

        // GameDir = "C:/Games/RTRGame/"
        GameDir = rpp::full_path(LauncherDir + "../");
        rpp::normalize(GameDir, '/');

        DataDir = LauncherDir + "data/";
        CoreDir = LauncherDir + "core/";

        // get all extension folders
        rpp::list_dirs(CoreExtensions, LauncherDir, "core_*");

        // we need to initialize launcher before we can do anything
        LoadSettings();
    }


    void LauncherCore::LoadSettings() { Settings.Load(); }
    void LauncherCore::SaveSettings() { Settings.Save(); }


    concurrency::task<void> LauncherCore::StartAsync()
    {
        if (mWorkerRunning) // don't allow restarting while we're busy
            return concurrency::task<void>();

        mWorkerRunning = true;
        return concurrency::create_task([this](){
            Start();
            mWorkerRunning = false;
        });
    }


    //// @note All the heavy lifting should go here
    void LauncherCore::Start()
    {
        if (Settings.BkScript) // generate bk script?
        {
            /// @todo Check if we really need to regenerate the script files?
            string coreEDA = CoreDir + "export_descr_advice.txt";
            string coreEA  = CoreDir + "text/export_advice.txt";
            rtw::ScriptWriter scripts(Settings.ModName, coreEDA, coreEA);
            rtw::ScriptParams bk_Script = rtw::ScriptParams("BK_Script", "no source");
            bk_Script.TurnsPerYear = Settings.TurnsPerYear;
            bk_Script.NumTurns     = Settings.TurnsPerYear * 100;
            scripts.AddScript(bk_Script);
            scripts.WriteScriptData(DataDir);
        }

        ValidateMapRWM();
    }




    GameVersion LauncherCore::GetGamePath(string& romeTW) const
    {
        if (rpp::file_exists(romeTW.assign(GameDir).append("RomeTW-ALX.exe")))
            return RomeALX;
        if (rpp::file_exists(romeTW.assign(GameDir).append("RomeTW-BI.exe")))
            return RomeBI;
        if (rpp::file_exists(romeTW.assign(GameDir).append("RomeTW.exe")))
            return RomeTW;
        romeTW.clear();
        return GameVersion::GameNotFound;
    }



    static const char* secOK = "[+] LaunchRTR [+]";
    static const char* secFF = "[!] LaunchRTR [!]";


    // this just sets up the command line args, settings, rome paths and calls the second
    // LaunchRTR function, which actually validates rome version etc
    bool LauncherCore::LaunchGame()
    {
        log("\n");
        logsec(secOK, "Initializing...\n");
        GameVer = GetGamePath(Executable);
        
        // switches:
        // -nm	No Movies
        // -ne	Windowed mode
        CmdLine = "-nm -enable_editor";
        if (Settings.ShowErr)		CmdLine += " -show_err";
        if (Settings.Windowed)		CmdLine += " -ne";
        if (Settings.LaunchStrat)	CmdLine += " -strat:", CmdLine += Settings.StratName;
        if (Settings.UseCaLog)		CmdLine += " -ca_log:ca_log.log -ca_fileopen_log:ca_fopen.log";

        //CmdLine += " -enable_texel_realignment"; // what does this one do? investigate.
        //CmdLine += " -force_ip:192.168.0.1:2900"; // investigate this further
        //CmdLine += " -quick_battle:rome_flat2_01"; // jumps into quickbattle grassy flatlands
        
        if (Settings.CheckBuildings) CmdLine += " -check_script_buildings";
        if (Settings.CheckImages)	 CmdLine += " -report_missing_images";
        if (Settings.ValidateModels) //CmdLine += " -util:validate_models,unit_models,encrypt,animdb,sound";
        {
            CmdLine += " -util:";
            // validate_models checks battle + strat models, unit_models checks only battle models
            // animdb generates Anims database
            // sound generates sounds/events.dat|.idx
            if (Settings.ValidateModels) CmdLine += "validate_models"; 
        }

        //CmdLine += " -battle_ed:imperial_campaign";
        //CmdLine += " -cbf";
        if (!Settings.ModName.empty()) // only if we have a modname
            CmdLine += " -mod:" + Settings.ModName;
        CmdLine += " -noalexander";
        logsec(secOK, "Args:%s\n", CmdLine.c_str());

        
        char errmsg[512];
        if (LaunchGame(errmsg))
            return true; // success

        char fmt[512];
        sprintf(fmt, "Failed to launch %s\n%s\n", Executable.c_str(), errmsg);
        logsec(secFF, fmt);
        MessageBoxA(0, fmt, "Launch Failed!", MB_OK);
        return false;
    }





    // validates rome versions, tries to patch the image, triggers CreateProcess
    bool LauncherCore::LaunchGame(char (&errmsg)[512])
    {
        logsec(secOK, "Validating RomeTW Path\n");
        if (GameVer == GameVersion::GameNotFound) {
            strcpy(errmsg, "Failed to find RomeTW-ALX.exe, RomeTW-BI.exe or RomeTW.exe");
            return false;
        }
        logsec(secOK, "Found %s\n", Executable.c_str());
        logsec(secOK, "Validating Rome version\n");

        // initialize imageFile, need to patch BI before we can use it:
        if (!CheckGameVersion(Executable)) {
            sprintf(errmsg, "Unsupported version of %s", Executable.c_str());
            return false;
        }
        logsec(secOK, "Loading Process: %s\n", Executable.c_str(), CmdLine.c_str());	

        if (GameVer != GameVersion::RomeALX)
        {
            char name[MAX_PATH];
            GetModuleFileNameA(NULL, name, MAX_PATH); // use RTRLauncher.exe instead
            Executable = name; // -- debug stuff -- 
        }

        //HMODULE ntdll = LoadLibraryA("ntdll");
        //vector<PCCH> procs;
        //shadow_listprocs(procs, ntdll);
        //for (auto proc : procs)
        //	log("%s\n", proc);

        //vector<PCCH> imports;
        //shadow_listimports(imports);
        //for (auto path : imports)
        //	log("import: %s\n", path);

        //vector<string> modules;
        //shadow_listmodules(modules);
        //for (auto& path : modules)
        //	log("loaded: %s\n", path.c_str());

        //Executable = GameDir + "RomeTW-BI.exe";
        //Executable = GameDir + "RomeTW.exe"; // -- debug stuff --
        string cmd = Executable + " " + CmdLine;
        if (RomeLoader::Start(errmsg, cmd, GameDir, Settings))
            return true; // success

        // CreateProcess or InjectAttach failed:
        int error = GetLastError();
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, error, 0, errmsg, sizeof(errmsg), 0);
        return false;
    }






    bool LauncherCore::CheckGameVersion(const string& romeTW) const
    {
        int size = rpp::file_size(romeTW);
        if (size < 5*1024*1024)
            return false; // encrypted exes not supported atm
        //// @note Steam update unencrypted all the exes
        return true;
    }




    void LauncherCore::ValidateMapRWM() const
    {
        static const char secOK[] = "[==] RWM Check [==]";
        logsec(secOK, "Validating map.rwm files.\n");

        string maps_base = "data/world/maps/base/";
        string camp_base = "data/world/maps/campaign/";
        vector<string> campaigns;
        rpp::list_dirs(campaigns, camp_base);

        vector<string> checksum_files = {
            "descr_disasters.txt","descr_events.txt","descr_regions.txt","descr_terrain.txt", 
            "map_climates.tga","map_FE.tga","map_features.tga","map_ground_types.tga",
            "map_heights.tga","map_regions.tga","map_roughness.tga","map_trade_routes.tga",
            "water_surface.tga"
        };

        // setup validate folders
        vector<string> validate = { maps_base };
        for (string& campaign : campaigns) 
            validate.emplace_back(camp_base + campaign + "/");
        
        for (string& folder : validate)
        {
            char buffer[512];
            string checksum_file(buffer, sprintf(buffer, "%s/%llu",
                getenv("TMP"), fnv_hash(folder.c_str(), folder.size())));

            uint64_t old = 0;
            if (rpp::file_exists(checksum_file))
                rpp::file(checksum_file, rpp::file::READONLY).read(&old, sizeof(old)); // load old checksum

            uint64_t hash = fnv_init(); // create new checksum:
            for (const string& fname : checksum_files) {
                string filepath = folder + fname;
                if (rpp::file_exists(filepath)) {
                    auto modified = rpp::file_modified(filepath);
                    fnv_combine(hash, modified);
                }
            }

            // do we need to delete map.rwm ?
            string rwmfile = folder + "map.rwm";
            if (hash != old) {
                logsec(secOK, "Deleting: %s\n", rwmfile.c_str());
                if (rpp::file_exists(rwmfile))
                    remove(rwmfile.c_str());
            }
            else logsec(secOK, "Passed:   %s\n", rwmfile.c_str());

            rpp::file::write_new(checksum_file, &hash, sizeof(hash)); // save checksum
        }
    }
