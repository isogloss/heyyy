#include "FrameCapture.h"
#include <iostream>
#include <algorithm>
#include <chrono>

using namespace Microsoft::WRL;

FrameCapture::FrameCapture() 
    : currentFrameIndex_(0), currentWidth_(0), currentHeight_(0), 
      currentFormat_(DXGI_FORMAT_UNKNOWN), initialized_(false) {
    
    // Initialize performance counter
    QueryPerformanceFrequency(&performanceFrequency_);
    QueryPerformanceCounter(&lastFrameTime_);
    
    // Initialize stats
    memset(&stats_, 0, sizeof(stats_));
    
    // Pre-allocate frame queue
    frameQueue_.resize(MAX_QUEUED_FRAMES);
    for (size_t i = 0; i < MAX_QUEUED_FRAMES; ++i) {
        frameQueue_[i] = std::make_shared<CapturedFrame>();
    }
}

FrameCapture::~FrameCapture() {
    Shutdown();
}

bool FrameCapture::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    if (initialized_) {
        return true;
    }

    if (!device || !context) {
        std::wcerr << L"Invalid device or context passed to FrameCapture::Initialize" << std::endl;
        return false;
    }

    device_ = device;
    context_ = context;

    std::wcout << L"FrameCapture initialized successfully" << std::endl;
    initialized_ = true;
    return true;
}

void FrameCapture::Shutdown() {
    std::lock_guard<std::mutex> lock(captureMutex_);
    
    initialized_ = false;
    
    // Clear frame queue
    for (auto& frame : frameQueue_) {
        if (frame) {
            frame->texture.Reset();
            frame->valid = false;
        }
    }
    
    sharedTexture_.Reset();
    stagingTexture_.Reset();
    context_.Reset();
    device_.Reset();
    
    std::wcout << L"FrameCapture shutdown complete" << std::endl;
}

bool FrameCapture::CaptureFrame(IDXGISwapChain* swapChain) {
    if (!initialized_ || !swapChain) {
        return false;
    }

    std::lock_guard<std::mutex> lock(captureMutex_);
    
    try {
        // Get the current frame from queue
        auto& currentFrame = frameQueue_[currentFrameIndex_];
        
        // Copy backbuffer to our texture
        if (CopyBackbuffer(swapChain)) {
            // Update frame info
            currentFrame->width = currentWidth_;
            currentFrame->height = currentHeight_;
            currentFrame->format = currentFormat_;
            currentFrame->valid = true;
            
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            currentFrame->timestamp = currentTime.QuadPart;
            
            // Move to next frame
            currentFrameIndex_ = (currentFrameIndex_ + 1) % MAX_QUEUED_FRAMES;
            
            // Update stats
            stats_.totalFramesCaptured++;
            UpdateStats();
            
            return true;
        }
        else {
            stats_.droppedFrames++;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in CaptureFrame: " << e.what() << std::endl;
        stats_.droppedFrames++;
        return false;
    }
    catch (...) {
        std::wcerr << L"Unknown exception in CaptureFrame" << std::endl;
        stats_.droppedFrames++;
        return false;
    }
}

std::shared_ptr<FrameCapture::CapturedFrame> FrameCapture::GetLatestFrame() {
    std::lock_guard<std::mutex> lock(captureMutex_);
    
    if (!initialized_ || frameQueue_.empty()) {
        return nullptr;
    }

    // Find the most recent valid frame
    size_t latestIndex = (currentFrameIndex_ + MAX_QUEUED_FRAMES - 1) % MAX_QUEUED_FRAMES;
    auto& latestFrame = frameQueue_[latestIndex];
    
    if (latestFrame && latestFrame->valid) {
        return latestFrame;
    }
    
    return nullptr;
}

void FrameCapture::OnResolutionChanged(UINT width, UINT height, DXGI_FORMAT format) {
    std::lock_guard<std::mutex> lock(captureMutex_);
    
    if (width == currentWidth_ && height == currentHeight_ && format == currentFormat_) {
        return; // No change
    }

    std::wcout << L"Resolution changed: " << width << L"x" << height 
              << L", Format: " << format << std::endl;

    currentWidth_ = width;
    currentHeight_ = height;
    currentFormat_ = format;

    // Recreate shared textures with new dimensions
    CreateSharedTexture(width, height, format);
}

bool FrameCapture::CreateSharedTexture(UINT width, UINT height, DXGI_FORMAT format) {
    if (!device_ || width == 0 || height == 0) {
        return false;
    }

    // Release existing textures
    sharedTexture_.Reset();
    stagingTexture_.Reset();

    // Create shared texture for GPU-to-GPU transfer
    D3D11_TEXTURE2D_DESC sharedDesc = {};
    sharedDesc.Width = width;
    sharedDesc.Height = height;
    sharedDesc.MipLevels = 1;
    sharedDesc.ArraySize = 1;
    sharedDesc.Format = format;
    sharedDesc.SampleDesc.Count = 1;
    sharedDesc.Usage = D3D11_USAGE_DEFAULT;
    sharedDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    sharedDesc.CPUAccessFlags = 0;
    sharedDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    HRESULT hr = device_->CreateTexture2D(&sharedDesc, nullptr, &sharedTexture_);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create shared texture. HR: 0x" << std::hex << hr << std::endl;
        return false;
    }

    // Create staging texture for CPU access (if needed for debugging)
    D3D11_TEXTURE2D_DESC stagingDesc = sharedDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    hr = device_->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture_);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create staging texture. HR: 0x" << std::hex << hr << std::endl;
        // Staging texture is optional, so don't fail
    }

    std::wcout << L"Created shared textures: " << width << L"x" << height << std::endl;
    return true;
}

