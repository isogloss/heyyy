#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

// Forward declaration
class SharedTexture;

/**
 * Handles frame capture from hooked D3D11 Present calls
 */
class FrameCapture {
public:
    // Initialize frame capture system
    static bool Initialize();
    
    // Shutdown frame capture system
    static void Shutdown();
    
    // Capture frame from swap chain (called from Present hook)
    static void CaptureFrame(IDXGISwapChain* swapChain);
    
    // Handle swap chain resize
    static void OnSwapChainResize(UINT width, UINT height);

private:
    // Get device and context from swap chain
    static bool GetDeviceFromSwapChain(IDXGISwapChain* swapChain, 
        ID3D11Device** device, ID3D11DeviceContext** context);
    
    // Initialize shared texture for current dimensions
    static bool InitializeSharedTexture(UINT width, UINT height, DXGI_FORMAT format);

    // Static members
    static ComPtr<ID3D11Device> s_device;
    static ComPtr<ID3D11DeviceContext> s_context;
    static std::unique_ptr<SharedTexture> s_sharedTexture;
    static UINT s_currentWidth;
    static UINT s_currentHeight;
    static DXGI_FORMAT s_currentFormat;
    static bool s_initialized;
};