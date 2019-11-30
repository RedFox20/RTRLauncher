#pragma once
/**
 * Some useful data type definitions reverse engineered from RomeTW-ALX engine
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // typedefs


struct CString
{
  char *str;
  int len;
};

struct WStrPoolNode
{
  WORD word0;
  WORD oldlength;
  WORD length;
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
  DWORD religionID;
  DWORD value;
};

struct CampaignStuff1
{
  DWORD unk0;
  int*  intArr8;
  DWORD capacity;
  DWORD count;
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
  DWORD dword18;
  BYTE f1C[8];
  CString regionDescr;
  RegionStuff1 f2C[5];
  RegionStuff1 rs68;
  RegionStuff1 f74[5];
  DWORD dwordB0;
  DWORD dwordB4;
  DWORD dwordB8;
  DWORD dwordBC;
  DWORD dwordC0;
  DWORD dwordC4;
  DWORD dwordC8;
  DWORD dwordCC;
  DWORD dwordD0;
  DWORD dwordD4;
  RegionReligionVal beliefs[10];
  BYTE fUnknown[80];
  DWORD dword178;
  DWORD dword17C;
  BYTE f180[4];
  DWORD f188count;
  int f188[3];
  int xCoord;
  int yCoord;
  int xPort;
  int yPort;
  WORD colorBG;
  BYTE colorR;
  BYTE f1A7[13];
  WString* wRegionDescr;
  char char1B8;
  BYTE f1B9[3];
  char char1BC;
  BYTE f1BD[3];
  CString string1C0;
  char colors[4];
  DWORD triumphLevel;
  DWORD unk1;
  DWORD unk2;
};
#pragma pack(pop)



#pragma pack(push, 1)
struct CampaignState
{
	BYTE colors[3];
	BYTE unk3;
	WString *campaignName;
	WString *campaignPath;
	int f4[7];
	void *something;
	int dword2C;
	int dword30;
	BYTE f34[20];
	CampaignStuff6 *f48;
	RegionStuff1 rest[3];
	CampaignRegion regions[200];
	DWORD regionCount;
	CampaignStuff1 rest2[20];
	BYTE rest22[16];
	CampaignStuff2 rest3[100];
	CampaignStuff3 stuff3;
	BYTE stuff5Count;
	BYTE _stuff5Count[3];
	CampaignStuff5 stuff5[100];
	BYTE stuff6[52];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CampaignSettings
{
  char char0;
  BYTE f1[3];
  DWORD dword4;
  BYTE byte8;
  BYTE f9[3];
  DWORD dwordC;
  BYTE f10[168];
  DWORD dwordB8;
  BYTE fBC[340];
  DWORD dword210;
  DWORD dword214;
  BYTE f218[28];
  char char234;
  BYTE f235[3];
  BYTE byte238;
  BYTE f239[111];
  char char2A8;
  BYTE f2A9[3];
  char char2AC;
  BYTE f2AD[3];
  char char2B0;
  BYTE f2B1[3];
  char char2B4;
  BYTE f2B5[7];
  DWORD dword2BC;
  DWORD dword2C0;
  DWORD dword2C4;
  BYTE f2C8[2];
  BYTE bMarianReformsActivated;
  BYTE bMarianReformsDisabled;
  BYTE bRebellingCharactersActive;
  BYTE bDisableGladiatorUprising;
  BYTE bNightBattlesEnabled;
  BYTE f2CF[1];
  float brigand_spawn_value;
  float pirate_spawn_value;
  BYTE f2D8[1];
  BYTE bShowDateAsTurnsRemaining;
  BYTE f2DA[2];
  DWORD dword2DC;
  BYTE f2E0[4];
  DWORD dword2E4;
  BYTE f2E8[600];
  char char540;
  BYTE f541[51267];
  WString someString;
  char rest[4];
};
#pragma pack(pop)


#pragma pack(push, 1)
struct GameState
{
  DWORD unk1;
  CampaignSettings cmpgn;
  CampaignState cmpgnState;
  char restUnk[197536];
};
#pragma pack(pop)
