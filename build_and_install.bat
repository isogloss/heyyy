@echo off
setlocal enabledelayedexpansion

echo =================================================================
echo              FiveM Capture System - Complete Build
echo =================================================================
echo.
echo This script will:
echo - Set up all dependencies automatically
echo - Build the FiveM Capture System executable
echo - Copy the final executable to your Downloads folder
echo.

:: Check if we're in the correct directory
if not exist "CMakeLists.txt" (
    echo ERROR: This script must be run from the root directory of the heyyy project!
    echo Please navigate to the directory containing CMakeLists.txt
    pause
    exit /b 1
)

:: Create Downloads directory path
set "DOWNLOADS_PATH=%USERPROFILE%\Downloads"
if not exist "%DOWNLOADS_PATH%" (
    echo ERROR: Downloads folder not found at %DOWNLOADS_PATH%
    echo Please ensure your Downloads folder exists
    pause
    exit /b 1
)

echo Step 1: Setting up build environment...
echo.

:: Create build directory
if not exist build mkdir build

:: Step 2: Setup Dependencies
echo Step 2: Setting up dependencies...
echo.

:: Create third_party directory structure if it doesn't exist
if not exist "third_party" mkdir "third_party"
cd third_party

:: Setup Microsoft Detours (placeholder)
if not exist "Detours" (
    echo Setting up Microsoft Detours placeholder...
    mkdir "Detours\include"
    mkdir "Detours\lib.X64"
    
    :: Create detours.h header file
    (
        echo #pragma once
        echo #include ^<windows.h^>
        echo.
        echo #define NO_ERROR    0L
        echo.
        echo extern "C" {
        echo     LONG WINAPI DetourTransactionBegin^(VOID^);
        echo     LONG WINAPI DetourUpdateThread^(HANDLE hThread^);
        echo     LONG WINAPI DetourAttach^(PVOID *ppPointer, PVOID pDetour^);
        echo     LONG WINAPI DetourDetach^(PVOID *ppPointer, PVOID pDetour^);
        echo     LONG WINAPI DetourTransactionCommit^(VOID^);
        echo }
    ) > "Detours\include\detours.h"
    
    echo Microsoft Detours placeholder created
) else (
    echo Microsoft Detours already available
)

:: Setup Dear ImGui
if not exist "imgui" (
    echo Downloading Dear ImGui...
    git clone --depth 1 https://github.com/ocornut/imgui.git
    if !errorlevel! neq 0 (
        echo ERROR: Failed to download Dear ImGui
        echo Please check your internet connection and Git installation
        cd ..
        pause
        exit /b 1
    )
    echo Dear ImGui downloaded successfully
) else (
    echo Dear ImGui already available
)

cd ..

:: Step 3: Configure CMake
echo.
echo Step 3: Configuring CMake...
echo.

cd build

:: Try with Visual Studio generator first
cmake .. -G "Visual Studio 16 2019" -A x64 2>nul
if !errorlevel! neq 0 (
    echo Visual Studio 2019 not found, trying Visual Studio 2022...
    cmake .. -G "Visual Studio 17 2022" -A x64 2>nul
    if !errorlevel! neq 0 (
        echo Visual Studio generators not found, using default...
        cmake ..
        if !errorlevel! neq 0 (
            echo ERROR: CMake configuration failed!
            echo.
            echo Please ensure you have:
            echo - CMake 3.16 or later installed
            echo - Visual Studio 2019/2022 with C++ support
            echo - Windows SDK installed
            echo.
            cd ..
            pause
            exit /b 1
        )
    )
)

echo CMake configuration successful!

:: Step 4: Build the project
echo.
echo Step 4: Building the project...
echo This may take a few minutes...
echo.

cmake --build . --config Release --verbose
if !errorlevel! neq 0 (
    echo.
    echo ERROR: Build failed!
    echo.
    echo Common solutions:
    echo - Ensure Visual Studio with C++ support is installed
    echo - Try running as Administrator
    echo - Check that Windows SDK is installed
    echo - Make sure DirectX SDK is available
    echo.
    cd ..
    pause
    exit /b 1
)

echo.
echo Build completed successfully!

:: Step 5: Find the executable
echo.
echo Step 5: Locating the executable...

set "EXE_PATH="
if exist "bin\Release\FiveMCapture.exe" (
    set "EXE_PATH=bin\Release\FiveMCapture.exe"
) else if exist "bin\FiveMCapture.exe" (
    set "EXE_PATH=bin\FiveMCapture.exe"
) else if exist "Release\FiveMCapture.exe" (
    set "EXE_PATH=Release\FiveMCapture.exe"
) else if exist "FiveMCapture.exe" (
    set "EXE_PATH=FiveMCapture.exe"
) else (
    echo ERROR: Could not find FiveMCapture.exe
    echo.
    echo Expected locations:
    echo - bin\Release\FiveMCapture.exe
    echo - bin\FiveMCapture.exe  
    echo - Release\FiveMCapture.exe
    echo - FiveMCapture.exe
    echo.
    echo Build may have failed or executable has different name.
    dir /s *.exe
    cd ..
    pause
    exit /b 1
)

echo Found executable at: %EXE_PATH%

:: Step 6: Copy to Downloads folder
echo.
echo Step 6: Copying executable to Downloads folder...

copy "%EXE_PATH%" "%DOWNLOADS_PATH%\FiveMCapture.exe"
if !errorlevel! neq 0 (
    echo ERROR: Failed to copy executable to Downloads folder
    echo Source: %EXE_PATH%
    echo Destination: %DOWNLOADS_PATH%\FiveMCapture.exe
    cd ..
    pause
    exit /b 1
)

cd ..

:: Step 7: Success!
echo.
echo =================================================================
echo                    BUILD AND INSTALL COMPLETE!
echo =================================================================
echo.
echo ✓ Dependencies set up successfully
echo ✓ Project built successfully
echo ✓ Executable copied to Downloads folder
echo.
echo The FiveMCapture.exe file is now available at:
echo %DOWNLOADS_PATH%\FiveMCapture.exe
echo.
echo Usage Instructions:
echo 1. Launch FiveM
echo 2. Run FiveMCapture.exe from your Downloads folder
echo 3. Press F1 in-game to toggle the control interface
echo 4. Use the interface to configure capture settings
echo.
echo Note: This application hooks into DirectX 11 applications like FiveM.
echo It may require administrator privileges and antivirus exceptions.
echo.
echo Press any key to run the application now, or close this window.
pause

:: Optional: Ask if user wants to run the application
echo.
set /p "RUN_APP=Would you like to run FiveMCapture.exe now? (y/n): "
if /i "!RUN_APP!"=="y" (
    echo Launching FiveMCapture.exe...
    start "" "%DOWNLOADS_PATH%\FiveMCapture.exe"
)

echo.
echo Thank you for using the FiveM Capture System!
pause