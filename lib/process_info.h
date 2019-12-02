#pragma once
/**
 * Copyright (c) 2014 - The RTR Project
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct unlocked_section
{
    PCHAR Ptr;      // pointer to unlocked data in this process
    DWORD Size;     // number of bytes locked
    DWORD _Flags;   // previous flags if the segment had to be unprotected

    explicit operator bool() const { return Ptr != nullptr; }
    
    //// @brief Find arbitrary data inside this section starting from the specified pointer
    void* find(const void* start, const void* data, int numBytes)
    {
        return find(start, Ptr + Size, data, numBytes);
    }

    //// @brief Find a string literal like "hello world"
    template<int SIZE> char* find(const void* start, const char(&str)[SIZE])
    {
        return (char*)find(start, Ptr + Size, str, SIZE - 1);
    }


    //// @brief Find arbitrary data inside this section and between [start, end]
    void* find(const void* start, const void* end, const void* data, int numBytes);
};

struct process_info
{
    HANDLE                  Process;
    IMAGE_DOS_HEADER*       Dos;            // Copy of process DOS Header
    IMAGE_NT_HEADERS*       Nt;             // Copy of process NT Headers
    PCHAR                   Image;          // Process Image header (in the process context)
    PCHAR                   ImageBase;      // Base pointer for start of process data
    int                     NumSections;    // Number of Section Headers
    IMAGE_SECTION_HEADER*   Sections;       // Image Section headers

    process_info(void* moduleHandle = nullptr);

    //// @note Finds a section by its name.
    //// @return NULL if section not found
    IMAGE_SECTION_HEADER* find_section(const char* name);

    //// @note Map the entire section by name
    unlocked_section map_section(const char* name);

    //// @note Map the entire section
    unlocked_section map_section(const IMAGE_SECTION_HEADER* section);

    //// @note Unmaps a previously mapped section
    void unmap_section(unlocked_section& map);
};