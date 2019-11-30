#include "RtwCulture.h"
#include <rpp/file_io.h>

namespace rtw
{

    static std::vector<Culture> Cultures;
    static rpp::load_buffer DC_TokenData; // keep token data in memory


    Culture* get_culture(strview factionName)
    {
        for (Culture& f : Cultures)
            if (f.name == factionName)
                return &f;
        return nullptr;
    }
    Culture* get_culture(int index)
    {
        if (index < 0 || index >= (int)Cultures.size())
            return nullptr;
        return &Cultures[index];
    }
    int num_cultures()
    {
        return (int)Cultures.size();
    }


    bool load_cultures(strview descr_cultures)
    {
        const char* secOK = "[+] DC Loader [+]";
        const char* secFF = "[!] DC Loader [!]";

        Cultures.clear();
        int linenum = 0;
        Culture* c  = 0;
        
        if (!(DC_TokenData = rpp::file::read_all(descr_cultures)))
            return false;

        strview line;
        rpp::line_parser parser = DC_TokenData;
        while (parser.read_line(line))
        {
            ++linenum;
            if (line[0] == ';' || line.is_whitespace())
                continue;

            strview id = line.next(" \t");
            if (id == "culture")
            {
                Cultures.emplace_back(line.next(" \t;"));
                c = &Cultures.back();
            }
            else if (id == "portrait_mapping")     c->portrait_mapping = line.next(" \t;");
            else if (id == "rebel_standard_index") c->rebel_standard_index = line.next_int();
        }
        return true;
    }
    
}
