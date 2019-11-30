#include "RomePatcher.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string.h>
#include <log.h>
#include <stdlib.h>
#include "Detours\detours.h"
#include "RtwTypes.h"
#include "ASM.h"
#include "distorm\distorm.h"
#include "distorm\mnemonics.h"

	// helper union for recasting function pointers
	template<class Func> union AnyFunc
	{
		Func  func;
		void* fptr;
		AnyFunc(Func fun) : func(fun) {}
		AnyFunc(int addr) : fptr((void*)addr) {}
	};




	RomePatcher::RomePatcher(const GameEngine& game) : Game(game)
	{
	}

	RomePatcher::~RomePatcher()
	{
	}

	extern void PrintVirtualMemory(DWORD baseAddress, DWORD regionSize);

	

	// create 6 byte asm sequence: push addr; ret; for absolute jump
	static void write_pushret(BYTE* dst, void (*nakedfunc)())
	{
		dst[0] = 0x68; // opcode PUSH imm32
		*(DWORD*)&dst[1] = (DWORD)nakedfunc; // imm32
		dst[5] = 0xC3; // opcode ret
	}

	static void write_patchcall(DWORD p, int padNops, void(*nakedfunc)())
	{
		memset(PBYTE(p), 0x90, padNops);
		write_pushret(PBYTE(p), nakedfunc);
	}

