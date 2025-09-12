# pick6 - FiveM Real-Time Frame Projector

A production-quality Windows application that captures and projects FiveM frames in real-time with minimal latency.

## Overview

pick6 is a single-executable Windows application that:
- Detects and injects into FiveM (Direct3D 11) processes at runtime
- Hooks the game's D3D11 Present calls to duplicate rendered frames
- Shares frames via GPU shared textures with keyed mutex synchronization
- Projects frames in real-time to a fullscreen, borderless window on any monitor
- Provides minimal GUI for configuration and diagnostics

## Key Features

- **Zero External Dependencies**: Single `.exe` file with embedded hook DLL
- **High Performance**: GPU-accelerated frame sharing with minimal CPU overhead
- **Multi-Monitor Support**: Select any available display for projection
- **Real-Time Performance**: <16ms latency with 60+ FPS projection
- **Robust Injection**: Automatic FiveM process detection with optional auto-reattach
- **Diagnostic Interface**: Real-time performance metrics and status monitoring

## Requirements

- **Platform**: Windows 10+ x64 only
- **Graphics**: DirectX 11 compatible GPU
- **Target**: FiveM application (process detection by name)
- **Permissions**: Administrator rights may be required for process injection

## Architecture

### Core Components

1. **Main Application** (`pick6.exe`)
   - Win32 GUI with monitor selection and controls
   - Process injection management
   - Frame projection and rendering
   - Real-time diagnostics

2. **Hook DLL** (embedded in executable)
   - D3D11 Present function hooking
   - Frame capture from FiveM render pipeline
   - Shared texture producer

3. **Shared Texture System**
   - GPU-accelerated frame sharing
   - Keyed mutex synchronization
   - Event-driven frame notifications

4. **Frame Projector**
   - D3D11 rendering pipeline
   - Fullscreen borderless window
   - VSync and performance controls

### Technical Implementation

- **Language**: C++17 with DirectX 11
- **Build System**: CMake with MSVC
- **Injection Method**: CreateRemoteThread + LoadLibrary
- **Synchronization**: IDXGIKeyedMutex + Windows Events
- **Rendering**: Hardware-accelerated D3D11 pipeline

## Build Instructions

### Windows (Production Build)

Requirements:
- Visual Studio 2019+ with C++ development tools
- Windows 10 SDK
- CMake 3.20+

```bash
# Clone repository
git clone <repository-url>
cd heyyy

# Create build directory
mkdir build
cd build

# Configure with Visual Studio
cmake .. -G "Visual Studio 16 2019" -A x64

# Build release version
cmake --build . --config Release

# Output: build/bin/Release/pick6.exe
```

### Linux (Syntax Check Only)

For development and syntax validation:

```bash
# Install dependencies
sudo apt-get install build-essential cmake

# Build basic version
mkdir build && cd build
cmake .. 
make

# Note: This creates a stub executable for syntax checking only
# The full application requires Windows and DirectX 11
```

## Usage

1. **Launch pick6.exe** as Administrator (if required)
2. **Select Monitor** from dropdown for projection display  
3. **Configure Options**:
   - Enable/disable VSync for projection window
   - Toggle auto-reattach to monitor FiveM process
4. **Start Capture** to begin injection and projection
5. **Monitor Diagnostics** for performance metrics

### Controls

- **Start/Stop Capture**: Begin/end frame projection
- **Monitor Selection**: Choose display for fullscreen projection  
- **VSync Toggle**: Control projection frame rate limiting
- **Auto-Reattach**: Automatically reconnect if FiveM restarts
- **ESC Key**: Exit projection window

## Performance Targets

- **CPU Overhead**: <2% additional load in FiveM process
- **Memory Usage**: <50MB additional in target process  
- **Latency**: <16ms from capture to projection
- **Frame Rate**: 60+ FPS projection capability
- **GPU Usage**: Minimal additional load via shared textures

## Limitations

- **FiveM Only**: Injection limited to FiveM processes for security
- **DirectX 11**: Only D3D11 games supported (FiveM uses D3D11)
- **Windows Only**: Uses Windows-specific APIs and DirectX
- **Single Target**: One FiveM process at a time

## Security & Safety

- Process injection is limited to FiveM executables only
- No generic process injection capabilities  
- Hook DLL is temporarily extracted and immediately cleaned up
- No network functionality or data transmission
- No recording, encoding, or persistent storage

## Troubleshooting

### Common Issues

1. **"FiveM Not Found"**
   - Ensure FiveM is running before starting capture
   - Check that FiveM process name matches detection patterns

2. **"Injection Failed"**
   - Run pick6.exe as Administrator
   - Disable antivirus temporarily if blocking injection
   - Ensure FiveM is not running with higher privileges

3. **"No Frames Received"**
   - Verify FiveM is using DirectX 11 (not Vulkan/OpenGL)
   - Check that FiveM is actively rendering (not minimized)

4. **Performance Issues**
   - Disable VSync if experiencing frame drops
   - Close other GPU-intensive applications
   - Use dedicated GPU for both FiveM and pick6

### Debug Mode

Enable detailed logging by setting environment variable:
```
set PICK6_DEBUG=1
pick6.exe
```

## Development

### Code Structure

```
src/
├── main.cpp                 # Application entry point
├── application.h/.cpp       # Main GUI and application logic
├── monitor_manager.h/.cpp   # Multi-monitor support
├── injection_manager.h/.cpp # Process injection system
├── frame_projector.h/.cpp   # D3D11 projection renderer
├── shared_texture.h/.cpp    # GPU texture sharing
└── hook/                    # Injected DLL components
    ├── hook_main.cpp        # DLL entry point
    ├── d3d11_hook.h/.cpp    # Present function hooking
    └── frame_capture.h/.cpp # Frame capture implementation
```

### Contributing

1. Ensure changes maintain Windows-only compatibility
2. Test with FiveM specifically (no generic injection)
3. Maintain single-executable distribution
4. Preserve performance characteristics
5. Follow existing C++17 and DirectX patterns

## License

[Specify license here]

## Disclaimer

This software is designed specifically for FiveM frame projection. Users are responsible for compliance with FiveM terms of service and applicable laws. The software performs process injection which may be flagged by security software.
