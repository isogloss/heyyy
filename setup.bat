@echo off
echo Setting up FiveM Capture System build environment...

:: Create build directory
if not exist build mkdir build

:: Download and setup Microsoft Detours
if not exist "third_party\Detours" (
    echo Downloading Microsoft Detours...
    mkdir "third_party\Detours\include"
    mkdir "third_party\Detours\lib.X64"
    
    echo Note: Please download Microsoft Detours from the official Microsoft GitHub repository
    echo https://github.com/Microsoft/Detours and extract to third_party/Detours/
    echo.
    echo For now, creating minimal placeholder structure...
    
    :: Create minimal detours.h placeholder
    echo #pragma once > "third_party\Detours\include\detours.h"
    echo #include ^<windows.h^> >> "third_party\Detours\include\detours.h"
    echo. >> "third_party\Detours\include\detours.h"
    echo #define NO_ERROR    0L >> "third_party\Detours\include\detours.h"
    echo. >> "third_party\Detours\include\detours.h"
    echo extern "C" { >> "third_party\Detours\include\detours.h"
    echo     LONG WINAPI DetourTransactionBegin(VOID); >> "third_party\Detours\include\detours.h"
    echo     LONG WINAPI DetourUpdateThread(HANDLE hThread); >> "third_party\Detours\include\detours.h"
    echo     LONG WINAPI DetourAttach(PVOID *ppPointer, PVOID pDetour); >> "third_party\Detours\include\detours.h"
    echo     LONG WINAPI DetourDetach(PVOID *ppPointer, PVOID pDetour); >> "third_party\Detours\include\detours.h"
    echo     LONG WINAPI DetourTransactionCommit(VOID); >> "third_party\Detours\include\detours.h"
    echo } >> "third_party\Detours\include\detours.h"
)

:: Download and setup Dear ImGui
if not exist "third_party\imgui" (
    echo Downloading Dear ImGui...
    pushd third_party
    git clone https://github.com/ocornut/imgui.git
    if errorlevel 1 (
        echo Failed to clone ImGui repository
        echo Please manually download from https://github.com/ocornut/imgui
    )
    popd
)

echo.
echo Dependencies setup complete!
echo.
echo Next steps:
echo 1. cd build
echo 2. cmake ..
echo 3. cmake --build . --config Release
echo.
pause