#include "remote_dll_injector.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <log.h>
#include <shadowlib.h>
#include <rpp/obfuscated_string.h>
#include <rpp/scope_guard.h>
#include <stdexcept>

/**
 * Copyright notes: Original code written by zwclose7
 */

using pLoadLibraryA   = HMODULE (WINAPI*)(LPCSTR);
using pGetProcAddress = FARPROC (WINAPI*)(HMODULE,LPCSTR);
using pDllMain        = BOOL    (WINAPI*)(HMODULE,DWORD,PVOID);
using pWriteConsoleA  = BOOL    (WINAPI*)(HANDLE, const void* text, DWORD len, LPDWORD written, LPVOID);
using pGetStdHandle   = HANDLE  (WINAPI*)(DWORD);
using pAllocConsole   = BOOL    (WINAPI*)();


enum ManualInjectResult
{
    DllMainFailed,         // DllMain returned FALSE - failed
    DllMainSuccess,        // DllMain returned TRUE - success
    DllImportFailed,       // A DLL import totally failed
    GetProcOrdinalFailed,  // Get Proc by Ordinal failed
    GetProcNameFailed      // Get Proc by Name failed
};

const char* string(ManualInjectResult result)
{
    switch (result)
    {
        case DllMainFailed:    return "DllMainFailed";
        case DllMainSuccess:   return "DllMainSuccess";
        case DllImportFailed:  return "DllImportFailed";
        case GetProcOrdinalFailed: return "GetProcOrdinalFailed";
        case GetProcNameFailed:    return "GetProcNameFailed";
        default: return "invalid ManualInjectResult";
    }
}


// Inline string copy, because we can't link against strcpy in the injected code thread
#define CopyString(dst, src, max) do { \
    char* __dst = dst; \
    char* __src = src; \
    for (int i = 0; i < (max) && (__dst[i] = __src[i]) != '\0'; ++i); \
    } while (false); 

struct MANUAL_INJECT
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
};

#pragma runtime_checks("", off) // disable any runtime checks (crashtastic)
#pragma strict_gs_check(off) // disable security cookie
static DWORD WINAPI LoadDll(MANUAL_INJECT* Injector)
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





struct FILE_INJECT
{
    HMODULE LoadedModule = nullptr;
    pLoadLibraryA  fnLoadLibraryA;
    pWriteConsoleA fnWriteConsoleA;
    pGetStdHandle  fnGetStdHandle;
    pAllocConsole  fnAllocConsole;
    CHAR           LibName[MAX_PATH];
};
#pragma runtime_checks("", off) // disable any runtime checks (crashtastic)
#pragma strict_gs_check(off) // disable security cookie
static DWORD WINAPI FileLoadDll(FILE_INJECT* injector)
{
    //// We don't want to pass LoadLibraryA as the CreateRemoteThread
    //// startup function - that method is easily detectable.
    //// That's why we use this proxy instead
    injector->LoadedModule = injector->fnLoadLibraryA(injector->LibName);
    if (injector->LoadedModule == nullptr)
        return GetLastError(); // loader failed!
    return 0;
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
    Image    = nullptr;
    Resource = nullptr;
}
remote_dll_injector::remote_dll_injector(int resourceId)
{
    Resource = LoadResource(nullptr, FindResourceA(nullptr, MAKEINTRESOURCEA(resourceId), RT_RCDATA));
    Image    = LockResource(Resource);
}
remote_dll_injector::remote_dll_injector(void* pInjectableDllImage)
{
    Image    = pInjectableDllImage;
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


void remote_dll_injector::inject_dllfile(void* targetProcessHandle, const char* filename)
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
    DWORD codeSize = 4096; // @todo Calculate code size in a reliable way
    PCHAR loader_data = shadow_valloc(process, nullptr, 4096, PAGE_READWRITE);
    PCHAR loader_code = shadow_valloc(process, nullptr, codeSize, PAGE_EXECUTE_READWRITE);
    scope_guard([&] {
        if (loader_data) shadow_vfree(process, loader_data);
        if (loader_code) shadow_vfree(process, loader_code);
    });
    if (!loader_data) throw std::runtime_error{"Unable to allocate memory for loader DATA in remote process"};
    if (!loader_code) throw std::runtime_error{"Unable to allocate memory for loader CODE in remote process"};

    FILE_INJECT FileInject;
    FileInject.fnLoadLibraryA  = shadow_kernel_getproc<pLoadLibraryA>("LoadLibraryA");
    FileInject.fnWriteConsoleA = shadow_kernel_getproc<pWriteConsoleA>("WriteConsoleA");
    FileInject.fnGetStdHandle  = shadow_kernel_getproc<pGetStdHandle>("GetStdHandle");
    FileInject.fnAllocConsole  = shadow_kernel_getproc<pAllocConsole>("AllocConsole");
    GetFullPathNameA(filename, sizeof(FileInject.LibName), FileInject.LibName, nullptr);

    logsec(secOK, "Allocated loader DATA: %p  CODE: %p\n", loader_data, loader_code);
    logsec(secOK, "Fullpath: %s\n", FileInject.LibName);

    // Write the loader DATA and CODE to target process
    shadow_vwrite(process, loader_data, &FileInject, sizeof(FileInject));
    shadow_vwrite(process, loader_code, FileLoadDll, codeSize);

    /// @note We never call FreeLibrary, because the remote process will be using it from now on
    HANDLE thread = shadow_create_thread(process, LPTHREAD_START_ROUTINE(loader_code), loader_data);

    // don't use WaitForSingleObject(hThread, INFINITE); here! Will deadlock if client DLL 
    // creates new thread during load, so we use a silly loop instead

    // Wait for loader thread to finish execution and grab the results
    ExitStatus status;
    while ((status = shadow_get_thread_status(thread)))
        Sleep(10); // wait for it...

    // Read back the result data
    logsec(secOK, "Reading back loader DATA\n");
    shadow_vread(process, loader_data, &FileInject, sizeof(FileInject));
    shadow_close_handle(thread);

    if (FileInject.LoadedModule == nullptr) // LoadLibrary failed
        throw std::runtime_error{"Remote LoadLibrary failed!"};
}


