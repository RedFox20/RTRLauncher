#pragma once
#include <malloc.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


struct mapped_section
{
	PCHAR Ptr;			// pointer to mapped data in THIS process context
	DWORD Size;			// number of bytes mapped
	PCHAR _ProcAddr;	// pointer to mapped data in the mapped process context
	bool  _Protected;	// true if the specified section is write protected

	inline operator bool() const { return Ptr != 0; }

	//// @brief Find arbitrary data inside this section starting from the specified pointer
	void* find(const void* start, const void* data, int numBytes);

	//// @brief Find arbitrary data inside this section
	inline void* find(const void* data, int numBytes)
	{
		return find(Ptr, data, numBytes);
	}

	//// @brief Find a string literal like "hello world"
	template<int SIZE> char* find(const char(&str)[SIZE])
	{
		return (char*)find(str, SIZE - 1);
	}

	//// @brief Find a string literal like "hello world"
	template<int SIZE> char* find(const void* start, const char(&str)[SIZE])
	{
		return (char*)find(start, str, SIZE - 1);
	}

	//// @brief Find arbitrary data - integers, floats, bytearrays, ...
	template<class T> T* find(const T& item)
	{
		return (T*)find(&item, sizeof(item));
	}

	//// @brief Find arbitrary data - integers, floats, bytearrays, ...
	template<class T> T* find(const void* start, const T& item)
	{
		return (T*)find(start, &item, sizeof(item));
	}
};

struct process_loader
{
	PROCESS_INFORMATION		ProcInfo;
	STARTUPINFOA			StartInfo;
	IMAGE_DOS_HEADER		Dos;			// Copy of process DOS Header
	IMAGE_NT_HEADERS		Nt;				// Copy of process NT Headers
	HANDLE					Process;		// Process handle
	PCHAR					Image;			// Process Image header (in the process context)
	PCHAR					ImageBase;		// Base pointer for start of process data
	int						NumSections;	// Number of Section Headers
	IMAGE_SECTION_HEADER	Sections[12];	// Copy of Image Section headers


	process_loader();


	//// @note Creates the process in an uninitialized state
	//// @return false If process creation failed
	bool LoadProcess(const char* commandLine, const char* workingDirectory = NULL);

	//// @note Start the process
	void Start();

	//// @note Destroy the process manually (if the calling application terminates before)
	void Abort();

	//// @note Waits on this process to finish
	void Wait(DWORD timeoutMillis = INFINITE);

	
	//// @note Finds a section by its name.
	//// @return NULL if section not found
	IMAGE_SECTION_HEADER* FindSection(const char* name);

	//// @note Creates a full copy of a section in remote process
	////       The memory buffer must be free()'d manually
	BYTE* CopySection(const char* name);
	BYTE* CopySection(const IMAGE_SECTION_HEADER& section);

	//// @note Finds and maps a section by name
	mapped_section MapSection(const char* name);
	mapped_section MapSection(const char* name, DWORD startIndex, DWORD bytesToMap);


	//// @note Map the entire section
	mapped_section MapSection(const IMAGE_SECTION_HEADER& section);
	//// @note Map only a chosen part of a section
	mapped_section MapSection(const IMAGE_SECTION_HEADER& section, DWORD startIndex, DWORD bytesToMap);
	//// @note Unmaps a previously mapped section
	void UnmapSection(mapped_section& map);
};