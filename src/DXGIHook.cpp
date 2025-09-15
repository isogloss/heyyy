#include "DXGIHook.h"
#include <iostream>
#include <d3d11.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

// Static member definitions
DXGIHook::Present_t DXGIHook::OriginalPresent = nullptr;
DXGIHook::ResizeBuffers_t DXGIHook::OriginalResizeBuffers = nullptr;

// Global hook instance
static DXGIHook* g_dxgiHook = nullptr;

DXGIHook* GetDXGIHook() {
    return g_dxgiHook;
}

DXGIHook::DXGIHook() 
    : hooksInstalled_(false), presentAddress_(nullptr), resizeBuffersAddress_(nullptr) {
    g_dxgiHook = this;
}

DXGIHook::~DXGIHook() {
    Shutdown();
    g_dxgiHook = nullptr;
}

bool DXGIHook::Initialize() {
    std::wcout << L"Initializing DXGI hooks..." << std::endl;

    if (hooksInstalled_) {
        std::wcout << L"Hooks already installed" << std::endl;
        return true;
    }

    if (!FindDXGIFunctions()) {
        std::wcerr << L"Failed to find DXGI functions" << std::endl;
        return false;
    }

    if (!InstallHooks()) {
        std::wcerr << L"Failed to install hooks" << std::endl;
        return false;
    }

    hooksInstalled_ = true;
    std::wcout << L"DXGI hooks installed successfully" << std::endl;
    return true;
}

void DXGIHook::Shutdown() {
    if (hooksInstalled_) {
        std::wcout << L"Removing DXGI hooks..." << std::endl;
        RemoveHooks();
        hooksInstalled_ = false;
    }
}

bool DXGIHook::FindDXGIFunctions() {
    // Create a temporary D3D11 device to get function addresses
    ComPtr<ID3D11Device> tempDevice;
    ComPtr<ID3D11DeviceContext> tempContext;
    ComPtr<IDXGISwapChain> tempSwapChain;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = GetConsoleWindow(); // Use console window temporarily
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc,
        &tempSwapChain, &tempDevice, &featureLevel, &tempContext);

    if (FAILED(hr)) {
        std::wcerr << L"Failed to create temporary D3D11 device for hook discovery" << std::endl;
        return false;
    }

    // Get vtable addresses
    void** swapChainVTable = *reinterpret_cast<void***>(tempSwapChain.Get());
    
    // IDXGISwapChain::Present is at index 8
    // IDXGISwapChain::ResizeBuffers is at index 13
    presentAddress_ = swapChainVTable[8];
    resizeBuffersAddress_ = swapChainVTable[13];

    if (!presentAddress_ || !resizeBuffersAddress_) {
        std::wcerr << L"Failed to locate DXGI function addresses" << std::endl;
        return false;
    }

    std::wcout << L"Found DXGI function addresses:" << std::endl;
    std::wcout << L"  Present: 0x" << std::hex << presentAddress_ << std::endl;
    std::wcout << L"  ResizeBuffers: 0x" << std::hex << resizeBuffersAddress_ << std::endl;

    return true;
}

bool DXGIHook::InstallHooks() {
    std::lock_guard<std::mutex> lock(hookMutex_);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    // Hook Present
    OriginalPresent = reinterpret_cast<Present_t>(presentAddress_);
    LONG result1 = DetourAttach(&reinterpret_cast<PVOID&>(OriginalPresent), HookedPresent);

    // Hook ResizeBuffers
    OriginalResizeBuffers = reinterpret_cast<ResizeBuffers_t>(resizeBuffersAddress_);
    LONG result2 = DetourAttach(&reinterpret_cast<PVOID&>(OriginalResizeBuffers), HookedResizeBuffers);

    LONG commitResult = DetourTransactionCommit();

    if (result1 != NO_ERROR || result2 != NO_ERROR || commitResult != NO_ERROR) {
        std::wcerr << L"Failed to install hooks. Results: " << result1 << L", " 
                  << result2 << L", " << commitResult << std::endl;
        return false;
    }

    return true;
}

bool DXGIHook::RemoveHooks() {
    std::lock_guard<std::mutex> lock(hookMutex_);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    LONG result1 = DetourDetach(&reinterpret_cast<PVOID&>(OriginalPresent), HookedPresent);
    LONG result2 = DetourDetach(&reinterpret_cast<PVOID&>(OriginalResizeBuffers), HookedResizeBuffers);

    LONG commitResult = DetourTransactionCommit();

    if (result1 != NO_ERROR || result2 != NO_ERROR || commitResult != NO_ERROR) {
        std::wcerr << L"Failed to remove hooks. Results: " << result1 << L", " 
                  << result2 << L", " << commitResult << std::endl;
        return false;
    }

    OriginalPresent = nullptr;
    OriginalResizeBuffers = nullptr;

    return true;
}

HRESULT WINAPI DXGIHook::HookedPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
    if (g_dxgiHook && g_dxgiHook->presentCallback_) {
        try {
            g_dxgiHook->presentCallback_(swapChain, syncInterval, flags);
        }
        catch (const std::exception& e) {
            std::wcerr << L"Exception in Present callback: " << e.what() << std::endl;
        }
        catch (...) {
            std::wcerr << L"Unknown exception in Present callback" << std::endl;
        }
    }

    // Call original function
    if (OriginalPresent) {
        return OriginalPresent(swapChain, syncInterval, flags);
    }

    return E_FAIL;
}

HRESULT WINAPI DXGIHook::HookedResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount, 
                                            UINT width, UINT height, DXGI_FORMAT newFormat, UINT flags) {
    if (g_dxgiHook && g_dxgiHook->resizeCallback_) {
        try {
            g_dxgiHook->resizeCallback_(swapChain, bufferCount, width, height, newFormat, flags);
        }
        catch (const std::exception& e) {
            std::wcerr << L"Exception in ResizeBuffers callback: " << e.what() << std::endl;
        }
        catch (...) {
            std::wcerr << L"Unknown exception in ResizeBuffers callback" << std::endl;
        }
    }

    // Call original function
    if (OriginalResizeBuffers) {
        return OriginalResizeBuffers(swapChain, bufferCount, width, height, newFormat, flags);
    }

    return E_FAIL;
}