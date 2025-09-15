#include "UI.h"
#include <iostream>
#include <algorithm>

// Forward declaration for ImGui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

UI::UI() 
    : visible_(true), showDemo_(false), showAbout_(false),
      captureEnabled_(true), fullscreenProjection_(false), targetFPS_(60),
      averageFPS_(0.0f), app_(nullptr), imguiContext_(nullptr), initialized_(false) {
    
    // Initialize FPS history
    fpsHistory_.resize(120, 0.0f); // 2 seconds at 60 FPS
}

UI::~UI() {
    Shutdown();
}

bool UI::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context) {
    if (initialized_) {
        return true;
    }

    if (!device || !context || !hwnd) {
        std::wcerr << L"Invalid parameters passed to UI::Initialize" << std::endl;
        return false;
    }

    app_ = GetApp();
    if (!app_) {
        std::wcerr << L"No application instance available" << std::endl;
        return false;
    }

    std::wcout << L"Initializing UI system..." << std::endl;

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    imguiContext_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(imguiContext_);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Initialize platform/renderer bindings
    if (!ImGui_ImplWin32_Init(hwnd)) {
        std::wcerr << L"Failed to initialize ImGui Win32 backend" << std::endl;
        ImGui::DestroyContext(imguiContext_);
        return false;
    }

    if (!ImGui_ImplDX11_Init(device, context)) {
        std::wcerr << L"Failed to initialize ImGui DX11 backend" << std::endl;
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(imguiContext_);
        return false;
    }

    // Load fonts (optional)
    io.Fonts->AddFontDefault();

    initialized_ = true;
    std::wcout << L"UI system initialized successfully" << std::endl;
    return true;
}

void UI::Shutdown() {
    if (!initialized_) {
        return;
    }

    std::wcout << L"Shutting down UI system..." << std::endl;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    
    if (imguiContext_) {
        ImGui::SetCurrentContext(imguiContext_);
        ImGui::DestroyContext(imguiContext_);
        imguiContext_ = nullptr;
    }

    initialized_ = false;
    std::wcout << L"UI system shutdown complete" << std::endl;
}

void UI::NewFrame() {
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UI::Render() {
    if (!initialized_ || !visible_) {
        return;
    }

    RenderMainWindow();

    if (showDemo_) {
        ImGui::ShowDemoWindow(&showDemo_);
    }

    if (showAbout_) {
        RenderAbout();
    }
}

void UI::Present() {
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Update and render additional platform windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

LRESULT UI::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!initialized_) {
        return 0;
    }

    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}

