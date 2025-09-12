#include "monitor_manager.h"

MonitorManager::MonitorManager() {
}

MonitorManager::~MonitorManager() {
}

bool MonitorManager::Initialize() {
    RefreshMonitors();
    return !m_monitors.empty();
}

void MonitorManager::RefreshMonitors() {
    m_monitors.clear();
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(this));
}

const MonitorInfo* MonitorManager::GetMonitor(int index) const {
    if (index < 0 || index >= static_cast<int>(m_monitors.size())) {
        return nullptr;
    }
    return &m_monitors[index];
}

int MonitorManager::GetMonitorFromPoint(int x, int y) const {
    POINT pt = { x, y };
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
    
    if (hMonitor != nullptr) {
        for (size_t i = 0; i < m_monitors.size(); ++i) {
            if (m_monitors[i].hMonitor == hMonitor) {
                return static_cast<int>(i);
            }
        }
    }
    
    return -1;
}

int MonitorManager::GetPrimaryMonitorIndex() const {
    for (size_t i = 0; i < m_monitors.size(); ++i) {
        if (m_monitors[i].isPrimary) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

BOOL CALLBACK MonitorManager::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, 
    LPRECT lprcMonitor, LPARAM dwData) {
    
    MonitorManager* manager = reinterpret_cast<MonitorManager*>(dwData);
    
    MONITORINFOEXW monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    
    if (GetMonitorInfoW(hMonitor, &monitorInfo)) {
        MonitorInfo info = {};
        info.hMonitor = hMonitor;
        info.deviceName = monitorInfo.szDevice;
        info.x = monitorInfo.rcMonitor.left;
        info.y = monitorInfo.rcMonitor.top;
        info.width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        info.height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
        info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
        
        manager->m_monitors.push_back(info);
    }
    
    return TRUE;
}