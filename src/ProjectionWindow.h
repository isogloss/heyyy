#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dwmapi.h>
#include <wrl/client.h>
#include <memory>
#include <string>

#include "FrameCapture.h"

/**
 * Projection window system for displaying captured frames
 * Supports borderless fullscreen and windowed modes
 */
class ProjectionWindow {
public:
    ProjectionWindow();
    ~ProjectionWindow();

    bool Initialize(HINSTANCE hInstance, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Window management
    bool CreateProjectionWindow();
    void ShowWindow(bool show);
    void SetFullscreen(bool fullscreen);
    bool IsFullscreen() const { return isFullscreen_; }

    // Frame rendering
    void RenderFrame(std::shared_ptr<FrameCapture::CapturedFrame> frame);
    void Present();

    // Window properties
    HWND GetHWND() const { return hwnd_; }
    void SetTitle(const std::wstring& title);

private:
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // DirectX setup
    bool CreateSwapChain();
    bool CreateRenderTargets();
    bool CreateShaders();
    bool CreateVertexBuffer();
    void CleanupRenderTargets();

    // Rendering
    void SetupRenderState();
    void RenderQuad(ID3D11ShaderResourceView* texture);

    // Window state
    HWND hwnd_;
    HINSTANCE hInstance_;
    std::wstring windowTitle_;
    bool isFullscreen_;
    bool isVisible_;

    // Saved window state for fullscreen transitions
    RECT windowedRect_;
    DWORD windowedStyle_;

    // DirectX resources
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
    
    // Rendering pipeline
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;

    // Window class registration
    static bool classRegistered_;
    static const wchar_t* WINDOW_CLASS_NAME;

    // Vertex structure for fullscreen quad
    struct Vertex {
        float position[3];
        float texCoord[2];
    };
};