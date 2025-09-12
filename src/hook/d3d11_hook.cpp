#include "d3d11_hook.h"
#include "frame_capture.h"
#include <d3d11.h>
#include <dxgi.h>

// Static member definitions
D3D11Hook* D3D11Hook::s_instance = nullptr;
D3D11Hook::PresentFunc D3D11Hook::s_originalPresent = nullptr;
D3D11Hook::ResizeBuffersFunc D3D11Hook::s_originalResizeBuffers = nullptr;

// Simple memory protection helper
class VTableHook {
public:
    static bool HookVTableFunction(void** vtable, int index, void* hookFunc, void** originalFunc) {
        if (!vtable || !hookFunc || !originalFunc) {
            return false;
        }

        DWORD oldProtect;
        if (!VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }

        *originalFunc = vtable[index];
        vtable[index] = hookFunc;

        VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProtect);
        return true;
    }

    static void UnhookVTableFunction(void** vtable, int index, void* originalFunc) {
        if (!vtable || !originalFunc) {
            return;
        }

        DWORD oldProtect;
        if (VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            vtable[index] = originalFunc;
            VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProtect);
        }
    }
};

D3D11Hook::D3D11Hook()
    : m_hooked(false)
    , m_presentVTable(nullptr)
    , m_resizeBuffersVTable(nullptr)
{
    s_instance = this;
}

D3D11Hook::~D3D11Hook() {
    Shutdown();
    s_instance = nullptr;
}

bool D3D11Hook::Initialize() {
    if (m_hooked) {
        return true;
    }

    // Find existing swap chain
    if (!FindSwapChain()) {
        return false;
    }

    // Install hooks
    if (!InstallHook()) {
        return false;
    }

    m_hooked = true;
    return true;
}

void D3D11Hook::Shutdown() {
    if (m_hooked) {
        UninstallHook();
        m_hooked = false;
    }
    
    m_swapChain.Reset();
}

bool D3D11Hook::FindSwapChain() {
    // Create temporary D3D11 device and swap chain to get vtable
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = 1;
    swapChainDesc.BufferDesc.Height = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = GetDesktopWindow();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    ComPtr<ID3D11Device> tempDevice;
    ComPtr<ID3D11DeviceContext> tempContext;
    ComPtr<IDXGISwapChain> tempSwapChain;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &swapChainDesc, tempSwapChain.GetAddressOf(),
        tempDevice.GetAddressOf(), &featureLevel, tempContext.GetAddressOf()
    );

    if (FAILED(hr)) {
        return false;
    }

    // Get vtable from temporary swap chain
    void** vtable = *reinterpret_cast<void***>(tempSwapChain.Get());
    m_presentVTable = vtable;
    m_resizeBuffersVTable = vtable;

    return true;
}

bool D3D11Hook::InstallHook() {
    if (!m_presentVTable) {
        return false;
    }

    // Hook Present (index 8 in IDXGISwapChain vtable)
    if (!VTableHook::HookVTableFunction(m_presentVTable, 8, 
        reinterpret_cast<void*>(PresentHook), 
        reinterpret_cast<void**>(&s_originalPresent))) {
        return false;
    }

    // Hook ResizeBuffers (index 13 in IDXGISwapChain vtable)
    if (!VTableHook::HookVTableFunction(m_resizeBuffersVTable, 13,
        reinterpret_cast<void*>(ResizeBuffersHook),
        reinterpret_cast<void**>(&s_originalResizeBuffers))) {
        
        // Rollback Present hook if ResizeBuffers fails
        VTableHook::UnhookVTableFunction(m_presentVTable, 8, 
            reinterpret_cast<void*>(s_originalPresent));
        return false;
    }

    return true;
}

void D3D11Hook::UninstallHook() {
    if (m_presentVTable && s_originalPresent) {
        VTableHook::UnhookVTableFunction(m_presentVTable, 8, 
            reinterpret_cast<void*>(s_originalPresent));
        s_originalPresent = nullptr;
    }

    if (m_resizeBuffersVTable && s_originalResizeBuffers) {
        VTableHook::UnhookVTableFunction(m_resizeBuffersVTable, 13,
            reinterpret_cast<void*>(s_originalResizeBuffers));
        s_originalResizeBuffers = nullptr;
    }

    m_presentVTable = nullptr;
    m_resizeBuffersVTable = nullptr;
}

HRESULT STDMETHODCALLTYPE D3D11Hook::PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
    // Capture frame before presenting
    if (s_instance) {
        FrameCapture::CaptureFrame(swapChain);
    }

    // Call original Present
    if (s_originalPresent) {
        return s_originalPresent(swapChain, syncInterval, flags);
    }

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE D3D11Hook::ResizeBuffersHook(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags) {
    // Notify frame capture of resize
    FrameCapture::OnSwapChainResize(width, height);

    // Call original ResizeBuffers
    if (s_originalResizeBuffers) {
        return s_originalResizeBuffers(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
    }

    return E_FAIL;
}