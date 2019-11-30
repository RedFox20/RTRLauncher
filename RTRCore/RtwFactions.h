#pragma once
#include "RtwCulture.h"

namespace rtw
{
    struct Faction
    {
        strview name;          // romans_julii
        Culture* culture;    // roman
        strview symbol;        // models_strat/symbol_julii.CAS
        strview rebel_symbol;  // models_strat/symbol_slaves.CAS
        unsigned char primary_colour[4];   // red 114, green 10, blue 11
        unsigned char secondary_colour[4]; // red 114, green 10, blue 11
        strview loading_logo;   // loading_screen/symbols/symbol128_julii.tga
        int standard_index;   // 0
        int logo_index;       // 23
        int small_logo_index; // 248
        int triumph_value;    // 5
        strview intro_movie;    // fmv/intros/julii_intro_final.wmv
        strview victory_movie;  // fmv/victory/julii_outro_320x240.wmv
        strview defeat_movie;   // fmv/lose/julii_eliminated.wmv
        strview death_movie;    // fmv/death/death_julii_grass_320x240.wmv
        bool custom_battle_availability; // yes
        bool can_sap;                    // yes
        bool prefers_naval_invasions;    // no

        Faction(strview factionName) : name(factionName), culture(0) {}
    };

    Faction* get_faction(strview factionName);
    Faction* get_faction(int index);
    int num_factions();

    bool load_factions(strview descr_sm_factions);

}
