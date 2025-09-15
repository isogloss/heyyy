# Build Instructions

## Windows (Primary Platform)

### Prerequisites
- Windows 10/11 (64-bit)
- Visual Studio 2019/2022 with C++ support
- CMake 3.16 or later
- Git (for dependency download)

### 🚀 Standalone Installer (Recommended for Distribution)
```batch
# Complete standalone solution - downloads source and builds automatically!
standalone_installer.bat
```

**What this script does:**
- Downloads source code from GitHub automatically
- Installs source to a protected hidden system directory
- Downloads all dependencies automatically
- Configures and builds the project
- Places the final FiveMCapture.exe in your Downloads folder
- Provides clear error messages and troubleshooting help
- **Perfect for distribution - only this single file needs to be shared**

### 🚀 One-Click Build & Install (For Developers)
```batch
# Complete automated solution - does everything for you!
build_and_install.bat
```

**What this script does:**
- Downloads all dependencies automatically
- Configures and builds the project
- Places the final FiveMCapture.exe in your Downloads folder
- Provides clear error messages and troubleshooting help

### Manual Build Process
```batch
# 1. Clone repository
git clone <repository-url>
cd heyyy

# 2. Run automated setup
setup.bat

# 3. Build project
build.bat
```

### Manual Build Process
```batch
# 1. Setup dependencies
setup.bat

# 2. Configure CMake
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64

# 3. Build
cmake --build . --config Release
```

### Expected Output
- Executable: `build/bin/Release/FiveMCapture.exe`
- Libraries: `build/lib/Release/`

## Linux/macOS (Development/Testing Only)

This project is Windows-specific and will not function on other platforms. However, you can verify code structure and CMake configuration:

```bash
# This will fail with "This project requires Windows" - expected behavior
./build.sh
```

## Dependencies

### Automatic (Recommended)
- Run `setup.bat` (Windows) or `setup.sh` (Linux/macOS)
- Dependencies are downloaded automatically

### Manual
1. **Microsoft Detours**
   - Download from: https://github.com/Microsoft/Detours
   - Extract to: `third_party/Detours/`

2. **Dear ImGui**
   - Clone from: https://github.com/ocornut/imgui
   - Clone to: `third_party/imgui/`

## Production Deployment

### Important Notes
1. **Microsoft Detours License**: Ensure compliance with Microsoft's license terms
2. **Antivirus Software**: May flag hook-based applications as suspicious
3. **Administrator Rights**: May be required for DirectX hooking
4. **FiveM Compatibility**: Test with current FiveM versions

### Performance Considerations
- Dedicated GPU recommended
- Adequate VRAM (2GB+ for 1080p, 4GB+ for 1440p)
- Close unnecessary background applications
- Use exclusive fullscreen mode for best performance

## Troubleshooting

### Build Issues
- **CMake not found**: Install CMake and add to PATH
- **Visual Studio not detected**: Install Visual Studio with C++ workload
- **Missing DirectX SDK**: Install Windows 10/11 SDK

### Runtime Issues
- **DirectX initialization failed**: Update graphics drivers
- **Hooks not working**: Run as Administrator
- **Poor performance**: Check GPU memory usage

### FiveM-Specific
- **No capture**: Ensure FiveM is using DirectX 11 mode
- **Crashes**: Check FiveM integrity, disable overlays
- **Detection**: Some anti-cheat systems may detect hooks