#include "process_info.h"


    void* unlocked_section::find(const void* start, const void* end, const void* data, int numBytes)
    {
        char ch = *(char*)data;
        char* ptr = (char*)start;
        for ( ; ptr < end; )
        {
            char* p = (char*)memchr(ptr, ch, (char*)end - ptr); // try to find first char
            if (p == nullptr) 
                return nullptr; // end of data
            if (memcmp(p, data, numBytes) == 0)
                return p; // found match
            ptr = ++p;
        }
        return nullptr;
    }

    process_info::process_info(void* moduleHandle)
    {
        Process = GetCurrentProcess();
        Image = PCHAR(moduleHandle ? moduleHandle : GetModuleHandleW(0));
        Dos = reinterpret_cast<IMAGE_DOS_HEADER*>(Image);
        Nt = reinterpret_cast<IMAGE_NT_HEADERS*>(Image + Dos->e_lfanew);
        ImageBase = PCHAR(Nt->OptionalHeader.ImageBase);

        // read all section headers
        NumSections = int(Nt->FileHeader.NumberOfSections);
        LONG offset = Dos->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + Nt->FileHeader.SizeOfOptionalHeader;
        Sections = reinterpret_cast<IMAGE_SECTION_HEADER*>(Image + offset);
    }


    IMAGE_SECTION_HEADER* process_info::find_section(const char* name)
    {
        int len = strlen(name);
        for (int i = 0; i < NumSections; ++i)
            if (memcmp(Sections[i].Name, name, len) == 0)
                return &Sections[i];
        return nullptr;
    }

    unlocked_section process_info::map_section(const char* name)
    {
        if (auto* section = find_section(name))
            return map_section(section);
        return unlocked_section { nullptr };
    }

    unlocked_section process_info::map_section(const IMAGE_SECTION_HEADER* section)
    {
        unlocked_section map = {
            ImageBase + section->VirtualAddress, // ptr to section
            section->SizeOfRawData,              // size of section
            0
        };
        
        if ((section->Characteristics & IMAGE_SCN_MEM_WRITE) == 0)
        {
            DWORD newFlags = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                ? PAGE_EXECUTE_READWRITE
                : PAGE_READWRITE;
            VirtualProtectEx(Process, map.Ptr, map.Size, newFlags, &map._Flags);
        }
        return map;
    }

    void process_info::unmap_section(unlocked_section& map)
    {
        if (!map.Ptr)
            return;

        if (map._Flags) // restore section protection flags
        {
            DWORD flags;
            VirtualProtectEx(Process, map.Ptr, map.Size, map._Flags, &flags);
        }
        map.Ptr = nullptr;
    }