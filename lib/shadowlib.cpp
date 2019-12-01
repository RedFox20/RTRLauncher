/**
 * Copyright (c) 2014 - ShadowFox ^_^
 */
#include "shadowlib.h"
#include <log.h>
#include <rpp/obfuscated_string.h>
#include <tlhelp32.h>
#define PSAPI_VERSION 1
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")
// ReSharper disable StringLiteralTypo
// ReSharper disable IdentifierTypo

#ifndef NOINLINE
#define NOINLINE __declspec(noinline)
#endif

#define SHADOWLIB_FORCE_DEBUG 1
#if _DEBUG || DEBUG || SHADOWLIB_FORCE_DEBUG
#  define indebug(...) __VA_ARGS__ 
#else
#  define indebug(...)
#endif

constexpr bool USER_LAND_FUNCTIONS = true;

typedef long (WINAPI*fnNtQueryVirtualMemory)   (void*, void*, DWORD, void*, ULONG, ULONG*);
typedef long (WINAPI*fnNtAllocateVirtualMemory)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);
typedef long (WINAPI*fnNtReadVirtualMemory)    (void*, void*, OUT void**, SIZE_T, OUT SIZE_T*);
typedef long (WINAPI*fnNtWriteVirtualMemory)   (void*, void*, void*, ULONG, OUT ULONG*);
typedef long (WINAPI*fnNtFreeVirtualMemory)    (void*, IN OUT void**, IN OUT ULONG*, ULONG);
typedef BOOL (WINAPI*fnVirtualFreeEx)          (void*, void*, SIZE_T, DWORD);
typedef long (WINAPI*fnNtUnmapViewOfSection)   (void*, void*);
typedef long (WINAPI*fnNtOpenThread)           (void*, DWORD, void*, void*);
typedef long (WINAPI*fnNtClose)                (void*);
typedef long (WINAPI*fnNtGetContextThread)     (void*, CONTEXT*);
typedef long (WINAPI*fnNtSetContextThread)     (void*, CONTEXT*);
typedef long (WINAPI*fnNtSuspendThread)        (void*, ULONG*);
typedef long (WINAPI*fnNtAlertResumeThread)    (void*, ULONG*);
typedef long (WINAPI*fnNtCreateThreadEx)       (OUT PHANDLE hThread, ACCESS_MASK DesiredAccess, LPVOID ObjectAttributes, HANDLE ProcessHandle, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, BOOL CreateSuspended, ULONG StackZeroBits, ULONG SizeOfStackCommit, ULONG SizeOfStackReserve, LPVOID lpBytesBuffer);
typedef long (WINAPI*fnNtCreateProcess)        (OUT PHANDLE ProcessHandle, IN ACCESS_MASK DesiredAccess, IN void* ObjectAttributes, IN HANDLE InheritFromProcessHandle, IN BOOLEAN InheritHandles, IN HANDLE SectionHandle OPTIONAL, IN HANDLE DebugPort OPTIONAL, IN HANDLE ExceptionPort OPTIONAL);


