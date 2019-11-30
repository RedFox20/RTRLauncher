#include "RtwScript.h"
#include <log.h>
#include <rpp/file_io.h>
#include <io/binary_writer.h>

namespace rtw
{
    static const char* scriptSec = "[==] RtwScript [==]";

    void ScriptWriter::AddScript(const ScriptParams& params)
    {
        Scripts.emplace_back(params); // this calls RtwScript(params) constructor
        AddDescr(params);
    }


    void ScriptWriter::WriteScriptData(const string& dataPath)
    {
        // write all the script files:
        string show_me = dataPath + "scripts/show_me/";
        for (Script& rtwScript : Scripts)
        {
            // put the path together:
            string dst = show_me + rtwScript.params.ScriptName + ".txt";

            logsec(scriptSec, "Creating %s/data/scripts/show_me/%s.txt\n", Mod.c_str(), rtwScript.params.ScriptName.c_str());

            // write a new file from the script data buffer:
            rpp::file::write_new(dst, rtwScript.data.view());
        }
        
        logsec(scriptSec, "Updating %s/data/export_descr_advice.txt\n", Mod.c_str());
        
        // write new export_descr_advice.txt
        binary_filewriter edaFile(dataPath + "export_descr_advice.txt");
        edaFile << ";; --- Rome Launcher Generated AdviceThreads --- ;;\r\n";
        edaFile << AdviceThreads;
        edaFile << rpp::file::read_all(SourceEDA);
        edaFile << "\r\n\r\n;; --- Rome Launcher Generated Triggers --- ;;\r\n";
        edaFile << Triggers;
        edaFile.flush();

        logsec(scriptSec, "Updating %s/data/text/export_advice.txt\n", Mod.c_str());

        // write new export_advice.txt
        binary_filewriter daFile(dataPath + "text/export_advice.txt");
        daFile << L"¬¬¬ Rome Launcher Generated Advice Texts ¬¬¬\r\n";
        daFile << rpp::file::read_all(SourceEA);
        daFile << DescrAdvice;
        daFile.flush();
    }


    void ScriptWriter::AddDescr(const ScriptParams& params)
    {
        const string& name = params.ScriptName;
        string thread  = name + "_Thread";
        string trigger = name + "_Trigger_";

        AdviceThreads << ";------------------------------------------\r\n";
        AdviceThreads << "AdviceThread " << thread << "\r\n";
        AdviceThreads << "\tGameArea Campaign\r\n\r\n";
        AdviceThreads << "\tItem " << name << "_Item\r\n";
        AdviceThreads << "\t\tUninhibitable\r\n";
        AdviceThreads << "\t\tVerbosity  0\r\n";
        AdviceThreads << "\t\tThreshold  1\r\n";
        AdviceThreads << "\t\tMaxRepeats  0\r\n";
        AdviceThreads << "\t\tRepeatInterval  1\r\n";
        AdviceThreads << "\t\tAttitude Normal\r\n";
        AdviceThreads << "\t\tPresentation Default\r\n";
        AdviceThreads << "\t\tTitle " << name << "_Title\r\n";
        AdviceThreads << "\t\tOn_display scripts/show_me/" << name << ".txt\r\n";
        AdviceThreads << "\t\tText " << name << "_Text\r\n\r\n";

        //  { WhenToTest, Condition (can be NULL) }
        static const char* triggers[][2] = {
            { "GameReloaded", NULL },
            { "SettlementSelected",  NULL},
            { "CharacterSelected", NULL },
            { "ButtonPressed",  "ButtonPressed faction_button"},
            { "ButtonPressed",  "ButtonPressed construction_button"},
            { "ButtonPressed",  "ButtonPressed recruitment_button"},
        };

        const int count = sizeof(triggers) / sizeof(const char*) / 2;
        for (int i = 0; i < count; ++i)
        {
            Triggers << ";------------------------------------------\r\n";
            Triggers << "Trigger " << trigger << i << "\r\n";
            Triggers << "\tWhenToTest " << triggers[i][0] << "\r\n";
            if (triggers[i][1])
                Triggers << "\tCondition " << triggers[i][1] << "\r\n";
            Triggers << "\tAdviceThread " << thread <<" 1\r\n";
        }

        std::wstring wname { name.begin(), name.end() };
        DescrAdvice << L"{" << wname << L"_Title} Script\r\n";
        DescrAdvice << L"{" << wname << L"_Text} Script started.\r\n";
    }


    Script::Script(const ScriptParams& params) : params(params)
    {
        rpp::string_buffer& s = data;
        s << "script\r\n"
            "	while I_AdvisorVisible\r\n"
            "	end_while\r\n"
            "	suspend_unscripted_advice true\r\n"
            "	declare_show_me\r\n";//Script manual start

        monitor_event("GameReloaded TrueCondition", [&]() {
            s << "		terminate_script\r\n"; // terminates on reload
        });
        monitor_event("ScrollAdviceRequested ScrollAdviceRequested end_game_scroll", [&]() {
            s << "		terminate_script\r\n"; // terminate on quit.
        });

        if (ScriptDebug)
            s << "	console_command disable_ai\r\n"; // since testing without this on is looong

        comment("TurnsPerYear: ", params.TurnsPerYear);
        
        /// @note Special version for 1TPY to add a winter every 3 turns
        if (params.TurnsPerYear == 1)
        {
            int winterCounter = 0;
            int date = params.StartDate;
            for (int turn = 0; turn < params.NumTurns; ++turn, ++date)
            {
                console_date(date);
                if (++winterCounter == 3) // winter every 3 turns
                {
                    console_season("winter");
                    winterCounter = 0; // reset counter
                }
                else console_season("summer");

                while_int("I_TurnNumber = ", turn, [&]() { // loop and wait for the year to end
                    s << "		suspend_unscripted_advice true\r\n";
                });
            }
        }
        /// @note 2TPY is RTW's default setting, so no extra hacks required if TPY == 2
        else if (params.TurnsPerYear != 2)
        {
            double ypt_frac = 1.0 / params.TurnsPerYear; // years per turn fraction
            double year = params.StartDate + ypt_frac;
            for (int turn = 0; turn < params.NumTurns; ++turn, year += ypt_frac)
            {
                int date = (int)floor(year);	// current date
                double year_frac = year - date; // progress of this year 0.0 - 1.0

                console_date(date);
                console_season(year_frac <= 0.66 ? "summer" : "winter"); // winter usually lasts 1/3 of the year
                while_int("I_TurnNumber = ", turn, [&]() { // loop and wait for the year to end
                    s << "		suspend_unscripted_advice true\r\n";
                });
            }
        }

        s << "	while I_TurnNumber < 99999\r\n"; // makes sure script is not terminated for other scripts.
        s << "		suspend_unscripted_advice true\r\n";
        s << "	end_while\r\n";
        s << "end_script"; // don't add a newline here, RTW is really picky about it

        // log script generation success
        logsec(scriptSec, "Script %s [%d TPY, %d turns]\n",
            params.ScriptName.c_str(), params.TurnsPerYear, params.NumTurns);
    }
        
}
