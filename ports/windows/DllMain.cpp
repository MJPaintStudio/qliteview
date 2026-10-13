#include <windows.h>

// Used by LiteView's resource/path helpers. Initialization runs explicitly
// from mbInit, outside the Windows loader lock.
HMODULE g_hModule = nullptr;
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
