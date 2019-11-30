/**
 * Copyright (c) 2014 - ShadowFox ^_^
 */
#include "shadowlib.h"
#include <log.h>
#include <tlhelp32.h>
#define PSAPI_VERSION 1
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

#ifndef NOINLINE
#define NOINLINE __declspec(noinline)
#endif
#define SHADOWLIB_FORCEDEBUG 0
#if DEBUG || SHADOWLIB_FORCEDEBUG
#define indebug(...) __VA_ARGS__
#else
#define indebug(...)
#endif

typedef long (WINAPI*fnNtQueryVirtualMemory)   (void*, void*, DWORD, void*, ULONG, ULONG*);
typedef long (WINAPI*fnNtAllocateVirtualMemory)(void*, IN OUT void**, ULONG_PTR, SIZE_T*, ULONG, ULONG);
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


static const char* memconst(DWORD value)
{
	switch (value)
	{
		case PAGE_NOACCESS:				return "NONE";
		case PAGE_READONLY:				return "R   ";
		case PAGE_READWRITE:			return "RW  ";
		case PAGE_WRITECOPY:			return "WCP ";
		case PAGE_EXECUTE:				return "E   ";
		case PAGE_EXECUTE_READ:			return "ER  ";
		case PAGE_EXECUTE_READWRITE:	return "ERW ";
		case PAGE_EXECUTE_WRITECOPY:	return "EWCP";
		case PAGE_GUARD:				return "GRD ";
		case PAGE_NOCACHE:				return "NCHE";
		case PAGE_WRITECOMBINE:			return "WCOM";
		case MEM_COMMIT:				return "CMIT";
		case MEM_RESERVE:				return "RESV";
		case MEM_FREE:					return "FREE";
		case MEM_PRIVATE:				return "PRIV";
		case MEM_MAPPED:				return "MAPD";
		case MEM_IMAGE:					return "IMAG";
	}
	return "INV@";
}
void NOINLINE shadow_vprint(MEMORY_BASIC_INFORMATION& i)
{
	log("ba: %8x ab: %8x ap: %s rz: %8x st: %s pr: %s ty: %s\n", i.BaseAddress, i.AllocationBase, 
		   memconst(i.AllocationProtect), i.RegionSize,
		   memconst(i.State), memconst(i.Protect), memconst(i.Type));
}
void NOINLINE shadow_vprint(HANDLE process, void* baseAddress, DWORD regionSize)
{
	MEMORY_BASIC_INFORMATION info;
	PCHAR nextBlock  = PCHAR(baseAddress);
	PCHAR endAddress = nextBlock + regionSize;
	while (nextBlock < endAddress && shadow_vquery(process, nextBlock, info))
	{
		shadow_vprint(info);
		nextBlock = (PCHAR)info.BaseAddress + info.RegionSize;
	}
}
void shadow_vprint(void* baseAddress, DWORD regionSize)
{
	shadow_vprint(HANDLE(-1), baseAddress, regionSize);
}


//// @note Count on the optimizer to copy the string directly in CODE
template<class... Args> 
static const char* codestr(char(&out)[32], Args... params)
{
	static_assert(sizeof...(Args) < 32, "Codestr maxlen is 31 chars");
	struct expand { template<class...T>expand(T...params){} };
	char* ptr = out + sizeof...(Args) - 1;
	expand{ *--ptr = params... };
	out[sizeof...(Args)] = 0;
	return sizeof...(Args);
}
static const char* deobfuscate(char(&str)[32], int len)
{
	for (int i = 0; i < len; ++i) str[i] = str[i] - (42 + i);
	return str;
}

#define LAZYFUNC_MASK 0xCAFEBABE

