#include "RtwFactions.h"
#include <rpp/file_io.h>
#include <log.h>
#include <vector>

namespace rtw
{
    static std::vector<Faction> Factions;
    static rpp::load_buffer DSF_TokenData; // keep token data in memory


    Faction* rtw_get_faction(strview factionName)
    {
        for (Faction& f : Factions)
            if (f.name == factionName)
                return &f;
        return nullptr;
    }
    Faction* rtw_get_faction(int index)
    {
        if (index < 0 || index >= (int)Factions.size())
            return nullptr;
        return &Factions[index];
    }
    int rtw_num_factions()
    {
        return (int)Factions.size();
    }


    bool rtw_load_factions(strview descr_sm_factions)
    {
        const char* secOK = "[+] DSF Loader [+]";
        const char* secFF = "[!] DSF Loader [!]";

        Factions.clear();
        int linenum = 0;
        Faction* f  = 0;
        
        if (!(DSF_TokenData = rpp::file::read_all(descr_sm_factions)))
            return false;

        strview line;
        rpp::line_parser parser = DSF_TokenData;
        while (parser.read_line(line))
        {
            ++linenum;
            if (line[0] == ';' || line.is_whitespace())
                continue;

            strview id = line.next(" \t");
            strview params = line.next(";").trim();
            if (id == "faction")
            {
                Factions.emplace_back(params);
                f = &Factions.back();
            }
            else if (id == "culture")
            {
                if (Culture* c = get_culture(params))
                    f->culture = c;
                else
                    logsec(secFF, "Invalid culture '") << params << "'\n";
            }
        }
        return true;
    }
}
