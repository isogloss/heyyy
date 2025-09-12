#include "frame_capture.h"
#include "../shared_texture.h"

// Static member definitions
ComPtr<ID3D11Device> FrameCapture::s_device;
ComPtr<ID3D11DeviceContext> FrameCapture::s_context;
std::unique_ptr<SharedTexture> FrameCapture::s_sharedTexture;
UINT FrameCapture::s_currentWidth = 0;
UINT FrameCapture::s_currentHeight = 0;
DXGI_FORMAT FrameCapture::s_currentFormat = DXGI_FORMAT_UNKNOWN;
bool FrameCapture::s_initialized = false;

bool FrameCapture::Initialize() {
    if (s_initialized) {
        return true;
    }

    s_initialized = true;
    return true;
}

void FrameCapture::Shutdown() {
    s_sharedTexture.reset();
    s_context.Reset();
    s_device.Reset();
    
    s_currentWidth = 0;
    s_currentHeight = 0;
    s_currentFormat = DXGI_FORMAT_UNKNOWN;
    s_initialized = false;
}

void FrameCapture::CaptureFrame(IDXGISwapChain* swapChain) {
    if (!swapChain) {
        return;
    }

    // Get device and context if we don't have them
    if (!s_device || !s_context) {
        if (!GetDeviceFromSwapChain(swapChain, s_device.GetAddressOf(), s_context.GetAddressOf())) {
            return;
        }
    }

    // Get back buffer from swap chain
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(hr)) {
        return;
    }

    // Get back buffer description
    D3D11_TEXTURE2D_DESC desc;
    backBuffer->GetDesc(&desc);

    // Check if we need to reinitialize shared texture
    if (!s_sharedTexture || 
        desc.Width != s_currentWidth || 
        desc.Height != s_currentHeight || 
        desc.Format != s_currentFormat) {
        
        if (!InitializeSharedTexture(desc.Width, desc.Height, desc.Format)) {
            return;
        }
    }

    // Update shared texture with current frame
    if (s_sharedTexture) {
        s_sharedTexture->UpdateFrame(s_context.Get(), backBuffer.Get());
    }
}

void FrameCapture::OnSwapChainResize(UINT width, UINT height) {
    // Reset shared texture to force reinitialization on next frame
    s_sharedTexture.reset();
    s_currentWidth = 0;
    s_currentHeight = 0;
    s_currentFormat = DXGI_FORMAT_UNKNOWN;
}

bool FrameCapture::GetDeviceFromSwapChain(IDXGISwapChain* swapChain, ID3D11Device** device, ID3D11DeviceContext** context) {
    if (!swapChain || !device || !context) {
        return false;
    }

    HRESULT hr = swapChain->GetDevice(IID_PPV_ARGS(device));
    if (FAILED(hr)) {
        return false;
    }

    (*device)->GetImmediateContext(context);
    return true;
}

bool FrameCapture::InitializeSharedTexture(UINT width, UINT height, DXGI_FORMAT format) {
    if (!s_device) {
        return false;
    }

    // Create new shared texture
    s_sharedTexture = std::make_unique<SharedTexture>();
    if (!s_sharedTexture->InitializeProducer(s_device.Get(), width, height, format)) {
        s_sharedTexture.reset();
        return false;
    }

    // Store current dimensions
    s_currentWidth = width;
    s_currentHeight = height;
    s_currentFormat = format;

    return true;
}