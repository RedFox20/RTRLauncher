/*
 * Memory DLL loading code
 * Version 0.0.4
 *
 * Copyright (c) 2004-2014 by Joachim Bauch / mail@joachim-bauch.de
 * http://www.joachim-bauch.de
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is MemoryModule.h
 *
 * The Initial Developer of the Original Code is Joachim Bauch.
 *
 * Portions created by Joachim Bauch are Copyright (C) 2004-2014
 * Joachim Bauch. All Rights Reserved.
 *
 */

#ifndef __MEMORY_MODULE_HEADER
#define __MEMORY_MODULE_HEADER

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


typedef void *HMEMORYMODULE;
typedef void *HMEMORYRSRC;
typedef void *HCUSTOMMODULE;



#ifdef __cplusplus
extern "C" {
#endif

	typedef HCUSTOMMODULE (*CustomLoadLibraryFunc)(LPCSTR, void *);
	typedef FARPROC (*CustomGetProcAddressFunc)(HCUSTOMMODULE, LPCSTR, void *);
	typedef void (*CustomFreeLibraryFunc)(HCUSTOMMODULE, void *);
	typedef int (WINAPI *ExeEntryProc)(void);

	typedef struct {
		PIMAGE_NT_HEADERS headers;
		unsigned char *codeBase;
		HCUSTOMMODULE *modules;
		int numModules;
		int initialized;
		int isDLL;
		int isRelocated;
		CustomLoadLibraryFunc loadLibrary;
		CustomGetProcAddressFunc getProcAddress;
		CustomFreeLibraryFunc freeLibrary;
		void *userdata;
		ExeEntryProc exeEntry;
	} MEMORYMODULE, *PMEMORYMODULE;


/**
 * Load EXE/DLL from memory location.
 *
 * All dependencies are resolved using default LoadLibrary/GetProcAddress
 * calls through the Windows API.
 */
PMEMORYMODULE MemoryLoadLibrary(const void *, const void* preferredBase);

/**
 * Load EXE/DLL from memory location using custom dependency resolvers.
 *
 * Dependencies will be resolved using passed callback methods.
 */
PMEMORYMODULE MemoryLoadLibraryEx(const void *, const void* preferredBase,
    CustomLoadLibraryFunc,
    CustomGetProcAddressFunc,
    CustomFreeLibraryFunc,
    void *);

/**
 * Get address of exported method.
 */
FARPROC MemoryGetProcAddress(PMEMORYMODULE, LPCSTR);

/**
 * Free previously loaded EXE/DLL.
 */
void MemoryFreeLibrary(PMEMORYMODULE);

/**
 * Execute entry point (EXE only). The entry point can only be executed
 * if the EXE has been loaded to the correct base address or it could
 * be relocated (i.e. relocation information have not been stripped by
 * the linker).
 *
 * Important: calling this function will not return, i.e. once the loaded
 * EXE finished running, the process will terminate.
 *
 * Returns a negative value if the entry point could not be executed.
 */
int MemoryCallEntryPoint(PMEMORYMODULE);

/**
 * Find the location of a resource with the specified type and name.
 */
HMEMORYRSRC MemoryFindResource(PMEMORYMODULE, LPCTSTR, LPCTSTR);

/**
 * Find the location of a resource with the specified type, name and language.
 */
HMEMORYRSRC MemoryFindResourceEx(PMEMORYMODULE, LPCTSTR, LPCTSTR, WORD);

/**
 * Get the size of the resource in bytes.
 */
DWORD MemorySizeofResource(PMEMORYMODULE, HMEMORYRSRC);

/**
 * Get a pointer to the contents of the resource.
 */
LPVOID MemoryLoadResource(PMEMORYMODULE, HMEMORYRSRC);

/**
 * Load a string resource.
 */
int MemoryLoadString(PMEMORYMODULE, UINT, LPTSTR, int);

/**
 * Load a string resource with a given language.
 */
int MemoryLoadStringEx(PMEMORYMODULE, UINT, LPTSTR, int, WORD);

#ifdef __cplusplus
}
#endif

#endif  // __MEMORY_MODULE_HEADER
