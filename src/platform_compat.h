#pragma once

// Linux build compatibility stub definitions
#ifdef LINUX_BUILD

#include <cstdint>
#include <cstddef>

// Windows types stubs
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HANDLE;
typedef void* HMODULE;
typedef void* HRSRC;
typedef void* HGLOBAL;
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

// D3D11/DXGI stubs
struct ID3D11Device {};
struct ID3D11DeviceContext {};
struct ID3D11Texture2D {};
struct ID3D11RenderTargetView {};
struct ID3D11VertexShader {};
struct ID3D11PixelShader {};
struct ID3D11InputLayout {};
struct ID3D11Buffer {};
struct ID3D11SamplerState {};
struct ID3D11ShaderResourceView {};
struct IDXGISwapChain {};
struct IDXGIKeyedMutex {};
struct IDXGIResource {};
struct DXGI_SWAP_CHAIN_DESC {};
struct D3D11_TEXTURE2D_DESC {};

enum DXGI_FORMAT { DXGI_FORMAT_UNKNOWN = 0 };
enum D3D_FEATURE_LEVEL { D3D_FEATURE_LEVEL_11_0 = 0 };

// Constants
#define FAILED(hr) (hr < 0)
#define SUCCEEDED(hr) (hr >= 0)
#define S_OK 0
#define E_FAIL -1
#define FALSE 0
#define TRUE 1

// Function stubs
inline void* GetModuleHandle(LPCWSTR) { return nullptr; }
inline DWORD GetLastError() { return 0; }
inline void CoInitialize(void*) {}
inline void CoUninitialize() {}

#endif // LINUX_BUILD