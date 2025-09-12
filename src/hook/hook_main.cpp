#include "d3d11_hook.h"
#include "frame_capture.h"
#include <windows.h>

// Global hook instance
static D3D11Hook* g_hook = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Initialize frame capture
        if (!FrameCapture::Initialize()) {
            return FALSE;
        }

        // Create and initialize hook
        g_hook = new D3D11Hook();
        if (!g_hook || !g_hook->Initialize()) {
            delete g_hook;
            g_hook = nullptr;
            FrameCapture::Shutdown();
            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH:
        // Cleanup hook
        if (g_hook) {
            g_hook->Shutdown();
            delete g_hook;
            g_hook = nullptr;
        }

        // Shutdown frame capture
        FrameCapture::Shutdown();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}