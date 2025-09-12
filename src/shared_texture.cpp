#include "shared_texture.h"
#include <dxgi1_2.h>

SharedTexture::SharedTexture()
    : m_frameEvent(nullptr)
    , m_sharedHandle(nullptr)
    , m_isProducer(false)
    , m_initialized(false)
    , m_frameCounter(0)
{
    ZeroMemory(&m_frameData, sizeof(m_frameData));
}

SharedTexture::~SharedTexture() {
    Cleanup();
}

bool SharedTexture::InitializeProducer(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format) {
    if (m_initialized) {
        return false;
    }

    m_device = device;
    m_isProducer = true;

    // Create shared texture
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

    HRESULT hr = device->CreateTexture2D(&desc, nullptr, m_sharedTexture.GetAddressOf());
    if (FAILED(hr)) {
        return false;
    }

    // Get keyed mutex
    hr = m_sharedTexture->QueryInterface(IID_PPV_ARGS(m_keyedMutex.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    // Get shared handle
    ComPtr<IDXGIResource> dxgiResource;
    hr = m_sharedTexture->QueryInterface(IID_PPV_ARGS(dxgiResource.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    hr = dxgiResource->GetSharedHandle(&m_sharedHandle);
    if (FAILED(hr)) {
        return false;
    }

    // Initialize frame data
    m_frameData.width = width;
    m_frameData.height = height;
    m_frameData.format = format;
    m_frameData.valid = false;
    m_frameData.frameNumber = 0;

    // Create synchronization objects
    if (!CreateSynchronization()) {
        return false;
    }

    m_initialized = true;
    return true;
}

bool SharedTexture::InitializeConsumer(ID3D11Device* device) {
    if (m_initialized || !m_sharedHandle) {
        return false;
    }

    m_device = device;
    m_isProducer = false;

    // Open shared texture
    ComPtr<ID3D11Resource> resource;
    HRESULT hr = device->OpenSharedResource(m_sharedHandle, IID_PPV_ARGS(resource.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    hr = resource->QueryInterface(IID_PPV_ARGS(m_sharedTexture.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    // Get keyed mutex
    hr = m_sharedTexture->QueryInterface(IID_PPV_ARGS(m_keyedMutex.GetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    // Get texture description for frame data
    D3D11_TEXTURE2D_DESC desc;
    m_sharedTexture->GetDesc(&desc);
    m_frameData.width = desc.Width;
    m_frameData.height = desc.Height;
    m_frameData.format = desc.Format;

    // Create synchronization objects
    if (!CreateSynchronization()) {
        return false;
    }

    m_initialized = true;
    return true;
}

bool SharedTexture::UpdateFrame(ID3D11DeviceContext* context, ID3D11Texture2D* sourceTexture) {
    if (!m_initialized || !m_isProducer || !sourceTexture) {
        return false;
    }

    // Acquire keyed mutex for producer
    HRESULT hr = m_keyedMutex->AcquireSync(MUTEX_KEY_PRODUCER, 10); // 10ms timeout
    if (FAILED(hr)) {
        return false;
    }

    // Copy source texture to shared texture
    context->CopyResource(m_sharedTexture.Get(), sourceTexture);

    // Update frame data
    m_frameData.timestamp = GetTickCount64();
    m_frameData.frameNumber = ++m_frameCounter;
    m_frameData.valid = true;

    // Release keyed mutex for consumer
    hr = m_keyedMutex->ReleaseSync(MUTEX_KEY_CONSUMER);
    if (FAILED(hr)) {
        return false;
    }

    // Signal frame ready
    SignalFrameReady();

    return true;
}

bool SharedTexture::GetLatestFrame(ID3D11DeviceContext* context, ID3D11Texture2D** outTexture, SharedFrameData* frameData) {
    if (!m_initialized || m_isProducer || !outTexture) {
        return false;
    }

    *outTexture = nullptr;

    // Wait for frame availability
    if (!WaitForFrame(16)) { // 16ms timeout for ~60 FPS
        return false;
    }

    // Acquire keyed mutex for consumer
    HRESULT hr = m_keyedMutex->AcquireSync(MUTEX_KEY_CONSUMER, 10);
    if (FAILED(hr)) {
        return false;
    }

    // Create staging texture if we need to read the data
    // For now, just return the shared texture directly
    *outTexture = m_sharedTexture.Get();
    m_sharedTexture->AddRef();

    if (frameData) {
        *frameData = m_frameData;
    }

    // Release keyed mutex for producer
    hr = m_keyedMutex->ReleaseSync(MUTEX_KEY_PRODUCER);
    if (FAILED(hr)) {
        (*outTexture)->Release();
        *outTexture = nullptr;
        return false;
    }

    return true;
}

bool SharedTexture::CreateSynchronization() {
    // Create named event for cross-process synchronization
    std::wstring eventName = L"Global\\Pick6FrameEvent_" + std::to_wstring(reinterpret_cast<uintptr_t>(m_sharedHandle));
    
    m_frameEvent = CreateEventW(nullptr, FALSE, FALSE, eventName.c_str());
    if (!m_frameEvent) {
        return false;
    }

    return true;
}

bool SharedTexture::WaitForFrame(DWORD timeoutMs) {
    if (!m_frameEvent) {
        return false;
    }

    DWORD result = WaitForSingleObject(m_frameEvent, timeoutMs);
    return result == WAIT_OBJECT_0;
}

void SharedTexture::SignalFrameReady() {
    if (m_frameEvent) {
        SetEvent(m_frameEvent);
    }
}

void SharedTexture::Cleanup() {
    m_keyedMutex.Reset();
    m_sharedTexture.Reset();
    m_device.Reset();

    if (m_frameEvent) {
        CloseHandle(m_frameEvent);
        m_frameEvent = nullptr;
    }

    m_sharedHandle = nullptr;
    m_initialized = false;
    m_frameCounter = 0;
    ZeroMemory(&m_frameData, sizeof(m_frameData));
}