static const char* memory_constant(DWORD value)
{
    switch (value)
    {
        case PAGE_NOACCESS:          return "NONE";
        case PAGE_READONLY:          return "R   ";
        case PAGE_READWRITE:         return "RW  ";
        case PAGE_WRITECOPY:         return "WCP ";
        case PAGE_EXECUTE:           return "E   ";
        case PAGE_EXECUTE_READ:      return "ER  ";
        case PAGE_EXECUTE_READWRITE: return "ERW ";
        case PAGE_EXECUTE_WRITECOPY: return "EWCP";
        case PAGE_GUARD:             return "GRD ";
        case PAGE_NOCACHE:           return "NCHE";
        case PAGE_WRITECOMBINE:      return "WCOM";
        case MEM_COMMIT:             return "CMIT";
        case MEM_RESERVE:            return "RESV";
        case MEM_FREE:               return "FREE";
        case MEM_PRIVATE:            return "PRIV";
        case MEM_MAPPED:             return "MAPD";
        case MEM_IMAGE:              return "IMAG";
    }
    return "INV@";
}
void NOINLINE shadow_vprint(MEMORY_BASIC_INFORMATION& i)
{
    log("ba: %8x ab: %8x ap: %s rz: %8x st: %s pr: %s ty: %s\n", i.BaseAddress, i.AllocationBase, 
           memory_constant(i.AllocationProtect), i.RegionSize,
           memory_constant(i.State), memory_constant(i.Protect), memory_constant(i.Type));
}
void NOINLINE shadow_vprint(HANDLE process, void* baseAddress, DWORD regionSize)
{
    MEMORY_BASIC_INFORMATION info;
    auto nextBlock  = PCHAR(baseAddress);
    PCHAR endAddress = nextBlock + regionSize;
    while (nextBlock < endAddress && shadow_vquery(process, nextBlock, info))
    {
        shadow_vprint(info);
        nextBlock = static_cast<PCHAR>(info.BaseAddress) + info.RegionSize;
    }
}
void shadow_vprint(void* baseAddress, DWORD regionSize)
{
    shadow_vprint(HANDLE(-1), baseAddress, regionSize);
}



constexpr uint32_t FUNCTION_MASK = 0xCAFEBABE;

#define NT_LOAD_FUNCTION(FuncName, failedResult) \
    constexpr auto ob##FuncName = make_obfuscated(#FuncName); \
    static fn##FuncName p##FuncName; \
    if (!p##FuncName) { \
        p##FuncName = (fn##FuncName)shadow_ntdll_getproc((ob##FuncName).to_string().c_str()); \
        if (!p##FuncName) { indebug(log("nlzf %s failed\n", (ob##FuncName).to_string().c_str())); \
        *(uint32_t*)(&p##FuncName) = (uint32_t)(p##FuncName) ^ FUNCTION_MASK; \
        return failedResult; } \
    }

#define UNMASK_FUNCTION(FuncName) ((decltype(p##FuncName))(uint32_t(p##FuncName) ^ FUNCTION_MASK))


#define DebugUserLandResult(result, function) \
    indebug(if (!(result)) log(function" failed: %p\nReason: %s\n", result, shadow_getsyserr()));


#define DebugNtResult(result, function) \
    indebug(if (result) log(function" failed: %p\nReason: %s\n", result, shadow_getsyserr()));


bool NOINLINE shadow_vquery(HANDLE process, void* address, MEMORY_BASIC_INFORMATION& info)
{
    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = VirtualQueryEx(process, address, &info, sizeof info) > 0;
        DebugUserLandResult(result, "VirtualQueryEx");
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtQueryVirtualMemory, false);
        long result = UNMASK_FUNCTION(NtQueryVirtualMemory)(process, address, 0, &info, sizeof info, nullptr);
        DebugNtResult(result, "NtQueryVirtualMemory");
        return result == 0;
    }
}
bool shadow_vquery(void* address, MEMORY_BASIC_INFORMATION& info)
{
    return shadow_vquery(HANDLE(-1), address, info);
}

PVOID NOINLINE shadow_valloc(HANDLE process, void* address, DWORD size, DWORD protect, DWORD flags)
{
    log("valloc %p %p %d %x %x => ", process, address, size, protect, flags);
    if (USER_LAND_FUNCTIONS)
    {
        PVOID result = VirtualAllocEx(process, address, size, flags, protect);
        DebugUserLandResult(result, "VirtualAllocEx");
        shadow_vprint(process, result, 1);
        return result;
    }
    else
    {
        DWORD filtered = 0; // only accept these filtered flags:
        if (flags & MEM_COMMIT)  filtered |= MEM_COMMIT;
        if (flags & MEM_RESERVE) filtered |= MEM_RESERVE;

        NT_LOAD_FUNCTION(NtAllocateVirtualMemory, nullptr);
        void* addressBase = address;
        SIZE_T regionSize = size;
        long result = UNMASK_FUNCTION(NtAllocateVirtualMemory)(process, &addressBase, 0, &regionSize, filtered, protect);
        DebugNtResult(result, "NtAllocateVirtualMemory");
        return result == 0 ? addressBase : nullptr;
    }
}
void* shadow_valloc(void* address, DWORD size, DWORD protect, DWORD flags)
{
    return shadow_valloc(HANDLE(-1), address, size, protect, flags);
}




