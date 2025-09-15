#include "ProjectionWindow.h"
#include <iostream>
#include <vector>

using namespace Microsoft::WRL;

// Static members
bool ProjectionWindow::classRegistered_ = false;
const wchar_t* ProjectionWindow::WINDOW_CLASS_NAME = L"FiveMCaptureProjectionWindow";

// Vertex shader source
const char* g_vertexShaderSource = R"(
struct VS_INPUT {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.position = float4(input.position, 1.0);
    output.texCoord = input.texCoord;
    return output;
}
)";

// Pixel shader source
const char* g_pixelShaderSource = R"(
Texture2D gameTexture : register(t0);
SamplerState textureSampler : register(s0);

struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET {
    return gameTexture.Sample(textureSampler, input.texCoord);
}
)";

ProjectionWindow::ProjectionWindow() 
    : hwnd_(nullptr), hInstance_(nullptr), isFullscreen_(false), isVisible_(false),
      windowTitle_(L"FiveM Game Capture"), windowedStyle_(0) {
    memset(&windowedRect_, 0, sizeof(windowedRect_));
}

ProjectionWindow::~ProjectionWindow() {
    Shutdown();
}

bool ProjectionWindow::Initialize(HINSTANCE hInstance, ID3D11Device* device, ID3D11DeviceContext* context) {
    hInstance_ = hInstance;
    device_ = device;
    context_ = context;

    if (!device_ || !context_) {
        std::wcerr << L"Invalid device or context passed to ProjectionWindow" << std::endl;
        return false;
    }

    std::wcout << L"Initializing projection window..." << std::endl;

    if (!CreateProjectionWindow()) {
        std::wcerr << L"Failed to create projection window" << std::endl;
        return false;
    }

    if (!CreateSwapChain()) {
        std::wcerr << L"Failed to create swap chain for projection window" << std::endl;
        return false;
    }

    if (!CreateRenderTargets()) {
        std::wcerr << L"Failed to create render targets" << std::endl;
        return false;
    }

    if (!CreateShaders()) {
        std::wcerr << L"Failed to create shaders" << std::endl;
        return false;
    }

    if (!CreateVertexBuffer()) {
        std::wcerr << L"Failed to create vertex buffer" << std::endl;
        return false;
    }

    std::wcout << L"Projection window initialized successfully" << std::endl;
    return true;
}

void ProjectionWindow::Shutdown() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    CleanupRenderTargets();
    
    vertexBuffer_.Reset();
    inputLayout_.Reset();
    pixelShader_.Reset();
    vertexShader_.Reset();
    samplerState_.Reset();
    blendState_.Reset();
    rasterizerState_.Reset();
    swapChain_.Reset();
    context_.Reset();
    device_.Reset();

    std::wcout << L"Projection window shutdown complete" << std::endl;
}

bool ProjectionWindow::CreateProjectionWindow() {
    // Register window class if not already done
    if (!classRegistered_) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance_;
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = WINDOW_CLASS_NAME;

        if (!RegisterClassExW(&wc)) {
            std::wcerr << L"Failed to register projection window class" << std::endl;
            return false;
        }
        classRegistered_ = true;
    }

    // Create window
    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        WINDOW_CLASS_NAME,
        windowTitle_.c_str(),
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance_, this);

    if (!hwnd_) {
        std::wcerr << L"Failed to create projection window" << std::endl;
        return false;
    }

    // Make window semi-transparent initially
    SetLayeredWindowAttributes(hwnd_, 0, 230, LWA_ALPHA);

    return true;
}

bool ProjectionWindow::CreateSwapChain() {
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ComPtr<IDXGIFactory> dxgiFactory;

    HRESULT hr = device_->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
    if (FAILED(hr)) return false;

    hr = dxgiDevice->GetParent(IID_PPV_ARGS(&dxgiAdapter));
    if (FAILED(hr)) return false;

    hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) return false;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd_;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    hr = dxgiFactory->CreateSwapChain(device_.Get(), &swapChainDesc, &swapChain_);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create projection swap chain. HR: 0x" << std::hex << hr << std::endl;
        return false;
    }

    return true;
}

bool ProjectionWindow::CreateRenderTargets() {
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) return false;

    hr = device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView_);
    if (FAILED(hr)) return false;

    return true;
}

bool ProjectionWindow::CreateShaders() {
    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    // Compile vertex shader
    HRESULT hr = D3DCompile(g_vertexShaderSource, strlen(g_vertexShaderSource),
                           "VertexShader", nullptr, nullptr, "main", "vs_5_0",
                           0, 0, &vsBlob, &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            std::wcerr << L"Vertex shader compilation error: " 
                      << (char*)errorBlob->GetBufferPointer() << std::endl;
        }
        return false;
    }

    hr = device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                    nullptr, &vertexShader_);
    if (FAILED(hr)) return false;

    // Compile pixel shader
    hr = D3DCompile(g_pixelShaderSource, strlen(g_pixelShaderSource),
                   "PixelShader", nullptr, nullptr, "main", "ps_5_0",
                   0, 0, &psBlob, &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            std::wcerr << L"Pixel shader compilation error: " 
                      << (char*)errorBlob->GetBufferPointer() << std::endl;
        }
        return false;
    }

    hr = device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                   nullptr, &pixelShader_);
    if (FAILED(hr)) return false;

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    hr = device_->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc),
                                   vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                   &inputLayout_);
    if (FAILED(hr)) return false;

    // Create sampler state
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    hr = device_->CreateSamplerState(&samplerDesc, &samplerState_);
    if (FAILED(hr)) return false;

    // Create blend state
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device_->CreateBlendState(&blendDesc, &blendState_);
    if (FAILED(hr)) return false;

    // Create rasterizer state
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;

    hr = device_->CreateRasterizerState(&rasterizerDesc, &rasterizerState_);
    if (FAILED(hr)) return false;

    return true;
}

