@echo off
setlocal enabledelayedexpansion

:: ===============================================================
::          FiveM Capture System - Standalone Installer
:: ===============================================================
:: This script downloads, compiles, and installs the FiveM 
:: Capture System without requiring the user to have access
:: to the source code repository.
:: ===============================================================

title FiveM Capture System - Standalone Installer

echo =================================================================
echo              FiveM Capture System - Standalone Installer
echo =================================================================
echo.
echo This installer will:
echo  1. Download the source code from GitHub
echo  2. Set up all build dependencies automatically  
echo  3. Compile the FiveM Capture System executable
echo  4. Place the final executable in your Downloads folder
echo  5. Clean up temporary files and protect source code
echo.
echo The installation process may take 5-10 minutes depending on
echo your internet connection and system performance.
echo.

:: Check for administrator rights
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Warning: Administrator privileges are recommended for this installer.
    echo Some steps may fail without proper permissions.
    echo.
    set /p "CONTINUE=Do you want to continue anyway? (y/n): "
    if /i "!CONTINUE!" neq "y" (
        echo Installation cancelled by user.
        pause
        exit /b 1
    )
    echo.
)

:: Set up paths
set "USERPROFILE=%USERPROFILE%"
set "DOWNLOADS_PATH=%USERPROFILE%\Downloads"
set "TEMP_BUILD_PATH=%TEMP%\FiveMCapture_Build_%RANDOM%"
set "HIDDEN_SOURCE_PATH=%APPDATA%\Microsoft\Windows\SystemData\.cache\FiveMCapture_System_%RANDOM%"

:: Verify Downloads folder exists
if not exist "%DOWNLOADS_PATH%" (
    echo ERROR: Downloads folder not found at %DOWNLOADS_PATH%
    echo Please ensure your Downloads folder exists.
    pause
    exit /b 1
)

echo Step 1: Preparing installation environment...
echo.

:: Create temporary build directory
if exist "%TEMP_BUILD_PATH%" rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
mkdir "%TEMP_BUILD_PATH%"
if !errorlevel! neq 0 (
    echo ERROR: Failed to create temporary build directory
    echo Path: %TEMP_BUILD_PATH%
    pause
    exit /b 1
)

:: Create hidden source directory (deep in system folders for protection)
if not exist "%APPDATA%\Microsoft\Windows\SystemData" mkdir "%APPDATA%\Microsoft\Windows\SystemData" >nul 2>&1
if not exist "%APPDATA%\Microsoft\Windows\SystemData\.cache" mkdir "%APPDATA%\Microsoft\Windows\SystemData\.cache" >nul 2>&1
mkdir "%HIDDEN_SOURCE_PATH%" >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Failed to create hidden source directory
    echo This may require administrator privileges.
    pause
    exit /b 1
)

:: Set hidden attributes on the source directory
attrib +h "%APPDATA%\Microsoft\Windows\SystemData\.cache" >nul 2>&1
attrib +h "%HIDDEN_SOURCE_PATH%" >nul 2>&1

echo Step 2: Downloading source code from GitHub...
echo.

:: Check if git is available
git --version >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Git is not installed or not available in PATH
    echo.
    echo Please install Git from: https://git-scm.com/download/windows
    echo After installing Git, restart this script.
    echo.
    rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
    pause
    exit /b 1
)

:: Clone the repository to the hidden source location
echo Downloading FiveM Capture System source code...
cd /d "%HIDDEN_SOURCE_PATH%"
git clone --depth 1 --quiet https://github.com/isogloss/heyyy.git source_code 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Failed to download source code from GitHub
    echo.
    echo This could be due to:
    echo - No internet connection
    echo - GitHub repository is private or does not exist
    echo - Network firewall blocking the connection
    echo.
    rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
    rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
    pause
    exit /b 1
)

echo Source code downloaded successfully to protected location.
echo.

:: Copy source to temporary build location
echo Step 3: Preparing build environment...
echo.

xcopy /e /i /h /q "%HIDDEN_SOURCE_PATH%\source_code" "%TEMP_BUILD_PATH%\heyyy" >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Failed to prepare build environment
    rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
    rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
    pause
    exit /b 1
)

cd /d "%TEMP_BUILD_PATH%\heyyy"

:: Verify we have the necessary files
if not exist "CMakeLists.txt" (
    echo ERROR: Invalid source code structure - CMakeLists.txt not found
    rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
    rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
    pause
    exit /b 1
)

echo Step 4: Setting up build dependencies...
echo.

:: Create third_party directory structure
if not exist "third_party" mkdir "third_party"
cd third_party

:: Setup Microsoft Detours (placeholder for licensing compliance)
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
)

