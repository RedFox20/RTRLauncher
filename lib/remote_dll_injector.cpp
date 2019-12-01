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

using pLoadLibraryA   = HMODULE (WINAPI*)(LPCSTR);
using pGetProcAddress = FARPROC (WINAPI*)(HMODULE,LPCSTR);
using pDllMain        = BOOL    (WINAPI*)(HMODULE,DWORD,PVOID);
using pWriteConsoleA  = BOOL    (WINAPI*)(HANDLE, const void* text, DWORD len, LPDWORD written, LPVOID);
using pGetStdHandle   = HANDLE  (WINAPI*)(DWORD);
using pAllocConsole   = BOOL    (WINAPI*)();


Enum(ManualInjectResult,
    DllMainFailed,         // DllMain returned FALSE - failed
    DllMainSuccess,        // DllMain returned TRUE - success
    DllImportFailed,       // A DLL import totally failed
    GetProcOrdinalFailed,  // Get Proc by Ordinal failed
    GetProcNameFailed      // Get Proc by Name failed
);

// Inline string copy, because we can't link against strcpy in the injected code thread
#define CopyString(dst, src, max) do { \
    char* __dst = dst; \
    char* __src = src; \
    for (int i = 0; i < (max) && (__dst[i] = __src[i]) != '\0'; ++i); \
    } while (false); 

typedef struct _MANUAL_INJECT
{
    char Err[128];
    PVOID                       ImageBase;
    PIMAGE_NT_HEADERS           NtHeaders;
    PIMAGE_BASE_RELOCATION      BaseRelocation;
    PIMAGE_IMPORT_DESCRIPTOR    ImportDirectory;
    pLoadLibraryA    fnLoadLibraryA;
    pGetProcAddress  fnGetProcAddress;
    pWriteConsoleA   fnWriteConsoleA;
    pGetStdHandle    fnGetStdHandle;
    pAllocConsole    fnAllocConsole;
} MANUAL_INJECT, *PMANUAL_INJECT;

