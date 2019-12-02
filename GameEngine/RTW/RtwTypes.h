#pragma once
/**
 * Some useful data type definitions reverse engineered from RomeTW-ALX engine
 */
#include <cstdint>

struct CString
{
    char *str;
    int len;
};

struct WStrPoolNode
{
    int16_t word0;
    int16_t oldlength;
    int16_t length;
    wchar_t buffer[1]; // start of inline buffer
};

struct WString
{
    WStrPoolNode* ptr;
};



#pragma pack(push, 1)
struct ParsedRegion
{
    CString Name;
    char colorB;
    char colorG;
    char colorR;
    char charB;
    char charC[4];
    CString LegionName;
    CString SettlementName;
    int dword20;
    char* rebelFaction;
    int dword28;
    int dword2C;
    int dword30;
    int dword34;
    int dword38;
    int dword3C;
    int dword40;
    int dword44;
    int dword48;
    int dword4C;
    int beliefs[10];
};
#pragma pack(pop)


struct RegionReligionVal
{
    uint32_t religionID;
    uint32_t value;
};

struct CampaignStuff1
{
    uint32_t unk0;
    int*  intArr8;
    uint32_t capacity;
    uint32_t count;
};
struct CampaignStuff2
{
    CString string0;
    int dword8;
    int dwordC;
    int dword10;
};
struct CampaignStuff4
{
    int dword0;
    int dword4;
};
struct CampaignStuff3
{
    int someCount;
    int capacity;
    CampaignStuff4* items;
    int dwordC;
    int dword10;
    int dword14;
};
struct CampaignStuff5
{
    int dword0;
    int dword4;
};
struct CampaignStuff6
{
    int dword0;
    int dword4;
    int dword8;
    int dwordC;
    int dword10;
    int dword14;
    int dword18;
    int dword1C;
    int dword20;
    int dword24;
    int dword28;
    int dword2C;
    int dword30;
};


struct RegionStuff1
{
    int*intArr8;
    int capacity;
    int count;
};



#pragma pack(push, 1)
struct CampaignRegion
{
    CString Name;
    CString settlementName;
    CString legionName;
    uint32_t dword18;
    uint8_t f1C[8];
    CString regionDescr;
    RegionStuff1 f2C[5];
    RegionStuff1 rs68;
    RegionStuff1 f74[5];
    uint32_t dwordB0;
    uint32_t dwordB4;
    uint32_t dwordB8;
    uint32_t dwordBC;
    uint32_t dwordC0;
    uint32_t dwordC4;
    uint32_t dwordC8;
    uint32_t dwordCC;
    uint32_t dwordD0;
    uint32_t dwordD4;
    RegionReligionVal beliefs[10];
    uint8_t fUnknown[80];
    uint32_t dword178;
    uint32_t dword17C;
    uint8_t f180[4];
    uint32_t f188count;
    int f188[3];
    int xCoord;
    int yCoord;
    int xPort;
    int yPort;
    int16_t colorBG;
    uint8_t colorR;
    uint8_t f1A7[13];
    WString* wRegionDescr;
    char char1B8;
    uint8_t f1B9[3];
    char char1BC;
    uint8_t f1BD[3];
    CString string1C0;
    char colors[4];
    uint32_t triumphLevel;
    uint32_t unk1;
    uint32_t unk2;
};
#pragma pack(pop)



#pragma pack(push, 1)
struct CampaignState
{
    uint8_t colors[3];
    uint8_t unk3;
    WString *campaignName;
    WString *campaignPath;
    int f4[7];
    void *something;
    int dword2C;
    int dword30;
    uint8_t  f34[20];
    CampaignStuff6 *f48;
    RegionStuff1 rest[3];
    CampaignRegion regions[200];
    uint32_t regionCount;
    CampaignStuff1 rest2[20];
    uint8_t rest22[16];
    CampaignStuff2 rest3[100];
    CampaignStuff3 stuff3;
    uint8_t stuff5Count;
    uint8_t _stuff5Count[3];
    CampaignStuff5 stuff5[100];
    uint8_t stuff6[52];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CampaignSettings
{
    char char0;
    uint8_t f1[3];
    uint32_t dword4;
    uint8_t byte8;
    uint8_t f9[3];
    uint32_t dwordC;
    uint8_t f10[168];
    uint32_t dwordB8;
    uint8_t fBC[340];
    uint32_t dword210;
    uint32_t dword214;
    uint8_t f218[28];
    char char234;
    uint8_t f235[3];
    uint8_t byte238;
    uint8_t f239[111];
    char char2A8;
    uint8_t f2A9[3];
    char char2AC;
    uint8_t f2AD[3];
    char char2B0;
    uint8_t f2B1[3];
    char char2B4;
    uint8_t f2B5[7];
    uint32_t dword2BC;
    uint32_t dword2C0;
    uint32_t dword2C4;
    uint8_t f2C8[2];
    uint8_t bMarianReformsActivated;
    uint8_t bMarianReformsDisabled;
    uint8_t bRebellingCharactersActive;
    uint8_t bDisableGladiatorUprising;
    uint8_t bNightBattlesEnabled;
    uint8_t f2CF[1];
    float brigand_spawn_value;
    float pirate_spawn_value;
    uint8_t f2D8[1];
    uint8_t bShowDateAsTurnsRemaining;
    uint8_t f2DA[2];
    uint32_t dword2DC;
    uint8_t f2E0[4];
    uint32_t dword2E4;
    uint8_t f2E8[600];
    char char540;
    uint8_t f541[51267];
    WString someString;
    char rest[4];
};
#pragma pack(pop)


#pragma pack(push, 1)
struct GameState
{
    uint32_t unk1;
    CampaignSettings cmpgn;
    CampaignState cmpgnState;
    char restUnk[197536];
};
#pragma pack(pop)
