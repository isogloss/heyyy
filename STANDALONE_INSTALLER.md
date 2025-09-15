# Standalone Installer for FiveM Capture System

## Overview

The standalone installer provides a single-file distribution solution for the FiveM Capture System. This allows you to distribute only the installer script without providing access to the source code repository.

## Files

- **`standalone_installer.bat`** - Windows standalone installer
- **`standalone_installer.sh`** - Linux/Unix standalone installer (for testing only)

## Features

### Source Code Protection
- Downloads source code from GitHub to a hidden system directory
- Installs source in a deep folder structure for protection: 
  - Windows: `%APPDATA%\Microsoft\Windows\SystemData\.cache\FiveMCapture_System_[RANDOM]\`
  - Linux/Unix: `$HOME/.local/share/FiveMCapture_System_[RANDOM]/`
- Sets hidden file attributes on Windows
- Uses random directory names to prevent easy discovery

### Automated Build Process
- Downloads all dependencies automatically (Dear ImGui, Microsoft Detours placeholder)
- Configures CMake with appropriate Visual Studio generators
- Compiles the project in Release mode
- Places final executable in user's Downloads folder
- Cleans up temporary build files

### Distribution Benefits
- **Single file distribution**: Only the installer script needs to be shared
- **No source code exposure**: Users never see or have access to source files
- **Automated setup**: No manual dependency management required
- **Error handling**: Clear error messages and troubleshooting guidance

## Usage

### For End Users (Windows)

1. Download `standalone_installer.bat`
2. Run the script (may require administrator privileges)
3. Wait for the installation to complete (5-10 minutes)
4. Find `FiveMCapture.exe` in your Downloads folder

### For Developers (Testing)

The Linux/Unix version can be used to test the download and dependency resolution:

```bash
chmod +x standalone_installer.sh
./standalone_installer.sh
```

Note: The build will fail on non-Windows systems as expected, but you can verify the source download and dependency setup.

## Security Considerations

### Source Protection
- Source code is installed in hidden system directories
- Directory names include random components
- On Windows, hidden file attributes are set
- Installation creates a log file for tracking

### Licensing Compliance
- Uses Microsoft Detours placeholder for licensing compliance
- Real Detours library should be obtained from Microsoft for production
- Dear ImGui is downloaded from official repository

## Troubleshooting

### Common Issues

1. **Administrator privileges required**
   - Some steps may need admin rights on Windows
   - Run the script as Administrator if issues occur

2. **Antivirus interference**
   - Some antivirus software may flag the compiled executable
   - Add exclusions if necessary

3. **Build failures**
   - Ensure Visual Studio 2019/2022 with C++ support is installed
   - Verify Windows SDK is available
   - Check that CMake 3.16+ is installed

### Installation Logs

The installer creates logs at:
- Windows: `%APPDATA%\Microsoft\Windows\SystemData\.cache\FiveMCapture_System_[RANDOM]\install_record.txt`
- Linux/Unix: `$HOME/.local/share/FiveMCapture_System_[RANDOM]/install_record.txt`

## Development Notes

### Customization
You can modify the installer to:
- Change the hidden installation directory
- Add additional security measures
- Modify build configurations
- Add custom post-installation steps

### Testing
Before distribution, test the installer on a clean Windows system to ensure:
- All dependencies are properly handled
- Build process completes successfully
- Final executable works as expected
- Source code remains protected

## Distribution

For distribution, you only need to share:
- `standalone_installer.bat` (Windows users)
- `standalone_installer.sh` (Linux/Unix testing)

The installer handles everything else automatically, including source code download, dependency management, compilation, and installation.