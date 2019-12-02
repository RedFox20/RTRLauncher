#pragma once
#include <cstdint>
#include <cstring>
#include "RTW/RomeAPI.h"
#include <log.h>

/**
 * Contains reusable ASM utilities for patching
 */

// helper union for recasting function pointers
template<class Func> union AnyFunc
{
    Func  func;
    void* fptr;
    AnyFunc(Func fun) : func(fun) {}
    AnyFunc(int address) : fptr(reinterpret_cast<void*>(address)) {}
};


/**
 * @return Cast `address` to a byte pointer
 */
inline uint8_t* to_pointer(uint32_t address)
{
    return reinterpret_cast<uint8_t*>(address);
}


/**
 * @return Reference to float at `address`
 */
inline float& float_at(uint32_t address)
{
    return *reinterpret_cast<float*>(address);
}


/**
 * @return Reference to uint32_t at `address`
 */
inline uint32_t& uint_at(uint32_t address)
{
    return *reinterpret_cast<uint32_t*>(address);
}


/**
 * Create a 6 byte asm sequence: push addr; ret; for absolute jump
 */
inline void write_pushret(uint8_t* dst, void (*nakedFunction)())
{
    dst[0] = 0x68; // opcode PUSH imm32
    *reinterpret_cast<uint32_t*>(&dst[1]) = reinterpret_cast<uint32_t>(nakedFunction); // imm32
    dst[5] = 0xC3; // opcode ret
}


/**
 * Write `count` number of NOP instructions at address
 */
inline void write_nops(uint32_t address, int count)
{
    memset(to_pointer(address), 0x90, count);
}


/**
 * Patches the start of a function prologues by padding it with NOP-s
 * and doing a push-ret to nakedFunction
 * @param address Address of function to patch
 * @param padNops Number of NOP op codes to pad with
 * @param nakedFunction A stripped down naked function
 */
inline void write_patchcall(uint32_t address, int padNops, void (*nakedFunction)())
{
    write_nops(address, padNops);
    write_pushret(to_pointer(address), nakedFunction);
}

/**
 * Writes a JNE 2-byte sequence `JNE rel8`
 */
static void write_jne(uint32_t address, int8_t short_offset)
{
    uint8_t* dst = reinterpret_cast<uint8_t*>(address);
    dst[0] = 0x75; // opcode JNE short rel8
    dst[1] = static_cast<int8_t>(short_offset); // jump offset
}

// performs an absolute jump to specified address
#define abs_jmp(ajmp) __asm{ push ajmp }__asm{ ret }

#define LBL1(X,Y) X##Y
#define LBL(X,Y) LBL1(X,Y)
// performs an absolute call to the specified address and reliably returns 
#define abs_call(acall) __asm{call LBL(lbl,__LINE__) } LBL(lbl,__LINE__): __asm{add dword ptr[esp],10}__asm{push acall}__asm{ret}

