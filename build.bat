@echo off
echo Building FiveM Capture System...

if not exist build mkdir build
cd build

echo Configuring CMake...
cmake .. -G "Visual Studio 16 2019" -A x64
if errorlevel 1 (
    echo CMake configuration failed!
    echo Trying with default generator...
    cmake ..
    if errorlevel 1 (
        echo CMake configuration failed with default generator too!
        pause
        exit /b 1
    )
)

echo Building project...
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo Executable should be in: build\bin\Release\FiveMCapture.exe
echo.
echo Note: This project requires Windows and will not function on other platforms.
echo The DirectX hooks will only work when FiveM (or other D3D11 applications) are running.
echo.
pause