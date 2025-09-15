#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dwmapi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <memory>
#include <functional>
#include <mutex>
#include <vector>
#include <thread>
#include <atomic>

// Link required libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dwmapi.lib")

// Forward declarations
class DXGIHook;
class FrameCapture;
class ProjectionWindow;
class UI;

/**
 * Main application class that orchestrates the FiveM capture system
 */
class FiveMCapture {
public:
    FiveMCapture();
    ~FiveMCapture();

    bool Initialize();
    void Run();
    void Shutdown();

    // Configuration
    struct Config {
        bool enableCapture = true;
        bool fullscreenProjection = false;
        int targetFPS = 60;
        bool showUI = true;
    };

    Config& GetConfig() { return config_; }

private:
    bool InitializeDX11();
    bool SetupHooks();
    void MessageLoop();
    void UpdateFrame();

    Config config_;
    
    // DirectX 11 resources
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11Device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11Context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;

    // Core components (need to be public for UI access)
public:
    std::unique_ptr<DXGIHook> dxgiHook_;
    std::unique_ptr<FrameCapture> frameCapture_;
    std::unique_ptr<ProjectionWindow> projectionWindow_;
    std::unique_ptr<UI> ui_;

private:

    // Threading
    std::atomic<bool> running_;
    std::mutex frameMutex_;
    
    HWND mainWindow_;
};

// Global instance accessor
FiveMCapture* GetApp();