bool NOINLINE shadow_vread(HANDLE process, void* address, void* buffer, DWORD size)
{
    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = ReadProcessMemory(process, address, buffer, size, nullptr);
        DebugUserLandResult(result, "ReadProcessMemory");
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtReadVirtualMemory, false);
        long result = UNMASK_FUNCTION(NtReadVirtualMemory)(process, address, &buffer, size, nullptr);
        DebugNtResult(result, "NtReadVirtualMemory");
        return result == 0;
    }
}
bool shadow_vread(void* address, void* buffer, DWORD size)
{
    return shadow_vread(HANDLE(-1), address, buffer, size);
}

bool NOINLINE shadow_vwrite(HANDLE process, void* address, void* buffer, DWORD size)
{
    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = WriteProcessMemory(process, address, buffer, size, nullptr);
        DebugUserLandResult(result, "WriteProcessMemory");
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtWriteVirtualMemory, false);
        long result = UNMASK_FUNCTION(NtWriteVirtualMemory)(process, address, buffer, size, nullptr);
        DebugNtResult(result, "NtWriteVirtualMemory");
    }

}
bool shadow_vwrite(void* address, void* buffer, DWORD size)
{
    return shadow_vwrite(HANDLE(-1), address, buffer, size);
}




bool NOINLINE shadow_vfree(HANDLE process, void* address)
{
    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = VirtualFreeEx(process, address, 0, MEM_RELEASE);
        DebugUserLandResult(result, "VirtualFreeEx");
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtFreeVirtualMemory, false);
        ULONG regionSize = 0;
        void* regionBase = address;
        long result = UNMASK_FUNCTION(NtFreeVirtualMemory)(process, &regionBase, &regionSize, MEM_RELEASE);
        DebugNtResult(result, "NtFreeVirtualMemory");
        return result == 0;
    }
}
bool shadow_vfree(void* address)
{
    return shadow_vfree(HANDLE(-1), address);
}

bool NOINLINE shadow_vunmap(HANDLE process, void* address)
{
    //if (USER_LAND_FUNCTIONS)
    //{
    //    BOOL result = UnmapViewOfFile2(process, address, 0);
    //    DebugUserLandResult(result, "UnmapViewOfFile2");
    //    return result;
    //}
    //else
    {
        NT_LOAD_FUNCTION(NtUnmapViewOfSection, false);
        long result = UNMASK_FUNCTION(NtUnmapViewOfSection)(process, address);
        DebugNtResult(result, "NtUnmapViewOfSection");
        return result == 0;
    }
}
bool shadow_vunmap(void* address)
{
    return shadow_vunmap(HANDLE(-1), address);
}




bool NOINLINE shadow_unmap(HANDLE process, void* address)
{
    MEMORY_BASIC_INFORMATION info;
    shadow_vquery(process, address, info);
    if (info.State != MEM_FREE)
    {
        if (info.Type == MEM_MAPPED || info.Type == MEM_IMAGE)
            return shadow_vunmap(process, info.AllocationBase);
        else// MEM_PRIVATE, state is MEM_RESERVE or MEM_COMMIT
            return shadow_vfree(process, info.AllocationBase);
    }
    return true; // success, the memory is already freed
}
bool shadow_unmap(void* address)
{
    return shadow_unmap(HANDLE(-1), address);
}
bool NOINLINE shadow_unmap(HANDLE process, void* baseAddress, DWORD regionSize)
{
    MEMORY_BASIC_INFORMATION info;
    PCHAR nextBlock  = PCHAR(baseAddress);
    PCHAR endAddress = nextBlock + regionSize;
    while (nextBlock < endAddress)
    {
        if (!shadow_unmap(process, nextBlock))
            return false;
        shadow_vquery(process, nextBlock, info);
        nextBlock = (PCHAR)info.BaseAddress + info.RegionSize;
    }
    return true;
}
bool shadow_unmap(void* baseAddress, DWORD regionSize)
{
    return shadow_unmap(HANDLE(-1), baseAddress, regionSize);
}