//typedef shadow_string sstr;
#define NTDLL_LAZYFUNC(FuncName, failedResult, shadowStr) \
	static fn##FuncName p##FuncName; \
	if (!p##FuncName) { \
		p##FuncName = (fn##FuncName)shadow_ntdll_getproc(shadowStr.get()); \
		if (!p##FuncName) { indebug(log("nlzf %s failed\n", shadowStr.get())); \
		*(int*)(&p##FuncName) = (int)(p##FuncName) ^ LAZYFUNC_MASK; \
		return failedResult; } \
	}
#define UNLAZYFUNC(FuncName) ((decltype(p##FuncName))(int(p##FuncName) ^ LAZYFUNC_MASK))

bool NOINLINE shadow_vquery(HANDLE process, void* address, MEMORY_BASIC_INFORMATION& info)
{
	//return VirtualQueryEx(process, address, &info, sizeof info) > 0;
	NTDLL_LAZYFUNC(NtQueryVirtualMemory, false, sstr2('NtQu','eryV','irtu','alMe','mory'));
	long result = UNLAZYFUNC(NtQueryVirtualMemory)(process, address, 0, &info, sizeof info, NULL);
	indebug(if (result) log("QVM failed: %p\n", result));
	return result == 0;
}
bool shadow_vquery(void* address, MEMORY_BASIC_INFORMATION& info)
{
	return shadow_vquery(HANDLE(-1), address, info);
}

PVOID NOINLINE shadow_valloc(HANDLE process, void* address, DWORD size, DWORD protect, DWORD flags)
{
	//log("valloc %p %p %d %x %x => ", process, address, size, protect, flags);
	//auto res = VirtualAllocEx(process, address, size, flags, protect);
	//log("%p\n", res);
	//shadow_vprint(process, res, 1);
	//return res;

	DWORD filtered = 0; // only accept these filtered flags:
	if (flags & MEM_COMMIT)  filtered |= MEM_COMMIT;
	if (flags & MEM_RESERVE) filtered |= MEM_RESERVE;

	NTDLL_LAZYFUNC(NtAllocateVirtualMemory, NULL, sstr('N','t','A','l','l','o','c','a','t','e','V','i','r','t','u','a','l','M','e','m','o','r','y'));
	void* addressBase = address;
	SIZE_T regionSize = size;
	long result = UNLAZYFUNC(NtAllocateVirtualMemory)(process, &addressBase, 0, &regionSize, filtered, protect);
	indebug(if (result) log("AVM failed: %p\n", result));
	return result == 0 ? addressBase : 0;
}
void* shadow_valloc(void* address, DWORD size, DWORD protect, DWORD flags)
{
	return shadow_valloc(HANDLE(-1), address, size, protect, flags);
}




bool NOINLINE shadow_vread(HANDLE process, void* address, void* buffer, DWORD size)
{
	//return ReadProcessMemory(process, address, buffer, size, NULL) == TRUE;

	NTDLL_LAZYFUNC(NtReadVirtualMemory, false, sstr('N','t','R','e','a','d','V','i','r','t','u','a','l','M','e','m','o','r','y'));
	long result = UNLAZYFUNC(NtReadVirtualMemory)(process, address, &buffer, size, NULL);
	indebug(if (result) log("RVM failed: %p\n", result));
	return result == 0;
}
bool shadow_vread(void* address, void* buffer, DWORD size)
{
	return shadow_vread(HANDLE(-1), address, buffer, size);
}

bool NOINLINE shadow_vwrite(HANDLE process, void* address, void* buffer, DWORD size)
{
	//auto res = WriteProcessMemory(process, address, buffer, size, NULL) == 0;
	//log("%s\n", res ? "true" : "false");
	//if (!res) log("Reason: %s\n", shadow_getsyserr());
	//return res;

	NTDLL_LAZYFUNC(NtWriteVirtualMemory, false, sstr('N','t','W','r','i','t','e','V','i','r','t','u','a','l','M','e','m','o','r','y'));
	long result = UNLAZYFUNC(NtWriteVirtualMemory)(process, address, buffer, size, NULL);
	indebug(if (result) log("WVM failed: %p\n", result));
	indebug(if (result) log("Reason: %s\n", shadow_getsyserr()));
	return result == 0;
}
bool shadow_vwrite(void* address, void* buffer, DWORD size)
{
	return shadow_vwrite(HANDLE(-1), address, buffer, size);
}




bool NOINLINE shadow_vfree(HANDLE process, void* address)
{
	//return VirtualFreeEx(process, address, 0, MEM_RELEASE) == TRUE;

	NTDLL_LAZYFUNC(NtFreeVirtualMemory, false, sstr('N','t','F','r','e','e','V','i','r','t','u','a','l','M','e','m','o','r','y'));
	ULONG regionSize = 0;
	void* regionBase = address;
	long result =  UNLAZYFUNC(NtFreeVirtualMemory)(process, &regionBase, &regionSize, MEM_RELEASE);
	indebug(if (result) log("FVM failed: %p\n", result));
	return result == 0;
}
bool shadow_vfree(void* address)
{
	return shadow_vfree(HANDLE(-1), address);
}

bool NOINLINE shadow_vunmap(HANDLE process, void* address)
{
	NTDLL_LAZYFUNC(NtUnmapViewOfSection, false, sstr('N','t','U','n','m','a','p','V','i','e','w','O','f','S','e','c','t','i','o', 'n'));
	long result = UNLAZYFUNC(NtUnmapViewOfSection)(process, address);
	indebug(if (result) log("UVOS failed: %p\n", result));
	return result == 0;
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
    PCHAR code = (PCHAR)hmodule;
	PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)code;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }
	PIMAGE_NT_HEADERS headers = (PIMAGE_NT_HEADERS)&code[dos_header->e_lfanew];
    if (headers->Signature != IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return NULL;
    }
    PIMAGE_DATA_DIRECTORY directory = &headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (directory->Size == 0) {
        SetLastError(ERROR_PROC_NOT_FOUND); // no export table found
        return NULL;
    }
    PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)(code + directory->VirtualAddress);
    if (!exports->NumberOfNames|| !exports->NumberOfFunctions) {
        SetLastError(ERROR_PROC_NOT_FOUND); // DLL doesn't export anything
        return NULL;
    }

    // search function name in list of exported names
    int idx = -1;
    PDWORD nameRef = PDWORD(code + exports->AddressOfNames);
    PWORD  ordinal = PWORD (code + exports->AddressOfNameOrdinals);
    for (UINT i = 0; i < exports->NumberOfNames; ++i, ++nameRef, ++ordinal) {
        if (_stricmp(name, code + *nameRef) == 0) {
            idx = *ordinal;
            break;
        }
    }

    if (idx == -1) {
        // exported symbol not found
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }
    if (DWORD(idx) > exports->NumberOfFunctions) {
        // name <-> ordinal number don't match
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
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
    PCHAR code = (PCHAR)hmodule;
	PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)code;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return false;
    }
	PIMAGE_NT_HEADERS headers = (PIMAGE_NT_HEADERS)&code[dos_header->e_lfanew];
    if (headers->Signature != IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return false;
    }
    PIMAGE_DATA_DIRECTORY dir = &headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (dir->Size == 0) {
        SetLastError(ERROR_PROC_NOT_FOUND); // no export table found
        return false;
    }
    PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)(code + dir->VirtualAddress);
	if (!exports->NumberOfNames || !exports->NumberOfFunctions) {
		SetLastError(ERROR_PROC_NOT_FOUND); // DLL doesn't export anything
        return false; 
	}

	out.clear();
	out.reserve(exports->NumberOfNames);

    // search function name in list of exported names
	int swlength = istartsWith ? strlen(istartsWith) : 0;
    PDWORD nameRef = PDWORD(code + exports->AddressOfNames);
    for (UINT i = 0; i < exports->NumberOfNames; ++i, ++nameRef) {
		PCHAR target = code + *nameRef;
		if (!swlength || _memicmp(target, istartsWith, swlength) == 0)
			out.push_back(target);
    }
	return true;
}