bool FrameCapture::CopyBackbuffer(IDXGISwapChain* swapChain) {
    if (!swapChain || !context_) {
        return false;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) {
        std::wcerr << L"Failed to get swap chain back buffer. HR: 0x" << std::hex << hr << std::endl;
        return false;
    }

    // Get backbuffer description
    D3D11_TEXTURE2D_DESC backBufferDesc;
    backBuffer->GetDesc(&backBufferDesc);

    // Check if we need to recreate our texture
    if (backBufferDesc.Width != currentWidth_ || 
        backBufferDesc.Height != currentHeight_ || 
        backBufferDesc.Format != currentFormat_ ||
        !sharedTexture_) {
        
        OnResolutionChanged(backBufferDesc.Width, backBufferDesc.Height, backBufferDesc.Format);
    }

    if (!sharedTexture_) {
        return false;
    }

    // Copy backbuffer to our shared texture
    context_->CopyResource(sharedTexture_.Get(), backBuffer.Get());

    // Update current frame's texture reference
    auto& currentFrame = frameQueue_[currentFrameIndex_];
    currentFrame->texture = sharedTexture_;

    return true;
}

void FrameCapture::UpdateStats() {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    // Calculate frame time
    double frameTimeMS = (double)(currentTime.QuadPart - lastFrameTime_.QuadPart) * 1000.0 / performanceFrequency_.QuadPart;
    frameTimes_.push_back(frameTimeMS);
    
    // Keep only last 60 frame times for moving average
    if (frameTimes_.size() > 60) {
        frameTimes_.erase(frameTimes_.begin());
    }

    // Calculate average FPS
    if (!frameTimes_.empty()) {
        double averageFrameTime = 0.0;
        for (double time : frameTimes_) {
            averageFrameTime += time;
        }
        averageFrameTime /= frameTimes_.size();
        
        stats_.averageFPS = averageFrameTime > 0.0 ? 1000.0 / averageFrameTime : 0.0;
        stats_.captureLatencyMS = averageFrameTime;
    }

    lastFrameTime_ = currentTime;
}