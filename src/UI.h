#pragma once

#include <windows.h>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <memory>

#include "FiveMCapture.h"

/**
 * ImGui-based user interface for the capture system
 */
class UI {
public:
    UI();
    ~UI();

    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Frame rendering
    void NewFrame();
    void Render();
    void Present();

    // UI state
    void SetVisible(bool visible) { visible_ = visible; }
    bool IsVisible() const { return visible_; }

    // Window message handling for ImGui
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void RenderMainWindow();
    void RenderCaptureControls();
    void RenderProjectionControls();
    void RenderPerformanceStats();
    void RenderAbout();

    // UI state
    bool visible_;
    bool showDemo_;
    bool showAbout_;
    
    // Control values
    bool captureEnabled_;
    bool fullscreenProjection_;
    int targetFPS_;
    
    // Performance display
    std::vector<float> fpsHistory_;
    float averageFPS_;
    
    // Forward declaration to avoid circular dependency
    class FiveMCapture* app_;

    // ImGui context
    ImGuiContext* imguiContext_;
    bool initialized_;
};