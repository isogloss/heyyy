#pragma once

#include <windows.h>
#include <string>

/**
 * Inter-process communication for sharing texture handles and synchronization
 */
class SharedMemory {
public:
    struct SharedData {
        HANDLE sharedTextureHandle;
        HANDLE frameEventHandle;
        UINT width;
        UINT height;
        UINT format;
        UINT64 frameCount;
        bool active;
    };

    SharedMemory();
    ~SharedMemory();

    // Producer side (hook DLL)
    bool CreateShared(const std::wstring& name);
    bool UpdateSharedData(const SharedData& data);

    // Consumer side (main application)  
    bool OpenShared(const std::wstring& name);
    bool GetSharedData(SharedData& data);

    // Common
    void Close();

private:
    HANDLE m_fileMapping;
    SharedData* m_sharedData;
    bool m_isCreator;
};