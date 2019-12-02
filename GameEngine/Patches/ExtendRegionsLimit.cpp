#include "ExtendRegionsLimit.h"
#include <distorm\distorm.h>
#include <ASM.h>
#include <RTW/RtwTypes.h>
#include <log.h>

ParsedRegion g_ParserRegions[1092]; // just .bss this array
CampaignState g_CampaignState;

static const int MaxInst = 1000;
static _DInst Inst[MaxInst];

void PrintInst(_CodeInfo& ci, _DInst& inst)
{
    _DecodedInst d;
    distorm_format32(&ci, &inst, &d);
    log("%08x %02d %-24s %s %s\n", ci.code, d.size, 
        d.instructionHex.p, d.mnemonic.p, d.operands.p);
}


static bool InstHasImm32(_DInst& inst)
{
    for (int i = 0; i < 4; ++i)
        if (inst.ops[i].type == O_IMM && inst.ops[i].size == 32)
            return true;
    return false;
}

// @TODO: This doesn't quite work yet, some random crashes here and there
void ExtendRegionsLimit(RomeExeVersion ver, unlocked_section& code)
{
    return; // DISABLED FOR NOW

    log("    EXPAND    regions limit => 1092\n");
    log("       REMAP 012EA780 => %8X\n", g_ParserRegions);

    ParsedRegion* pRegionsArr = (ParsedRegion*)0x012EA780; // gRegionsArr
    UINT sequence = 0x00012EA7; // chop off 0x80

    // Functions where to look and replace the value:
    int repRegionsArr[][2] = {
        { 0x008B13A4, 0x008B1BDE }, // proc Parse_DescrRegions, endp Parse_DescrRegions
        { 0x008B1F24, 0x008B2029 }, // proc Initialize_RegionsArr
        { 0x008B2044, 0x008B213B }, // proc CheckRegion
        { 0x008B22B4, 0x008B2674 }, // sub_8B22B4
        { 0x008B26B0, 0x008B2952 }, // LoadRegionData
        { 0x008B216C, 0x008B222B }, // sub_8B216C  gRegionsArr_colorB
        { 0x008B222C, 0x008B22B0 }, // sub_8B222C  gRegionsArr_DefaultFactionId
    };

    int regionsArrCnt = 0;
    const int count = sizeof(repRegionsArr) / sizeof(repRegionsArr[0]);
    for (int i = 0; i < count; ++i)
    {
        BYTE* proc = (BYTE*)repRegionsArr[i][0];
        BYTE* endp = (BYTE*)repRegionsArr[i][1];
        _CodeInfo ci = { 0 };
        ci.code    = proc;
        ci.codeLen = endp - proc;
        ci.dt      = Decode32Bits;
        for (;;)
        {
            uint numDecoded;
            _DecodeResult result = distorm_decompose32(&ci, Inst, MaxInst, &numDecoded);

            // process all decoded instructions and look out for MOV or LEA
            for (uint i = 0; i < numDecoded; ++i)
            {
                _DInst& inst = Inst[i];
                if (inst.flags == FLAG_NOT_DECODABLE)
                    continue; // skip undecodable instructions
        
                
                bool hasImm32 = InstHasImm32(inst);
                if (inst.dispSize == 32 || hasImm32)
                {
                    const uint Base = 0x012EA780; // g_RegionsArr
                    const uint Max  = 0x012EA7F8; // F8 is non-inclusive
                    if (inst.dispSize)
                    {
                        uint disp = (uint)inst.disp;
                        if (Base <= disp && disp < Max)
                        {
                            BYTE* addr = (BYTE*)ci.code + inst.addr;
                            PrintInst(ci, inst);
                            ++regionsArrCnt;
                        }
                    }
                    if (hasImm32)
                    {
                        uint disp = inst.imm.dword;
                        if (Base <= disp && disp < Max)
                        {
                            BYTE* addr = (BYTE*)ci.code + inst.addr;
                            PrintInst(ci, inst);
                            ++regionsArrCnt;
                        }
                    }
                    
                }
            }

            if (result == DECRES_MEMORYERR) // we can decode more
            {
                ci.code	    = proc + ci.nextOffset;
                ci.codeLen -= ci.nextOffset;
                continue;
            }
            break;
        }
    }
    log("       Total %d redirects\n\n", regionsArrCnt);



    //log("       VirtualProtect old GameState.Campaign\n");
    //// VirtualProtect the old game state campaign offset
    //DWORD oldPrtct;
    //// 0x0165DF60 g_GlobalGameState
    ////            offset 0x0CD90 CampaignState
    //VirtualProtect((void*)(0x0165DF60 + 0x0CD90), sizeof(CampaignState), PAGE_NOACCESS, &oldPrtct);

}

