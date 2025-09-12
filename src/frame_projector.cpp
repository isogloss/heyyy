#include "frame_projector.h"
#include "monitor_manager.h"
#include <d3dcompiler.h>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

const wchar_t* const FrameProjector::PROJECTION_WINDOW_CLASS = L"Pick6ProjectionWindow";

// Simple vertex shader for fullscreen quad
static const char* g_vertexShaderSource = R"(
struct VS_INPUT {
    float2 pos : POSITION;
    float2 tex : TEXCOORD0;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.tex = input.tex;
    return output;
}
)";

// Simple pixel shader for texture display
static const char* g_pixelShaderSource = R"(
Texture2D frameTexture : register(t0);
SamplerState frameSampler : register(s0);

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_Target {
    return frameTexture.Sample(frameSampler, input.tex);
}
)";

// Vertex structure
struct Vertex {
    float x, y;    // Position
    float u, v;    // Texture coordinates
};

FrameProjector::FrameProjector()
    : m_hwnd(nullptr)
    , m_renderThread(nullptr)
    , m_stopEvent(nullptr)
    , m_projecting(false)
    , m_vsyncEnabled(true)
    , m_monitorIndex(0)
    , m_lastStatsUpdate(0)
    , m_frameCount(0)
    , m_lastFrameTime(0)
{
    m_stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    ZeroMemory(&m_statistics, sizeof(m_statistics));
}

FrameProjector::~FrameProjector() {
    StopProjection();
    
    if (m_stopEvent) {
        CloseHandle(m_stopEvent);
    }
}

bool FrameProjector::StartProjection(int monitorIndex) {
    if (m_projecting) {
        return true;
    }

    m_monitorIndex = monitorIndex;

    // Initialize Direct3D
    if (!InitializeD3D11()) {
        return false;
    }

    // Create projection window
    if (!CreateProjectionWindow(monitorIndex)) {
        return false;
    }

    // Create swap chain
    if (!CreateSwapChain()) {
        return false;
    }

    // Create render pipeline
    if (!CreateRenderPipeline()) {
        return false;
    }

    // Initialize shared texture for receiving frames
    m_sharedTexture = std::make_unique<SharedTexture>();
    if (!m_sharedTexture->InitializeConsumer(m_device.Get())) {
        return false;
    }

    // Start render thread
    ResetEvent(m_stopEvent);
    m_renderThread = CreateThread(nullptr, 0, RenderThreadProc, this, 0, nullptr);
    if (!m_renderThread) {
        return false;
    }

    m_projecting = true;
    m_statistics.projecting = true;
    return true;
}

void FrameProjector::StopProjection() {
    if (m_projecting) {
        // Signal stop
        if (m_stopEvent) {
            SetEvent(m_stopEvent);
        }

        // Wait for render thread
        if (m_renderThread) {
            WaitForSingleObject(m_renderThread, 5000);
            CloseHandle(m_renderThread);
            m_renderThread = nullptr;
        }

        // Cleanup
        m_sharedTexture.reset();
        m_currentFrameSRV.Reset();
        m_samplerState.Reset();
        m_vertexBuffer.Reset();
        m_inputLayout.Reset();
        m_pixelShader.Reset();
        m_vertexShader.Reset();
        m_renderTargetView.Reset();
        m_swapChain.Reset();
        m_context.Reset();
        m_device.Reset();

        if (m_hwnd) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        m_projecting = false;
        m_statistics.projecting = false;
    }
}

bool FrameProjector::InitializeD3D11() {
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // Adapter
        D3D_DRIVER_TYPE_HARDWARE,   // Driver type
        nullptr,                    // Software rasterizer
        createDeviceFlags,          // Flags
        featureLevels,              // Feature levels
        ARRAYSIZE(featureLevels),   // Num feature levels
        D3D11_SDK_VERSION,          // SDK version
        m_device.GetAddressOf(),    // Device
        nullptr,                    // Feature level out
        m_context.GetAddressOf()    // Context
    );

    return SUCCEEDED(hr);
}

bool FrameProjector::CreateProjectionWindow(int monitorIndex) {
    // Register window class
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = GetModuleHandle(nullptr);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wcex.lpszClassName = PROJECTION_WINDOW_CLASS;

    RegisterClassExW(&wcex);

    // Get monitor information
    MonitorManager monitorManager;
    monitorManager.Initialize();
    const MonitorInfo* monitor = monitorManager.GetMonitor(monitorIndex);
    
    int x = 0, y = 0, width = 1920, height = 1080;
    if (monitor) {
        x = monitor->x;
        y = monitor->y;
        width = monitor->width;
        height = monitor->height;
    }

    // Create fullscreen borderless window
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        PROJECTION_WINDOW_CLASS,
        L"pick6",
        WS_POPUP,
        x, y, width, height,
        nullptr, nullptr, GetModuleHandle(nullptr), this
    );

    if (!m_hwnd) {
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    return true;
}

