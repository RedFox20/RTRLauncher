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
// ReSharper disable CppUnreachableCode

#define SHADOWLIB_FORCE_DEBUG 1
#if _DEBUG || DEBUG || SHADOWLIB_FORCE_DEBUG
#  define indebug(...) __VA_ARGS__ 
#else
#  define indebug(...)
#endif

constexpr bool USER_LAND_FUNCTIONS = true;

typedef long (WINAPI*fnNtQueryVirtualMemory)   (void*, void*, DWORD, void*, ULONG, ULONG*);
typedef long (WINAPI*fnNtAllocateVirtualMemory)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);
typedef long (WINAPI*fnNtReadVirtualMemory)    (void*, void*, OUT void*, SIZE_T, OUT SIZE_T*);
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
        default:                     return "INV@";
    }
    
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
        return failedResult; } \
    }
        //*(uint32_t*)(&p##FuncName) = (uint32_t)(p##FuncName) ^ FUNCTION_MASK; \
//#define UNMASK_FUNCTION(FuncName) ((decltype(p##FuncName))(uint32_t(p##FuncName) ^ FUNCTION_MASK))
#define UNMASK_FUNCTION(FuncName) (p##FuncName)


#define DebugUserLandResult(result, function, ...) \
    if (!(result)) { \
        log(function" failed: %s\n", ##__VA_ARGS__, shadow_getsyserr()); \
        __debugbreak(); \
    }


#define DebugNtResult(result, function, ...) \
    if ((result) != 0) { \
        log(function" failed: %s\n", ##__VA_ARGS__, shadow_getsyserr(result)); \
        __debugbreak(); \
    }


bool check_process_alive(HANDLE process, const char* where)
{
    auto status = shadow_get_process_status(process);
    if (!status)
    {
        log("%s failed: process has terminated with exit_code=0x%X (%s)\n",
            where, status.exit_code, shadow_getsyserr(status.exit_code));
        return false;
    }
    return true;
}


bool NOINLINE shadow_vquery(HANDLE process, void* address, MEMORY_BASIC_INFORMATION& info)
{
    if (!check_process_alive(process, "shadow_vquery"))
        return false;
    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = VirtualQueryEx(process, address, &info, sizeof(info)) > 0;
        DebugUserLandResult(result, "VirtualQueryEx(%p, %p)", process, address);
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtQueryVirtualMemory, false);
        long result = UNMASK_FUNCTION(NtQueryVirtualMemory)(process, address, 0, &info, sizeof(info), nullptr);
        DebugNtResult(result, "NtQueryVirtualMemory(%p, %p)", process, address);
        return result == 0;
    }
}
bool shadow_vquery(void* address, MEMORY_BASIC_INFORMATION& info)
{
    return shadow_vquery(HANDLE(-1), address, info);
}

PCHAR NOINLINE shadow_valloc(HANDLE process, void* address, DWORD size, DWORD protect, DWORD flags)
{
    if (!check_process_alive(process, "shadow_valloc"))
        return nullptr;
    if (USER_LAND_FUNCTIONS)
    {
        PVOID result = VirtualAllocEx(process, address, size, flags, protect);
        log("valloc %p %p %d => ", process, address, size);
        shadow_vprint(process, result, 1);
        DebugUserLandResult(result, "VirtualAllocEx(%p, %p, %d)", process, address, size);
        return PCHAR(result);
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
        DebugNtResult(result, "NtAllocateVirtualMemory(%p, %p, %d)", process, address, size);
        return result == 0 ? PCHAR(addressBase) : nullptr;
    }
}
PCHAR shadow_valloc(void* address, DWORD size, DWORD protect, DWORD flags)
{
    return shadow_valloc(HANDLE(-1), address, size, protect, flags);
}




bool NOINLINE shadow_vread(HANDLE process, void* address, void* buffer, DWORD size)
{
    if (!check_process_alive(process, "shadow_vread"))
        return false;

    SIZE_T read;
    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = ReadProcessMemory(process, address, buffer, size, &read);
        DebugUserLandResult(result, "ReadProcessMemory(%p, %p, %d) read: %d", process, address, size, read);
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtReadVirtualMemory, false);
        long result = UNMASK_FUNCTION(NtReadVirtualMemory)(process, address, buffer, size, &read);
        DebugNtResult(result, "NtReadVirtualMemory(%p, %p, %d) read: %d", process, address, size, read);
        return result == 0;
    }
}
bool shadow_vread(void* address, void* buffer, DWORD size)
{
    return shadow_vread(HANDLE(-1), address, buffer, size);
}

