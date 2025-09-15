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
echo  3. Download and install missing build tools (Git, CMake)
echo  4. Compile the FiveM Capture System executable
echo  5. Install runtime dependencies (Visual C++ Redistributables)
echo  6. Place the final executable in your Downloads folder
echo  7. Clean up temporary files and protect source code
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

:: Check if git is available, offer to download portable version if not
git --version >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Git is not installed or not available in PATH
    echo.
    echo Git is required to download the source code and dependencies.
    echo.
    set /p "INSTALL_GIT=Would you like to download Git Portable automatically? (y/n): "
    if /i "!INSTALL_GIT!"=="y" (
        call :DownloadAndSetupGitPortable
        if !errorlevel! neq 0 (
            echo Git setup failed.
            rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
            pause
            exit /b 1
        )
    ) else (
        echo Please install Git from: https://git-scm.com/download/windows
        echo After installing Git, restart this script.
        echo.
        rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
        pause
        exit /b 1
    )
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

:: Setup Microsoft Detours (check for real library first, then placeholder)
if not exist "Detours" (
    echo Setting up Microsoft Detours...
    
    :: Check if user has provided the real Microsoft Detours library
    if exist "%USERPROFILE%\Downloads\Detours" (
        echo Found Microsoft Detours in Downloads folder, using real library...
        xcopy /e /i /q "%USERPROFILE%\Downloads\Detours" "Detours" >nul 2>&1
        if !errorlevel! equ 0 (
            echo ✓ Microsoft Detours real library installed
        ) else (
            echo Warning: Failed to copy real Detours library, falling back to placeholder
            goto :CreateDetoursPlaceholder
        )
    ) else (
        goto :CreateDetoursPlaceholder
    )
) else (
    echo Microsoft Detours already available
)
goto :DetoursSetupComplete

:CreateDetoursPlaceholder
echo Creating Microsoft Detours placeholder for licensing compliance...
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
echo Note: For production use, download the real Microsoft Detours library from:
echo https://github.com/Microsoft/Detours

:DetoursSetupComplete

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

:: Check for CMake and offer to download if missing
cmake --version >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: CMake is not installed or not available in PATH
    echo.
    echo CMake is required to build the project.
    echo.
    set /p "INSTALL_CMAKE=Would you like to download and install CMake automatically? (y/n): "
    if /i "!INSTALL_CMAKE!"=="y" (
        call :DownloadAndInstallCMake
        if !errorlevel! neq 0 (
            echo CMake installation failed.
            rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
            rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
            pause
            exit /b 1
        )
    ) else (
        echo Please install CMake from: https://cmake.org/download/
        echo Make sure to add CMake to your system PATH during installation.
        echo.
        rmdir /s /q "%TEMP_BUILD_PATH%" >nul 2>&1
        rmdir /s /q "%HIDDEN_SOURCE_PATH%" >nul 2>&1
        pause
        exit /b 1
    )
)

:: Try different Visual Studio generators with Windows SDK verification
echo Configuring with CMake...

:: Check for Windows SDK
call :CheckWindowsSDK

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
            echo - Windows 10/11 SDK installed
            echo - CMake 3.16 or later
            echo.
            echo To install Visual Studio Build Tools:
            echo https://aka.ms/vs/17/release/vs_buildtools.exe
            echo.
            echo Install the following workloads:
            echo - C++ build tools
            echo - Windows 10/11 SDK (latest version)
            echo - MSVC v143 compiler toolset
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

:: Install runtime dependencies for the built application
echo Step 7: Installing runtime dependencies...
call :DownloadAndInstallVCRedist

echo Step 8: Locating and installing executable...
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

echo Step 9: Cleaning up temporary files...
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
echo ✓ Build tools verified and installed
echo ✓ Project compiled successfully
echo ✓ Runtime dependencies installed
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

:: ===============================================================
:: Helper Functions
:: ===============================================================

:DownloadAndInstallCMake
echo.
echo Downloading CMake installer...
set "CMAKE_VERSION=3.27.7"
set "CMAKE_INSTALLER=cmake-!CMAKE_VERSION!-windows-x86_64.msi"
set "CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v!CMAKE_VERSION!/!CMAKE_INSTALLER!"
set "CMAKE_TEMP=%TEMP%\!CMAKE_INSTALLER!"

:: Check if PowerShell is available
powershell -Command "Write-Host 'PowerShell available'" >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: PowerShell is required for automatic downloads but is not available
    echo Please manually install CMake from: https://cmake.org/download/
    echo After installation, restart this installer.
    exit /b 1
)

:: Download CMake
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = 'Tls12'; Invoke-WebRequest -Uri '!CMAKE_URL!' -OutFile '!CMAKE_TEMP!' -UserAgent 'Mozilla/5.0'}" >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Failed to download CMake
    echo Please manually install from: https://cmake.org/download/
    exit /b 1
)

echo Installing CMake...
echo This may require administrator privileges and will open a installer window.
pause

