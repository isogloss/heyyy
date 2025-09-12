#ifdef LINUX_BUILD
#include "platform_compat.h"

// Simple main for Linux build testing - just shows project structure
int main() {
    // This is a Windows-only DirectX 11 application
    // This Linux build is just for syntax checking
    return 0;
}
#else

#include "application.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    // Initialize COM for DirectX
    CoInitialize(nullptr);
    
    Application app;
    
    if (!app.Initialize(hInstance)) {
        CoUninitialize();
        return -1;
    }
    
    int result = app.Run();
    
    app.Shutdown();
    CoUninitialize();
    
    return result;
}

#endif