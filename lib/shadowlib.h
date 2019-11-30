/**
 * Copyright (c) 2014 - ShadowFox ^_^
 */
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>


/**
 * @brief Print virtual memory block information
 */
void shadow_vprint(MEMORY_BASIC_INFORMATION& i);
/**
 * @brief Print a large section of remote process memory blocks
 */
void shadow_vprint(HANDLE process, void* baseAddress, DWORD regionSize);
/**
 * @brief Print a large section of this process memory blocks
 */
void shadow_vprint(void* baseAddress, DWORD regionSize);



/**
 * @brief Shadow virtual query for a remote process
 */
bool shadow_vquery(HANDLE process, void* address, MEMORY_BASIC_INFORMATION& info);
/**
 * @brief Shadow virtual query for this process
 */
bool shadow_vquery(void* address, MEMORY_BASIC_INFORMATION& info);
/**
 * @brief Alloc a memory block in a remote process
 * @param protect [PAGE_READWRITE] Memory block protection flags - default RW
 * @param flags   [MEM_RESERVE|MEM_COMMIT] Memory alloc flags - default MEM_RESERVE|MEM_COMMIT
 */
void* shadow_valloc(HANDLE process, void* address, DWORD size, DWORD protect = PAGE_READWRITE, 
						  DWORD flags = MEM_RESERVE | MEM_COMMIT);
/**
 * @brief Alloc a memory block for this process
 * @param protect [PAGE_READWRITE] Memory block protection flags - default RW
 * @param flags   [MEM_RESERVE|MEM_COMMIT] Memory alloc flags - default MEM_RESERVE|MEM_COMMIT
 */
void* shadow_valloc(void* address, DWORD size, DWORD protect = PAGE_READWRITE, 
						  DWORD flags = MEM_RESERVE | MEM_COMMIT);



/**
 * @brief Read a memory block from a remote process
 */
bool shadow_vread(HANDLE process, void* address, void* buffer, DWORD size);
/**
 * @brief Read a memory block for this process
 */
bool shadow_vread(void* address, void* buffer, DWORD size);
/**
 * @brief Write a memory block into a remote process
 */
bool shadow_vwrite(HANDLE process, void* address, void* buffer, DWORD size);
/**
 * @brief Write a memory block into this process
 */
bool shadow_vwrite(void* address, void* buffer, DWORD size);



/**
 * @brief Free (MEM_FREE a virtual memory block (MEM_FREE) for a remote process
 */
bool shadow_vfree(HANDLE process, void* address);
/**
 * @brief Free a virtual memory block (MEM_FREE) for this process
 */
bool shadow_vfree(void* address);
/**
 * @brief Unmap a mapped (MEM_MAPPED|MEM_IMAGE) virtual memory block of a remote process
 */
bool shadow_vunmap(HANDLE process, void* address);
/**
 * @brief Unmap a mapped (MEM_MAPPED|MEM_IMAGE) virtual memory block of this process
 */
bool shadow_vunmap(void* address);



/**
 * @brief Unmap/Free the block of memory at address for the specified process
 */
bool shadow_unmap(HANDLE process, void* address);
/**
 * @brief Unmap/Free the block of memory at address for this process
 */
bool shadow_unmap(void* address);
/**
 * @brief Unmap/Free a region of memory with the specified base addr and region size for the specified process
 */
bool shadow_unmap(HANDLE process, void* baseAddress, DWORD regionSize);
/**
 * @brief Unmap/Free a region of memory with the specified base addr and region size for this process
 */
bool shadow_unmap(void* baseAddress, DWORD regionSize);



/**
 * @brief Manually gets proc address without using interceptable GetProcAddress
 */
FARPROC shadow_getproc(void* hmodule, LPCSTR name);
/**
 * @brief Manually get proc address directly from NTDLL.DLL
 * @brief No LoadLibrary / GetProcAddress calls are made
 */
FARPROC shadow_ntdll_getproc(const char* func);
/**
 * @brief Manually get proc address directly from KERNEL32.DLL
 * @brief No LoadLibrary / GetProcAddress calls are made
 */
FARPROC shadow_kernel_getproc(const char* func);