HANDLE NOINLINE shadow_create_thread(HANDLE process, LPTHREAD_START_ROUTINE start, void* param, bool suspended)
{
	//DWORD tid;
	//return CreateRemoteThread(process, 0, 256 * 1024, start, param, 
	//						  suspended ? CREATE_SUSPENDED : 0, &tid);

	NTDLL_LAZYFUNC(NtCreateThreadEx, NULL, sstr('N','t','C','r','e','a','t','e','T','h','r','e','a','d','E','x'));
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

	HANDLE thread = NULL;
	long result = UNLAZYFUNC(NtCreateThreadEx)(&thread, 0x1FFFFF, 0, process, start, param,
									(BOOL)suspended, 0, 0, 0, &nb);
	indebug(if (result) log("CTE failed: %p\n", result));
	return thread;
}
HANDLE shadow_create_thread(LPTHREAD_START_ROUTINE start, void* param, bool suspended)
{
	return shadow_create_thread(HANDLE(-1), start, param, suspended);
}




HANDLE NOINLINE shadow_open_thread(DWORD threadId, DWORD flags)
{
	//return OpenThread(flags, FALSE, threadId);

	NTDLL_LAZYFUNC(NtOpenThread, NULL, sstr('N','t','O','p','e','n','T','h','r','e','a','d'));
	DWORD attr[] = { 0x18 /*length*/, 0, 0, 0, 0, 0 };
	DWORD ids[2] = { 0, threadId };
	HANDLE h = NULL;
	long result = UNLAZYFUNC(NtOpenThread)(&h, flags, &attr, &ids);
	indebug(if (result) log("OT failed: %p\n", result));
	return result == 0 ? h : NULL;
}
void NOINLINE shadow_close_handle(HANDLE thread)
{
	//CloseHandle(thread);
	//return;

	NTDLL_LAZYFUNC(NtClose, , sstr('N','t','C','l','o','s','e'));
	long result = UNLAZYFUNC(NtClose)(thread);
	indebug(if (result) log("C failed: %p\n", result));
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
	//ctx.ContextFlags = getFlags;
	//return GetThreadContext(thread, &ctx) == TRUE;

	NTDLL_LAZYFUNC(NtGetContextThread, false, sstr('N','t','G','e','t','C','o','n','t','e','x','t','T','h','r','e','a','d'));
	ctx.ContextFlags = getFlags;
	long result = UNLAZYFUNC(NtGetContextThread)(thread, &ctx);
	indebug(if (result) log("GCT failed: %p\n", result));
	return result == 0;
}
bool NOINLINE shadow_set_threadctx(HANDLE thread, CONTEXT& ctx, DWORD setFlags)
{
	//ctx.ContextFlags = setFlags;
	//return SetThreadContext(thread, &ctx) == TRUE;

	NTDLL_LAZYFUNC(NtSetContextThread, false, sstr('N','t','S','e','t','C','o','n','t','e','x','t','T','h','r','e','a','d'));
	ctx.ContextFlags = setFlags;
	long result = UNLAZYFUNC(NtSetContextThread)(thread, &ctx);
	indebug(if (result) log("SCT failed: %p\n", result));
	return result == 0;
}




bool NOINLINE shadow_suspend_thread(HANDLE thread)
{
	//return SuspendThread(thread) >= 0;

	NTDLL_LAZYFUNC(NtSuspendThread, false, sstr('N','t','S','u','s','p','e','n','d','T','h','r','e','a','d'));
	long result =  UNLAZYFUNC(NtSuspendThread)(thread, NULL);
	indebug(if (result) log("ST failed: %p\n", result));
	return result == 0;
}
bool NOINLINE shadow_resume_thread(HANDLE thread)
{
	//return ResumeThread(thread) >= 0;

	NTDLL_LAZYFUNC(NtAlertResumeThread, false, sstr('N','t','A','l','e','r','t','R','e','s','u','m','e','T','h','r','e','a','d'));
	long result = UNLAZYFUNC(NtAlertResumeThread)(thread, NULL);
	indebug(if (result) log("ART failed: %p\n", result));
	return result == 0;
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
	return shadow_listimports(imports, GetModuleHandleA(NULL));
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




const char* shadow_getsyserr()
{
	static char errmsg[1024];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, errmsg, sizeof errmsg, 0);
	return errmsg;
}