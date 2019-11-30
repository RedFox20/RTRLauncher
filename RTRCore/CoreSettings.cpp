#include "CoreSettings.h"
#include <log.h>
#include <rpp/file_io.h>
#include <rpp/strview.h>

namespace core
{
    using rpp::strview;

    static const char secLoad[] = "[+] CFGLoader [+]";

    //// @note Mod specific master config file
    static const char ConfigFile[] = "launcher.cfg";

    //// @note User specific config file
    static const char UserConfig[] = "launcher.user.cfg";


    inline void assign(string& setting, rpp::strview inValue) {
        inValue.to_string(setting);
    }
    inline void assign(int& setting, rpp::strview inValue) {
        setting = inValue.to_int();
    }
    inline void assign(bool& setting, rpp::strview inValue) {
        setting = inValue.to_bool();
    }
    template<class T> static inline void get_setting(rpp::strview key, rpp::strview value, T& setting) {
        assign(setting, value);
        logsec(secLoad, "  %-14.*s", key.length(), key.c_str()) << setting << '\n';
    }

    
    ////////////////////////// Config & UserConfig Loading ////////////////////////////

    void CoreSettings::Load()
    {
        const char* filename = ConfigFile;
        if (rpp::file_exists(UserConfig))
        {
            log("\n == Loading User Settings ==\n");
            filename = UserConfig;
        }
        else log("\n == Loading Core Settings ==\n");

        rpp::load_buffer buff = rpp::file::read_all(filename);
        rpp::line_parser lineParser = buff;
        rpp::strview line;
        while (lineParser.read_line(line))
        {
            if (line.empty() || line[0] == '#' || line[0] == '/')
                continue; // it's a comment

            rpp::strview key   = line.next(" \t");
            rpp::strview value = line.trim();
            if      (key == "title")         get_setting(key, value, Title);
            else if (key == "strat_name")    get_setting(key, value, StratName);
            else if (key == "mod_name")      get_setting(key, value, ModName);
            else if (key == "bk_bitmap")     get_setting(key, value, BkBitmap);
            else if (key == "bk_script")     get_setting(key, value, BkScript);
            else if (key == "turns_py")      get_setting(key, value, TurnsPerYear);
            else if (key == "windowed")      get_setting(key, value, Windowed);
            else if (key == "show_err")      get_setting(key, value, ShowErr);
            else if (key == "launch_strat")  get_setting(key, value, LaunchStrat);
            else if (key == "attach_vsjit")  get_setting(key, value, DebugAttach);
            else if (key == "inject_dll")    get_setting(key, value, Inject);
            else if (key == "dbg_ca_logs")   get_setting(key, value, UseCaLog);
            else if (key == "dbg_buildings") get_setting(key, value, CheckBuildings);
            else if (key == "dbg_images")    get_setting(key, value, CheckImages);
            else if (key == "dbg_models")    get_setting(key, value, ValidateModels);
        }
        log(" ===========================\n\n");
    }



    ///////////////////////// UserConfig Save //////////////////////////

    struct SettingsWriteMap
    {
        rpp::string_buffer out;
        std::vector<strview> keys;
        SettingsWriteMap() : keys({ 
            "title", "strat_name", "mod_name", "bk_bitmap", "bk_script",
            "turns_py", "windowed", "show_err", "launch_strat", "attach_vsjit", 
            "inject_dll", "dbg_ca_logs", "dbg_buildings", "dbg_images", "dbg_models" }) {
        }
        template<class T> void save_existing(strview key, const T& value) {
            out.writef("%-14.*s", key.length(), key.str);
            out.write(value);
            out.write('\n');
            logsec(secLoad, "  %-14.*s", key.length(), key.str) << value << '\n';
            for (size_t i = 0; i < keys.size(); ++i) { // remove this key:
                if (keys[i] == key) {
                    keys.erase(keys.begin() + i);
                    break;
                }
            }
        }
        bool save_key(strview key, const CoreSettings& settings)
        {
            if      (key == "title")         save_existing(key, settings.Title);
            else if (key == "strat_name")    save_existing(key, settings.StratName);
            else if (key == "mod_name")      save_existing(key, settings.ModName);
            else if (key == "bk_bitmap")     save_existing(key, settings.BkBitmap);
            else if (key == "bk_script")     save_existing(key, settings.BkScript);
            else if (key == "turns_py")      save_existing(key, settings.TurnsPerYear);
            else if (key == "windowed")      save_existing(key, settings.Windowed);
            else if (key == "show_err")      save_existing(key, settings.ShowErr);
            else if (key == "launch_strat")  save_existing(key, settings.LaunchStrat);
            else if (key == "attach_vsjit")  save_existing(key, settings.DebugAttach);
            else if (key == "inject_dll")    save_existing(key, settings.Inject);
            else if (key == "dbg_ca_logs")   save_existing(key, settings.UseCaLog);
            else if (key == "dbg_buildings") save_existing(key, settings.CheckBuildings);
            else if (key == "dbg_images")    save_existing(key, settings.CheckImages);
            else if (key == "dbg_models")    save_existing(key, settings.ValidateModels);
            else return false; // key not found
            return true; // key saved
        }
    };

    void CoreSettings::Save()
    {
        log("\n == Saving User Settings ==\n");

        // reload the file (we want to preserve comments etc)
        rpp::load_buffer buff = rpp::file::read_all(UserConfig);
        rpp::line_parser lineParser = buff;

        // now we write back to config file and modify only the lines we care about
        SettingsWriteMap map;
        strview line;
        while (lineParser.read_line(line))
        {
            line.trim_start();
            if (line.empty() || line[0] == '#' || line[0] == '/')
            {
                map.out.writeln(line); // write empty or commented lines out too
                continue;
            }
            strview key = strview{line}.next(" \t");
            if (!map.save_key(key, *this)) // did we fail to save an existing setting?
                map.out.writeln(line); // write out just this line then
        }

        while (!map.keys.empty()) // write out any stragglers
            map.save_key(map.keys[0], *this);

        log(" ==========================\n\n");

        // write the new launcher.user.cfg file:
        if (map.out.size())
            rpp::file::write_new(UserConfig, map.out.view());
    }
}