bool NOINLINE shadow_vwrite(HANDLE process, void* address, void* buffer, DWORD size)
{
    if (!check_process_alive(process, "shadow_vwrite"))
        return false;

    DWORD written;
    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = WriteProcessMemory(process, address, buffer, size, &written);
        DebugUserLandResult(result && size == written,
            "WriteProcessMemory(%p, %p, %d) written: %d", process, address, size, written);
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtWriteVirtualMemory, false);
        long result = UNMASK_FUNCTION(NtWriteVirtualMemory)(process, address, buffer, size, &written);
        DebugNtResult(result, "NtWriteVirtualMemory(%p, %p, %d) written: %d", process, address, size, written);
        return result == 0;
    }
}
bool shadow_vwrite(void* address, void* buffer, DWORD size)
{
    return shadow_vwrite(HANDLE(-1), address, buffer, size);
}




bool NOINLINE shadow_vfree(HANDLE process, void* address)
{
    if (!check_process_alive(process, "shadow_vfree"))
        return false;

    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = VirtualFreeEx(process, address, 0, MEM_RELEASE);
        DebugUserLandResult(result, "VirtualFreeEx(%p, %p)", process, address);
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtFreeVirtualMemory, false);
        ULONG regionSize = 0;
        void* regionBase = address;
        long result = UNMASK_FUNCTION(NtFreeVirtualMemory)(process, &regionBase, &regionSize, MEM_RELEASE);
        DebugNtResult(result, "NtFreeVirtualMemory(%p, %p)", process, address);
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
    //    DebugUserLandResult(result, "UnmapViewOfFile2(%p, %p)", process, address);
    //    return result;
    //}
    //else
    {
        NT_LOAD_FUNCTION(NtUnmapViewOfSection, false);
        long result = UNMASK_FUNCTION(NtUnmapViewOfSection)(process, address);
        DebugNtResult(result, "NtUnmapViewOfSection(%p, %p)", process, address);
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
    auto nextBlock  = PCHAR(baseAddress);
    auto endAddress = nextBlock + regionSize;
    while (nextBlock < endAddress)
    {
        if (!shadow_unmap(process, nextBlock))
            return false;
        shadow_vquery(process, nextBlock, info);
        nextBlock = PCHAR(info.BaseAddress) + info.RegionSize;
    }
    return true;
}
bool shadow_unmap(void* baseAddress, DWORD regionSize)
{
    return shadow_unmap(HANDLE(-1), baseAddress, regionSize);
}


PIMAGE_NT_HEADERS NOINLINE get_nt_headers(HMODULE module)
{
    auto code = reinterpret_cast<PCHAR>(module);
    auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(code);
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return nullptr;
    }
    auto headers = reinterpret_cast<PIMAGE_NT_HEADERS>(&code[dos_header->e_lfanew]);
    if (headers->Signature != IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return nullptr;
    }
    return headers;
}

bool is_valid_nt_module(HMODULE module)
{
    return get_nt_headers(module) != nullptr;
}

PIMAGE_DATA_DIRECTORY NOINLINE get_image_data_directory(HMODULE module, int image_directory_entry_id)
{
    auto headers = get_nt_headers(module);
    if (!headers) return nullptr;

    PIMAGE_DATA_DIRECTORY directory = &headers->OptionalHeader.DataDirectory[image_directory_entry_id];
    if (directory->Size == 0) {
        SetLastError(ERROR_BAD_EXE_FORMAT); // no export table found
        return nullptr;
    }
    return directory;
}

PIMAGE_EXPORT_DIRECTORY NOINLINE get_module_exports(HMODULE module)
{
    PIMAGE_DATA_DIRECTORY dir = get_image_data_directory(module, IMAGE_DIRECTORY_ENTRY_EXPORT);
    if (!dir) return nullptr;

    auto code = reinterpret_cast<PCHAR>(module);
    auto exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(code + dir->VirtualAddress);
    if (!exports->NumberOfNames || !exports->NumberOfFunctions) {
        SetLastError(ERROR_PROC_NOT_FOUND); // DLL doesn't export anything
        return nullptr;
    }
    return exports;
}

