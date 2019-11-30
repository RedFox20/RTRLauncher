#include "RtwDictionary.h"
#include <rpp/file_io.h>
#include <utf8.h>

namespace rtw
{
    static Dictionary Dicts[TD_Invalid];


    const Dictionary& get_dict(ETextDictionary id)
    {
        return Dicts[id];
    }

    bool load_dict(ETextDictionary id, strview exported_dictionary_file)
    {
        const char* secOK = "[+] Dict Loader [+]";
        const char* secFF = "[!] Dict Loader [!]";

        int linenum = 0;
        Dictionary& d = Dicts[id];
        rpp::load_buffer wbuff = rpp::file::read_all(exported_dictionary_file);
        if (!wbuff)
            return false;

        // convert UCS2 file to UTF8
        d.TokenData = utf8_convert((wchar_t*)wbuff.data() + 1, wbuff.size() - sizeof(wchar_t));

        strview line;
        rpp::line_parser parser = d.TokenData;
        while (parser.read_line(line))
        {
            ++linenum;
        }
        return true;
    }

}
