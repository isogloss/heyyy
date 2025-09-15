@echo off
echo =================================================================
echo        FiveM Capture System - Installer Test Script  
echo =================================================================
echo.
echo This script tests the standalone installer in a safe environment.
echo It will create a temporary directory and test the full installation
echo process without affecting your system.
echo.
echo WARNING: This will attempt to download dependencies and may take
echo several minutes to complete.
echo.

set /p "CONFIRM=Do you want to proceed with the installer test? (y/n): "
if /i "%CONFIRM%" neq "y" (
    echo Test cancelled by user.
    pause
    exit /b 1
)

echo.
echo Starting installer test...

:: Create a temporary test environment  
set "TEST_DIR=%TEMP%\FiveMCapture_InstallTest_%RANDOM%"
mkdir "%TEST_DIR%"
if %errorlevel% neq 0 (
    echo ERROR: Failed to create test directory
    pause
    exit /b 1
)

:: Copy the installer to the test directory
copy "standalone_installer.bat" "%TEST_DIR%\standalone_installer.bat" >nul
if %errorlevel% neq 0 (
    echo ERROR: Failed to copy installer to test directory
    rmdir /s /q "%TEST_DIR%" >nul 2>&1
    pause
    exit /b 1
)

echo Test directory created: %TEST_DIR%
echo Running standalone installer...
echo.

:: Run the installer in the test directory
cd /d "%TEST_DIR%"
call standalone_installer.bat

echo.
echo =================================================================
echo                    INSTALLER TEST COMPLETE
echo =================================================================
echo.
echo Test directory: %TEST_DIR%
echo.
echo If the test was successful, you should find:
echo - FiveMCapture.exe in your Downloads folder
echo - Hidden source files in your AppData directory
echo.

set /p "CLEANUP=Do you want to clean up the test directory? (y/n): "
if /i "%CLEANUP%"=="y" (
    cd /d %TEMP%
    rmdir /s /q "%TEST_DIR%" >nul 2>&1
    echo Test directory cleaned up.
)

echo.
echo Test complete!
pause