#pragma once

struct remote_dll_injector
{
    /**
     * Inject to the specified process
     * @param targetProcessHANDLE - Process HANDLE to inject the dll into
     * @param filename The filename of the module to load ie: "GameEngine.dll"
     * @note Throws on failure
     */
    static void inject_dllfile(void* targetProcessHANDLE, const char* filename);

    /**
     * Enables DEBUG privileges - sometimes required
     * to open remote processes with higher access rights
     */
    static void enable_debug_privilege();
};
