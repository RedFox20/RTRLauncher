#pragma once
#include "PatchUtils.h"

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

inline void EnableBiggerUnitSize(RomeExeVersion ver)
{
    if (ver == RomeALX_1_9)
    {
        log("    REPLACE   unit size_min 6  => 0  RomeALX_1_9\n");
        /// .text:008D3E1A cmp     edx, 6 ; 3 bytes
        to_pointer(0x008D3E1A)[2] = 0; // change 6 to 0

        log("    REPLACE   unit size_max 60 => 75  RomeALX_1_9\n");
        /// .text:008D3E23 cmp edx, 60     ; 3 bytes
        to_pointer(0x008D3E23)[2] = 75; // change 60 to 75
        
        /// jg  loc_8D745E  ; 6 bytes
        //memset((BYTE*)0x008D3E23, 0x90, 9); // 9 bytes of NOP's to completely remove limits
        log("    PATCH     unit size_0 div0fix  RomeALX_1_9\n");
        /// .text:006A5275  idiv eax,ebp  ; 2 bytes
        /// .text:006A5277  neg  eax      ; 2 bytes
        /// .text:006A5279  add  eax,64   ; 3 bytes
        uint8_t* dst = to_pointer(0x006A5275);
        write_pushret(dst, UnitSize0_DivByZeroPatch);
        dst[6] = 0x90; // pad with nops
    }
}