/**
 * @brief List all the procs in a module already loaded into memory
 * @param out Destination for the names of all the exported procs
 * @param hmodule Pointer to the start of the module. Can be a loaded DLL or just a loaded dll file buffer
 * @param istartsWith [optional] Case insensitive filter for a starts-with string comparison
 * @return false if failed, true otherwise. Resulting names in out vector.
 */
bool shadow_listprocs(std::vector<const char*>& out, void* hmodule, const char* istartsWith = NULL);



/**
 * @brief Creates a new thread in the specified remote process
 */
HANDLE shadow_create_thread(HANDLE process, LPTHREAD_START_ROUTINE start, void* param, bool suspended = false);
/**
 * @brief Creates a new thread in the specified remote process
 */
HANDLE shadow_create_thread(LPTHREAD_START_ROUTINE start, void* param, bool suspended = false);
/**
 * @brief Opens a thread by its ID straight from the kernel
 */
HANDLE shadow_open_thread(DWORD threadId, DWORD flags);
/**
 * @brief Closes an opened NT handle
 */
void shadow_close_handle(HANDLE thread);



/**
 * @brief Gets all threads associated with a remote process
 */
int shadow_get_threads(std::vector<DWORD>& out, DWORD processId);
/**
 * @brief Gets all threads associated with the current process
 */
int shadow_get_threads(std::vector<DWORD>& out);
/**
 * @brief Gets the count of all threads running on a remote process
 */
int shadow_get_threadcount(DWORD processId);
/**
 * @brief Gets the count of all threads running on this process
 */
int shadow_get_threadcount();
/**
 * @brief Gets whether the specified thread is running, using the thread HANDLE
 */
bool shadow_get_threadrunning(HANDLE thread);
/**
 * @brief Gets whether the specified thread is running, using the thread ID
 */
bool shadow_get_threadrunning(DWORD threadID);



/**
 * @brief Gets thread context with the specified optional getFlags
 */
bool shadow_get_threadctx(HANDLE thread, CONTEXT& ctx, DWORD getFlags = CONTEXT_FULL);
/**
 * @brief Sets thread context with the specified optional setFlags
 */
bool shadow_set_threadctx(HANDLE thread, CONTEXT& ctx, DWORD setFlags = CONTEXT_FULL);



/**
 * @brief Suspends the specified thread, assuming the handle has appropriate access rights
 */
bool shadow_suspend_thread(HANDLE thread);
/**
 * @brief Resumes the specified thread, assuming the handle has appropriate access rights
 */
bool shadow_resume_thread(HANDLE thread);



/**
 * @brief Sets the thread's EIP to a new location. Suspends any active threads. Doesn't reset stack.
 */
bool shadow_set_threadip(HANDLE thread, void* newInstructionPointer);



void shadow_scanmodules(HANDLE process, void* baseAddress, DWORD regionSize);
void shadow_scanmodules(void* baseAddress, DWORD regionSize);

bool shadow_listimports(std::vector<PCCH>& imports, HMODULE module);
bool shadow_listimports(std::vector<PCCH>& imports);

void shadow_unloadmodules();

/**
 * @brief List all loaded DLL modules for the current process
 */
bool shadow_listmodules(std::vector<std::string>& modules);

const char* shadow_getsyserr();

/**
 * @brief Hides your short string value encoded on the stack.
 *        Can be safely stored indefinitely. Max length is 28 chars.
 *        Calling get() unwinds the string for temporary use,
 *        resolving to a const char*
 */
struct shadow_string
{
	static const int SIZE = 32;
	char str[SIZE];

	template<class... Args> shadow_string(Args... params)
	{
		static_assert(sizeof...(Args) <= SIZE, "Max string length is 31");
		char* ptr = str + sizeof...(Args);
		expand{ *--ptr = params... }; // reverse magic

		int i = 0;
		for (; i < sizeof...(Args); ++i)
			str[i] = str[i] + (42 + i); // rot42+i
		str[sizeof...(Args)] = '\0' + 42 + i;
	}

	struct expand { template<typename ...T> __forceinline expand(T...){} };

