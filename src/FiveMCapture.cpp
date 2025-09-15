#include "FiveMCapture.h"
#include "DXGIHook.h"
#include "FrameCapture.h"
#include "ProjectionWindow.h"
#include "UI.h"

#include <iostream>
#include <chrono>
#include <wrl/client.h>

using namespace Microsoft::WRL;

// Global app instance
static std::unique_ptr<FiveMCapture> g_app = nullptr;

FiveMCapture* GetApp() {
    return g_app.get();
}

FiveMCapture::FiveMCapture() 
    : running_(false), mainWindow_(nullptr) {
}

FiveMCapture::~FiveMCapture() {
    Shutdown();
}

bool FiveMCapture::Initialize() {
    std::wcout << L"Initializing FiveM Capture System..." << std::endl;

    // Initialize DirectX 11
    if (!InitializeDX11()) {
        std::wcerr << L"Failed to initialize DirectX 11" << std::endl;
        return false;
    }

    // Create main window for UI
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        if (auto* app = GetApp()) {
            if (app->ui_ && app->ui_->IsVisible()) {
                auto result = app->ui_->HandleMessage(hwnd, msg, wParam, lParam);
                if (result != 0) return result;
            }
        }

        switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_F1) {
                if (auto* app = GetApp()) {
                    app->config_.showUI = !app->config_.showUI;
                    if (app->ui_) {
                        app->ui_->SetVisible(app->config_.showUI);
                    }
                }
            }
            break;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    };
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FiveMCaptureMainWindow";

    RegisterClassExW(&wc);

    mainWindow_ = CreateWindowExW(
        0, L"FiveMCaptureMainWindow", L"FiveM Capture System",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!mainWindow_) {
        std::wcerr << L"Failed to create main window" << std::endl;
        return false;
    }

    ShowWindow(mainWindow_, SW_SHOW);
    UpdateWindow(mainWindow_);

    // Initialize components
    frameCapture_ = std::make_unique<FrameCapture>();
    if (!frameCapture_->Initialize(d3d11Device_.Get(), d3d11Context_.Get())) {
        std::wcerr << L"Failed to initialize frame capture" << std::endl;
        return false;
    }

    projectionWindow_ = std::make_unique<ProjectionWindow>();
    if (!projectionWindow_->Initialize(GetModuleHandle(nullptr), d3d11Device_.Get(), d3d11Context_.Get())) {
        std::wcerr << L"Failed to initialize projection window" << std::endl;
        return false;
    }

    ui_ = std::make_unique<UI>();
    if (!ui_->Initialize(mainWindow_, d3d11Device_.Get(), d3d11Context_.Get())) {
        std::wcerr << L"Failed to initialize UI" << std::endl;
        return false;
    }

    ui_->SetVisible(config_.showUI);

    // Setup hooks last
    if (!SetupHooks()) {
        std::wcerr << L"Failed to setup DirectX hooks" << std::endl;
        return false;
    }

    running_ = true;
    std::wcout << L"FiveM Capture System initialized successfully!" << std::endl;
    std::wcout << L"Press F1 to toggle UI" << std::endl;

    return true;
}

void FiveMCapture::Run() {
    if (!running_) return;

    std::wcout << L"Starting main loop..." << std::endl;
    MessageLoop();
}

void FiveMCapture::Shutdown() {
    running_ = false;

    std::wcout << L"Shutting down FiveM Capture System..." << std::endl;

    if (dxgiHook_) {
        dxgiHook_->Shutdown();
        dxgiHook_.reset();
    }

    if (ui_) {
        ui_->Shutdown();
        ui_.reset();
    }

    if (projectionWindow_) {
        projectionWindow_->Shutdown();
        projectionWindow_.reset();
    }

    if (frameCapture_) {
        frameCapture_->Shutdown();
        frameCapture_.reset();
    }

    if (mainWindow_) {
        DestroyWindow(mainWindow_);
        mainWindow_ = nullptr;
    }

    d3d11Context_.Reset();
    d3d11Device_.Reset();
    swapChain_.Reset();

    std::wcout << L"Shutdown complete." << std::endl;
}

bool FiveMCapture::InitializeDX11() {
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = mainWindow_;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_DEBUG, nullptr, 0,
        D3D11_SDK_VERSION, &swapChainDesc,
        &swapChain_, &d3d11Device_, &featureLevel, &d3d11Context_);

    if (FAILED(hr)) {
        std::wcerr << L"Failed to create D3D11 device and swap chain. HR: 0x" 
                  << std::hex << hr << std::endl;
        return false;
    }

    std::wcout << L"DirectX 11 initialized successfully" << std::endl;
    return true;
}

bool FiveMCapture::SetupHooks() {
    dxgiHook_ = std::make_unique<DXGIHook>();
    
    if (!dxgiHook_->Initialize()) {
        return false;
    }

    // Set up callbacks
    dxgiHook_->SetPresentCallback([this](IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
        if (config_.enableCapture && frameCapture_) {
            frameCapture_->CaptureFrame(swapChain);
        }
    });

    dxgiHook_->SetResizeCallback([this](IDXGISwapChain* swapChain, UINT bufferCount, 
                                       UINT width, UINT height, DXGI_FORMAT newFormat, UINT flags) {
        if (frameCapture_) {
            frameCapture_->OnResolutionChanged(width, height, newFormat);
        }
    });

    return true;
}

void FiveMCapture::MessageLoop() {
    MSG msg = {};
    auto lastUpdateTime = std::chrono::high_resolution_clock::now();
    const auto targetFrameTime = std::chrono::microseconds(1000000 / config_.targetFPS);

    while (running_) {
        // Process Windows messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running_ = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!running_) break;

        auto currentTime = std::chrono::high_resolution_clock::now();
        if (currentTime - lastUpdateTime >= targetFrameTime) {
            UpdateFrame();
            lastUpdateTime = currentTime;
        }

        // Small sleep to prevent 100% CPU usage
        Sleep(1);
    }
}

void FiveMCapture::UpdateFrame() {
    std::lock_guard<std::mutex> lock(frameMutex_);

    // Update UI
    if (ui_ && config_.showUI) {
        ui_->NewFrame();
        ui_->Render();
        ui_->Present();
    }

    // Update projection window
    if (projectionWindow_ && frameCapture_) {
        auto latestFrame = frameCapture_->GetLatestFrame();
        if (latestFrame && latestFrame->valid) {
            projectionWindow_->RenderFrame(latestFrame);
            projectionWindow_->Present();
        }
    }

    // Present main window
    if (swapChain_) {
        swapChain_->Present(1, 0);
    }
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Enable console for debugging
    AllocConsole();
    FILE* pCout;
    FILE* pCin;
    FILE* pCerr;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    freopen_s(&pCin, "CONIN$", "r", stdin);
    freopen_s(&pCerr, "CONOUT$", "w", stderr);

    std::wcout << L"FiveM Capture System Starting..." << std::endl;

    // Create and initialize application
    g_app = std::make_unique<FiveMCapture>();
    
    if (!g_app->Initialize()) {
        std::wcerr << L"Failed to initialize application!" << std::endl;
        return -1;
    }

    // Run main loop
    g_app->Run();

    // Cleanup
    g_app.reset();

    FreeConsole();
    return 0;
}