FARPROC NOINLINE shadow_getproc(void* hmodule, LPCSTR name)
{
    auto code = (PCHAR)hmodule;
    auto dos_header = (PIMAGE_DOS_HEADER)code;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return nullptr;
    }
    auto headers = (PIMAGE_NT_HEADERS)&code[dos_header->e_lfanew];
    if (headers->Signature != IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return nullptr;
    }
    PIMAGE_DATA_DIRECTORY directory = &headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (directory->Size == 0) {
        SetLastError(ERROR_PROC_NOT_FOUND); // no export table found
        return nullptr;
    }
    auto exports = (PIMAGE_EXPORT_DIRECTORY)(code + directory->VirtualAddress);
    if (!exports->NumberOfNames|| !exports->NumberOfFunctions) {
        SetLastError(ERROR_PROC_NOT_FOUND); // DLL doesn't export anything
        return nullptr;
    }

    // search function name in list of exported names
    int idx = -1;
    auto nameRef = PDWORD(code + exports->AddressOfNames);
    auto ordinal = PWORD (code + exports->AddressOfNameOrdinals);
    for (UINT i = 0; i < exports->NumberOfNames; ++i, ++nameRef, ++ordinal) {
        if (_stricmp(name, code + *nameRef) == 0) {
            idx = *ordinal;
            break;
        }
    }

    if (idx == -1) {
        // exported symbol not found
        SetLastError(ERROR_PROC_NOT_FOUND);
        return nullptr;
    }
    if (DWORD(idx) > exports->NumberOfFunctions) {
        // name <-> ordinal number don't match
        SetLastError(ERROR_PROC_NOT_FOUND);
        return nullptr;
    }

    // AddressOfFunctions contains the RVAs to the "real" functions
    return FARPROC(code + *PDWORD(code + exports->AddressOfFunctions + idx*4));
}

static HMODULE NOINLINE shadow_findmodule(const char* startsWith)
{
    HMODULE mods[512];
    DWORD cbSize;
    HANDLE process = GetCurrentProcess();
    if (EnumProcessModules(process, mods, sizeof mods, &cbSize))
    {
        int slen = strlen(startsWith);
        char modName[MAX_PATH];
        int count = cbSize / sizeof HMODULE;
        for (int i = 0; i < count; ++i)
        {
            if (GetModuleBaseNameA(process, mods[i], modName, MAX_PATH))
                if (memicmp(modName, startsWith, slen) == 0)
                    return mods[i];
        }
    }
    return NULL;
}

FARPROC NOINLINE shadow_ntdll_getproc(const char* func)
{
    //// @note We don't want anyone to see us LoadLibrary-ing "ntdll"
    static HMODULE module;
    if (!module)
    {
        HMODULE mods[4];
        DWORD cbSize;
        EnumProcessModules(GetCurrentProcess(), mods, sizeof mods, &cbSize);
        module = mods[1]; // NTDLL.DLL always the second element
    }
    return shadow_getproc(module, func);
}
FARPROC NOINLINE shadow_kernel_getproc(const char* func)
{
    //// @note We don't want anyone to see us LoadLibrary-ing "kernel32"
    static HMODULE module;
    if (!module)
    {
        HMODULE mods[4];
        DWORD cbSize;
        EnumProcessModules(GetCurrentProcess(), mods, sizeof mods, &cbSize);
        module = mods[2]; // KERNEL32.DLL always the third element
    }
    return shadow_getproc(module, func);
}




