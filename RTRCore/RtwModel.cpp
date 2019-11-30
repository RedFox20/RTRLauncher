#include "RtwModel.h"
#include <rpp/file_io.h>
#include <log.h>

namespace rtw
{
    static int ModelsCount = 0;
    static ModelSoldier Models[500];
    static rpp::load_buffer DMB_TokenData; // keep token data in memory

    ModelSoldier* rtw_find_model(strview type)
    {
        int count = ModelsCount;
        for (int i = 0; i < count; ++i)
            if (Models[i].type == type)
                return &Models[i];
        return nullptr;
    }

    void ModelSoldier::init(strview _typ)
    {
        type = _typ;
        skeleton.clear();
        skele_horse.clear();
        skele_elephant.clear();
        skele_camel.clear();
        skele_chariot.clear();
        scale = 1.0f;
        indiv_range = 40;
        models.clear();
        textures.clear();
        sprites.clear();
    }

    bool load_models(strview descr_model_battle)
    {
        const char* secOK = "[+] DMB Loader [+]";
        const char* secFF = "[!] DMB Loader [!]";

        ModelsCount = 0;
        int linenum = 0;
        ModelSoldier* ms = 0;
        
        logsec(secOK, "Opening descr_model_battle.txt ");
        if (!(DMB_TokenData = rpp::file::read_all(descr_model_battle))) {
            log("failed!\n");
            return false;
        }
        log("success\n");

        strview line;
        rpp::line_parser parser = DMB_TokenData;
        while (parser.read_line(line))
        {
            ++linenum;
            if (line[0] == ';' || line.is_whitespace())
                continue;

            strview id     = line.next(" \t");
            strview param1 = line.next(" \t,;");
            if (id == "type")
            {
                if (ModelsCount == 500)
                {
                    logsec(secFF, "DMB Model Limit (500) reached. DMB load failed.\n");
                    return false;
                }
                (ms = &Models[ModelsCount++])->init(param1);
            }
            else if (id == "skeleton")         ms->skeleton      = param1;
            else if (id == "skeleton_horse")   ms->skele_horse   = param1;
            else if (id == "skeleton_camel")   ms->skele_camel   = param1;
            else if (id == "skeleton_chariot") ms->skele_chariot = param1;
            else if (id == "scale")            ms->scale = param1.to_float();
            else if (id == "indiv_range")      ms->indiv_range = param1.to_int();
            else if (id == "texture")
            {
                ModelTexture texture;
                texture.faction = param1;
                texture.path = line.next(" \t;");
                ms->textures.emplace_back(texture);
            }
            else if (id.starts_with("model_"))
            {
                strview param2 = line.next(" \t;");
                short lodrange = param2 == "max" ? 0 : param2.to_int();
                ModelType mtype;
                if (id == "model_flexi")        mtype = MT_Flexi;
                else if (id == "model_flexi_m") mtype = MT_Flexi_M;
                else if (id == "model_flexi_c") mtype = MT_Flexi_C;
                else                            mtype = MT_Mesh;
                ms->models.emplace_back(param1, lodrange, mtype);
            }
            else if (id == "model_sprite")
            {
                ModelSprite sprite;
                char ch = param1[0];
                if ('0' <= ch && ch <= '9') { // numeric?
                    sprite.lod = param1.next_float();
                } else {
                    sprite.faction = param1;
                    sprite.lod     = line.next_float();
                }
                sprite.path = line.next(" \t,;");
                ms->sprites.push_back(sprite);
            }
            else if (id == "model_tri")
            {
                ms->tri.range = param1.next_int();
                ms->tri.r = line.next_float();
                ms->tri.g = line.next_float();
                ms->tri.b = line.next_float();
            }
        }

        logsec(secOK, "Successfully loaded %d models\n", ModelsCount);
        return true;
    }
   
}