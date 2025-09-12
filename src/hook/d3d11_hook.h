#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

/**
 * D3D11 Present hook for capturing frames
 */
class D3D11Hook {
public:
    D3D11Hook();
    ~D3D11Hook();

    // Initialize the hook
    bool Initialize();
    
    // Shutdown and cleanup
    void Shutdown();
    
    // Check if hook is active
    bool IsActive() const { return m_hooked; }

private:
    // Hook functions
    static HRESULT STDMETHODCALLTYPE PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
    static HRESULT STDMETHODCALLTYPE ResizeBuffersHook(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);

    // Original functions
    typedef HRESULT(STDMETHODCALLTYPE* PresentFunc)(IDXGISwapChain*, UINT, UINT);
    typedef HRESULT(STDMETHODCALLTYPE* ResizeBuffersFunc)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    // Hook installation
    bool InstallHook();
    void UninstallHook();
    
    // Get swap chain from device
    bool FindSwapChain();

    // Static members for hooks
    static D3D11Hook* s_instance;
    static PresentFunc s_originalPresent;
    static ResizeBuffersFunc s_originalResizeBuffers;

    // Members
    bool m_hooked;
    ComPtr<IDXGISwapChain> m_swapChain;
    void** m_presentVTable;
    void** m_resizeBuffersVTable;
};