#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>

/**
 * High-performance frame capture system
 * Handles GPU-to-GPU texture copying and frame queuing
 */
class FrameCapture {
public:
    struct CapturedFrame {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        DXGI_FORMAT format;
        UINT width;
        UINT height;
        UINT64 timestamp;
        bool valid;

        CapturedFrame() : format(DXGI_FORMAT_UNKNOWN), width(0), height(0), 
                         timestamp(0), valid(false) {}
    };

    FrameCapture();
    ~FrameCapture();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Frame capture from swap chain
    bool CaptureFrame(IDXGISwapChain* swapChain);
    
    // Get latest captured frame (thread-safe)
    std::shared_ptr<CapturedFrame> GetLatestFrame();
    
    // Handle resolution changes
    void OnResolutionChanged(UINT width, UINT height, DXGI_FORMAT format);

    // Statistics
    struct Stats {
        UINT64 totalFramesCaptured;
        UINT64 droppedFrames;
        double averageFPS;
        double captureLatencyMS;
    };

    const Stats& GetStats() const { return stats_; }

private:
    bool CreateSharedTexture(UINT width, UINT height, DXGI_FORMAT format);
    bool CopyBackbuffer(IDXGISwapChain* swapChain);
    void UpdateStats();

    // DirectX resources
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> sharedTexture_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTexture_;

    // Frame management
    static constexpr size_t MAX_QUEUED_FRAMES = 3;
    std::vector<std::shared_ptr<CapturedFrame>> frameQueue_;
    size_t currentFrameIndex_;
    
    // Synchronization
    std::mutex captureMutex_;
    std::condition_variable captureCondition_;
    
    // Current capture state
    UINT currentWidth_;
    UINT currentHeight_;
    DXGI_FORMAT currentFormat_;
    bool initialized_;

    // Performance tracking
    Stats stats_;
    LARGE_INTEGER performanceFrequency_;
    LARGE_INTEGER lastFrameTime_;
    std::vector<double> frameTimes_;
};