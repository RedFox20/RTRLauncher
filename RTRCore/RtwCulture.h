#pragma once
#include <rpp/strview.h>

namespace rtw
{
    using rpp::strview;
    
    struct Culture
    {
        strview name;                 // roman
        strview portrait_mapping;     // roman
        int rebel_standard_index = 0; // 0

        Culture(strview cultureName) : name{cultureName} {}
    };


    Culture* get_culture(strview factionName);
    Culture* get_culture(int index);
    int num_cultures();

    bool load_cultures(strview descr_cultures);
}
