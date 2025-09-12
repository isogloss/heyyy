#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <vector>

class MonitorManager;
class InjectionManager;
class FrameProjector;

/**
 * Main application class that manages the pick6 GUI and coordinates all components
 */
class Application {
public:
    Application();
    ~Application();

    // Initialize the application
    bool Initialize(HINSTANCE hInstance);
    
    // Run the main message loop
    int Run();
    
    // Shutdown the application
    void Shutdown();

    // Get the main window handle
    HWND GetMainWindow() const { return m_hwnd; }

private:
    // Window procedures
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // UI event handlers
    void OnStartStop();
    void OnMonitorSelect();
    void OnVSyncToggle();
    void OnAutoReattachToggle();
    void OnClose();

    // UI update methods
    void UpdateUI();
    void UpdateDiagnostics();
    void CreateControls();
    void LayoutControls();

    // Core functionality
    void StartCapture();
    void StopCapture();
    bool IsCapturing() const { return m_capturing; }

    // Members
    HINSTANCE m_hInstance;
    HWND m_hwnd;
    
    // UI Controls
    HWND m_btnStartStop;
    HWND m_cmbMonitors;
    HWND m_chkVSync;
    HWND m_chkAutoReattach;
    HWND m_lblStatus;
    HWND m_txtDiagnostics;

    // Core components
    std::unique_ptr<MonitorManager> m_monitorManager;
    std::unique_ptr<InjectionManager> m_injectionManager;
    std::unique_ptr<FrameProjector> m_frameProjector;

    // State
    bool m_capturing;
    bool m_vsyncEnabled;
    bool m_autoReattachEnabled;
    int m_selectedMonitor;

    // Window class name
    static const wchar_t* const WINDOW_CLASS_NAME;
};