#pragma runtime_checks("", off) // disable any runtime checks (crashtastic)
#pragma strict_gs_check(off) // disable security cookie
static DWORD WINAPI LoadDll(PMANUAL_INJECT Injector)
{
    PIMAGE_BASE_RELOCATION IBR   = Injector->BaseRelocation;
    PIMAGE_IMPORT_DESCRIPTOR IID = Injector->ImportDirectory;
    auto ImgBase = static_cast<PCHAR>(Injector->ImageBase);
    auto delta   = DWORD(ImgBase - Injector->NtHeaders->OptionalHeader.ImageBase); // Calculate the delta
    
    Injector->fnAllocConsole();
    HANDLE STDOUT = Injector->fnGetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;

    Injector->fnWriteConsoleA(STDOUT, "RelocatingTheImage", sizeof("RelocatingTheImage")-1, &written, nullptr);
    while (IBR->VirtualAddress) // Relocate the image
    {
        if (IBR->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
        {
            DWORD count = DWORD(IBR->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            auto list = PWORD(IBR + 1);

            for (DWORD i = 0; i < count; ++i)
            {
                if (list[i])
                {
                    auto ptr = PDWORD(ImgBase + (IBR->VirtualAddress + (list[i] & 0xFFF)));
                    *ptr += delta;
                }
            }
        }
        IBR = PIMAGE_BASE_RELOCATION(PBYTE(IBR) + IBR->SizeOfBlock);
    }

    Injector->fnWriteConsoleA(STDOUT, "ResolveDLLImports", sizeof("ResolveDLLImports")-1, &written, nullptr);
    while (IID->Characteristics) // Resolve DLL imports
    {
        auto OrigFirstThunk = PIMAGE_THUNK_DATA(ImgBase + IID->OriginalFirstThunk);
        auto FirstThunk     = PIMAGE_THUNK_DATA(ImgBase + IID->FirstThunk);

        HMODULE hModule = Injector->fnLoadLibraryA(ImgBase + IID->Name);
        if (!hModule)
        {
            CopyString(Injector->Err, ImgBase + IID->Name, sizeof(Injector->Err));
            return DllImportFailed;
        }

        while (OrigFirstThunk->u1.AddressOfData)
        {
            if (OrigFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
            {
                // Import by ordinal
                auto ProcName = PCHAR(OrigFirstThunk->u1.Ordinal & 0xFFFF);
                auto Function = DWORD(Injector->fnGetProcAddress(hModule, ProcName));
                if (!Function)
                {
                    CopyString(Injector->Err, ProcName, sizeof(Injector->Err));
                    return GetProcOrdinalFailed;
                }
                FirstThunk->u1.Function = Function;
            }
            else
            {
                // Import by name
                auto IBN = PIMAGE_IMPORT_BY_NAME(ImgBase + OrigFirstThunk->u1.AddressOfData);
                auto Function = DWORD(Injector->fnGetProcAddress(hModule, IBN->Name));
                if (!Function)
                {
                    CopyString(Injector->Err, IBN->Name, sizeof(Injector->Err));
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
        Injector->fnWriteConsoleA(STDOUT, "CallEntryPoint", sizeof("CallEntryPoint")-1, &written, nullptr);
        auto EntryPoint = pDllMain(ImgBase + Injector->NtHeaders->OptionalHeader.AddressOfEntryPoint);
        return EntryPoint(HMODULE(ImgBase), 1, nullptr); // Call the entry point
    }
    return DllMainSuccess; // EntryPoint is optional for this Manual Injection
}
#pragma runtime_checks("", restore) // restore any specified runtime checks





typedef struct _FILE_INJECT
{
    pLoadLibraryA  fnLoadLibraryA;
    pWriteConsoleA fnWriteConsoleA;
    pGetStdHandle  fnGetStdHandle;
    pAllocConsole  fnAllocConsole;
    CHAR           LibName[MAX_PATH];
} FILE_INJECT, *PFILE_INJECT;
#pragma runtime_checks("", off) // disable any runtime checks (crashtastic)
#pragma strict_gs_check(off) // disable security cookie
static DWORD WINAPI FileLoadDll(PFILE_INJECT Injector)
{
    //// We don't want to pass LoadLibraryA as the CreateRemoteThread
    //// startup function - that method is easily detectable.
    //// That's why we use this proxy instead
    Injector->fnAllocConsole();
    HANDLE STDOUT = Injector->fnGetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    Injector->fnWriteConsoleA(STDOUT, "FileLoadDll\n", 12, &written, nullptr);
    int len = 0; while (Injector->LibName[len]) ++len;
    Injector->fnWriteConsoleA(STDOUT, Injector->LibName, len, &written, nullptr);

    return (DWORD)Injector->fnLoadLibraryA(Injector->LibName);
}
#pragma runtime_checks("", restore) // restore any specified runtime checks






void inject_result::unload()
{
    if (Process && Process != INVALID_HANDLE_VALUE)
    {
        if (DllImage) // free remote DLL image
        {
            VirtualFreeEx(Process, DllImage, 0, MEM_RELEASE);
            DllImage = nullptr;
        }
        Process = nullptr;
    }
}

remote_dll_injector::remote_dll_injector()
{
    Image	 = nullptr;
    Resource = nullptr;
}
remote_dll_injector::remote_dll_injector(int resourceId)
{
    Resource = LoadResource(nullptr, FindResourceA(nullptr, MAKEINTRESOURCEA(resourceId), RT_RCDATA));
    Image    = LockResource(Resource);
}
remote_dll_injector::remote_dll_injector(void* pInjectableDllImage)
{
    Image	 = pInjectableDllImage;
    Resource = nullptr;
}





remote_dll_injector::~remote_dll_injector()
{
    if (Resource)
        FreeResource(Resource);
}

static const char secOK[] = "[==] Injector [==]";
static const char secFF[] = "[!!] Injector [!!]";
static bool DebugPrivilegeSet = false;

void remote_dll_injector::enable_debug_privilege()
{
    logsec(secOK, "Enabling Debug Privilege\n");
    if (shadow_enable_debug_privilege(GetCurrentProcess()))
    {
        DebugPrivilegeSet = true;
        logsec(secOK, "CURRENT_PROCESS Debug Privilege SET SUCCESS\n");
    }
    else logsec(secFF, "CURRENT_PROCESS Debug Privilege SET FAILED\n");
}

constexpr auto LoadLibraryAName = make_obfuscated("LoadLibraryA");
constexpr auto GetProcAddressName = make_obfuscated("GetProcAddress");

bool validate_injected_code(HANDLE process, PCHAR codeAddress, void* referenceCode, DWORD codeSize)
{
    std::vector<CHAR> buffer; buffer.resize(codeSize);
    shadow_vread(process, codeAddress, buffer.data(), codeSize);
    return memcmp(buffer.data(), referenceCode, codeSize) == 0;
}


bool remote_dll_injector::inject_dllfile(void* targetProcessHandle, const char* filename)
{
    // Set DEBUG privilege ENABLED for the Current process to get sufficient rights
    if (!DebugPrivilegeSet)
        enable_debug_privilege();

    if (shadow_enable_debug_privilege(targetProcessHandle))
        logsec(secOK, "TARGET_PROCESS Debug Privilege SET SUCCESS\n");
    else
        logsec(secFF, "TARGET_PROCESS Debug Privilege SET FAILED\n");

    auto process = HANDLE(targetProcessHandle);
    // allocate memory for the loader code in the remote process
    //  === loader ===
    //  [ global data ]
    //  [ loader code ]
    DWORD codeSize = 8*1024; // @todo Calculate code size in a reliable way
    PCHAR loader = shadow_valloc(process, nullptr, codeSize + 4096, PAGE_EXECUTE_READWRITE); // Allocate memory for the loader code
    if (!loader)
    {
        logsec(secFF, "Unable to allocate memory for loader in remote process\n");
        return false;
    }

    FILE_INJECT FileInject;
    FileInject.fnLoadLibraryA  = shadow_kernel_getproc<pLoadLibraryA>("LoadLibraryA");
    FileInject.fnWriteConsoleA = shadow_kernel_getproc<pWriteConsoleA>("WriteConsoleA");
    FileInject.fnGetStdHandle  = shadow_kernel_getproc<pGetStdHandle>("GetStdHandle");
    FileInject.fnAllocConsole  = shadow_kernel_getproc<pAllocConsole>("AllocConsole");
    GetFullPathNameA(filename, sizeof(FileInject.LibName), FileInject.LibName, nullptr);

    logsec(secOK, "Allocated loader at: %p\n", loader);
    logsec(secOK, "Fullpath: %s\n", FileInject.LibName);

    // Write the loader information to target process
    shadow_vwrite(process, loader, &FileInject, sizeof(FileInject));

    PCHAR codeAddr = loader + sizeof(PFILE_INJECT);
    codeAddr += reinterpret_cast<int>(codeAddr) % 64; // align code to 64-byte boundary

    // Write the loader code to target process and validate it
    shadow_vwrite(process, codeAddr, FileLoadDll, codeSize);
    if (!validate_injected_code(process, codeAddr, FileLoadDll, codeSize))
        logsec(secFF, "Injected Code validation failed! Bytes do not match.\n");

    /// @note We never call FreeLibrary, because the remote process will be using it from now on
    HANDLE hThread = shadow_create_thread(process, LPTHREAD_START_ROUTINE(codeAddr), loader);

    // don't use WaitForSingleObject(hThread, INFINITE); here! Will deadlock if client DLL 
    // creates new thread during load, so we use a silly loop instead
    HMODULE loadedModule = nullptr;
    DWORD returnCode;
    // get return code, which is actually the loaded HMODULE
    while (GetExitCodeThread(hThread, &returnCode))
    {
        consolef("ReturnCode: %d\n", returnCode);
        if (returnCode != STILL_ACTIVE)
        {
            loadedModule = HMODULE(returnCode);
            break;
        }
        Sleep(100); // wait for it...
        consolef("waiting...\n");
    }

    consolef("error (if any): %s\n", shadow_getsyserr());
    consolef("error (if any): %s\n", shadow_getsyserr(returnCode));

    // Close thread and unload loader from memory
    shadow_close_handle(hThread);
    //shadow_vfree(process, loader);

    if (loadedModule == nullptr) // LoadLibrary failed
    {
        logsec(secFF, "Remote LoadLibrary failed! %s\n", shadow_getsyserr());
        return false;
    }
    return true;
}


inject_result remote_dll_injector::inject_dllimage(void* targetProcessHandle) const
{
    // Set DEBUG privilege ENABLED for the Current process to get sufficient rights
    if (!DebugPrivilegeSet)
        enable_debug_privilege();

    inject_result result = { targetProcessHandle, nullptr };

    // allocate memory for the image
    auto DOS = static_cast<PIMAGE_DOS_HEADER>(Image);
    if (DOS->e_magic != IMAGE_DOS_SIGNATURE) // check signature
    {
        logsec(secFF, "Invalid DOS Image signature\n");
        return result;
    }

    auto NT = reinterpret_cast<PIMAGE_NT_HEADERS>(PBYTE(Image) + DOS->e_lfanew);
    if (!(NT->FileHeader.Characteristics & IMAGE_FILE_DLL)) // ensure it's a DLL
    {
        logsec(secFF, "Image file is not a DLL\n");
        return result;
    }

    HANDLE hProcess = targetProcessHandle;
    PCHAR image = shadow_valloc(hProcess, nullptr, NT->OptionalHeader.SizeOfImage, PAGE_EXECUTE_READWRITE); // Allocate memory for the DLL
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
    for (int i = 0; i < int(NT->FileHeader.NumberOfSections); ++i)
    {
        IMAGE_SECTION_HEADER& sec = ISH[i];
        shadow_vwrite(hProcess, image + sec.VirtualAddress, PCHAR(Image) + sec.PointerToRawData, sec.SizeOfRawData);
    }

    // allocate memory for the loader code in the remote process
    //  === loader ===
    //  [ global data ]
    //  [ loader code ]
    DWORD codeSize = 8*1024; // @todo Calculate code size in a reliable way
    PCHAR loader = shadow_valloc(hProcess, nullptr, codeSize + 4096, PAGE_EXECUTE_READWRITE); // Allocate memory for the loader code
    if (!loader)
    {
        logsec(secFF, "Unable to allocate memory for loader in remote process\n");
        shadow_vfree(hProcess, image);
        return result;
    }

    logsec(secOK, "Allocated loader at: %p\n", loader);

    MANUAL_INJECT ManualInject;
    ManualInject.Err[0]    = '\0';
    ManualInject.ImageBase = image;
    ManualInject.NtHeaders        =        PIMAGE_NT_HEADERS(image + DOS->e_lfanew);
    ManualInject.BaseRelocation   =   PIMAGE_BASE_RELOCATION(image + NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
    ManualInject.ImportDirectory  = PIMAGE_IMPORT_DESCRIPTOR(image + NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    ManualInject.fnLoadLibraryA   = shadow_kernel_getproc<pLoadLibraryA>(LoadLibraryAName.to_string().c_str());
    ManualInject.fnGetProcAddress = shadow_kernel_getproc<pGetProcAddress>(GetProcAddressName.to_string().c_str());
    ManualInject.fnWriteConsoleA  = shadow_kernel_getproc<pWriteConsoleA>("WriteConsoleA");
    ManualInject.fnGetStdHandle   = shadow_kernel_getproc<pGetStdHandle>("GetStdHandle");
    ManualInject.fnAllocConsole   = shadow_kernel_getproc<pAllocConsole>("AllocConsole");

    // Write the loader information to target process
    shadow_vwrite(hProcess, loader, &ManualInject, sizeof(ManualInject));

    PCHAR codeAddr = loader + sizeof(PMANUAL_INJECT);
    codeAddr += reinterpret_cast<int>(codeAddr) % 64; // align code to 64-byte boundary

    // Write the loader code to target process
    shadow_vwrite(hProcess, codeAddr, &LoadDll, codeSize);
    if (!validate_injected_code(hProcess, codeAddr, &LoadDll, codeSize))
        logsec(secFF, "Injected Code validation failed! Bytes do not match.\n");

    // Create a remote thread to execute the loader code
    HANDLE hThread = shadow_create_thread(hProcess, LPTHREAD_START_ROUTINE(codeAddr), loader);
    if (!hThread)
    {
        logsec(secFF, "Unable to create remote thread for loader\n");
        shadow_vfree(hProcess, loader);
        shadow_vfree(hProcess, image);
        return result;
    }

    // Wait for loader thread to finish execution and grab the results
    DWORD returnCode;
    while (GetExitCodeThread(hThread, &returnCode)
        && int(returnCode) == STILL_ACTIVE)
    {
        Sleep(10); // wait for it...
    }

    auto injectResult = ManualInjectResult(returnCode);

    // load back the injection struct to get the error string
    shadow_vread(hProcess, loader, &ManualInject, sizeof(ManualInject));

    // Close thread and unload loader from memory
    shadow_close_handle(hThread);
    shadow_vfree(hProcess, loader);

    if (injectResult != DllMainSuccess) // on failure release the image
    {
        logsec(secFF, "Loader failed during runtime (ErrCode %d): "
               "'%s' (%s) in module %s\n",
            returnCode, enum_cstr(injectResult), shadow_getsyserr(returnCode), ManualInject.Err);

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
    if (!DebugPrivilegeSet)
        enable_debug_privilege();

    MEMORY_BASIC_INFORMATION info;
    void* hProcess   = targetProcessHANDLE;
    auto baseAddress = PCHAR(0x00400000);
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