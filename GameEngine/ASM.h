#pragma once
#include <vector>

typedef unsigned char byte;
typedef unsigned int  uint;

/**
 * This utility class automatically replaces calls such as
 * lea edx, [0xCAFEBABE]
 * mov ecx,  0xCAFEBABE
 * 
 * Supported MOV regs: EAX, EBX, ECX, EDX, ESI, EDI
 * Supported LEA regs: EAX, EBX, ECX, EDX, ESI, EDI
 * With:
 * mov <reg>, <immediate addr>
 */
struct MovLeaReplacer
{
	std::vector<byte*> Targets; // target .text segment addresses

	inline MovLeaReplacer() {}
	inline int size() const { return (int)Targets.size(); }
	inline void clear() { Targets.clear(); }

	/**
	 * Add a pointer to .text segment address where you wish to replace
	 * a MOV or LEA opcode
	 */
	inline void add(uint address)
	{
		Targets.push_back((byte*)address);
	}

	/**
	 * Add a pointer to .text segment address where you wish to replace
	 * a MOV or LEA opcode
	 */
	inline void add(void* address)
	{
		Targets.push_back((byte*)address);
	}

	/**
	 * Replaces all specified target addresses as MOV <reg>, <newAddress>.
	 * The <reg> parameter is auto-detected from the original target address.
	 *
	 * !!Iterates address entries in reverse order!!
	 * Pops addresses as the patch succeed, returns on first failure
	 * If all cases succeed, the Targets vector will be empty
	 *
	 * @param newAddress New address value to use
	 * @return NULL on success, failure target address if failed
	 */
	byte* replaceAsMovAddr(uint newAddress);
	inline byte* replaceAsMovAddr(void* newAddress)
	{
		return replaceAsMovAddr((uint)newAddress);
	}
};