bool NOINLINE shadow_listprocs(std::vector<const char*>& out, void* hmodule, const char* istartsWith)
{
    auto code = (PCHAR)hmodule;
    auto dos_header = (PIMAGE_DOS_HEADER)code;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return false;
    }
    auto headers = (PIMAGE_NT_HEADERS)&code[dos_header->e_lfanew];
    if (headers->Signature != IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return false;
    }
    PIMAGE_DATA_DIRECTORY dir = &headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (dir->Size == 0) {
        SetLastError(ERROR_PROC_NOT_FOUND); // no export table found
        return false;
    }
    auto exports = (PIMAGE_EXPORT_DIRECTORY)(code + dir->VirtualAddress);
    if (!exports->NumberOfNames || !exports->NumberOfFunctions) {
        SetLastError(ERROR_PROC_NOT_FOUND); // DLL doesn't export anything
        return false; 
    }

    out.clear();
    out.reserve(exports->NumberOfNames);

    // search function name in list of exported names
    int swlength = istartsWith ? strlen(istartsWith) : 0;
    auto nameRef = PDWORD(code + exports->AddressOfNames);
    for (UINT i = 0; i < exports->NumberOfNames; ++i, ++nameRef) {
        PCHAR target = code + *nameRef;
        if (!swlength || _memicmp(target, istartsWith, swlength) == 0)
            out.push_back(target);
    }
    return true;
}




HANDLE NOINLINE shadow_create_thread(HANDLE process, LPTHREAD_START_ROUTINE start, void* param, bool suspended)
{
    if (USER_LAND_FUNCTIONS)
    {
        DWORD tid;
        HANDLE thread = CreateRemoteThread(process, nullptr, 256 * 1024, start, param, 
                                           suspended ? CREATE_SUSPENDED : 0, &tid);
        DebugUserLandResult(thread, "CreateRemoteThread");
        return thread;
    }
    else
    {
        NT_LOAD_FUNCTION(NtCreateThreadEx, nullptr);
        DWORD temp1[2] = { 0 };
        DWORD temp2[3] = { 0 };
        struct NtCreateThreadExBuffer {
            ULONG  Size;
            ULONG  Unk1,Unk2;
            PULONG Unk3;
            ULONG  Unk4,Unk5,Unk6;
            PULONG Unk7;
            ULONG  Unk8;
        } nb = { sizeof nb, 0x10003, 0x8, temp2, 0, 0x10004, 4, temp1, 0 };

        HANDLE thread = nullptr;
        long result = UNMASK_FUNCTION(NtCreateThreadEx)(&thread, 0x1FFFFF, nullptr, process, start, param,
                                      suspended, 0, 0, 0, &nb);
        DebugNtResult(result, "NtCreateThreadEx");
        return thread;
    }
}
HANDLE shadow_create_thread(LPTHREAD_START_ROUTINE start, void* param, bool suspended)
{
    return shadow_create_thread(HANDLE(-1), start, param, suspended);
}




HANDLE NOINLINE shadow_open_thread(DWORD threadId, DWORD flags)
{
    if (USER_LAND_FUNCTIONS)
    {
        HANDLE thread = OpenThread(flags, false, threadId);
        DebugUserLandResult(thread, "OpenThread");
        return thread;
    }
    else
    {
        NT_LOAD_FUNCTION(NtOpenThread, nullptr);
        DWORD attr[] = { 0x18 /*length*/, 0, 0, 0, 0, 0 };
        DWORD ids[2] = { 0, threadId };
        HANDLE h = nullptr;
        long result = UNMASK_FUNCTION(NtOpenThread)(&h, flags, &attr, &ids);
        DebugNtResult(result, "NtOpenThread");
        return result == 0 ? h : nullptr;
    }
}
void NOINLINE shadow_close_handle(HANDLE thread)
{
    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = CloseHandle(thread);
        DebugUserLandResult(result, "CloseHandle");
    }
    else
    {
        NT_LOAD_FUNCTION(NtClose, );
        long result = UNMASK_FUNCTION(NtClose)(thread);
        DebugNtResult(result, "NtClose");
    }
}