	struct unwind
	{
		char str[SIZE];
		unwind(const char* src)
		{
			for (int i = 0;; ++i)
				if (!(str[i] = src[i] - (42 + i))) // un rot42+i
					break;
		}
		~unwind()
		{
			// ensure string is cleared in memory after use
			for (int i = 0; i < sizeof str; ++i) str[i] = 0;
		}
		operator const char*() const { return str; }
	};

	//// @brief Unwind the string to a readable form
	unwind get() const
	{
		return unwind(str);
	}
};

typedef shadow_string sstr;


struct shadow_string2
{
	static const int SIZE = 32;
	char str[SIZE];
//0x2d2c2b2a
//0x312f2d2b
//0x35322f2c
//0x3935312d
//0x3d38332e
//0x413b352f
//0x453e3730
//0x49413931
	typedef unsigned int uint;
	__forceinline shadow_string2()
	{
		*(uint*)str = 0 + 0x2d2c2b2a;
	}
	__forceinline shadow_string2(uint a)
	{
		uint* i = (uint*)str;
		i[0] = a + 0x2d2c2b2a, i[1] = 0 + 0x312f2d2b;
	}
	__forceinline shadow_string2(uint a, uint b)
	{
		uint* i = (uint*)str;
		i[0] = a + 0x2d2c2b2a, i[1] = b + 0x312f2d2b;
		i[2] = 0 + 0x35322f2c;
	}
	__forceinline shadow_string2(uint a, uint b, uint c)
	{
		uint* i = (uint*)str;
		i[0] = a + 0x2d2c2b2a, i[1] = b + 0x312f2d2b;
		i[2] = c + 0x35322f2c, i[3] = 0 + 0x3935312d;
	}
	__forceinline shadow_string2(uint a, uint b, uint c, uint d)
	{
		uint* i = (uint*)str;
		i[0] = a + 0x2d2c2b2a, i[1] = b + 0x312f2d2b;
		i[2] = c + 0x35322f2c, i[3] = d + 0x3935312d;
		i[4] = 0 + 0x3d38332e;
	}
	__forceinline shadow_string2(uint a, uint b, uint c, uint d,
								 uint e)
	{
		uint* i = (uint*)str;
		i[0] = a + 0x2d2c2b2a, i[1] = b + 0x312f2d2b;
		i[2] = c + 0x35322f2c, i[3] = d + 0x3935312d;
		i[4] = e + 0x3d38332e, i[5] = 0 + 0x413b352f;
	}
	__forceinline shadow_string2(uint a, uint b, uint c, uint d,
								 uint e, uint f)
	{
		uint* i = (uint*)str;
		i[0] = a + 0x2d2c2b2a, i[1] = b + 0x312f2d2b;
		i[2] = c + 0x35322f2c, i[3] = d + 0x3935312d;
		i[4] = e + 0x3d38332e, i[5] = f + 0x413b352f;
		i[6] = 0 + 0x453e3730;
	}
	__forceinline shadow_string2(uint a, uint b, uint c, uint d,
								 uint e, uint f, uint g)
	{
		uint* i = (uint*)str;
		i[0] = a + 0x2d2c2b2a, i[1] = b + 0x312f2d2b;
		i[2] = c + 0x35322f2c, i[3] = d + 0x3935312d;
		i[4] = e + 0x3d38332e, i[5] = f + 0x413b352f;
		i[6] = g + 0x453e3730, i[7] = 0 + 0x49413931;
	}
//0x2d2c2b2a
//0x312f2d2b
//0x35322f2c
//0x3935312d
//0x3d38332e
//0x413b352f
//0x453e3730
//0x49413931
	struct unwind
	{
		char str[SIZE];
		unwind(const char* src)
		{
			uint* a = (uint*)str, *b = (uint*)src;
			uint base = 0x2d2c2b2a;
			for (int i = 0; i < 8; ++i, base += 0x04030201)
				a[i] = _byteswap_ulong(b[i] - base);
		}
		~unwind()
		{
			// ensure string is cleared in memory after use
			for (int i = 0; i < sizeof str; ++i) str[i] = 0;
		}
		operator const char*() const { return str; }
	};
	unwind get() const
	{
		return unwind(str);
	}
};
typedef shadow_string2 sstr2;

