#include "application.h"

#ifdef LINUX_BUILD
#include "platform_compat.h"
#endif

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
#ifdef LINUX_BUILD
    // Linux build - just return success for syntax checking
    return 0;
#else
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
#endif
}