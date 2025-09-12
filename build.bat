@echo off
REM Build script for pick6 on Windows

echo Building pick6 - FiveM Frame Projector
echo =====================================

REM Check for cmake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found. Please install CMake and ensure it's in PATH.
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure for Visual Studio (try multiple versions)
echo Configuring build...
cmake .. -G "Visual Studio 17 2022" -A x64 >nul 2>&1
if errorlevel 1 (
    cmake .. -G "Visual Studio 16 2019" -A x64 >nul 2>&1
    if errorlevel 1 (
        cmake .. -G "Visual Studio 15 2017 Win64" >nul 2>&1
        if errorlevel 1 (
            echo ERROR: No compatible Visual Studio found.
            echo Please install Visual Studio 2017, 2019, or 2022 with C++ development tools.
            pause
            exit /b 1
        )
    )
)

REM Build release version
echo Building Release version...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed. Check output above for errors.
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Output: %CD%\bin\Release\pick6.exe
echo.
echo Run pick6.exe as Administrator for best results.
pause