FARPROC NOINLINE shadow_getproc(HMODULE module, LPCSTR name)
{
    PIMAGE_EXPORT_DIRECTORY exports = get_module_exports(module);
    if (!exports) return nullptr;

    // search function name in list of exported names
    int idx = -1;
    auto code = reinterpret_cast<PCHAR>(module);
    auto nameRef = PDWORD(code + exports->AddressOfNames);
    auto ordinal = PWORD (code + exports->AddressOfNameOrdinals);
    for (UINT i = 0; i < exports->NumberOfNames; ++i, ++nameRef, ++ordinal) {
        const char* funcName = code + *nameRef;
        if (_stricmp(name, funcName) == 0) {
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
    auto rva = PDWORD(code + exports->AddressOfFunctions + idx*sizeof(DWORD));
    auto proc = FARPROC(code + *rva);
    return proc;
}


ModuleProcCache::ModuleProcCache(HMODULE module)
{
    PIMAGE_EXPORT_DIRECTORY exports = get_module_exports(module);
    if (!exports) return;

    auto code = reinterpret_cast<PCHAR>(module);
    auto nameRef = PDWORD(code + exports->AddressOfNames);
    auto ordinal = PWORD (code + exports->AddressOfNameOrdinals);
    for (UINT i = 0; i < exports->NumberOfNames; ++i, ++nameRef, ++ordinal)
    {
        rpp::strview funcName = code + *nameRef;
        auto rva = PDWORD(code + exports->AddressOfFunctions + (*ordinal)*sizeof(DWORD));
        auto proc = FARPROC(code + *rva);
        procs[funcName] = proc;
    }
}

FARPROC ModuleProcCache::find(rpp::strview name) const
{
    auto it = procs.find(name);
    if (it != procs.end())
        return it->second;
    SetLastError(ERROR_PROC_NOT_FOUND); // function not found
    return nullptr;
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
                if (_memicmp(modName, startsWith, slen) == 0)
                    return mods[i];
        }
    }
    return nullptr;
}

FARPROC NOINLINE shadow_ntdll_getproc(rpp::strview func)
{
    static ModuleProcCache module { shadow_findmodule("NTDLL") };
    return module.find(func);
}
FARPROC NOINLINE shadow_kernel_getproc(rpp::strview func)
{
    static ModuleProcCache module { shadow_findmodule("KernelBase") };
    return module.find(func);
}




std::vector<rpp::strview> NOINLINE shadow_listprocs(HMODULE module, const char* istartsWith)
{
    std::vector<rpp::strview> out;
    PIMAGE_EXPORT_DIRECTORY exports = get_module_exports(module);
    if (!exports) return out;

    auto code = reinterpret_cast<PCHAR>(module);
    out.reserve(exports->NumberOfNames);

    // search function name in list of exported names
    size_t swlength = istartsWith ? strlen(istartsWith) : 0;
    auto nameRef = PDWORD(code + exports->AddressOfNames);
    for (UINT i = 0; i < exports->NumberOfNames; ++i, ++nameRef) {
        PCHAR target = code + *nameRef;
        if (!swlength || _memicmp(target, istartsWith, swlength) == 0)
            out.emplace_back(target);
    }
    return out;
}




HANDLE NOINLINE shadow_create_thread(HANDLE process, LPTHREAD_START_ROUTINE start, void* param, bool suspended)
{
    if (true || USER_LAND_FUNCTIONS)
    {
        DWORD tid;
        HANDLE thread = CreateRemoteThread(process, nullptr, 1024 * 1024, start, param, 
                                           suspended ? CREATE_SUSPENDED : 0, &tid);
        DebugUserLandResult(thread, "CreateRemoteThread(%p)", process);
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
        DebugNtResult(result, "NtCreateThreadEx(%p)", process);
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
        DebugUserLandResult(thread, "OpenThread(%d)", threadId);
        return thread;
    }
    else
    {
        NT_LOAD_FUNCTION(NtOpenThread, nullptr);
        DWORD attr[] = { 0x18 /*length*/, 0, 0, 0, 0, 0 };
        DWORD ids[2] = { 0, threadId };
        HANDLE h = nullptr;
        long result = UNMASK_FUNCTION(NtOpenThread)(&h, flags, &attr, &ids);
        DebugNtResult(result, "NtOpenThread(%d)", threadId);
        return result == 0 ? h : nullptr;
    }
}
void NOINLINE shadow_close_handle(HANDLE handle)
{
    if (USER_LAND_FUNCTIONS)
    {
        BOOL result = CloseHandle(handle);
        DebugUserLandResult(result, "CloseHandle(%p)", handle);
    }
    else
    {
        NT_LOAD_FUNCTION(NtClose, );
        long result = UNMASK_FUNCTION(NtClose)(handle);
        DebugNtResult(result, "NtClose(%p)", handle);
    }
}