void remote_dll_injector::inject_dllimage(void* targetProcessHandle) const
{
    // Set DEBUG privilege ENABLED for the Current process to get sufficient rights
    if (!DebugPrivilegeSet)
        enable_debug_privilege();

    // allocate memory for the image
    auto DOS = static_cast<PIMAGE_DOS_HEADER>(Image);
    if (DOS->e_magic != IMAGE_DOS_SIGNATURE) // check signature
        throw std::runtime_error{"Invalid DOS Image signature"};

    auto NT = reinterpret_cast<PIMAGE_NT_HEADERS>(PBYTE(Image) + DOS->e_lfanew);
    if (!(NT->FileHeader.Characteristics & IMAGE_FILE_DLL)) // ensure it's a DLL
        throw std::runtime_error{"Image file is not a DLL"};

    HANDLE process = targetProcessHandle;
    PCHAR image = shadow_valloc(process, nullptr, NT->OptionalHeader.SizeOfImage, PAGE_EXECUTE_READWRITE); // Allocate memory for the DLL
    if (!image) throw std::runtime_error{"Failed to allocate in remote process"};
    bool freeImage = true;
    scope_guard([&] { if (freeImage) shadow_vfree(process, image); });

    // Copy headers into target process
    if (!shadow_vwrite(process, image, Image, NT->OptionalHeader.SizeOfHeaders))
        throw std::runtime_error{"Unable to copy headers to target process"};

    // Copy sections to target process
    PIMAGE_SECTION_HEADER ISH = IMAGE_FIRST_SECTION(NT);
    for (int i = 0; i < int(NT->FileHeader.NumberOfSections); ++i)
    {
        IMAGE_SECTION_HEADER& sec = ISH[i];
        shadow_vwrite(process, image + sec.VirtualAddress, PCHAR(Image) + sec.PointerToRawData, sec.SizeOfRawData);
    }

    // allocate memory for the loader code in the remote process
    //  === loader ===
    //  [ global data ]
    //  [ loader code ]
    DWORD codeSize = 16*1024; // @todo Calculate code size in a reliable way
    PCHAR loader_data = shadow_valloc(process, nullptr, 4096, PAGE_READWRITE);
    PCHAR loader_code = shadow_valloc(process, nullptr, codeSize, PAGE_EXECUTE_READWRITE);
    scope_guard([&] {
        if (loader_data) shadow_vfree(process, loader_data);
        if (loader_code) shadow_vfree(process, loader_code);
    });
    if (!loader_data) throw std::runtime_error{"Unable to allocate memory for loader DATA in remote process"};
    if (!loader_code) throw std::runtime_error{"Unable to allocate memory for loader CODE in remote process"};

    logsec(secOK, "Allocated loader DATA: %p  CODE: %p\n", loader_data, loader_code);
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

    // Write the loader DATA and CODE to target process
    shadow_vwrite(process, loader_data, &ManualInject, sizeof(ManualInject));
    shadow_vwrite(process, loader_code, &LoadDll, codeSize);

    // Create a remote thread to execute the loader code
    HANDLE thread = shadow_create_thread(process, LPTHREAD_START_ROUTINE(loader_code), loader_data);
    if (!thread) throw std::runtime_error{"Unable to create remote thread for loader."};

    // Wait for loader thread to finish execution and grab the results
    ExitStatus status;
    while ((status = shadow_get_thread_status(thread)))
        Sleep(10); // wait for it...

    // load back the injection struct to get the error string
    logsec(secOK, "Reading back loader DATA\n");
    shadow_vread(process, loader_data, &ManualInject, sizeof(ManualInject));
    shadow_close_handle(thread);

    auto injectResult = ManualInjectResult(status.exit_code);
    if (injectResult != DllMainSuccess) // on failure release the image
    {
        logsec(secFF, "Loader failed during runtime (ErrCode %d): '%s' (%s) in module %s\n",
            status.exit_code, string(injectResult), shadow_getsyserr(status.exit_code), ManualInject.Err);
        throw std::runtime_error{"Loader failed during runtime."};
    }

    logsec(secOK, "Loader finished successfully\n");
    // WARNING: we can't release the image since the remote process might now be using it
    freeImage = false;
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