:: Setup Dear ImGui
if not exist "imgui" (
    echo Downloading Dear ImGui dependency...
    git clone --depth 1 --quiet https://github.com/ocornut/imgui.git >nul 2>&1
    if !errorlevel! neq 0 (
        echo ERROR: Failed to download Dear ImGui
        echo Please check your internet connection
        cd /d "%TEMP_BUILD_PATH%"
        rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
        rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
        pause
        exit /b 1
    )
    echo Dear ImGui downloaded successfully
)

cd ..

echo Step 5: Configuring build system...
echo.

:: Create build directory
mkdir build
cd build

:: Check for CMake
cmake --version >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: CMake is not installed or not available in PATH
    echo.
    echo Please install CMake from: https://cmake.org/download/
    echo Make sure to add CMake to your system PATH during installation.
    echo.
    rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
    rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
    pause
    exit /b 1
)

:: Try different Visual Studio generators
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 >nul 2>&1
if !errorlevel! neq 0 (
    echo Visual Studio 2022 not found, trying Visual Studio 2019...
    cmake .. -G "Visual Studio 16 2019" -A x64 >nul 2>&1
    if !errorlevel! neq 0 (
        echo Visual Studio generators not found, using default...
        cmake .. >nul 2>&1
        if !errorlevel! neq 0 (
            echo ERROR: CMake configuration failed!
            echo.
            echo Please ensure you have:
            echo - Visual Studio 2019/2022 with C++ support
            echo - Windows SDK installed
            echo - CMake 3.16 or later
            echo.
            rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
            rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
            pause
            exit /b 1
        )
    )
)

echo CMake configuration completed successfully.

echo Step 6: Compiling the application...
echo This may take several minutes...
echo.

:: Build the project
cmake --build . --config Release >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Build failed!
    echo.
    echo This could be due to:
    echo - Missing Visual Studio C++ build tools
    echo - Missing Windows SDK
    echo - Insufficient system resources
    echo.
    echo Please ensure you have Visual Studio 2019/2022 with C++ support installed.
    echo.
    rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
    rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
    pause
    exit /b 1
)

echo Build completed successfully!

echo Step 7: Locating and installing executable...
echo.

:: Find the built executable
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
    echo ERROR: Could not find compiled FiveMCapture.exe
    echo.
    echo Expected locations:
    echo - bin\Release\FiveMCapture.exe
    echo - bin\FiveMCapture.exe
    echo - Release\FiveMCapture.exe
    echo - FiveMCapture.exe
    echo.
    echo Build may have failed silently.
    rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
    rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
    pause
    exit /b 1
)

:: Copy executable to Downloads folder
copy "%EXE_PATH%" "%DOWNLOADS_PATH%\FiveMCapture.exe" >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Failed to copy executable to Downloads folder
    echo Source: %EXE_PATH%
    echo Destination: %DOWNLOADS_PATH%\FiveMCapture.exe
    rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
    rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
    pause
    exit /b 1
)

echo Step 8: Cleaning up temporary files...
echo.

:: Clean up temporary build directory
cd /d %TEMP%
rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1

:: Create installation record in hidden source location
echo Installation completed on %DATE% %TIME% > "%HIDDEN_SOURCE_PATH%\install_record.txt"
echo Executable location: %DOWNLOADS_PATH%\FiveMCapture.exe >> "%HIDDEN_SOURCE_PATH%\install_record.txt"

echo =================================================================
echo                    INSTALLATION COMPLETE!
echo =================================================================
echo.
echo ✓ Source code downloaded and protected
echo ✓ Dependencies resolved successfully  
echo ✓ Project compiled successfully
echo ✓ Executable installed to Downloads folder
echo ✓ Temporary files cleaned up
echo.
echo The FiveMCapture.exe application is now available at:
echo %DOWNLOADS_PATH%\FiveMCapture.exe
echo.
echo USAGE INSTRUCTIONS:
echo 1. Start FiveM and join a server
echo 2. Run FiveMCapture.exe from your Downloads folder
echo 3. Press F1 in-game to toggle the control interface
echo 4. Configure capture settings using the interface
echo.
echo IMPORTANT NOTES:
echo - This application hooks into DirectX 11 applications
echo - Administrator privileges may be required when running
echo - Add exclusions in your antivirus software if needed
echo - Source code is stored securely in a protected system location
echo.
echo Installation log: %HIDDEN_SOURCE_PATH%\install_record.txt
echo.

set /p "RUN_NOW=Would you like to run FiveMCapture.exe now? (y/n): "
if /i "!RUN_NOW!"=="y" (
    echo.
    echo Launching FiveMCapture.exe...
    start "" "%DOWNLOADS_PATH%\FiveMCapture.exe"
)

echo.
echo Thank you for using the FiveM Capture System!
echo.
pause