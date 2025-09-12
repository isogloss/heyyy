#include "injection_manager.h"
#include <tlhelp32.h>
#include <shlobj.h>
#include <fstream>
#include <filesystem>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

InjectionManager::InjectionManager()
    : m_injected(false)
    , m_targetProcessId(0)
    , m_targetProcess(nullptr)
    , m_injectedThread(nullptr)
    , m_monitorThread(nullptr)
    , m_stopEvent(nullptr)
{
    m_stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

InjectionManager::~InjectionManager() {
    StopInjection();
    
    if (m_stopEvent) {
        CloseHandle(m_stopEvent);
    }
}

bool InjectionManager::InjectIntoFiveM() {
    if (m_injected) {
        return true;
    }

    // Find FiveM process
    DWORD processId = FindFiveMProcess();
    if (processId == 0) {
        m_statistics.processFound = false;
        m_statistics.lastError = ERROR_NOT_FOUND;
        return false;
    }

    m_statistics.processFound = true;
    m_statistics.processId = processId;
    m_targetProcessId = processId;

    // Extract hook DLL to temp location
    std::wstring dllPath = ExtractHookDLL();
    if (dllPath.empty()) {
        m_statistics.lastError = GetLastError();
        return false;
    }

    // Inject the DLL
    if (!InjectDLL(processId, dllPath)) {
        CleanupExtractedDLL();
        return false;
    }

    // Start monitoring thread
    ResetEvent(m_stopEvent);
    m_monitorThread = CreateThread(nullptr, 0, MonitorThreadProc, this, 0, nullptr);

    m_injected = true;
    m_statistics.injected = true;
    return true;
}

void InjectionManager::StopInjection() {
    if (m_injected) {
        // Signal stop event
        if (m_stopEvent) {
            SetEvent(m_stopEvent);
        }

        // Wait for monitor thread to exit
        if (m_monitorThread) {
            WaitForSingleObject(m_monitorThread, 5000);
            CloseHandle(m_monitorThread);
            m_monitorThread = nullptr;
        }

        // Clean up process handles
        if (m_targetProcess) {
            CloseHandle(m_targetProcess);
            m_targetProcess = nullptr;
        }

        if (m_injectedThread) {
            CloseHandle(m_injectedThread);
            m_injectedThread = nullptr;
        }

        // Clean up extracted DLL
        CleanupExtractedDLL();

        m_injected = false;
        m_targetProcessId = 0;
        m_statistics.injected = false;
        m_statistics.processId = 0;
    }
}

InjectionStatistics InjectionManager::GetStatistics() const {
    // Update process found status
    if (m_targetProcessId != 0) {
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_targetProcessId);
        m_statistics.processFound = (process != nullptr);
        if (process) {
            // Get process path
            wchar_t processPath[MAX_PATH] = {};
            GetModuleFileNameExW(process, nullptr, processPath, MAX_PATH);
            m_statistics.processPath = processPath;
            CloseHandle(process);
        }
    } else {
        // Try to find FiveM process
        m_statistics.processId = FindFiveMProcess();
        m_statistics.processFound = (m_statistics.processId != 0);
    }
    
    return m_statistics;
}

