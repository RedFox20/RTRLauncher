/**
 * Copyright (c) 2014 - ShadowFox ^_^
 */
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <map>
#include <rpp/strview.h>


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
PCHAR shadow_valloc(HANDLE process, void* address, DWORD size, DWORD protect = PAGE_READWRITE, 
                    DWORD flags = MEM_RESERVE | MEM_COMMIT);
/**
 * @brief Alloc a memory block for this process
 * @param protect [PAGE_READWRITE] Memory block protection flags - default RW
 * @param flags   [MEM_RESERVE|MEM_COMMIT] Memory alloc flags - default MEM_RESERVE|MEM_COMMIT
 */
PCHAR shadow_valloc(void* address, DWORD size, DWORD protect = PAGE_READWRITE, 
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
 * Gets the export directory info from a module image
 */
PIMAGE_NT_HEADERS get_nt_headers(HMODULE module);

/**
 * @return TRUE if this module contains a valid IMAGE_NT_HEADERS
 */
bool is_valid_nt_module(HMODULE module);

/**
 * @param module
 * @param image_directory_entry_id Example: IMAGE_DIRECTORY_ENTRY_EXPORT
 * @return If it's a valid module, returns the specified module data directory
 */
PIMAGE_DATA_DIRECTORY get_image_data_directory(HMODULE module, int image_directory_entry_id);

/**
 * Gets the export directory info from a module image
 */
PIMAGE_EXPORT_DIRECTORY get_module_exports(HMODULE module);


class ModuleProcCache
{
    std::map<rpp::strview, FARPROC> procs;
public:
    explicit ModuleProcCache(HMODULE module);
    FARPROC find(rpp::strview name) const;
};


/**
 * @brief Manually gets proc address without using interceptable GetProcAddress
 */
FARPROC shadow_getproc(HMODULE module, LPCSTR name);
/**
 * @brief Manually get proc address directly from NTDLL.DLL
 * @brief No LoadLibrary / GetProcAddress calls are made
 */
FARPROC shadow_ntdll_getproc(rpp::strview func);
/**
 * @brief Manually get proc address directly from KERNEL32.DLL
 * @brief No LoadLibrary / GetProcAddress calls are made
 */
FARPROC shadow_kernel_getproc(rpp::strview func);

template<class Func> Func shadow_kernel_getproc(rpp::strview func)
{
    FARPROC proc = shadow_kernel_getproc(func);
    return reinterpret_cast<Func>(proc);
}



/**
 * @brief List all the procs in a module already loaded into memory
 * @param module Pointer to the start of the module. Can be a loaded DLL or just a loaded dll file buffer
 * @param istartsWith [optional] Case insensitive filter for a starts-with string comparison
 */
std::vector<rpp::strview> shadow_listprocs(HMODULE module, const char* istartsWith = nullptr);



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
void shadow_close_handle(HANDLE handle);


struct ProcessInfo
{
    std::string name;
    DWORD pid;
};

/**
 * @brief Gets all processes which partially match the name
 */
std::vector<ProcessInfo> shadow_get_processes(rpp::strview name);


/**
 * @brief Gets all threads associated with a remote process
 */
std::vector<DWORD> shadow_get_threads(DWORD processId);
/**
 * @brief Gets all threads associated with the current process
 */
std::vector<DWORD> shadow_get_threads();
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


/**
 * Enabled Debug privileges in the target process
 */
bool shadow_enable_debug_privilege(HANDLE proccess);


void shadow_scanmodules(HANDLE process, void* baseAddress, DWORD regionSize);
void shadow_scanmodules(void* baseAddress, DWORD regionSize);

std::vector<rpp::strview> shadow_list_imports(HMODULE module);
std::vector<rpp::strview> shadow_list_imports();

void shadow_unloadmodules();

/**
 * @brief List all loaded DLL modules for the current process
 */
bool shadow_listmodules(std::vector<std::string>& modules);


/**
 * Opens the process handle with PROCESS_ALL_ACCESS flags
 */
HANDLE shadow_open_process_all_access(DWORD processId);


struct ExitStatus
{
    DWORD exit_code = 0;
    bool is_running = false;
    explicit operator bool() const { return is_running; }
};

/**
 * @return Status and exit code of the process
 */
ExitStatus shadow_get_process_status(HANDLE process);
ExitStatus shadow_get_process_status(DWORD processId);

/**
 * @return Status and exit code of the thread
 */
ExitStatus shadow_get_thread_status(HANDLE thread);
ExitStatus shadow_get_thread_status(DWORD threadId);


const char* shadow_getsyserr(DWORD error = 0);

