#pragma once
#include "RtwModel.h"
#include "RtwFactions.h"

namespace rtw
{
    enum EUnitCategory : unsigned int
    {
        UCat_Invalid,
        UCat_Infantry,
        UCat_Cavalry,
        UCat_Siege,
        UCat_Handler,
        UCat_Ship,
        UCat_NonCombatant
    };
    enum EUnitClass : unsigned int
    {
        UCls_Light,
        UCls_Heavy,
        UCls_Spearman,
        UCls_Missile,
    };
    enum EUnitAttributes : unsigned int // attributes
    {
        UAtt_None               = 0,
        UAtt_Sap                = 1 << 0,  // can_sap
        UAtt_Withdraw           = 1 << 1,  // can_withdraw
        UAtt_FormedCharge       = 1 << 2,  // can_formed_charge
        UAtt_FeignRout          = 1 << 3,  // can_feign_rout
        UAtt_RunAmok            = 1 << 4,  // can_run_amok
        UAtt_HideForest         = 1 << 5,  // hide_forest
        UAtt_HideImprovedForest = 1 << 6,  // hide_improved_forest
        UAtt_HideLongGrass      = 1 << 7,  // hide_long_grass
        UAtt_HideAnywhere       = 1 << 8,  // hide_anywhere
        UAtt_SeaFaring          = 1 << 9,  // sea_faring
        UAtt_Command            = 1 << 10, // command
        UAtt_Heavy              = 1 << 11, // heavy
        UAtt_Hardy              = 1 << 12, // hardy
        UAtt_VeryHardy          = 1 << 13, // very_hardy
        UAtt_PowerCharge        = 1 << 14, // power_charge
        UAtt_Slave              = 1 << 15, // slave
        UAtt_FrightenFoot       = 1 << 16, // frighten_foot
        UAtt_FrightenMounted    = 1 << 17, // frighten_mounted
        UAtt_MercenaryUnit      = 1 << 18, // mercenary_unit
        UAtt_NoCustom           = 1 << 19, // no_custom
        UAtt_IsPeasant          = 1 << 20, // is_peasant
        UAtt_GeneralUnit        = 1 << 21, // general_unit
        UAtt_GeneralUnitUpgrade = 1 << 22, // general_unit_upgrade
        UAtt_CantabrianCircle   = 1 << 23, // cantabrian_circle
        UAtt_Druid              = 1 << 24, // druid
        UAtt_ScreechingWomen    = 1 << 25, // screeching_women
        UAtt_Warcy              = 1 << 26, // warcry
        UAtt_LegionaryName      = 1 << 27, // legionary_name
        UAtt_CanSwim            = 1 << 28, // can_swim
        UAtt_CanHorde           = 1 << 29, // can_horde
    };
    enum EUnitFormation : unsigned int
    {
        UFrm_Horde,
        UFrm_Column,
        UFrm_Square,
        UFrm_Wedge,
        UFrm_SquareHollow,
        UFrm_Testudo,
        UFrm_Phalanx,
        UFrm_Schiltrom,
        UFrm_ShieldWall,
        UFrm_Square2,
        UFrm_Unknown,
    };
    enum EUnitAbility : unsigned int
    {
        UAbi_Unknown,
        UAbi_Testudo,
        UAbi_Phalanx,
        UAbi_Wedge,
        UAbi_DropEngines,
        UAbi_FlamingAmmo,
        UAbi_WarCry,
        UAbi_Chant,
        UAbi_Curse,
        UAbi_Beserk,
        UAbi_Rally,
        UAbi_KillElephants,
        UAbi_MoveAndShoot,
        UAbi_CantabrianCircle,
        UAbi_ShieldWall,
        UAbi_Stealth,
        UAbi_FeignedRout,
        UAbi_Schiltrom,
    };
    enum EUnitMissile : unsigned int
    {
        UMis_None,          // no
        UMis_Arrow,         // arrow
        UMis_Javelin,       // javelin
        UMis_Bullet,        // bullet
        UMis_Stone,         // stone
        UMis_Pilum,         // pilum
        UMis_BambooJavelin, // bamboo_javelin
    };
    enum EUnitWeapon : unsigned int
    {
        UWep_Melee,        // melee
        UWep_Thrown,       // thrown
        UWep_Missile,      // missile
        UWep_SiegeMissile, // siege_missile
    };
    enum EUnitTech : unsigned int
    {
        UTec_Simple,    // simple
        UTec_Blade,     // blade
        UTec_Archery,   // archery
        UTec_Siege,     // siege
        UTec_Other,     // other (elephants only)
    };
    enum EUnitDamage : unsigned int
    {
        UDam_Piercing,  // piercing
        UDam_Blunt,     // blunt
        UDam_Slashing,  // slashing
        UDam_Fire,      // fire (probably not used?)
    };
    enum EUnitSound : unsigned int
    {
        USnd_None,      // none
        USnd_Knife,     // knife
        USnd_Mace,      // mace
        USnd_Axe,       // axe
        USnd_Sword,     // sword
        USnd_Spear,     // spear
    };
    enum EUnitWeaponAttr : unsigned int
    {
        UWepAtt_None       = 0,    // no
        UWepAtt_Spear      = 1<<0, // spear
        UWepAtt_LightSpear = 1<<1, // light_spear
        UWepAtt_Prec       = 1<<2, // prec (precursor weapon like pilum)
        UWepAtt_Ap         = 1<<3, // ap (armor piercing - axe type weapons)
        UWepAtt_Bp         = 1<<4, // bp (body piercing - ballistas etc)
        UWepAtt_Area       = 1<<5, // area (area of effect attack)
        UWepAtt_Fire       = 1<<6, // fire (fire type attack)
        UWepAtt_Launching  = 1<<7, // launching (attack launches other units)
        UWepAtt_Thrown     = 1<<8, // thrown (attack is thrown at enemies)
        UWepAtt_ShortPike  = 1<<9, // short_pike
        UWepAtt_LongPike   = 1<<10,// long_pike
        UWepAtt_SpearBonus12 = 1<<11, // spear_bonus_12
        UWepAtt_SpearBonus10 = 1<<12, // spear_bonus_10
        UWepAtt_SpearBonus8  = 1<<13, // spear_bonus_8
        UWepAtt_SpearBonus6  = 1<<14, // spear_bonus_6
        UWepAtt_SpearBonus4  = 1<<15, // spear_bonus_4
    };
    enum EUnitArmorSound : unsigned int
    {
        UArmSnd_Flesh,   // flesh
        UArmSnd_Leather, // leather
        UArmSnd_Metal,   // metal
    };
    enum EUnitDiscipline : unsigned int
    {
        UDis_Normal,      // normal (default) discipline level
        UDis_Low,         // low (I guess they don't listen much?)
        UDis_Disciplined, // disciplined (orderly)
        UDis_Impetuous,   // impetuous (may charge without orders)
        UDis_Berserker,   // berserker (goes berserk)
    };
    enum EUnitTraining : unsigned int // unit training level - decides how tidy a formation is
    {
        UTra_Untrained,     // untrained
        UTra_Trained,       // trained
        UTra_HighlyTrained, // highly_trained
    };

