#include "RomeAPI.h"
#include <stdexcept>

static RomeExeVersion Version;

RTWGameSettings* GetSettingsPointer(RomeExeVersion version)
{
    switch (version)
    {
        default:
        case RomeTW_1_5:  return *reinterpret_cast<RTWGameSettings**>(0x273845C);
        case RomeBI_1_5:  return *reinterpret_cast<RTWGameSettings**>(0x273845C);
        case RomeALX_1_9: return *reinterpret_cast<RTWGameSettings**>(0x273845C);
        case RomeALX_1_9_1: return *reinterpret_cast<RTWGameSettings**>(0x273845C);
    }
}

void RTW::Initialize(RomeExeVersion version)
{
    Version = version;
}

RTWGameSettings& RTW::Settings()
{
    RTWGameSettings* settings = GetSettingsPointer(Version);
    if (!settings)
    {
        throw std::runtime_error{"RTW::Settings is null: RomeTW has not initialized settings yet"};
    }
    return *settings;
}

float RTW::UnitScale()
{
    return Settings().unit_scale;
}

void RTW::UnitScale(float scale)
{
    Settings().unit_scale = scale;
}