std::vector<ProcessInfo> shadow_get_processes(rpp::strview name)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    std::vector<ProcessInfo> out;
    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            std::string exeFile = entry.szExeFile;
            if (name.empty() || rpp::strview{exeFile}.contains(name))
            {
                out.push_back({std::move(exeFile), entry.th32ProcessID});
            }
        }
    }
    return out;
}


std::vector<DWORD> NOINLINE shadow_get_threads(DWORD processId)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, processId);
    THREADENTRY32 entry = { sizeof(entry), 0 };
    std::vector<DWORD> out;
    if (Thread32First(snapshot, &entry)) do
    {
        if (entry.th32OwnerProcessID == processId)
            out.push_back(entry.th32ThreadID);
    } 
    while (Thread32Next(snapshot, &entry));
    CloseHandle(snapshot);
    return out;
}
std::vector<DWORD> shadow_get_threads()
{
    return shadow_get_threads(GetCurrentProcessId());
}

int NOINLINE shadow_get_threadcount(DWORD processId)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 entry = { sizeof(entry), 0 };
    BOOL ret = Process32First(snapshot, &entry);
    while (ret && entry.th32ProcessID != processId)
        ret = Process32Next(snapshot, &entry);
    CloseHandle(snapshot);
    return ret ? int(entry.cntThreads) : 0;
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
        DebugUserLandResult(result, "GetThreadContext(%p)", thread);
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtGetContextThread, false);
        ctx.ContextFlags = getFlags;
        long result = UNMASK_FUNCTION(NtGetContextThread)(thread, &ctx);
        DebugNtResult(result, "NtGetContextThread(%p)", thread);
        return result == 0;
    }
}
bool NOINLINE shadow_set_threadctx(HANDLE thread, CONTEXT& ctx, DWORD setFlags)
{
    if (USER_LAND_FUNCTIONS)
    {
        ctx.ContextFlags = setFlags;
        BOOL result = SetThreadContext(thread, &ctx);
        DebugUserLandResult(result, "SetThreadContext(%p)", thread);
        return result;
    }
    else
    {
        NT_LOAD_FUNCTION(NtSetContextThread, false);
        ctx.ContextFlags = setFlags;
        long result = UNMASK_FUNCTION(NtSetContextThread)(thread, &ctx);
        DebugNtResult(result, "NtSetContextThread(%p)", thread);
        return result == 0;
    }
}




bool NOINLINE shadow_suspend_thread(HANDLE thread)
{
    if (USER_LAND_FUNCTIONS)
    {
        int result = SuspendThread(thread);
        bool success = result >= 0;
        DebugUserLandResult(success, "SuspendThread(%p)", thread);
        return success;
    }
    else
    {
        NT_LOAD_FUNCTION(NtSuspendThread, false);
        long result =  UNMASK_FUNCTION(NtSuspendThread)(thread, nullptr);
        DebugNtResult(result, "NtSuspendThread(%p)", thread);
        return result == 0;
    }
}
bool NOINLINE shadow_resume_thread(HANDLE thread)
{
    if (USER_LAND_FUNCTIONS)
    {
        int result = ResumeThread(thread);
        bool success = result >= 0;
        DebugUserLandResult(success, "ResumeThread(%p)", thread);
        return result >= 0;
    }
    else
    {
        NT_LOAD_FUNCTION(NtAlertResumeThread, false);
        long result = UNMASK_FUNCTION(NtAlertResumeThread)(thread, nullptr);
        DebugNtResult(result, "NtAlertResumeThread(%p)", thread);
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
        ctx.Eip = DWORD(entryPoint);
        if (shadow_set_threadctx(thread, ctx, CONTEXT_CONTROL))
            return true;
    }
    return false; // failed
}



