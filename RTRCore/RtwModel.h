#pragma once
#include <rpp/strview.h>
#include <vector>

namespace rtw
{
    using rpp::strview;
    using std::vector;
    
    enum ModelType : short
    {
        MT_Flexi,   // model_flexi
        MT_Flexi_M, // model_flexi_m
        MT_Flexi_C, // model_flexi_c
        MT_Mesh,    // model_mesh (never used)
    };

    struct ModelFlexi
    {
        strview path;     // data/models_unit/east_female_peasant_300.cas
        short lod;      // if max range then lod=0, otherwise lod > 0
        ModelType type; // MT_Flexi etc.

        inline ModelFlexi(strview p, short l, ModelType t) 
            : path{p}, lod(l), type(t) {}
    };

    struct ModelSprite
    {
        strview faction; // gauls
        strview path;    // RTR/data/sprites/gauls_celt_standard_sprite.spr
        float lod;     // 60.0
    };

    struct ModelTexture
    {
        strview faction; // gauls
        strview path;    // RTR/data/models_unit/textures/officer_barb_generale_gaul.tga
    };

    struct ModelTri
    {
        int range;     // 400
        float r, g, b; // 0.5f, 0.5f, 0.5f
    };

    struct ModelSoldier
    {
        strview type;           // celt_general
        strview skeleton;       // fs_s1_swordsman
        strview skele_horse;    // fs_prome_hc_swordsman
        strview skele_elephant; // fs_forest_elepehant_rider
        strview skele_chariot;  // fs_chariot_sword
        strview skele_camel;    // fs_prome_hc_swordsman
        float scale;          // (optional) default: 1.0f 
        int   indiv_range;    // 40 - individual render range
        
        vector<ModelTexture> textures; //
        vector<ModelFlexi>   models;   //
        vector<ModelSprite>  sprites;  //
        ModelTri             tri;      // 400, 0.5f, 0.5f, 0.5f

        void init(strview type);
    };



    /**
     * @brief Performs linear search to find a model entry
     * @param type Name of the model to find
     * @return Pointer to model or NULL if model not found
     */
    ModelSoldier* find_model(strview type);

    /**
     * Resets the global DescrModelBattle table and loads new entries
     * from descr_model_battle.txt
     * @param descr_model_battle Path to descr_model_battle.txt
     * @return TRUE on success, FALSE if critical parse error encountered
     */
    bool load_models(strview descr_model_battle);
}