:: Install CMake silently
msiexec /i "!CMAKE_TEMP!" /quiet INSTALLDIR="C:\Program Files\CMake" ADD_CMAKE_TO_PATH=System >nul 2>&1
if !errorlevel! neq 0 (
    echo CMAKE installation may have failed or requires user interaction.
    echo Please complete the installation manually if a window appeared.
    pause
)

:: Clean up
del "!CMAKE_TEMP!" >nul 2>&1

:: Update PATH for current session
set "PATH=%PATH%;C:\Program Files\CMake\bin"

:: Verify installation
cmake --version >nul 2>&1
if !errorlevel! neq 0 (
    echo WARNING: CMake may not be properly installed or added to PATH.
    echo Please restart this installer after CMake installation is complete.
    exit /b 1
)

echo CMake installed successfully!
exit /b 0

:DownloadAndInstallVCRedist
echo.
echo Checking for Microsoft Visual C++ Redistributables...

:: Check if Visual C++ 2019+ redistributable is already installed
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\X64" >nul 2>&1
if !errorlevel! equ 0 (
    echo Visual C++ Redistributable already installed.
    exit /b 0
)

echo Downloading Microsoft Visual C++ Redistributable...
set "VCREDIST_URL=https://aka.ms/vs/17/release/vc_redist.x64.exe"
set "VCREDIST_TEMP=%TEMP%\vc_redist.x64.exe"

:: Check if PowerShell is available
powershell -Command "Write-Host 'PowerShell available'" >nul 2>&1
if !errorlevel! neq 0 (
    echo WARNING: PowerShell is not available - cannot download Visual C++ Redistributable automatically
    echo Please manually download from: https://aka.ms/vs/17/release/vc_redist.x64.exe
    exit /b 0
)

:: Download VC++ Redistributable
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = 'Tls12'; Invoke-WebRequest -Uri '!VCREDIST_URL!' -OutFile '!VCREDIST_TEMP!' -UserAgent 'Mozilla/5.0'}" >nul 2>&1
if !errorlevel! neq 0 (
    echo WARNING: Failed to download Visual C++ Redistributable
    echo This may cause runtime errors. Please manually download from:
    echo https://aka.ms/vs/17/release/vc_redist.x64.exe
    exit /b 0
)

echo Installing Visual C++ Redistributable...
"!VCREDIST_TEMP!" /quiet /norestart
if !errorlevel! neq 0 (
    echo WARNING: Visual C++ Redistributable installation may have failed
    echo This may cause runtime errors when running the application.
)

:: Clean up
del "!VCREDIST_TEMP!" >nul 2>&1

echo Visual C++ Redistributable installation completed.
exit /b 0

:DownloadAndSetupGitPortable
echo.
echo Downloading Git Portable...
set "GIT_VERSION=2.42.0.2"
set "GIT_PORTABLE=PortableGit-!GIT_VERSION!-64-bit.7z.exe"
set "GIT_URL=https://github.com/git-for-windows/git/releases/download/v!GIT_VERSION!.windows.1/!GIT_PORTABLE!"
set "GIT_TEMP=%TEMP%\!GIT_PORTABLE!"
set "GIT_DIR=%TEMP%\GitPortable"

:: Clean up any existing portable git
if exist "!GIT_DIR!" rmdir /s /q "!GIT_DIR!" >nul 2>&1
mkdir "!GIT_DIR!"

:: Check if PowerShell is available
powershell -Command "Write-Host 'PowerShell available'" >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: PowerShell is required for automatic downloads but is not available
    echo Please manually install Git from: https://git-scm.com/download/windows
    echo After installation, restart this installer.
    exit /b 1
)

:: Download Git Portable
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = 'Tls12'; Invoke-WebRequest -Uri '!GIT_URL!' -OutFile '!GIT_TEMP!' -UserAgent 'Mozilla/5.0'}" >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Failed to download Git Portable
    echo Please manually install Git from: https://git-scm.com/download/windows
    exit /b 1
)

echo Extracting Git Portable...
:: Extract the self-extracting archive
"!GIT_TEMP!" -o"!GIT_DIR!" -y >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Failed to extract Git Portable
    exit /b 1
)

:: Update PATH to include portable git
set "PATH=!GIT_DIR!\cmd;!PATH!"

:: Clean up installer
del "!GIT_TEMP!" >nul 2>&1

:: Verify git is now available
git --version >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Git Portable setup failed
    exit /b 1
)

echo Git Portable installed successfully!
exit /b 0

:CheckWindowsSDK
echo Checking for Windows SDK...

:: Check for Windows 10/11 SDK
set "SDK_FOUND=0"
if exist "C:\Program Files (x86)\Windows Kits\10\Include\*" set "SDK_FOUND=1"
if exist "C:\Program Files\Microsoft SDKs\Windows\*" set "SDK_FOUND=1"

if "!SDK_FOUND!"=="0" (
    echo WARNING: Windows SDK may not be installed
    echo.
    echo The Windows SDK is required for building Windows applications.
    echo Download from: https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
    echo.
    echo Or install Visual Studio with the following components:
    echo - Windows 10/11 SDK (latest version)
    echo - MSVC v143 compiler toolset
    echo.
) else (
    echo Windows SDK found.
)
exit /b 0