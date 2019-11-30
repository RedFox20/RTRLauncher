#pragma once
#include <string>
#include <rpp/sprint.h>
#include <cstdarg>

namespace rtw
{
    using std::string;

    /**
     * @brief	Parameters for generating new script files
     */
    struct ScriptParams
    {
        string ScriptName;
        string ScriptSource; /// @todo Parse script source files
        int  NumTurns;       /// number of turns to generate the 4TPY script
        int  TurnsPerYear;   /// number of turns per year to generate (default is 2)
        int  StartDate;      /// starting date of the campaign (default is -280 BC)

        ScriptParams(string scriptName, string scriptSource)
            : ScriptName{scriptName}, ScriptSource{scriptSource}
        {
            // give default values
            NumTurns     = 400;  // 100 years by default
            TurnsPerYear = 2;    // 2 turns per year by default
            StartDate    = -280;
        }
    };


    struct Script
    {
        #if _DEBUG
            static const bool ScriptDebug = true; // script debugging flag, should be disabled in release
        #else
            static const bool ScriptDebug = false;
        #endif

        // RtwScript public members
        ScriptParams params; // script specific parameters
        rpp::string_buffer data;  // generated script data


        /// @note initialize Script with specified params
        Script(const ScriptParams& params);


        // some private helper funcs:
    private:


        rpp::string_buffer& indent(int indentCount = 1)
        {
            for (int i = 0; i < indentCount; ++i)
                data.write('\t');
            return data;
        }

        void comment(const char* text)
        {
            if (ScriptDebug) data << ";;; " << text << "\r\n";
        }

        void comment(const char* text, int arg1)
        {
            if (ScriptDebug) data << ";;; " << text << arg1 << "\r\n";
        }

        void console_season(const char* winterOrSummer, int indentation = 1)
        {
            indent(indentation) << "console_command season " << winterOrSummer << "\r\n";
        }

        void console_date(int intArg, int indentation = 1)
        {
            indent(indentation) << "console_command date " << intArg << "\r\n";
        }

        template<class LambdaFunc> 
        void monitor_event(const char* conditions, LambdaFunc body, int indentation = 1)
        {
            indent(indentation) << "monitor_event " << conditions << "\r\n";
            body();
            indent(indentation) << "end_monitor\r\n";
        }

        template<class LambdaFunc>
        void if_compare_int(const char* condition, int intArg, LambdaFunc body, int indentation = 2)
        {
            indent(indentation) << "if " << condition << intArg << "\r\n";
            body();
            indent(indentation) << "end_if\r\n";
        }

        template<class LambdaFunc>
        void while_int(const char* condition, int intArg, LambdaFunc body, int indentation = 1)
        {
            indent(indentation) << "while " << condition << intArg << "\r\n";
            body();
            indent(indentation) << "end_while\r\n";
        }
        void while_int(const char* condition, int intArg, int indentation = 0)
        {
            indent(indentation) << "while " << condition << intArg << "\r\n";
            indent(indentation) << "end_while\r\n";
        }
        template<class LambdaFunc>
        void while_(const char* condition, LambdaFunc body, int indentation = 0)
        {
            indent(indentation) << "while " << condition << "\r\n";
            body();
            indent(indentation) << "end_while\r\n";
        }
        void while_(const char* condition, int indentation = 0)
        {
            indent(indentation) << "while " << condition << "\r\n";
            indent(indentation) << "end_while\r\n";	
        }
        void while_fmt(int indentation, const char* condition, ...)
        {
            indent(indentation) << "while ";
            va_list ap;
            va_start(ap, condition);
            //data.writevf(condition, ap) << "\r\n";
            indent(indentation) << "end_while\r\n";
        }
    };


    /**
     * @brief This class collects script trigger and advice data
     *        And can be used to write new EDA and DA files
     */
    struct ScriptWriter
    {
        string Mod;       // mod folder name only (ex: "RTR")
        string SourceEDA; // source Export_Descr_Advice.txt
        string SourceEA;  // source Descr_Advice.txt

        rpp::string_buffer AdviceThreads;
        rpp::string_buffer Triggers;
        rpp::string_buffer DescrAdvice;

        std::vector<Script> Scripts; // all the generated scripts

        /**
         * @param	srcEDAPath	Full path to source (clean) export_descr_advice.txt
         * @param	srcEAPath 	Full path to source (clean) export_advice.txt
         */
        ScriptWriter(string mod, string srcEDAPath, string srcEAPath)
            : Mod{mod}, SourceEDA{srcEDAPath}, SourceEA{srcEAPath}
        {
        }

        /**
         * @brief	Generates new script data from the specified source files
         * @param	params	Options for controlling the script generation.
         */
        void AddScript(const ScriptParams& params);

        /**
         * @brief	Writes all collected script data to RTR/data/
         * @param	dataPath  	Full path to RTR/data
         */
        void WriteScriptData(const string& dataPath);

    private:
        void AddDescr(const ScriptParams& params);
    };
}
