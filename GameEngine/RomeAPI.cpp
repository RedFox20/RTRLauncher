#include "RomeAPI.h"

struct GameSettings
{
    char some_variables[0x2A14];
    float unit_scale;
};

GameSettings& settings() { return **(GameSettings**)(0x273845C); }

float RTW::UnitScale()
{
    return settings().unit_scale;
}
void RTW::UnitScale(float scale)
{
    settings().unit_scale = scale;
}

