#include "shared_memory.h"

SharedMemory::SharedMemory()
    : m_fileMapping(nullptr)
    , m_sharedData(nullptr)
    , m_isCreator(false)
{
}

SharedMemory::~SharedMemory() {
    Close();
}

bool SharedMemory::CreateShared(const std::wstring& name) {
    if (m_fileMapping) {
        return false;
    }

    m_fileMapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(SharedData),
        name.c_str()
    );

    if (!m_fileMapping) {
        return false;
    }

    m_sharedData = static_cast<SharedData*>(MapViewOfFile(
        m_fileMapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedData)
    ));

    if (!m_sharedData) {
        CloseHandle(m_fileMapping);
        m_fileMapping = nullptr;
        return false;
    }

    // Initialize shared data
    ZeroMemory(m_sharedData, sizeof(SharedData));
    m_isCreator = true;
    return true;
}

bool SharedMemory::OpenShared(const std::wstring& name) {
    if (m_fileMapping) {
        return false;
    }

    m_fileMapping = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        name.c_str()
    );

    if (!m_fileMapping) {
        return false;
    }

    m_sharedData = static_cast<SharedData*>(MapViewOfFile(
        m_fileMapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedData)
    ));

    if (!m_sharedData) {
        CloseHandle(m_fileMapping);
        m_fileMapping = nullptr;
        return false;
    }

    m_isCreator = false;
    return true;
}

bool SharedMemory::UpdateSharedData(const SharedData& data) {
    if (!m_sharedData) {
        return false;
    }

    *m_sharedData = data;
    return true;
}

bool SharedMemory::GetSharedData(SharedData& data) {
    if (!m_sharedData) {
        return false;
    }

    data = *m_sharedData;
    return true;
}

void SharedMemory::Close() {
    if (m_sharedData) {
        UnmapViewOfFile(m_sharedData);
        m_sharedData = nullptr;
    }

    if (m_fileMapping) {
        CloseHandle(m_fileMapping);
        m_fileMapping = nullptr;
    }

    m_isCreator = false;
}