bool FrameProjector::CreateSwapChain() {
    // Get window size
    RECT rect;
    GetClientRect(m_hwnd, &rect);
    UINT width = rect.right - rect.left;
    UINT height = rect.bottom - rect.top;

    // Create DXGI factory
    ComPtr<IDXGIFactory> factory;
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(factory.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferCount = 2;
    desc.BufferDesc.Width = width;
    desc.BufferDesc.Height = height;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = m_hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE; // Even for fullscreen, we use windowed mode
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = factory->CreateSwapChain(m_device.Get(), &desc, m_swapChain.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Create render target view
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf());
    return SUCCEEDED(hr);
}

bool FrameProjector::CreateRenderPipeline() {
    HRESULT hr;

    // Compile vertex shader
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> errorBlob;
    hr = D3DCompile(g_vertexShaderSource, strlen(g_vertexShaderSource), nullptr, nullptr, nullptr,
        "main", "vs_5_0", 0, 0, vsBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        nullptr, m_vertexShader.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Compile pixel shader
    ComPtr<ID3DBlob> psBlob;
    hr = D3DCompile(g_pixelShaderSource, strlen(g_pixelShaderSource), nullptr, nullptr, nullptr,
        "main", "ps_5_0", 0, 0, psBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
        nullptr, m_pixelShader.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = m_device->CreateInputLayout(inputElements, ARRAYSIZE(inputElements),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_inputLayout.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Create fullscreen quad vertices
    Vertex vertices[] = {
        { -1.0f, -1.0f, 0.0f, 1.0f },  // Bottom-left
        { -1.0f,  1.0f, 0.0f, 0.0f },  // Top-left
        {  1.0f, -1.0f, 1.0f, 1.0f },  // Bottom-right
        {  1.0f,  1.0f, 1.0f, 0.0f }   // Top-right
    };

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    hr = m_device->CreateBuffer(&bufferDesc, &initData, m_vertexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Create sampler state
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = m_device->CreateSamplerState(&samplerDesc, m_samplerState.GetAddressOf());
    return SUCCEEDED(hr);
}

void FrameProjector::RenderFrame() {
    if (!m_context || !m_renderTargetView) {
        return;
    }

    // Clear render target
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

    // Try to get latest frame from shared texture
    ComPtr<ID3D11Texture2D> frameTexture;
    SharedFrameData frameData;
    if (m_sharedTexture && m_sharedTexture->GetLatestFrame(m_context.Get(), frameTexture.GetAddressOf(), &frameData)) {
        // Create shader resource view for the frame
        ComPtr<ID3D11ShaderResourceView> frameSRV;
        HRESULT hr = m_device->CreateShaderResourceView(frameTexture.Get(), nullptr, frameSRV.GetAddressOf());
        if (SUCCEEDED(hr)) {
            m_currentFrameSRV = frameSRV;
            m_statistics.framesReceived++;
        }
    }

    // If we have a frame to display, render it
    if (m_currentFrameSRV) {
        // Set viewport
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(rect.right - rect.left);
        viewport.Height = static_cast<float>(rect.bottom - rect.top);
        viewport.MaxDepth = 1.0f;

        m_context->RSSetViewports(1, &viewport);

        // Set render target
        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

        // Set shaders and resources
        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        m_context->PSSetShaderResources(0, 1, m_currentFrameSRV.GetAddressOf());
        m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

        // Set vertex buffer and input layout
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetInputLayout(m_inputLayout.Get());
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        // Draw fullscreen quad
        m_context->Draw(4, 0);

        m_statistics.framesRendered++;
    }

    // Present
    UINT presentFlags = 0;
    UINT syncInterval = m_vsyncEnabled ? 1 : 0;
    m_swapChain->Present(syncInterval, presentFlags);

    m_frameCount++;
}

LRESULT CALLBACK FrameProjector::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    FrameProjector* projector = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        projector = static_cast<FrameProjector*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(projector));
    } else {
        projector = reinterpret_cast<FrameProjector*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (projector) {
        return projector->HandleWindowMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT FrameProjector::HandleWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            // Allow ESC to close projection window
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
        }
        break;

    case WM_CLOSE:
        // Don't close immediately, let the application handle it
        return 0;

    case WM_DESTROY:
        break;

    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

DWORD WINAPI FrameProjector::RenderThreadProc(LPVOID lpParameter) {
    FrameProjector* projector = static_cast<FrameProjector*>(lpParameter);
    projector->RunRenderLoop();
    return 0;
}

void FrameProjector::RunRenderLoop() {
    DWORD lastFrameTime = GetTickCount();

    while (WaitForSingleObject(m_stopEvent, 0) == WAIT_TIMEOUT) {
        DWORD currentTime = GetTickCount();
        
        // Render frame
        RenderFrame();

        // Update statistics
        UpdateStatistics();

        // Target ~60 FPS if VSync is disabled
        if (!m_vsyncEnabled) {
            DWORD frameTime = currentTime - lastFrameTime;
            if (frameTime < 16) { // 60 FPS = ~16ms per frame
                Sleep(16 - frameTime);
            }
        }

        lastFrameTime = currentTime;
    }
}

void FrameProjector::UpdateStatistics() {
    DWORD currentTime = GetTickCount();
    
    if (currentTime - m_lastStatsUpdate >= 1000) { // Update every second
        m_statistics.fps = m_frameCount * 1000.0f / (currentTime - m_lastStatsUpdate);
        m_frameCount = 0;
        m_lastStatsUpdate = currentTime;
    }

    // Update latency (simplified - would need proper timestamp tracking)
    m_statistics.latencyMs = 16; // Placeholder
}