int NOINLINE shadow_get_threads(std::vector<DWORD>& out, DWORD processId)
{
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, processId);
    THREADENTRY32 entry = { sizeof entry };
    out.clear();
    if (Thread32First(snapshot, &entry)) do
    {
        if (entry.th32OwnerProcessID == processId)
            out.push_back(entry.th32ThreadID);
    } 
    while (Thread32Next(snapshot, &entry));
    CloseHandle(snapshot);
    return (int)out.size();
}
int shadow_get_threads(std::vector<DWORD>& out)
{
    return shadow_get_threads(out, GetCurrentProcessId());
}

int NOINLINE shadow_get_threadcount(DWORD processId)
{
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 entry = { sizeof entry };
    BOOL ret = Process32First(snapshot, &entry);
    while (ret && entry.th32ProcessID != processId)
        ret = Process32Next(snapshot, &entry);
    CloseHandle(snapshot);
    return ret ? (int)entry.cntThreads : 0;
}
int shadow_get_threadcount()
{
    return shadow_get_threadcount(GetCurrentProcessId());
}

bool NOINLINE shadow_get_threadrunning(HANDLE thread)
{
    DWORD exitCode = 0;
    return GetExitCodeThread(thread, &exitCode) && exitCode == STILL_ACTIVE;
}
bool NOINLINE shadow_get_threadrunning(DWORD threadID)
{
    HANDLE thread = shadow_open_thread(threadID, THREAD_QUERY_LIMITED_INFORMATION);
    bool result = shadow_get_threadrunning(thread);
    shadow_close_handle(thread);
    return result;
}




bool NOINLINE shadow_get_threadctx(HANDLE thread, CONTEXT& ctx, DWORD getFlags)
{
    if (USER_LAND_FUNCTIONS)
    {
        ctx.ContextFlags = getFlags;
        BOOL result = GetThreadContext(thread, &ctx);
        DebugUserLandResult(result, "GetThreadContext");
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtGetContextThread, false);
        ctx.ContextFlags = getFlags;
        long result = UNMASK_FUNCTION(NtGetContextThread)(thread, &ctx);
        DebugNtResult(result, "NtGetContextThread");
        return result == 0;
    }
}
bool NOINLINE shadow_set_threadctx(HANDLE thread, CONTEXT& ctx, DWORD setFlags)
{
    if (USER_LAND_FUNCTIONS)
    {
        ctx.ContextFlags = setFlags;
        BOOL result = SetThreadContext(thread, &ctx);
        DebugUserLandResult(result, "SetThreadContext");
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtSetContextThread, false);
        ctx.ContextFlags = setFlags;
        long result = UNMASK_FUNCTION(NtSetContextThread)(thread, &ctx);
        DebugNtResult(result, "NtSetContextThread");
        return result == 0;
    }
}




bool NOINLINE shadow_suspend_thread(HANDLE thread)
{
    if (USER_LAND_FUNCTIONS)
    {
        int result = SuspendThread(thread);
        indebug(if (result < 0) log("SuspendThread failed: %d\nReason: %s\n", result, shadow_getsyserr()));
        return result >= 0;
    }
    else
    {
        NT_LOAD_FUNCTION(NtSuspendThread, false);
        long result =  UNMASK_FUNCTION(NtSuspendThread)(thread, nullptr);
        DebugNtResult(result, "NtSuspendThread");
        return result == 0;
    }
}
bool NOINLINE shadow_resume_thread(HANDLE thread)
{
    if (USER_LAND_FUNCTIONS)
    {
        int result = ResumeThread(thread);
        indebug(if (result < 0) log("ResumeThread failed: %d\nReason: %s\n", result, shadow_getsyserr()));
        return result >= 0;
    }
    else
    {
        NT_LOAD_FUNCTION(NtAlertResumeThread, false);
        long result = UNMASK_FUNCTION(NtAlertResumeThread)(thread, nullptr);
        DebugNtResult(result, "NtAlertResumeThread");
        return result == 0;
    }
}




