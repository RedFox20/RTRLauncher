#pragma once

/**
 * Specific Rome EXE version
 * For each version, addresses are different
 */
enum RomeExeVersion
{
    RomeTW_1_5,   // RTW 1.5
    RomeTW_1_5_1, // RomeTW Gold (Steam)
    RomeBI_1_6,   // BI 1.6
    RomeBI_1_6_1, // RomeTW Gold BI (Steam)
    RomeALX_1_9,  // ALX 1.9
    RomeALX_1_9_1, // ALX Steam
};

/**
 * Some game settings struct for RomeTW,
 * @todo The layout might be different for different RomeExeVersion's
 */
struct RTWGameSettings
{
    char some_variables[0x2A14];
    float unit_scale;
};

/**
 * This is the reverse engineered global RomeTW API
 */
struct RTW
{
    static void Initialize(RomeExeVersion version);
    static RTWGameSettings& Settings();

    static float UnitScale();
    static void UnitScale(float scale);
};