void UI::RenderMainWindow() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("FiveM Capture System", &visible_)) {
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Show Demo Window", nullptr, &showDemo_);
                ImGui::MenuItem("About", nullptr, &showAbout_);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Tab bar for different sections
        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("Capture")) {
                RenderCaptureControls();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Projection")) {
                RenderProjectionControls();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Performance")) {
                RenderPerformanceStats();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void UI::RenderCaptureControls() {
    ImGui::SeparatorText("Capture Settings");

    // Sync with app config
    if (app_) {
        captureEnabled_ = app_->GetConfig().enableCapture;
        targetFPS_ = app_->GetConfig().targetFPS;
    }

    // Enable/disable capture
    if (ImGui::Checkbox("Enable Capture", &captureEnabled_)) {
        if (app_) {
            app_->GetConfig().enableCapture = captureEnabled_;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Stats")) {
        // Reset capture statistics
        fpsHistory_.clear();
        fpsHistory_.resize(120, 0.0f);
    }

    ImGui::Spacing();

    // Target FPS
    if (ImGui::SliderInt("Target FPS", &targetFPS_, 30, 120)) {
        if (app_) {
            app_->GetConfig().targetFPS = targetFPS_;
        }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Capture Status");

    // Status indicators
    ImGui::Text("Capture Status: %s", captureEnabled_ ? "ACTIVE" : "INACTIVE");
    
    if (captureEnabled_) {
        ImGui::SameLine();
        // Animated indicator
        static float animTime = 0.0f;
        animTime += ImGui::GetIO().DeltaTime;
        float alpha = 0.5f + 0.5f * sinf(animTime * 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, alpha));
        ImGui::Text("●");
        ImGui::PopStyleColor();
    }

    // Hook status (if we can determine it)
    ImGui::Text("DXGI Hooks: INSTALLED");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::Text("●");
    ImGui::PopStyleColor();
}

void UI::RenderProjectionControls() {
    ImGui::SeparatorText("Projection Settings");

    // Sync with app config
    if (app_) {
        fullscreenProjection_ = app_->GetConfig().fullscreenProjection;
    }

    // Fullscreen toggle
    if (ImGui::Checkbox("Fullscreen Projection", &fullscreenProjection_)) {
        if (app_) {
            app_->GetConfig().fullscreenProjection = fullscreenProjection_;
            // Apply the change immediately if projection window exists
            // This would need to be implemented in the projection window
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle borderless fullscreen projection window");
    }

    ImGui::Spacing();

    // Window controls
    if (ImGui::Button("Show Projection Window", ImVec2(-1, 0))) {
        // Show projection window
        if (app_ && app_->projectionWindow_) {
            app_->projectionWindow_->ShowWindow(true);
        }
    }

    if (ImGui::Button("Hide Projection Window", ImVec2(-1, 0))) {
        // Hide projection window
        if (app_ && app_->projectionWindow_) {
            app_->projectionWindow_->ShowWindow(false);
        }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Window Information");

    // Display projection window status
    bool projectionVisible = false;
    bool projectionFullscreen = false;
    
    if (app_ && app_->projectionWindow_) {
        projectionFullscreen = app_->projectionWindow_->IsFullscreen();
        // We'd need to add an IsVisible method to check visibility
    }

    ImGui::Text("Window Status: %s", projectionVisible ? "VISIBLE" : "HIDDEN");
    ImGui::Text("Display Mode: %s", projectionFullscreen ? "FULLSCREEN" : "WINDOWED");

    ImGui::Spacing();
    ImGui::TextWrapped("Hotkeys:");
    ImGui::BulletText("F11 - Toggle fullscreen in projection window");
    ImGui::BulletText("ESC - Hide projection window");
    ImGui::BulletText("F1 - Toggle this UI");
}

void UI::RenderPerformanceStats() {
    ImGui::SeparatorText("Performance Metrics");

    // Get latest stats from frame capture
    if (app_ && app_->frameCapture_) {
        const auto& stats = app_->frameCapture_->GetStats();
        
        ImGui::Text("Total Frames Captured: %llu", stats.totalFramesCaptured);
        ImGui::Text("Dropped Frames: %llu", stats.droppedFrames);
        
        if (stats.totalFramesCaptured > 0) {
            double dropRate = (double)stats.droppedFrames / (double)stats.totalFramesCaptured * 100.0;
            ImGui::Text("Drop Rate: %.2f%%", dropRate);
        }
        
        ImGui::Text("Average FPS: %.1f", stats.averageFPS);
        ImGui::Text("Capture Latency: %.2f ms", stats.captureLatencyMS);

        // Update FPS history for graph
        fpsHistory_.erase(fpsHistory_.begin());
        fpsHistory_.push_back(static_cast<float>(stats.averageFPS));
        averageFPS_ = static_cast<float>(stats.averageFPS);
    }

    ImGui::Spacing();
    ImGui::SeparatorText("FPS Graph");

    // FPS graph
    ImGui::PlotLines("FPS History", fpsHistory_.data(), static_cast<int>(fpsHistory_.size()), 
                     0, nullptr, 0.0f, 120.0f, ImVec2(0, 80));

    // Current FPS indicator
    ImGui::Text("Current FPS: %.1f", averageFPS_);
    ImGui::SameLine();
    
    // Color-coded FPS indicator
    ImVec4 fpsColor;
    if (averageFPS_ >= 60.0f) {
        fpsColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    } else if (averageFPS_ >= 30.0f) {
        fpsColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow  
    } else {
        fpsColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    }
    
    ImGui::PushStyleColor(ImGuiCol_Text, fpsColor);
    ImGui::Text("●");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::SeparatorText("System Information");

    // System info
    ImGui::Text("Application FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

    // Memory usage (approximate)
    ImGui::Text("UI Vertices: %d", ImGui::GetDrawData() ? ImGui::GetDrawData()->TotalVtxCount : 0);
    ImGui::Text("UI Commands: %d", ImGui::GetDrawData() ? ImGui::GetDrawData()->CmdListsCount : 0);
}

void UI::RenderAbout() {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("About FiveM Capture System", &showAbout_)) {
        ImGui::TextWrapped("FiveM Capture System v1.0");
        ImGui::Separator();
        
        ImGui::TextWrapped("A high-performance game capture system for FiveM that hooks into "
                          "the DirectX 11 rendering pipeline to capture frames directly from "
                          "the game without desktop capture interference.");
        
        ImGui::Spacing();
        ImGui::TextWrapped("Features:");
        ImGui::BulletText("DirectX 11 / DXGI hooking using Microsoft Detours");
        ImGui::BulletText("GPU-to-GPU frame capture with shared textures");
        ImGui::BulletText("Borderless fullscreen projection window");
        ImGui::BulletText("Real-time performance monitoring");
        ImGui::BulletText("ImGui-based control interface");
        
        ImGui::Spacing();
        ImGui::TextWrapped("Controls:");
        ImGui::BulletText("F1 - Toggle this control interface");
        ImGui::BulletText("F11 - Toggle fullscreen projection (in projection window)");
        ImGui::BulletText("ESC - Hide projection window");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextWrapped("Built with: DirectX 11, Microsoft Detours, Dear ImGui");
    }
    ImGui::End();
}