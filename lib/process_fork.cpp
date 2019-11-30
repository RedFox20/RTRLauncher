//--------------------------------------------------------
// Dynamic Process Forking of Portable Executable
// Author : Vrillon / Venus
// Date   : 07/14/2008
//--------------------------------------------------------
/**************************************************  *******/
/* With this header, you can create and run a process	*/
/* from memory and not from a file.					  */
/**************************************************  *******/

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include "process_fork.h"


/////////////////////////////////////////////////////////////
// NtUnmapViewOfSection (ZwUnmapViewOfSection)
// Used to unmap a section from a process.

typedef long (__stdcall* NtUnmapViewOfSectionF)(HANDLE,PVOID);
NtUnmapViewOfSectionF NtUnmapViewOfSection = (NtUnmapViewOfSectionF)GetProcAddress(
												LoadLibraryA("ntdll.dll"),"NtUnmapViewOfSection");




/**
 * Fork Process
 * Dynamically create a process based on the parameter 'lpImage'. The parameter should have the entire
 * image of a portable executable file from address 0 to the end.
 * @param lpImage PE Executable Image Data
 * @param cmdParams Command line parameters to pass to the forked process
 * @param workingDir New working directory for the forked process
 * @param procInfo Forked Process information (proc handle, thread handle, etc.)
 * @return TRUE if process was created successfully, FALSE on failure
 */
bool process_fork(PVOID lpImage, const char* cmdParams, const char* workingDir, PROCESS_INFORMATION* procInfo)
{
	// Get the file name for the dummy process
	char curProcName[MAX_PATH] = ""; // name of the current module
	DWORD curProcNameSize = GetModuleFileNameA(0, curProcName, MAX_PATH);

	// Open the dummy process in binary mode
	FILE* fFile = fopen(curProcName, "rb");
	if (!fFile)
	{
		return false;
	}

	// Read the DOS header
	IMAGE_DOS_HEADER localDosHeader;
	fread(&localDosHeader, sizeof(localDosHeader), 1, fFile);
	if (localDosHeader.e_magic != IMAGE_DOS_SIGNATURE)
	{
		fclose(fFile);
		return false;
	}

	// Grab NT Headers
	IMAGE_NT_HEADERS localNtHeader;
	fseek(fFile, localDosHeader.e_lfanew, SEEK_SET);
	fread(&localNtHeader, sizeof(localNtHeader), 1, fFile);
	fclose(fFile); // close file
	if (localNtHeader.Signature != IMAGE_NT_SIGNATURE)
	{
		return false;
	}


	// Get Size and Image Base
	PCHAR localImageBase = (PCHAR)localNtHeader.OptionalHeader.ImageBase;
	DWORD localImageSize = localNtHeader.OptionalHeader.SizeOfImage;


	// Grab DOS Header for Forking Process
	PCHAR Image = (PCHAR)lpImage;
	PIMAGE_DOS_HEADER DOS = PIMAGE_DOS_HEADER(Image);
	PIMAGE_NT_HEADERS NT  = PIMAGE_NT_HEADERS(Image + DOS->e_lfanew);
	if (DOS->e_magic != IMAGE_DOS_SIGNATURE || NT->Signature != IMAGE_NT_SIGNATURE)
	{
		return false;
	}

	// Get proper image size
	DWORD imageSize = NT->OptionalHeader.SizeOfImage;

	// Allocate memory for image base
	PCHAR tempImage = new CHAR[imageSize + 8192]; // alloc with reserve
	PCHAR imageBaseMem = tempImage;
	if (DWORD off = ((DWORD)tempImage % 4096))
		imageBaseMem += 4096 - off;				// fix alignment

	memcpy(imageBaseMem, Image, NT->OptionalHeader.SizeOfHeaders);

	DWORD numSections = NT->FileHeader.NumberOfSections;
	PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(NT);
	// Copy Sections To Buffer
	for (DWORD i = 0; i < numSections; ++i)
	{
		IMAGE_SECTION_HEADER& sec = sections[i];
		CHAR* addr = imageBaseMem + sec.VirtualAddress;

		bool isAligned = ((DWORD)addr % 4096) == 0;
		memcpy(imageBaseMem + sec.VirtualAddress, Image + sec.PointerToRawData, sec.SizeOfRawData);
	}

	memset(procInfo, 0, sizeof(PROCESS_INFORMATION)); // memset to 0, just in case
	STARTUPINFO startInfo = { sizeof(STARTUPINFO) };

	// set the command line args
	curProcName[curProcNameSize++] = ' '; // add a space separator
	strcpy(curProcName + curProcNameSize, cmdParams); // copy the cmd line args

	if (CreateProcessA(0, curProcName, 0, 0, 0, CREATE_SUSPENDED, 0, workingDir, &startInfo, procInfo))
	{
		HANDLE process = procInfo->hProcess;
		HANDLE thread  = procInfo->hThread;
		CONTEXT	context = { 0 };
		context.ContextFlags = CONTEXT_FULL;
		GetThreadContext(thread, &context);


		PCHAR remoteImgBase = (PCHAR)NT->OptionalHeader.ImageBase;
		DWORD remoteImgSize = NT->OptionalHeader.SizeOfImage;


		// at this point, our process has "localImageSize" amount of bytes
		// we need to check if this is enough, or if we need to allocate more
		if (localImageSize < remoteImgSize) // need to alloc more?
		{
			// allocate a few more bytes in the remote process
			if (!VirtualAllocEx(process, remoteImgBase + localImageSize, remoteImgSize - localImageSize,
				MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
			{
				TerminateProcess(process, 0);

				DWORD err = GetLastError();
				delete[] tempImage;
				return false;
			}
		}
		DWORD oldprotect;
		if (!VirtualProtectEx(process, remoteImgBase, localImageSize, PAGE_EXECUTE_READWRITE, &oldprotect))
		{
			TerminateProcess(process, 0);

			DWORD err = GetLastError();
			delete[] tempImage;
			return false;
		}

		// Write Image to Process
		DWORD written = 0;
		if (!WriteProcessMemory(process, remoteImgBase, imageBaseMem, imageSize, &written))
		{
			TerminateProcess(process, 0);

			DWORD err = GetLastError();
			delete[] tempImage;
			return false;
		}

		// Set Image Base (this shouldn't fail since last WriteProcessMemory worked)
		WriteProcessMemory(process, (PVOID)(context.Ebx + 8), &remoteImgBase, 4, &written);


		// Set the new entry point
		context.Eax = (DWORD)remoteImgBase + NT->OptionalHeader.AddressOfEntryPoint;
		SetThreadContext(thread, &context);
	}

	delete[] tempImage;
	return true;
}
