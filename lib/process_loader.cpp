#include "process_loader.h"

    
    void* mapped_section::find(const void* start, const void* data, int numBytes)
    {
        char ch = *(char*)data;
        char* ptr = (char*)start;
        char* end = Ptr + Size;
        for ( ; ptr < end; )
        {
            char* p = (char*)memchr(ptr, ch, end - ptr); // try to find first char
            if (p == nullptr) 
                return nullptr; // end of data
            if (memcmp(p, data, numBytes) == 0)
                return p; // found match
            ptr = ++p;
        }
        return nullptr;
    }

    process_loader::process_loader()
    {
        // these have to be zeroed, otherwise CreateProcess will fail
        ZeroMemory(&ProcInfo, sizeof ProcInfo);
        ZeroMemory(&StartInfo, sizeof StartInfo);
        StartInfo.cb = sizeof STARTUPINFO;
        Process = nullptr;
    }


    bool process_loader::LoadProcess(const char* commandLine, const char* workingDir)
    {
        if (!CreateProcessA(0, (char*)commandLine, 0, 0, FALSE, CREATE_SUSPENDED, 0, workingDir, &StartInfo, &ProcInfo))
            return false; // failed to create process

        CONTEXT context = { CONTEXT_INTEGER };
        GetThreadContext(ProcInfo.hThread, &context);
        Process = ProcInfo.hProcess;

        // read process image pointer and dos/nt headers
        ReadProcessMemory(Process, (PCHAR)context.Ebx + 8, &Image, sizeof PCHAR, 0);
        ReadProcessMemory(Process, Image, &Dos, sizeof Dos, 0);
        ReadProcessMemory(Process, Image + Dos.e_lfanew, &Nt, sizeof Nt, 0);

        // read all section headers
        NumSections = (int)Nt.FileHeader.NumberOfSections;
        LONG offset = Dos.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + Nt.FileHeader.SizeOfOptionalHeader;
        ReadProcessMemory(Process, Image + offset, Sections, sizeof(IMAGE_SECTION_HEADER) * NumSections, 0);
    
        // get pointer to process data
        ImageBase = (PCHAR)Nt.OptionalHeader.ImageBase;
        return true;
    }

    void process_loader::Start()
    {
        ResumeThread(ProcInfo.hThread); // launch!
    }

    void process_loader::Abort()
    {
        if (Process)
        {
            TerminateProcess(Process, -1);
            Process = 0;
        }
    }

    void process_loader::Wait(DWORD timeoutMillis)
    {
        WaitForSingleObject(Process, timeoutMillis);
    }

    
    IMAGE_SECTION_HEADER* process_loader::FindSection(const char* name)
    {
        int len = strlen(name);
        for (int i = 0; i < NumSections; ++i)
            if (memcmp(Sections[i].Name, name, len) == 0)
                return &Sections[i];
        return nullptr;
    }


    BYTE* process_loader::CopySection(const char* name)
    {
        if (auto* section = FindSection(name))
            return CopySection(*section);
        return nullptr;
    }
    BYTE* process_loader::CopySection(const IMAGE_SECTION_HEADER& section)
    {
        BYTE* buffer = (BYTE*)malloc(section.Misc.VirtualSize);
        ReadProcessMemory(Process, ImageBase + section.VirtualAddress, buffer, section.Misc.VirtualSize, 0);
        return buffer;
    }



    mapped_section process_loader::MapSection(const char* name)
    {
        if (auto* section = FindSection(name))
            return MapSection(*section);
        return mapped_section { nullptr };
    }
    mapped_section process_loader::MapSection(const char* name, DWORD startIndex, DWORD bytesToMap)
    {
        if (auto* section = FindSection(name))
            return MapSection(*section, startIndex, bytesToMap);
        return mapped_section { nullptr };
    }


    mapped_section process_loader::MapSection(const IMAGE_SECTION_HEADER& section)
    {
        return MapSection(section, 0, section.Misc.VirtualSize);
    }
    mapped_section process_loader::MapSection(const IMAGE_SECTION_HEADER& section, DWORD startIndex, DWORD bytesToMap)
    {
        mapped_section map = { 0 };
        DWORD sectionSize = section.Misc.VirtualSize;
        if (startIndex > sectionSize)
            return map;

        // set starting offset/size and make sure resulting Size is valid
        map.Size   = bytesToMap;
        if (startIndex + bytesToMap > sectionSize)
            map.Size = sectionSize - startIndex;

        // remember the mapped address and the section protection
        map._ProcAddr = ImageBase + section.VirtualAddress + startIndex;
        map._Protected = (section.Characteristics & IMAGE_SCN_MEM_WRITE) == 0; // no write access?

        // allocate mapping buffer and read the requested contents
        map.Ptr = (PCHAR)malloc(map.Size);
        ReadProcessMemory(Process, map._ProcAddr, map.Ptr, map.Size, 0);
        return map;
    }


    void process_loader::UnmapSection(mapped_section& map)
    {
        if (!map.Ptr)
            return;

        DWORD protectFlags;
        if (map._Protected) // unprotect the section if needed:
            VirtualProtectEx(Process, map._ProcAddr, map.Size, PAGE_READWRITE, &protectFlags);
        
        // flush back the data:
        WriteProcessMemory(Process, map._ProcAddr, map.Ptr,  map.Size, 0);
        
        if (map._Protected) // restore correct protection flags if needed:
            VirtualProtectEx(Process, map._ProcAddr, map.Size, protectFlags, &protectFlags);

        // release the temp mapping buffer
        free(map.Ptr);
        map.Ptr = nullptr;
    }