bool NOINLINE shadow_set_threadip(HANDLE thread, void* entryPoint)
{
    if (shadow_get_threadrunning(thread) && !shadow_suspend_thread(thread))
        return false; // failed to suspend thread

    CONTEXT ctx = { 0 };
    if (shadow_get_threadctx(thread, ctx, CONTEXT_CONTROL))
    {
        ctx.Eip = (DWORD)entryPoint;
        if (shadow_set_threadctx(thread, ctx, CONTEXT_CONTROL))
            return true;
    }
    return false; // failed
}




void NOINLINE shadow_scanmodules(HANDLE process, void* baseAddress, DWORD regionSize)
{
    MEMORY_BASIC_INFORMATION info;
    PCHAR nextBlock  = PCHAR(baseAddress);
    PCHAR endAddress = nextBlock + regionSize;

    IMAGE_DOS_HEADER doshdr;
    char path[MAX_PATH];

    while (nextBlock < endAddress)
    {
        shadow_vquery(process, nextBlock, info);
        shadow_vprint(info);
        if (info.State != MEM_FREE)
        {
            ReadProcessMemory(process, info.BaseAddress, &doshdr, sizeof doshdr, 0);
            if (doshdr.e_magic == IMAGE_DOS_SIGNATURE)
            {
                GetModuleFileNameA((HMODULE)info.BaseAddress, path, MAX_PATH);
                log("Module: %s\n", path);
            }
        }
        nextBlock = (PCHAR)info.BaseAddress + info.RegionSize;
    }
}
void shadow_scanmodules(void* baseAddress, DWORD regionSize)
{
    shadow_scanmodules(HANDLE(-1), baseAddress, regionSize);
}


bool shadow_listimports(std::vector<PCCH>& imports, HMODULE module)
{
    imports.clear();

    PCHAR image = PCHAR(module);
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)image;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return false;
    }
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)&image[dos->e_lfanew];
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return false;
    }
    PIMAGE_DATA_DIRECTORY dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (dir->Size == 0) {
        SetLastError(ERROR_PROC_NOT_FOUND); // no export table found
        return false;
    }
    PIMAGE_IMPORT_DESCRIPTOR import = (PIMAGE_IMPORT_DESCRIPTOR)(image + dir->VirtualAddress);
    for (; import->Name; ++import)
    {
        PCHAR lib = image + import->Name;
        imports.push_back(lib);
    }
    return true;
}
bool shadow_listimports(std::vector<PCCH>& imports)
{
    return shadow_listimports(imports, GetModuleHandleA(nullptr));
}



bool NOINLINE shadow_listmodules(std::vector<std::string>& modules)
{
    HMODULE mods[512];
    DWORD cbSize;
    HANDLE process = GetCurrentProcess();
    if (EnumProcessModules(process, mods, sizeof mods, &cbSize))
    {
        char modName[MAX_PATH];
        int count = cbSize / sizeof HMODULE;
        for (int i = 0; i < count; ++i)
        {
            if (int len = GetModuleBaseNameA(process, mods[i], modName, MAX_PATH))
                modules.emplace_back(modName, len);
        }
        return true;
    }
    return false;
}




const char* shadow_getsyserr(long error)
{
    static char err[1024];
    DWORD errorCode = error == 0 ? GetLastError() : error;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errorCode, 0, err, sizeof(err), nullptr);
    return err;
}