bool ProjectionWindow::CreateVertexBuffer() {
    // Create fullscreen quad vertices
    Vertex vertices[] = {
        { {-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f} }, // Bottom-left
        { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f} }, // Top-left
        { { 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f} }, // Bottom-right
        { { 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f} }  // Top-right
    };

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pSysMem = vertices;

    HRESULT hr = device_->CreateBuffer(&bufferDesc, &subresourceData, &vertexBuffer_);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create vertex buffer" << std::endl;
        return false;
    }

    return true;
}

void ProjectionWindow::CleanupRenderTargets() {
    renderTargetView_.Reset();
}

void ProjectionWindow::ShowWindow(bool show) {
    if (hwnd_) {
        ::ShowWindow(hwnd_, show ? SW_SHOW : SW_HIDE);
        isVisible_ = show;
    }
}

void ProjectionWindow::SetFullscreen(bool fullscreen) {
    if (!hwnd_ || fullscreen == isFullscreen_) {
        return;
    }

    if (fullscreen) {
        // Save current window state
        GetWindowRect(hwnd_, &windowedRect_);
        windowedStyle_ = GetWindowLong(hwnd_, GWL_STYLE);

        // Set fullscreen style
        SetWindowLong(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);

        // Get primary monitor size
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY), &mi);

        // Set fullscreen position and size
        SetWindowPos(hwnd_, HWND_TOPMOST,
                    mi.rcMonitor.left, mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_FRAMECHANGED);
    } else {
        // Restore windowed mode
        SetWindowLong(hwnd_, GWL_STYLE, windowedStyle_);
        SetWindowPos(hwnd_, HWND_NOTOPMOST,
                    windowedRect_.left, windowedRect_.top,
                    windowedRect_.right - windowedRect_.left,
                    windowedRect_.bottom - windowedRect_.top,
                    SWP_FRAMECHANGED);
    }

    isFullscreen_ = fullscreen;

    // Resize swap chain buffers
    if (swapChain_) {
        CleanupRenderTargets();
        swapChain_->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTargets();
    }
}

void ProjectionWindow::SetTitle(const std::wstring& title) {
    windowTitle_ = title;
    if (hwnd_) {
        SetWindowTextW(hwnd_, title.c_str());
    }
}

void ProjectionWindow::RenderFrame(std::shared_ptr<FrameCapture::CapturedFrame> frame) {
    if (!frame || !frame->valid || !frame->texture || !context_ || !renderTargetView_) {
        return;
    }

    // Create shader resource view for the captured texture
    ComPtr<ID3D11ShaderResourceView> srv;
    HRESULT hr = device_->CreateShaderResourceView(frame->texture.Get(), nullptr, &srv);
    if (FAILED(hr)) {
        return;
    }

    SetupRenderState();
    RenderQuad(srv.Get());
}

void ProjectionWindow::Present() {
    if (swapChain_) {
        swapChain_->Present(1, 0); // VSync enabled
    }
}

void ProjectionWindow::SetupRenderState() {
    // Set viewport
    RECT rect;
    GetClientRect(hwnd_, &rect);
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)(rect.right - rect.left);
    viewport.Height = (float)(rect.bottom - rect.top);
    viewport.MaxDepth = 1.0f;

    context_->RSSetViewports(1, &viewport);
    context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), nullptr);

    // Clear render target
    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);

    // Set shaders and states
    context_->IASetInputLayout(inputLayout_.Get());
    context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
    context_->PSSetSamplers(0, 1, samplerState_.GetAddressOf());
    context_->OMSetBlendState(blendState_.Get(), nullptr, 0xFFFFFFFF);
    context_->RSSetState(rasterizerState_.Get());
}

void ProjectionWindow::RenderQuad(ID3D11ShaderResourceView* texture) {
    // Set texture
    context_->PSSetShaderResources(0, 1, &texture);

    // Set vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context_->IASetVertexBuffers(0, 1, vertexBuffer_.GetAddressOf(), &stride, &offset);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Draw fullscreen quad
    context_->Draw(4, 0);
}

LRESULT CALLBACK ProjectionWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ProjectionWindow* window = nullptr;
    
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<ProjectionWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<ProjectionWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->HandleMessage(msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT ProjectionWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            ShowWindow(false);
        } else if (wParam == VK_F11) {
            SetFullscreen(!isFullscreen_);
        }
        break;

    case WM_CLOSE:
        ShowWindow(false);
        return 0;

    case WM_SIZE:
        if (swapChain_ && wParam != SIZE_MINIMIZED) {
            CleanupRenderTargets();
            swapChain_->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTargets();
        }
        break;
    }

    return DefWindowProc(hwnd_, msg, wParam, lParam);
}