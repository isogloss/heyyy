#pragma once

// Linux build compatibility stub definitions
#ifdef LINUX_BUILD

#include <cstdint>
#include <cstddef>
#include <memory>

// Windows types stubs
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HANDLE;
typedef void* HMODULE;
typedef void* HRSRC;
typedef void* HGLOBAL;
typedef void* HMONITOR;
typedef void* HDC;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int BOOL;
typedef long LONG;
typedef unsigned short WORD;
typedef uint64_t UINT64;
typedef int64_t LONG_PTR;
typedef size_t SIZE_T;
typedef unsigned long ULONG_PTR;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t WCHAR;
typedef LONG HRESULT;
typedef void* FARPROC;
typedef ULONG_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef struct tagRECT { LONG left, top, right, bottom; } RECT, *LPRECT;
typedef struct tagPOINT { LONG x, y; } POINT;

// D3D11/DXGI stubs
struct ID3D11Device { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11DeviceContext { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11Texture2D { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11RenderTargetView { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11VertexShader { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11PixelShader { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11InputLayout { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11Buffer { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11SamplerState { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11ShaderResourceView { virtual void AddRef() {} virtual void Release() {} };
struct IDXGISwapChain { virtual void AddRef() {} virtual void Release() {} };
struct IDXGIKeyedMutex { virtual void AddRef() {} virtual void Release() {} };
struct IDXGIResource { virtual void AddRef() {} virtual void Release() {} };
struct ID3D11Resource { virtual void AddRef() {} virtual void Release() {} };

enum DXGI_FORMAT { 
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28
};
enum D3D_FEATURE_LEVEL { 
    D3D_FEATURE_LEVEL_11_0 = 0,
    D3D_FEATURE_LEVEL_11_1 = 1
};

struct DXGI_SWAP_CHAIN_DESC {};
struct D3D11_TEXTURE2D_DESC { UINT Width, Height; DXGI_FORMAT Format; };
struct PROCESSENTRY32W { DWORD dwSize; DWORD th32ProcessID; wchar_t szExeFile[260]; };

// Callback type definitions
#define CALLBACK
#define WINAPI

// Message constants
#define WM_COMMAND 0x0111
#define WM_SIZE 0x0005
#define WM_CLOSE 0x0010
#define WM_DESTROY 0x0002
#define WM_NCCREATE 0x0081

// Other constants
#define FAILED(hr) (hr < 0)
#define SUCCEEDED(hr) (hr >= 0)
#define S_OK 0
#define E_FAIL -1
#define FALSE 0
#define TRUE 1
#define MAX_PATH 260
#define INFINITE 0xFFFFFFFF

// Namespace for Microsoft::WRL
namespace Microsoft { 
    namespace WRL { 
        template<typename T>
        class ComPtr {
        public:
            ComPtr() : ptr_(nullptr) {}
            ~ComPtr() { if (ptr_) ptr_->Release(); }
            T* Get() const { return ptr_; }
            T** GetAddressOf() { return &ptr_; }
            void Reset() { if (ptr_) { ptr_->Release(); ptr_ = nullptr; } }
            T* operator->() const { return ptr_; }
        private:
            T* ptr_;
        };
    }
}

// Function stubs
inline void* GetModuleHandle(LPCWSTR) { return nullptr; }
inline DWORD GetLastError() { return 0; }
inline void CoInitialize(void*) {}
inline void CoUninitialize() {}
inline BOOL GetClientRect(HWND, RECT*) { return TRUE; }
inline HWND GetDesktopWindow() { return nullptr; }
inline DWORD GetTickCount() { return 0; }
inline UINT64 GetTickCount64() { return 0; }
inline void Sleep(DWORD) {}

#endif // LINUX_BUILD