    struct UnitSoldier
    {
        ModelSoldier* model; // DMB unit model
        int soldiersCount;   // number of ordinary soldiers
        int extrasCount;     // # of pigs, dogs, elephants, chariots, artillery pieces, etc.
        float collisionMass; // collision mass of men, 1.0 is normal
    };
    struct UnitFormation
    {
        float closeWidth, closeDepth; // close width & depth in meters
        float looseWidth, looseDepth; // loose width & depth in meters
        int numRanks;                 // number of default ranks for the unit
        EUnitFormation mainFormation; // square, horde, etc.
        EUnitFormation specialFormation; // phalanx, wedge, etc.
    };
    struct UnitAttack
    {
        int attack;           // attack factor
        int charge;           // attack bonus while charging
        EUnitMissile missile; // missile weapon type
        float range;          // range of missile (in meters)
        int ammo;             // missile ammunition per soldier
        EUnitWeapon weapon;   // Weapon type
        EUnitTech tech;       // Tech type
        EUnitDamage damage;   // Damage type
        EUnitSound sound;     // Sound type when weapon hits
        int delay;            // delay between attacks in 1/10th second increments
        float lethality;      // attack lethality modifier
    };
    struct UnitArmour
    {
        int armour;  // generic protection factor
        int defense; // defensive skill (not used when shot at)
        int shield;  // shield factor (only used for attack from front or left)
        EUnitArmorSound sound; // sound type when hit
    };
    struct UnitGroundModifier
    {
        int scrub;  // first ground stat
        int sand;   // second ground stat
        int forest; // third ground stat
        int snow;   // fourth ground stat
    };
    struct UnitMental
    {
        int morale; // base morale
        EUnitDiscipline discipline; // unit discipline
        EUnitTraining training;     // unit training level
    };
    struct UnitCost
    {
        int buildTime;   // number of turns to train
        int recruitCost; // recruit cost in campaigns
        int upkeepCost;  // upkeep cost in campaigns
        int weaponCost;  // cost for upgrading weapons in battle/campaign
        int armourCost;  // cost for upgrading armour in battle/campaign
        int battleCost;  // recruit cost for custom battles
    };
    struct UnitOwnership
    {
        vector<Faction*> factions; // factions that own this unit
        vector<Culture*> cultures; // cultures that own this unit
    };

    struct Unit
    {
        strview type;            // barb peasant
        strview dictionary;      // barb_peasant
        EUnitCategory category;  // infantry
        EUnitClass    unitClass; // light
        strview voice_type;      // Light_1

        UnitSoldier soldier;     // barb_peasant, 60, 0, 0.7
        EUnitAttributes attrs;   // attributes  very_hardy, etc..
        UnitFormation formation; // formation closeW,closeD, looseW,looseD, ranks, form1 (,form2)

        struct { int soldierHP, mountHP; } health;
        UnitAttack priAttack;
        EUnitWeaponAttr priAttr;
        UnitAttack secAttack;
        EUnitWeaponAttr secAttr;
        UnitArmour priArmour;
        UnitArmour secArmour;
        int heat;                  // 2 -- extra fatigue suffered by unit in hot climates
        UnitGroundModifier ground; // stat_ground
        UnitMental mental;         // stat_mental
        int chargeDist;            // charge distance for the unit
        int fireDelay;             // firing delay between shots
        struct { int food1, food2; } food;
        UnitCost cost;             // cost stats of the unit
        UnitOwnership ownership;   // unit ownership values
    };

    Unit* get_unit(strview unitName);
    Unit* get_unit(int index);
    int num_units();
    bool load_units(strview export_descr_units);
}
