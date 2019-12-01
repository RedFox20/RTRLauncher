#pragma once

struct inject_result
{
    void* Process;
    void* DllImage; // LPVOID if we loaded from resource / image data

    explicit operator bool() const { return DllImage != nullptr; }

    /**
     * Unloads the injected DLL from remote process
     */
    void unload();
};

struct remote_dll_injector
{
    void* Image;    // injectable Dll *image
    void* Resource; // managed resource

    remote_dll_injector();
    remote_dll_injector(int resourceId);
    remote_dll_injector(void* pInjectableDllImage);
    ~remote_dll_injector();

    explicit operator bool() const { return Image != nullptr; }

    /**
     * Inject to the specified process
     * Copyright notes: Original code written by zwclose7
     * @param targetProcessHANDLE - Process HANDLE to inject the dll into
     */
    inject_result inject_dllimage(void* targetProcessHANDLE) const;

    /**
     * Inject to the specified process
     * @param targetProcessHANDLE - Process HANDLE to inject the dll into
     * @param filename The filename of the module to load ie: "GameEngine.dll"
     * @return true on success, false on failure
     */
    static bool inject_dllfile(void* targetProcessHANDLE, const char* filename);

    /**
     * Enables DEBUG privileges - sometimes required
     * to open remote processes with higher access rights
     */
    static void enable_debug_privilege();

    /**
     * Reserve some memory after target process's ImageBase
     * This is crucial for EXE loading to work in later inject/replace stages
     * @note This should be called after CreateProcess SUSPENDED and before inject_dll
     * @param targetProcessHANDLE - Process HANDLE to inject the dll into
     * @param reserveSize The number of bytes to reserve from ImageBase to ImageBase + reserveSize
     * @return TRUE if required memory was successfully reserved, FALSE if a memory block was in the way
     */
    static bool reserve_target_memory(void* targetProcessHANDLE, int reserveSize);
};
