#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include "shared_texture.h"

using Microsoft::WRL::ComPtr;

/**
 * Statistics for frame projector
 */
struct ProjectorStatistics {
    UINT64 framesReceived = 0;
    UINT64 framesRendered = 0;
    float fps = 0.0f;
    UINT32 latencyMs = 0;
    bool projecting = false;
};

/**
 * Projects frames from shared texture to fullscreen window
 */
class FrameProjector {
public:
    FrameProjector();
    ~FrameProjector();

    // Start projection on specified monitor
    bool StartProjection(int monitorIndex);
    
    // Stop projection
    void StopProjection();
    
    // Check if projecting
    bool IsProjecting() const { return m_projecting; }
    
    // Set VSync enabled/disabled
    void SetVSyncEnabled(bool enabled) { m_vsyncEnabled = enabled; }
    
    // Get statistics
    ProjectorStatistics GetStatistics() const { return m_statistics; }

private:
    // Initialize Direct3D
    bool InitializeD3D11();
    
    // Create fullscreen window
    bool CreateProjectionWindow(int monitorIndex);
    
    // Create swap chain
    bool CreateSwapChain();
    
    // Create render pipeline
    bool CreateRenderPipeline();
    
    // Render frame
    void RenderFrame();
    
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Rendering thread
    static DWORD WINAPI RenderThreadProc(LPVOID lpParameter);
    void RunRenderLoop();
    
    // Update statistics
    void UpdateStatistics();

    // D3D11 objects
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain> m_swapChain;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11SamplerState> m_samplerState;
    ComPtr<ID3D11ShaderResourceView> m_currentFrameSRV;

    // Window and threading
    HWND m_hwnd;
    HANDLE m_renderThread;
    HANDLE m_stopEvent;
    
    // Shared texture for receiving frames
    std::unique_ptr<SharedTexture> m_sharedTexture;
    
    // State
    bool m_projecting;
    bool m_vsyncEnabled;
    int m_monitorIndex;
    
    // Statistics
    mutable ProjectorStatistics m_statistics;
    DWORD m_lastStatsUpdate;
    UINT64 m_frameCount;
    DWORD m_lastFrameTime;
    
    // Window class name
    static const wchar_t* const PROJECTION_WINDOW_CLASS;
};