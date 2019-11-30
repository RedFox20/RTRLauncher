#pragma once
#include <rpp/strview.h>
#include <unordered_map>

namespace rtw
{
    using rpp::strview;

    enum ETextDictionary
    {
        TD_ExportUnits,
        TD_Invalid,
    };

    struct Dictionary
    {
        std::string TokenData; // buffer for permanently storing the UTF8 source buffer
        std::unordered_map<strview, strview> Dict;

        strview operator[](strview key) const
        {
            auto it = Dict.find(key);
            return it == Dict.end() ? strview{} : it->second;
        }
    };

    const Dictionary& get_dict(ETextDictionary id);

    bool load_dict(ETextDictionary id, strview exported_dictionary_file);
}
