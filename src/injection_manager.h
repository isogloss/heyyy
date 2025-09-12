#pragma once

#include <windows.h>
#include <string>
#include <memory>

/**
 * Statistics for injection manager
 */
struct InjectionStatistics {
    bool processFound = false;
    bool injected = false;
    DWORD processId = 0;
    std::wstring processPath;
    DWORD lastError = 0;
};

/**
 * Manages process injection into FiveM
 */
class InjectionManager {
public:
    InjectionManager();
    ~InjectionManager();

    // Inject hook DLL into FiveM process
    bool InjectIntoFiveM();
    
    // Stop injection and clean up
    void StopInjection();
    
    // Check if injection is active
    bool IsInjected() const { return m_injected; }
    
    // Get statistics
    InjectionStatistics GetStatistics() const;

private:
    // Find FiveM process
    DWORD FindFiveMProcess() const;
    
    // Extract hook DLL from resources to temporary location
    std::wstring ExtractHookDLL();
    
    // Inject DLL using CreateRemoteThread + LoadLibrary
    bool InjectDLL(DWORD processId, const std::wstring& dllPath);
    
    // Clean up extracted DLL
    void CleanupExtractedDLL();
    
    // Monitor injected process
    void MonitorProcess();
    
    // Static thread procedure for monitoring
    static DWORD WINAPI MonitorThreadProc(LPVOID lpParameter);

    // Members
    bool m_injected;
    DWORD m_targetProcessId;
    HANDLE m_targetProcess;
    HANDLE m_injectedThread;
    HANDLE m_monitorThread;
    HANDLE m_stopEvent;
    std::wstring m_extractedDllPath;
    mutable InjectionStatistics m_statistics;
};