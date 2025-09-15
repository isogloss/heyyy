#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <detours.h>
#include <functional>
#include <memory>
#include <mutex>

/**
 * DXGI/D3D11 hooking system using Microsoft Detours
 * Intercepts DirectX calls to capture frames from FiveM
 */
class DXGIHook {
public:
    DXGIHook();
    ~DXGIHook();

    bool Initialize();
    void Shutdown();

    // Hook callbacks
    using PresentCallback = std::function<void(IDXGISwapChain*, UINT, UINT)>;
    using ResizeCallback = std::function<void(IDXGISwapChain*, UINT, UINT, DXGI_FORMAT, UINT)>;

    void SetPresentCallback(PresentCallback callback) { presentCallback_ = callback; }
    void SetResizeCallback(ResizeCallback callback) { resizeCallback_ = callback; }

private:
    // Original function pointers
    typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
    typedef HRESULT(WINAPI* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    static Present_t OriginalPresent;
    static ResizeBuffers_t OriginalResizeBuffers;

    // Hook functions
    static HRESULT WINAPI HookedPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
    static HRESULT WINAPI HookedResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount, 
                                             UINT width, UINT height, DXGI_FORMAT newFormat, UINT flags);

    // Hook installation
    bool InstallHooks();
    bool RemoveHooks();
    bool FindDXGIFunctions();

    PresentCallback presentCallback_;
    ResizeCallback resizeCallback_;
    
    bool hooksInstalled_;
    std::mutex hookMutex_;

    // D3D11/DXGI function addresses
    void* presentAddress_;
    void* resizeBuffersAddress_;
};

// Global hook instance
DXGIHook* GetDXGIHook();