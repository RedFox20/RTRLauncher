#pragma once
#include "PatchUtils.h"

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

static void EnableUnitCanSwim(RomeExeVersion ver)
{
    if (ver == RomeALX_1_9)
    {
        log("    ENABLE    unit can_swim\n");
        ///.text:008D43C1  test    eax, eax        ; 2 bytes
        ///.text:008D43C3  jnz     loc_8D479D      ; 6 bytes
        write_patchcall(0x008D43C1, 8, UnitCanSwim_EnablePatch);
    }
}