// performs an absolute jump to specified address
#define abs_jmp(ajmp) __asm{ push ajmp }__asm{ ret }
#define LBL1(X,Y) X##Y
#define LBL(X,Y) LBL1(X,Y)
// performs an absolute call to the specified address and reliably returns 
#define abs_call(acall) __asm{call LBL(lbl,__LINE__) } LBL(lbl,__LINE__): __asm{add dword ptr[esp],10}__asm{push acall}__asm{ret}

	static void __declspec(naked) UnitSize0_DivByZeroPatch()
	{
		__asm {
			test  ebp, ebp
			jnz   notzero
				mov   eax, -36   ;// hack
				abs_jmp(0x006A527C); // jump back and contiue
			notzero:
				idiv  ebp        ;// 2 bytes
				neg   eax        ;// 2 bytes
				add   eax, 64    ;// 3 bytes
				abs_jmp(0x006A527C); // jump back and contiue
		}
	}
	static void __declspec(naked) UnitCanSwim_EnablePatch()
	{
		__asm {
			test eax, eax
			jnz  begin_if ;// true?
				abs_jmp(0x008D43C9); // jump to next ELSEIF instead
			begin_if:
				mov    al, byte ptr[edi + 0x201] ;// a1 = unit->Attributes1;
				or     al, 0x40                  ;// a1 |= can_swim;
				mov    [edi + 0x200], al         ;// unit->Attributes1 = a1;
				abs_jmp(0x008D479D); // jump to end of the IF chain
		}
	}
	static void __declspec(naked) UnitCanHorde_EnablePatch()
	{
		__asm {
			test eax, eax
			jnz  begin_if ;// true?
				abs_jmp(0x008D43E0); // jump to next ELSEIF instead
			begin_if:
				mov    al, byte ptr[edi + 0x201] ;// a1 = unit->Attributes1;
				or     al, 0x80                  ;// a1 |= can_horde;
				mov    [edi + 0x200], al         ;// unit->Attributes1 = a1;
				abs_jmp(0x008D479D); // jump to end of the IF chain
		}
	}

	static void __declspec(naked) SmFactionsHorde_AllChecks() {
		#define DEFSTR(string) static const char string[] = #string;
		DEFSTR(horde_min_units)
		DEFSTR(horde_max_units)
		DEFSTR(horde_max_units_reduction_every_horde)
		DEFSTR(horde_unit_per_settlement_population)
		DEFSTR(horde_min_named_characters)
		DEFSTR(horde_max_percent_army_stack)
		DEFSTR(horde_disband_percent_on_settlement_capture)
		DEFSTR(horde_unit)
//	parser_skiptoken(parser)
//		lea   ecx, [esp + 0x170] // &parser
//		abs_call(0x00DBE9B4);    // int __thiscall Parser_SkipToken(ParseBuffer* this<ecx>)
//		test  al,al
#define parser_skiptoken() __asm{lea ecx,[esp+0x170]}abs_call(0x00DBE9B4);__asm{test al,al}
//	parser_matchtoken(parser, "str")
//		push  horde_min_units    // "horde_min_units"
//		lea   ecx, [esp + 0x170] // &parser
//		abs_call(0x00DBEDF0);    // int Parser_MatchToken(ParseBuffer* this<ecx>, char* str)
#define parser_matchtoken(str)__asm{push offset str}__asm{lea ecx,[esp+0x170]}abs_call(0x00DBEDF0)
//	if (parser_match(parser, "str")) parser_skiptoken(parser);
//		_parser_matchtoken
//		jz    if1
//			_parser_skiptoken();
//		if1:
#define parser_match(nextid,str) parser_matchtoken(str);__asm{jz LBL(if,nextid)}parser_skiptoken();LBL(if,nextid):
		__asm {
			parser_match(1,horde_min_units);
			parser_match(2,horde_max_units);
			parser_match(3,horde_max_units_reduction_every_horde);
			parser_match(4,horde_unit_per_settlement_population);
			parser_match(5,horde_min_named_characters);
			parser_match(6,horde_max_percent_army_stack);
			parser_match(7,horde_disband_percent_on_settlement_capture);
			_while: // while (Parser_MatchToken(parser, "horde_unit"))
				parser_matchtoken(horde_unit)
				jz  _ewhile
					parser_skiptoken();
				jmp _while
			_ewhile:
			abs_jmp(0x008AC794);   // return to: if (Parser_MatchToken(&parser, a2, "can_sap"))
		}
	}




	struct ParseBuffer
	{
		char* End;
		char* _Something;
		char* _Buffer;
		char* Ptr;

		char* Buffer1;
		char* End1;

		static bool is_endof_line(char ch)
		{
			return ch == '\n' || ch == ';';
		}
		static bool is_endof_token(char ch)
		{
			return ch != ',' && ch != '\t' && ch && ch != '\r' && is_endof_line(ch);
		}
		static bool is_delimiter(char ch)
		{
			return ch == ' ' || ch == '\t' || !ch || ch == '\r' || ch == ',' || ch == ':';
		}
		bool MatchToken(const char* str)
		{
			const char* str1 = Ptr;
			const char* str2 = str;
			while (*str2 && *str1 == *str2)
			{
				if (is_endof_line(*str1) || str1 >= End)
					return false;
				++str1;
				++str2;
			}
			return is_delimiter(*str1);
		}
		bool SkipLine()
		{
			int isSuccess; // eax@10
			char Char = *Ptr;
			while (Char == '\n' )
			{
			LABEL_6:
				++Ptr;
				Buffer1 = Ptr;
				++End1;
				if (is_delimiter(*Ptr))
				{
					while (Ptr < End )
					{
						++Ptr;
						if (!is_delimiter(*Ptr))
							goto LABEL_10;
					}
					isSuccess = 0;
				}
				else
				{
					LABEL_10:
						isSuccess = 1;
				}
				if (!isSuccess)
					return false;
				Char = *Ptr;
				if (!is_endof_line(*Ptr))
					return true;
			}
			while (Ptr < End)
			{
				if (*++Ptr == '\n')
					goto LABEL_6;
			}
			return false;
		}
	};






	static void __declspec(naked) SmFactionsHorde_AllChecks2()
	{
		static ParseBuffer* parser;
		__asm {
			lea  ecx,[esp+0x170]
			mov  parser, ecx
			push ebx
			push esi
			push edi
			push ebp
			mov  ebp, esp
		}

		if (parser->MatchToken("can_sap")) printf("can sap!\n");
		if (parser->MatchToken("horde_min_units")) parser->SkipLine();
		if (parser->MatchToken("horde_max_units")) parser->SkipLine();
		if (parser->MatchToken("horde_max_units_reduction_every_horde")) parser->SkipLine();
		if (parser->MatchToken("horde_unit_per_settlement_population")) parser->SkipLine();
		if (parser->MatchToken("horde_min_named_characters")) parser->SkipLine();
		if (parser->MatchToken("horde_max_percent_army_stack")) parser->SkipLine();
		if (parser->MatchToken("horde_disband_percent_on_settlement_capture")) parser->SkipLine();
		while (parser->MatchToken("horde_unit"))
		{
			parser->SkipLine();
		}

		__asm {
			mov  esp, ebp
			pop  ebp
			pop  edi
			pop  esi
			pop  ebx
		}
		abs_jmp(0x008AC794);   // return to: if (Parser_MatchToken(&parser, a2, "can_sap"))
	}

	void RomePatcher::RunPatcher(void* exeModule)
	{
		log("%s\n", GetCommandLineA());

		process_info proc(exeModule); // current process info
		log("IMAGE      0x%p\n", proc.Image);
		log("IMAGE_BASE 0x%p\n", proc.ImageBase);
		log("NT HDRS    0x%p\n", proc.Dos->e_lfanew);
		log("NT ENTRY   0x%p\n", proc.Nt->OptionalHeader.AddressOfEntryPoint);
		log("SECTIONS   %d\n",   proc.NumSections);

		log("\nSECTION  .text\n");

		IMAGE_SECTION_HEADER* sec = proc.find_section(".text");
		if (unlocked_section map = proc.map_section(sec))
		{
			log("    VirtAddr  0x%08x\n", proc.ImageBase + sec->VirtualAddress);
			log("    VirtSize  0x%08x\n", sec->SizeOfRawData);
			log("    FPData    0x%08x\n", sec->PointerToRawData);
			log("\n");

			log("    ENABLE    shield_wall  schiltrom\n");
			{
				/// .text:008D4964
				/// mov  [edi+94h], al ; 6 bytes
				/// mov  [edi+93h], al ; 6 bytes
				memset((BYTE*)0x008D4964, 0x90, 12); // 12 bytes of NOP's
			}
			log("    REPLACE   unit size_min 6  => 0\n");
			{
				/// .text:008D3E1A cmp     edx, 6 ; 3 bytes
				((BYTE*)0x008D3E1A)[2] = 0; // change 6 to 0
			}
			log("    REPLACE  unit size_max 60 => 75\n");
			{
				/// .text:008D3E23 cmp edx, 60     ; 3 bytes
				((BYTE*)0x008D3E23)[2] = 75; // change 60 to 75
				/// jg  loc_8D745E  ; 6 bytes
				//memset((BYTE*)0x008D3E23, 0x90, 9); // 9 bytes of NOP's to completely remove limits
			}
			log("    PATCH     unit size_0 div0fix\n");
			{
				/// .text:006A5275  idiv eax,ebp  ; 2 bytes
				/// .text:006A5277  neg  eax      ; 2 bytes
				/// .text:006A5279  add  eax,64   ; 3 bytes
				BYTE* dst = (BYTE*)0x006A5275;
				write_pushret(dst, UnitSize0_DivByZeroPatch);
				dst[6] = 0x90; // pad with nops
			}
			log("    ENABLE    unit can_swim\n");
			{
				///.text:008D43C1  test    eax, eax        ; 2 bytes
				///.text:008D43C3  jnz     loc_8D479D      ; 6 bytes
				write_patchcall(0x008D43C1, 8, UnitCanSwim_EnablePatch);
			}
			log("    ENABLE    unit can_horde\n");
			{
				///.text:008D43D8  test    eax, eax        ; 2 bytes
				///.text:008D43DA  jnz     loc_8D479D      ; 6 bytes
				write_patchcall(0x008D43D8, 8, UnitCanHorde_EnablePatch);
			}
			log("    ENABLE    sm_factions horde\n");
			{
				///.text:008AC6C5  movzx   eax, al    ; 3 bytes
				///.text:008AC6C8  test    eax, eax   ; 2 bytes
				///.text:008AC6CA  jnz     loc_8AD479 ; 6 bytes
				write_patchcall(0x008AC6C5, 11, SmFactionsHorde_AllChecks2);
			}
			//log("    DETOUR  unknown_function  detour_func1\n");
			{
				/// for a detour we need 6 bytes:
				/// push dword TestProcessLoader ; 5 bytes
				///	ret                          ; 1 byte
				/// 68 XX XX XX XX c3

				/// .text:00EC99B0
				/// push ebp        ; 1 byte
				/// mov  ebp, esp   ; 2 bytes
				/// sub  esp, 24h   ; 3 bytes

				//BYTE* p = (BYTE*)0x00EC99B0;
				//p[0] = 0x68; // PUSH
				//*(int*)&p[1] = (int)detour_func1;
				//p[5] = 0xC3; // RET

				
				//DetourTransactionBegin();
				//DetourUpdateThread(GetCurrentThread());
				//DetourAttach(&(PVOID&)Real_Target, AnyFunc<TargetFunc>(&CDetour::My_Target).fptr);
				//if (DetourTransactionCommit() == S_OK)
				//	log("    DETOUR  success\n");
				//else
				//	log("    DETOUR  failed\n");
			}

			// staging atm. so set to false
			if (false)
			{
				ApplyRegionsPatch(map);
			}

			proc.unmap_section(map);
		}

		log("\nSECTION  .data1\n");
		sec = proc.find_section(".data1");
		if (unlocked_section map = proc.map_section(".data1"))
		{
			log("    VirtAddr  0x%08x\n", proc.ImageBase + sec->VirtualAddress);
			log("    VirtSize  0x%08x\n", sec->Misc.VirtualSize);
			log("    FPData    0x%08x\n", sec->PointerToRawData);
			log("\n");
			log("    Ptr       0x%08x\n", map.Ptr);
			log("    Size      0x%08x\n", map.Size);

			log("    REPLACE   data/sky/clouds  RTR/data/clouds\n");
			{
				int count = 0;
				char* ptr = map.Ptr;
				while (ptr = map.find(ptr, "data/sky/clouds")) {
					log("        - 0x%x\n", ptr);
					memcpy(ptr, "RTR/data", 8);
					ptr += 16; // skip over the found string
					++count;
				}
			}
			proc.unmap_section(map);
		}

		log("=========================\n");
	}



	void PrintInst(_CodeInfo& ci, _DInst& inst)
	{
		_DecodedInst d;
		distorm_format32(&ci, &inst, &d);
		log("%08x %02d %-24s %s %s\n", ci.code, d.size, 
			d.instructionHex.p, d.mnemonic.p, d.operands.p);
	}

	ParsedRegion g_ParserRegions[1092]; // just .bss this array
	CampaignState g_CampaignState;

	static const int MaxInst = 1000;
	static _DInst Inst[MaxInst];

	static bool InstHasImm32(_DInst& inst)
	{
		for (int i = 0; i < 4; ++i)
			if (inst.ops[i].type == O_IMM && inst.ops[i].size == 32)
				return true;
		return false;
	}

	void RomePatcher::ApplyRegionsPatch(unlocked_section& map)
	{
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