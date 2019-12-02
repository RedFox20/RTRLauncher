#pragma once
#include "PatchUtils.h"

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

/**
 * From RomeTW-ALX 1.9
 */
struct ParseBuffer
{
    char* End;
    char* _Something;
    char* _Buffer;
    char* Ptr;
    char* Buffer1;
    char* End1;

    static bool is_endof_line(char ch)  { return ch == '\n' || ch == ';'; }
    static bool is_endof_token(char ch) { return ch != ',' && ch != '\t' && ch && ch != '\r' && is_endof_line(ch); }
    static bool is_delimiter(char ch)   { return ch == ' ' || ch == '\t' || !ch || ch == '\r' || ch == ',' || ch == ':'; }
    bool MatchToken(const char* str) const;
    bool SkipLine();
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


static void EnableUnitCanHorde(RomeExeVersion ver)
{
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
}



inline bool ParseBuffer::MatchToken(const char* str) const
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

inline bool ParseBuffer::SkipLine()
{
    int isSuccess; // eax@10
    char Char = *Ptr;
    while (Char == '\n')
    {
    LABEL_6:
        ++Ptr;
        Buffer1 = Ptr;
        ++End1;
        if (is_delimiter(*Ptr))
        {
            while (Ptr < End)
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


