// Stub library for Microsoft Detours when the actual library is not available
// This allows the project to compile for testing purposes
// For production use, download the actual Microsoft Detours library

#include <windows.h>

extern "C" {
    __declspec(dllexport) LONG WINAPI DetourTransactionBegin(VOID) {
        OutputDebugStringA("[DETOURS STUB] DetourTransactionBegin called\n");
        return 0;  // NO_ERROR
    }

    __declspec(dllexport) LONG WINAPI DetourUpdateThread(HANDLE hThread) {
        OutputDebugStringA("[DETOURS STUB] DetourUpdateThread called\n");
        return 0;  // NO_ERROR
    }

    __declspec(dllexport) LONG WINAPI DetourAttach(PVOID *ppPointer, PVOID pDetour) {
        OutputDebugStringA("[DETOURS STUB] DetourAttach called - Hook installation stubbed\n");
        return 0;  // NO_ERROR
    }

    __declspec(dllexport) LONG WINAPI DetourDetach(PVOID *ppPointer, PVOID pDetour) {
        OutputDebugStringA("[DETOURS STUB] DetourDetach called - Hook removal stubbed\n");
        return 0;  // NO_ERROR
    }

    __declspec(dllexport) LONG WINAPI DetourTransactionCommit(VOID) {
        OutputDebugStringA("[DETOURS STUB] DetourTransactionCommit called\n");
        return 0;  // NO_ERROR
    }
}