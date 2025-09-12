#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

/**
 * Frame data shared between hook and projector
 */
struct SharedFrameData {
    UINT width;
    UINT height;
    DXGI_FORMAT format;
    UINT64 timestamp;
    UINT64 frameNumber;
    bool valid;
};

/**
 * Manages shared GPU texture for frame data transfer
 */
class SharedTexture {
public:
    SharedTexture();
    ~SharedTexture();

    // Initialize as producer (hook side)
    bool InitializeProducer(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format);
    
    // Initialize as consumer (projector side) 
    bool InitializeConsumer(ID3D11Device* device);
    
    // Producer: Update shared texture with new frame
    bool UpdateFrame(ID3D11DeviceContext* context, ID3D11Texture2D* sourceTexture);
    
    // Consumer: Get latest frame from shared texture
    bool GetLatestFrame(ID3D11DeviceContext* context, ID3D11Texture2D** outTexture, SharedFrameData* frameData);
    
    // Get shared texture handle for cross-process sharing
    HANDLE GetSharedHandle() const { return m_sharedHandle; }
    
    // Set shared handle (consumer side)
    void SetSharedHandle(HANDLE handle) { m_sharedHandle = handle; }
    
    // Get frame data
    const SharedFrameData& GetFrameData() const { return m_frameData; }
    
    // Cleanup
    void Cleanup();

private:
    // Create synchronization objects
    bool CreateSynchronization();
    
    // Wait for frame availability (consumer)
    bool WaitForFrame(DWORD timeoutMs = 16); // ~60 FPS timeout
    
    // Signal frame ready (producer)
    void SignalFrameReady();

    // D3D11 objects
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11Texture2D> m_sharedTexture;
    ComPtr<IDXGIKeyedMutex> m_keyedMutex;
    
    // Synchronization
    HANDLE m_frameEvent;
    HANDLE m_sharedHandle;
    
    // Frame information
    SharedFrameData m_frameData;
    
    // State
    bool m_isProducer;
    bool m_initialized;
    UINT64 m_frameCounter;
    
    // Sync keys for keyed mutex
    static const UINT MUTEX_KEY_PRODUCER = 0;
    static const UINT MUTEX_KEY_CONSUMER = 1;
};