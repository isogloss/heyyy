#pragma once

#include <windows.h>
#include <vector>
#include <string>

/**
 * Information about a display monitor
 */
struct MonitorInfo {
    HMONITOR hMonitor;
    std::wstring deviceName;
    int width;
    int height;
    int x;
    int y;
    bool isPrimary;
};

/**
 * Manages display monitor enumeration and selection
 */
class MonitorManager {
public:
    MonitorManager();
    ~MonitorManager();

    // Initialize the monitor manager
    bool Initialize();
    
    // Get list of available monitors
    const std::vector<MonitorInfo>& GetMonitors() const { return m_monitors; }
    
    // Get monitor info by index
    const MonitorInfo* GetMonitor(int index) const;
    
    // Refresh monitor list (call when display configuration changes)
    void RefreshMonitors();
    
    // Get monitor that contains the specified point
    int GetMonitorFromPoint(int x, int y) const;
    
    // Get primary monitor index
    int GetPrimaryMonitorIndex() const;

private:
    // Monitor enumeration callback
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, 
        LPRECT lprcMonitor, LPARAM dwData);
    
    std::vector<MonitorInfo> m_monitors;
};