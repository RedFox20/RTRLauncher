#include "remote_dll_injector.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <log.h>
#include <enumerator.h>
#include <shadowlib.h>
#include <rpp/obfuscated_string.h>

/**
 * Copyright notes: Original code written by zwclose7
 */

typedef HMODULE (WINAPI*pLoadLibraryA)  (LPCSTR);
typedef FARPROC (WINAPI*pGetProcAddress)(HMODULE,LPCSTR);
typedef BOOL    (WINAPI*pDllMain)       (HMODULE,DWORD,PVOID);

Enum(ManualInjectResult,
    DllMainFailed,			// DllMain returned FALSE - failed
    DllMainSuccess,			// DllMain returned TRUE - success
    DllImportFailed,		// A DLL import totally failed
    GetProcOrdinalFailed,	// Get Proc by Ordinal failed
    GetProcNameFailed		// Get Proc by Name failed
);
typedef struct _MANUAL_INJECT
{
    PCHAR						PtrErr;
    PVOID						ImageBase;
    PIMAGE_NT_HEADERS			NtHeaders;
    PIMAGE_BASE_RELOCATION		BaseRelocation;
    PIMAGE_IMPORT_DESCRIPTOR	ImportDirectory;
    pLoadLibraryA				fnLoadLibraryA;
    pGetProcAddress				fnGetProcAddress;
} MANUAL_INJECT, *PMANUAL_INJECT;
#pragma runtime_checks("scu", off) // disable any runtime checks (crashtastic)
static DWORD WINAPI LoadDll(PMANUAL_INJECT Injector)
{
    PIMAGE_BASE_RELOCATION IBR   = Injector->BaseRelocation;
    PIMAGE_IMPORT_DESCRIPTOR IID = Injector->ImportDirectory;
    PBYTE ImgBase				 = (PBYTE)Injector->ImageBase;
    DWORD delta					 = (DWORD)(ImgBase - Injector->NtHeaders->OptionalHeader.ImageBase); // Calculate the delta

    while (IBR->VirtualAddress) // Relocate the image
    {
        if (IBR->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
        {
            const int count = (IBR->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            PWORD list		= (PWORD)(IBR + 1);

            for (int i = 0; i < count; ++i)
            {
                if (list[i])
                {
                    PDWORD ptr =(PDWORD)(ImgBase + (IBR->VirtualAddress + (list[i] & 0xFFF)));
                    *ptr += delta;
                }
            }
        }
        IBR = (PIMAGE_BASE_RELOCATION)((PBYTE)IBR + IBR->SizeOfBlock);
    }
    while (IID->Characteristics) // Resolve DLL imports
    {
        PIMAGE_THUNK_DATA OrigFirstThunk = (PIMAGE_THUNK_DATA)(ImgBase + IID->OriginalFirstThunk);
        PIMAGE_THUNK_DATA FirstThunk	 = (PIMAGE_THUNK_DATA)(ImgBase + IID->FirstThunk);

        HMODULE hModule = Injector->fnLoadLibraryA((PCHAR)ImgBase + IID->Name);
        if (!hModule)
        {
            Injector->PtrErr = (PCHAR)ImgBase + IID->Name;
            return DllImportFailed;
        }

        while (OrigFirstThunk->u1.AddressOfData)
        {
            if (OrigFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
            {
                // Import by ordinal
                PCHAR ProcName = (PCHAR)(OrigFirstThunk->u1.Ordinal & 0xFFFF);
                DWORD Function = (DWORD)Injector->fnGetProcAddress(hModule, ProcName);
                if (!Function)
                {
                    Injector->PtrErr = ProcName;
                    return GetProcOrdinalFailed;
                }
                FirstThunk->u1.Function = Function;
            }
            else
            {
                // Import by name
                PIMAGE_IMPORT_BY_NAME IBN = (PIMAGE_IMPORT_BY_NAME)(ImgBase + OrigFirstThunk->u1.AddressOfData);
                DWORD Function			  = (DWORD)Injector->fnGetProcAddress(hModule, IBN->Name);
                if (!Function)
                {
                    Injector->PtrErr = IBN->Name;
                    return GetProcNameFailed;
                }
                FirstThunk->u1.Function = Function;
            }
            ++OrigFirstThunk;
            ++FirstThunk;
        }
        ++IID;
    }

    if (Injector->NtHeaders->OptionalHeader.AddressOfEntryPoint)
    {
        pDllMain EntryPoint = (pDllMain)(ImgBase + Injector->NtHeaders->OptionalHeader.AddressOfEntryPoint);
        return EntryPoint((HMODULE)ImgBase, 1, 0); // Call the entry point
    }
    return DllMainSuccess; // EntryPoint is optional for this Manual Injection
}
#pragma optimize("", off) // disable all optimizations - we need this function to exist
static DWORD WINAPI LoadDllEnd()
{
    return 0;
}
#pragma optimize("", on) // restore optimization flags
#pragma runtime_checks("scu", restore) // restore any specified runtime checks





typedef struct _FILE_INJECT
{
    pLoadLibraryA  fnLoadLibraryA;
    CHAR           LibName[MAX_PATH];
} FILE_INJECT, *PFILE_INJECT;
#pragma runtime_checks("scu", off) // disable any runtime checks (crashtastic)
static DWORD WINAPI FileLoadDll(PFILE_INJECT Injector)
{
    //// @brief We don't want to pass LoadLibraryA as the CreateRemoteThread
    //// @brief startup function - that method is easily detectable.
    //// @brief That's why we use this proxy instead
    return (DWORD)Injector->fnLoadLibraryA(Injector->LibName);
}
#pragma optimize("", off) // disable all optimizations - we need this function to exist
static DWORD WINAPI FileLoadDllEnd()
{
    return 0;
}
#pragma optimize("", on) // restore optimization flags
#pragma runtime_checks("scu", restore) // restore any specified runtime checks






void inject_result::unload()
{
    if (Process && Process != INVALID_HANDLE_VALUE)
    {
        if (DllImage) // free remote DLL image
        {
            VirtualFreeEx(Process, DllImage, 0, MEM_RELEASE);
            DllImage = NULL;
        }
        Process = NULL;
    }
}

remote_dll_injector::remote_dll_injector()
{
    Image	 = NULL;
    Resource = NULL;
}
remote_dll_injector::remote_dll_injector(int resourceId)
{
    Resource = LoadResource(0, FindResourceA(0, MAKEINTRESOURCEA(resourceId), RT_RCDATA));
    Image    = LockResource(Resource);
}
remote_dll_injector::remote_dll_injector(void* pInjectableDllImage)
{
    Image	 = pInjectableDllImage;
    Resource = NULL;
}





remote_dll_injector::~remote_dll_injector()
{
    if (Resource)
        FreeResource(Resource);
}

static const char secOK[] = "[==] Injector [==]";
static const char secFF[] = "[!!] Injector [!!]";
static bool DebugPriviledgeSet = false;

void remote_dll_injector::enable_debug_priviledge()
{
    logsec(secOK, "Enabling Debug Priviledge\n");
    HANDLE hToken;
    if (OpenProcessToken((HANDLE)-1, TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken))
    {
        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes    = SE_PRIVILEGE_ENABLED;
        tp.Privileges[0].Luid.LowPart  = 20; // SeDebugPriviledge
        tp.Privileges[0].Luid.HighPart = 0;
        if (AdjustTokenPrivileges(hToken, FALSE, &tp, 0, 0, 0))
        {
            logsec(secOK, "Debug Priviledge SET SUCCESS\n");
            DebugPriviledgeSet = true;
        }
        else
            logsec(secFF, "Debug Priviledge SET FAILED\n");
        CloseHandle(hToken);
    }
    else
        logsec(secFF, "Failed to open current process handle for priviledge adjustment\n");
}



constexpr auto LoadLibraryAName = make_obfuscated("LoadLibraryA");
constexpr auto GetProcAddressName = make_obfuscated("GetProcAddress");


bool remote_dll_injector::inject_dllfile(void* targetProcessHandle, const char* filename)
{
    // Set DEBUG priviledge ENABLED for the Current process to get sufficient rights
    if (!DebugPriviledgeSet)
        enable_debug_priviledge();

    HANDLE process = (HANDLE)targetProcessHandle;
    // allocate memory for the loader code in the remote process
    PCHAR loader = (PCHAR)shadow_valloc(process, nullptr, 4096, PAGE_EXECUTE_READWRITE); // Allocate memory for the loader code
    if (!loader)
    {
        logsec(secFF, "Unable to allocate memory for loader in remote process\n");
        return false;
    }


    FILE_INJECT FileInject;
    FileInject.fnLoadLibraryA = (pLoadLibraryA)shadow_kernel_getproc(LoadLibraryAName.to_string().c_str());
    GetFullPathNameA(filename, sizeof(FileInject.LibName), FileInject.LibName, nullptr);

    logsec(secOK, "Fullpath: %s\n", FileInject.LibName);

    DWORD pid = GetProcessId(process);

    // Write the loader information to target process
    shadow_vwrite(process, loader, &FileInject, sizeof FileInject);

    DWORD span = (DWORD)FileLoadDllEnd - (DWORD)FileLoadDll;
    // Write the loader code to target process
    shadow_vwrite(process, (PFILE_INJECT(loader)+1), FileLoadDll, span);


    /// @note We never call FreeLibrary, because the remote process will be using it from now on
    HANDLE hThread = shadow_create_thread(process, LPTHREAD_START_ROUTINE(PFILE_INJECT(loader)+1), loader);

    // don't use WaitForSingleObject(hThread, INFINITE); here! Will deadlock if client DLL 
    // creates new thread during load, so we use a silly loop instead
    HMODULE hModule;
    // get return code, which is actually the loaded HMODULE
    while (GetExitCodeThread(hThread, (DWORD*)&hModule) && int(hModule) == STILL_ACTIVE)
        Sleep(1); // wait for it...
    shadow_close_handle(hThread);

    // free loader
    shadow_vfree(process, loader);

    if (!hModule) // LoadLibrary failed
    {
        logsec(secFF, "Remote LoadLibrary failed!\n");
        return false;
    }
    return true;
}


inject_result remote_dll_injector::inject_dllimage(void* targetProcessHandle)
{
    // Set DEBUG priviledge ENABLED for the Current process to get sufficient rights
    if (!DebugPriviledgeSet)
        enable_debug_priviledge();

    inject_result result = { targetProcessHandle };

    // allocate memory for the image
    PIMAGE_DOS_HEADER DOS = (PIMAGE_DOS_HEADER)Image;
    if (DOS->e_magic != IMAGE_DOS_SIGNATURE) // check signature
    {
        logsec(secFF, "Invalid DOS Image signature\n");
        return result;
    }

    PIMAGE_NT_HEADERS NT = (PIMAGE_NT_HEADERS)((LPBYTE)Image + DOS->e_lfanew);
    if (!(NT->FileHeader.Characteristics & IMAGE_FILE_DLL)) // ensure it's a DLL
    {
        logsec(secFF, "Image file is not a DLL\n");
        return result;
    }

    HANDLE hProcess = targetProcessHandle;
    PCHAR image = (PCHAR)shadow_valloc(hProcess, nullptr, NT->OptionalHeader.SizeOfImage, PAGE_EXECUTE_READWRITE); // Allocate memory for the DLL
    if (!image)
    {
        logsec(secFF, "Failed to allocate in remote process\n");
        return result;
    }

    // Copy headers into target process
    if (!shadow_vwrite(hProcess, image, Image, NT->OptionalHeader.SizeOfHeaders))
    {
        logsec(secFF, "Unable to copy headers to target process\n");
        shadow_vfree(hProcess, image);
        return result;
    }

    // Copy sections to target process
    PIMAGE_SECTION_HEADER ISH = IMAGE_FIRST_SECTION(NT);
    for (int i = 0; i < (int)NT->FileHeader.NumberOfSections; ++i)
    {
        IMAGE_SECTION_HEADER& sec = ISH[i];
        shadow_vwrite(hProcess, image + sec.VirtualAddress, (char*)Image + sec.PointerToRawData, sec.SizeOfRawData);
    }

    // allocate memory for the loader code in the remote process
    PCHAR loader = (PCHAR)shadow_valloc(hProcess, nullptr, 4096, PAGE_EXECUTE_READWRITE); // Allocate memory for the loader code
    if (!loader)
    {
        logsec(secFF, "Unable to allocate memory for loader in remote process\n");
        shadow_vfree(hProcess, image);
        return result;
    }



    MANUAL_INJECT ManualInject = { nullptr };
    ManualInject.ImageBase        = image;
    ManualInject.NtHeaders        = (PIMAGE_NT_HEADERS)(image + DOS->e_lfanew);
    ManualInject.BaseRelocation   = (PIMAGE_BASE_RELOCATION)(image + NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
    ManualInject.ImportDirectory  = (PIMAGE_IMPORT_DESCRIPTOR)(image + NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    ManualInject.fnLoadLibraryA   = (pLoadLibraryA)shadow_kernel_getproc(LoadLibraryAName.to_string().c_str());
    ManualInject.fnGetProcAddress = (pGetProcAddress)shadow_kernel_getproc(GetProcAddressName.to_string().c_str());

    // Write the loader information to target process
    shadow_vwrite(hProcess, loader, &ManualInject, sizeof(ManualInject));
    // Write the loader code to target process
    shadow_vwrite(hProcess, (PVOID)((PMANUAL_INJECT)loader+1), LoadDll, (DWORD)LoadDllEnd-(DWORD)LoadDll);


    // Create a remote thread to execute the loader code
    HANDLE hThread = shadow_create_thread(hProcess, (LPTHREAD_START_ROUTINE)((PMANUAL_INJECT)loader + 1), loader);
    if (!hThread)
    {
        logsec(secFF, "Unable to create remote thread for loader\n");
        shadow_vfree(hProcess, loader);
        shadow_vfree(hProcess, image);
        return result;
    }

    // Wait for loader thread to finish execution and grab the results
    ManualInjectResult returnCode;
    while (GetExitCodeThread(hThread, (DWORD*)&returnCode) && int(returnCode) == STILL_ACTIVE)
        Sleep(1); // wait for it...

    CHAR errString[128] = "Unknown";
    CHAR* loaderProcErrPtr = 0; // pointer to errmsg in loader proc

    shadow_vread(hProcess, loader, &loaderProcErrPtr, sizeof PCHAR);
    if (loaderProcErrPtr)
        shadow_vread(hProcess, loaderProcErrPtr, errString, 128);

    // Close thread and unload loader from memory
    shadow_close_handle(hThread);
    shadow_vfree(hProcess, loader);

    if (returnCode != DllMainSuccess) // on failure release the image
    {
        logsec(secFF, "Loader failed during runtime (ErrCode %d): "
               "%s in module %s\n", returnCode, enum_cstr(returnCode), errString);
        shadow_vfree(hProcess, image);
        return result;
    }

    logsec(secOK, "Loader finished successfully\n");

    // we can't release the image since the remote process might now be using it :)
    result.DllImage = image;
    return result;
}



bool remote_dll_injector::reserve_target_memory(void* targetProcessHANDLE, int reserveSize)
{
    // Set DEBUG priviledge ENABLED for the Current process to get sufficient rights
    if (!DebugPriviledgeSet)
        enable_debug_priviledge();

    MEMORY_BASIC_INFORMATION info;
    void* hProcess    = targetProcessHANDLE;
    PCHAR baseAddress = PCHAR(0x00400000);
    logsec(secOK, "Reserve up to: %8x\n", baseAddress + reserveSize);
    shadow_vprint(hProcess, baseAddress, reserveSize);

    // find the first free block after ImageBase
    PCHAR nextBlock = baseAddress;
    for (;;)
    {
        shadow_vquery(hProcess, nextBlock, info);
        if (info.State == MEM_FREE)
            break;
        nextBlock += info.RegionSize;
    }

    if (int rem = (int)nextBlock % 0x10000) // align nextblock to 64KB boundary
        nextBlock += 0x10000 - rem;

    int imageSize = nextBlock - baseAddress;
    int extraReserve = reserveSize - imageSize;
    if (extraReserve < 0)
        return true; // OK, we have enough mem reserved

    // align extraReserve to 64KB
    if (int rem = extraReserve % 0x10000)
        extraReserve += 0x10000 - rem;
    //extraReserve += 0x10000; // add 64KB buffer after the block we want

    // only reserve, no commit
    PVOID mem = shadow_valloc(hProcess, nextBlock, extraReserve);
    if (!mem)
    {
        logsec(secFF, "reserve_target_memory 0x%p failed: %s\n", nextBlock, shadow_getsyserr());
        return false;
    }

    logsec(secOK, "reserve_target_memory 0x%p success\n", mem);
    logsec(secOK, "After reserve: \n");
    shadow_vprint(hProcess, baseAddress, reserveSize);
    return true;
}