DWORD InjectionManager::FindFiveMProcess() const {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            // Look for FiveM.exe or similar process names
            std::wstring processName = entry.szExeFile;
            std::transform(processName.begin(), processName.end(), processName.begin(), ::towlower);
            
            if (processName.find(L"fivem") != std::wstring::npos) {
                DWORD processId = entry.th32ProcessID;
                CloseHandle(snapshot);
                return processId;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

std::wstring InjectionManager::ExtractHookDLL() {
    // Get temporary directory
    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath) == 0) {
        return L"";
    }

    // Create unique filename
    wchar_t tempFile[MAX_PATH];
    if (GetTempFileNameW(tempPath, L"p6h", 0, tempFile) == 0) {
        return L"";
    }

    // Replace .tmp with .dll
    std::wstring dllPath = tempFile;
    if (dllPath.size() > 4) {
        dllPath = dllPath.substr(0, dllPath.size() - 4) + L".dll";
    }

    // Find hook DLL resource
    HRSRC resource = FindResourceW(nullptr, L"HOOK_DLL", RT_RCDATA);
    if (!resource) {
        return L"";
    }

    HGLOBAL resourceData = LoadResource(nullptr, resource);
    if (!resourceData) {
        return L"";
    }

    void* data = LockResource(resourceData);
    DWORD size = SizeofResource(nullptr, resource);
    
    if (!data || size == 0) {
        return L"";
    }

    // Write DLL to temp file
    std::ofstream file(dllPath, std::ios::binary);
    if (!file.is_open()) {
        return L"";
    }

    file.write(static_cast<const char*>(data), size);
    file.close();

    m_extractedDllPath = dllPath;
    return dllPath;
}

bool InjectionManager::InjectDLL(DWORD processId, const std::wstring& dllPath) {
    // Open target process
    m_targetProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, processId);

    if (!m_targetProcess) {
        m_statistics.lastError = GetLastError();
        return false;
    }

    // Allocate memory in target process for DLL path
    size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    void* remotePath = VirtualAllocEx(m_targetProcess, nullptr, pathSize, 
        MEM_COMMIT, PAGE_READWRITE);

    if (!remotePath) {
        m_statistics.lastError = GetLastError();
        return false;
    }

    // Write DLL path to target process
    SIZE_T written;
    if (!WriteProcessMemory(m_targetProcess, remotePath, dllPath.c_str(), 
        pathSize, &written)) {
        VirtualFreeEx(m_targetProcess, remotePath, 0, MEM_RELEASE);
        m_statistics.lastError = GetLastError();
        return false;
    }

    // Get LoadLibraryW address
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32) {
        VirtualFreeEx(m_targetProcess, remotePath, 0, MEM_RELEASE);
        m_statistics.lastError = GetLastError();
        return false;
    }

    FARPROC loadLibraryW = GetProcAddress(kernel32, "LoadLibraryW");
    if (!loadLibraryW) {
        VirtualFreeEx(m_targetProcess, remotePath, 0, MEM_RELEASE);
        m_statistics.lastError = GetLastError();
        return false;
    }

    // Create remote thread to load DLL
    m_injectedThread = CreateRemoteThread(m_targetProcess, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryW),
        remotePath, 0, nullptr);

    if (!m_injectedThread) {
        VirtualFreeEx(m_targetProcess, remotePath, 0, MEM_RELEASE);
        m_statistics.lastError = GetLastError();
        return false;
    }

    // Wait for thread completion
    WaitForSingleObject(m_injectedThread, INFINITE);

    // Clean up remote memory
    VirtualFreeEx(m_targetProcess, remotePath, 0, MEM_RELEASE);

    return true;
}

void InjectionManager::CleanupExtractedDLL() {
    if (!m_extractedDllPath.empty()) {
        DeleteFileW(m_extractedDllPath.c_str());
        m_extractedDllPath.clear();
    }
}

DWORD WINAPI InjectionManager::MonitorThreadProc(LPVOID lpParameter) {
    InjectionManager* manager = static_cast<InjectionManager*>(lpParameter);
    return manager->MonitorProcess();
}

void InjectionManager::MonitorProcess() {
    while (WaitForSingleObject(m_stopEvent, 1000) == WAIT_TIMEOUT) {
        // Check if target process is still alive
        if (m_targetProcess) {
            DWORD exitCode;
            if (GetExitCodeProcess(m_targetProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                // Process has exited
                m_statistics.processFound = false;
                m_statistics.injected = false;
                m_injected = false;
                break;
            }
        }
        
        // If auto-reattach is enabled and we lost connection, try to reconnect
        // This would be implemented based on application settings
        // For now, just monitor the process
    }
    
    return 0;
}