bool NOINLINE shadow_enable_debug_privilege(HANDLE proccess)
{
    bool result = false;
    HANDLE hToken;
    if (OpenProcessToken(proccess, TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken))
    {
        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes    = SE_PRIVILEGE_ENABLED;
        tp.Privileges[0].Luid.LowPart  = 20; // SeDebugPrivilege
        tp.Privileges[0].Luid.HighPart = 0;
        if (AdjustTokenPrivileges(hToken, false, &tp, 0, nullptr, nullptr))
        {
            result = true;
        }
        CloseHandle(hToken);
    }
    else
    {
        log("Failed to open process handle for privilege adjustment\n");
    }
    return result;
}



void NOINLINE shadow_scanmodules(HANDLE process, void* baseAddress, DWORD regionSize)
{
    MEMORY_BASIC_INFORMATION info;
    auto nextBlock  = PCHAR(baseAddress);
    auto endAddress = nextBlock + regionSize;

    IMAGE_DOS_HEADER doshdr;
    char path[MAX_PATH];

    while (nextBlock < endAddress)
    {
        shadow_vquery(process, nextBlock, info);
        shadow_vprint(info);
        if (info.State != MEM_FREE)
        {
            ReadProcessMemory(process, info.BaseAddress, &doshdr, sizeof(doshdr), nullptr);
            if (doshdr.e_magic == IMAGE_DOS_SIGNATURE)
            {
                GetModuleFileNameA(HMODULE(info.BaseAddress), path, MAX_PATH);
                log("Module: %s\n", path);
            }
        }
        nextBlock = PCHAR(info.BaseAddress) + info.RegionSize;
    }
}
void shadow_scanmodules(void* baseAddress, DWORD regionSize)
{
    shadow_scanmodules(HANDLE(-1), baseAddress, regionSize);
}


std::vector<rpp::strview> shadow_list_imports(HMODULE module)
{
    std::vector<rpp::strview> imports;

    PIMAGE_DATA_DIRECTORY dir = get_image_data_directory(module, IMAGE_DIRECTORY_ENTRY_IMPORT);
    if (!dir) return imports;

    auto code = PCHAR(module);
    auto import = PIMAGE_IMPORT_DESCRIPTOR(code + dir->VirtualAddress);
    for (; import->Name; ++import)
    {
        char* lib = code + import->Name;
        imports.emplace_back(lib);
    }
    return imports;
}
std::vector<rpp::strview> shadow_list_imports()
{
    return shadow_list_imports(GetModuleHandleA(nullptr));
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



HANDLE NOINLINE shadow_open_process_all_access(DWORD processId)
{
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processId);
    if (!process)
    {
        log("OpenProcess(%d) failed: %s\n", processId, shadow_getsyserr());
    }
    return process;
}


ExitStatus shadow_get_process_status(HANDLE process)
{
    ExitStatus status;
    if (GetExitCodeProcess(process, &status.exit_code) && status.exit_code == STILL_ACTIVE)
    {
        status.exit_code = 0;
        status.is_running = true;
    }
    return status;
}
ExitStatus shadow_get_process_status(DWORD processId)
{
    ExitStatus status;
    if (HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId))
    {
        status = shadow_get_process_status(process);
        CloseHandle(process);
    }
    return status;
}


ExitStatus shadow_get_thread_status(HANDLE thread)
{
    ExitStatus status;
    if (GetExitCodeThread(thread, &status.exit_code) && status.exit_code == STILL_ACTIVE)
    {
        status.exit_code = 0;
        status.is_running = true;
    }
    return status;
}
ExitStatus shadow_get_thread_status(DWORD threadId)
{
    ExitStatus status;
    if (HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, threadId))
    {
        status = shadow_get_thread_status(thread);
        CloseHandle(thread);
    }
    return status;
}


const char* shadow_getsyserr(DWORD error)
{
    if (error == 0xC0000005)
        return "Memory Access Violation";

    static char err[1024];
    DWORD errorCode = error == 0 ? GetLastError() : error;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errorCode, 0, err, sizeof(err), nullptr);
    return err;
}