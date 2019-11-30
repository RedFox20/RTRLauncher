#include "ASM.h"



enum ModRM_Mod
{
	RegMem		= 0x00,	// [reg]        | operand's memory address is in Reg
	RegMemByte	= 0x01,	// [reg+off8]   | operand's memory address is Reg + byte displacement
	RegMemDWord = 0x02, // [reg+off32]  | operand's memory address is Reg + DWord displacement
	Reg			= 0x03,	// reg          | operand is reg itself
};
enum ModRM_RegMem		// ModRM RegMem values:			[reg], [sib], disp32
{
	RMEax = 0, RMEcx, RMEdx, RMEbx, RMSib, RMDisp32, RMEsi, RMEdi,
};
enum ModRM_RegMemByte	// ModRM RegMemByte values:		[reg]+disp8, [sib]+disp8
{
	RMEaxDisp8 = 0, RMEcxDisp8, RMEdxDisp8, RMEbxDisp8, RMSibDisp8, RMEbpDisp8, RMEsiDisp8, RMEdiDisp8,
};
enum ModRM_RegMemDWord	// ModRM RegMemDWord values:	[reg]+disp32, [sib]+disp32
{
	RMEaxDisp32 = 0, RMEcxDisp32, RMEdxDisp32, RMEbxDisp32, RMSibDisp32, RMEbpDisp32, RMEsiDisp32, RMEdiDisp32,
};
enum ModRM_Reg			// ModRM MemReg values:			reg
{
	EAX= 0, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
	AX = 0, CX, DX, BX, SP, BP, SI, DI,
	AL = 0, CL, DL, BL, AH, CH, DH, BH,
	XMM0=0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7
};

const char* ModRM_Reg_Str(byte reg32)
{
	static const char values[][4] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
	return values[reg32];
}

struct ModRM // Mod R/M byte fields
{
	// Mod R/M byte: 00 000 000
	byte R_M : 3; // third  000
	byte REG : 3; // second 000
	byte Mod : 2; // first  00
};
struct SIB // scale-index base
{
	// SIB byte: 00 000 000
	byte Base : 3;  // third 000
	byte Index : 3; // second 000
	byte Scale : 2; // first 00
};
#pragma pack(push, 1)
struct MovReplacer
{
	byte PatchSize;
	byte Opcode;
	uint Imm32;
	byte Nop1;
	byte Nop2;

	inline MovReplacer() : PatchSize(0), Opcode(0), Imm32(0), Nop1(0x90), Nop2(0x90) {}
	inline MovReplacer(byte opcode, byte patchSize)
		: PatchSize(patchSize), Opcode(opcode), Imm32(0), Nop1(0x90), Nop2(0x90) {}
	inline void Patch(void* dst) const
	{
		memcpy(dst, &Opcode, PatchSize);
	}
	inline operator bool() const
	{
		return PatchSize && Opcode;
	}
};
#pragma pack(pop)


// get a simple mov reg32, imm32 instruction opcode based on the input
static MovReplacer GetMovRMOpcode(ModRM rm, byte* args, bool isLEA = false)
{
	// eax, ecx, edx, ebx, esp, ebp, esi, edi
	static byte movs[] = { 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF };
	switch (rm.Mod)
	{
		case RegMem:
			if (rm.R_M == RMDisp32) {
				printf("  mov %s, 0x%08x\n", ModRM_Reg_Str(rm.REG), *(uint*)args);
				return{ movs[rm.REG], 6 }; // opcode+modrm+4
			}
			break;
		case RegMemByte:
			break; // not supported
		case RegMemDWord:
			if (rm.R_M == RMSibDisp32) { // SIB not completely supported... might break stuff
				SIB sib = *(SIB*)&args[0];
				printf("mov %s, [SIB+0x%08x]\n", ModRM_Reg_Str(rm.REG), *(uint*)&args[1]);
				return{ movs[rm.REG], 7 }; // opcode+modrm+sib+4
			}
			printf("  %s %s, [%s+0x%08x]\n", isLEA ? "lea":"mov", ModRM_Reg_Str(rm.REG), ModRM_Reg_Str(rm.R_M), *(uint*)args);
			return{ movs[rm.REG], 6 }; // opcode+modrm+4
		case Reg:
			break; // not supported
	}
	return {}; // invalid opcode
}

// dynamically patches specific MOV or LEA instructions to MOV reg, imm32
static bool DynamicMovPatch(byte* instr, uint imm32)
{
	MovReplacer result;
	switch (byte opcode = instr[0])
	{
		case 0x8B:									// MOV [modrm]  reg32,  r/m16/32  (size 5)
			result = GetMovRMOpcode(*(ModRM*)&instr[1], instr + 2);
			break;
		case 0xA1:									// MOV EAX, [off32]  (size 5)
			 // use MOV eax, imm32 (0xBB)
			result = MovReplacer(0xBB, 5); // patch 5 bytes
			break;
		case 0xB8: case 0xB9: case 0xBA: case 0xBB: // MOV reg32, imm32 (size 5)
		case 0xBC: case 0xBD: case 0xBE: case 0xBF:
			result = MovReplacer(opcode, 5); // reuse the existing opcode
			break;
		case 0x8D:									// LEA [modrm] reg32, [off32]
			result = GetMovRMOpcode(*(ModRM*)&instr[1], instr + 2, true);
			break;
	}
	if (result)
	{
		result.Imm32 = imm32;
		result.Patch(instr);

		byte debug[8];
		result.Patch(debug);
		return true;
	}
	printf("Unsupported opcode to replace!\n");
	return false;
}

byte* MovLeaReplacer::replaceAsMovAddr(unsigned int newAddress)
{
	for (int i = Targets.size() - 1; i >= 0; --i)
	{
		byte* addr = Targets[i];
		if (!DynamicMovPatch(addr, newAddress))
			return addr; // return failed opcode address
		Targets.pop_back();
	}
	return NULL;
}