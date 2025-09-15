# FiveM Capture System

A high-performance game capture system specifically designed for FiveM that hooks into DirectX 11/DXGI rendering pipeline to capture frames directly from the game without capturing the desktop or interfering with other applications.

## Features

- **DirectX 11/DXGI Hooking**: Uses Microsoft Detours to intercept Present() and other DirectX calls
- **High-Performance Capture**: GPU-to-GPU texture sharing for minimal performance impact
- **Borderless Fullscreen Projection**: Dedicated projection window with fullscreen support
- **Real-time UI Controls**: ImGui-based interface for configuration and monitoring
- **Performance Monitoring**: Built-in FPS tracking, drop rate monitoring, and latency measurement
- **Production Ready**: Full error handling, thread synchronization, and resource management

## Requirements

- Windows 10/11 (64-bit)
- DirectX 11 compatible GPU
- Visual Studio 2019/2022 or compatible C++20 compiler
- CMake 3.16 or later

## Dependencies

All dependencies are automatically downloaded and configured during build:

- **Microsoft Detours**: Function hooking library
- **Dear ImGui**: Immediate mode GUI library
- **DirectX 11 SDK**: Graphics API (included with Windows SDK)

## Building

### Standalone Installer (Recommended for Distribution)

**🚀 NEW: Complete standalone installer for end users!**

```batch
# For distribution - downloads source automatically:
standalone_installer.bat
```

**What this does:**
- Downloads source code from GitHub automatically
- Installs source to a protected hidden system directory 
- Configures and builds the project with all dependencies
- Places the final FiveMCapture.exe in your Downloads folder
- Provides complete installation without exposing source code
- Works as a single distributable file

### One-Click Setup (For Developers)

**🚀 NEW: Complete automated build and install!**

```batch
# Simple one-click solution:
build_and_install.bat
```

This script automatically:
- Downloads all dependencies
- Builds the project 
- Places FiveMCapture.exe in your Downloads folder
- Handles all errors with helpful messages

### Manual Setup

```bash
# Clone the repository
git clone <repository-url>
cd heyyy

# Run setup script (downloads dependencies)
./setup.sh

# Build the project
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Manual Setup

1. **Download Dependencies**:
   - Microsoft Detours: Download from Microsoft and extract to `third_party/Detours/`
   - Dear ImGui: Clone from https://github.com/ocornut/imgui to `third_party/imgui/`

2. **Build**:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

## Usage

### Quick Start

1. **Launch the Application**:
   ```bash
   cd build/bin
   ./FiveMCapture.exe
   ```

2. **Start FiveM**: Launch your FiveM game instance

3. **Enable Capture**: The system will automatically hook into FiveM's rendering pipeline

4. **Control Interface**: Press **F1** to toggle the control UI

### Controls

- **F1**: Toggle control interface visibility
- **F11**: Toggle fullscreen projection (when projection window is active)
- **ESC**: Hide projection window

### Configuration

Use the ImGui interface to configure:

- **Capture Settings**: Enable/disable capture, adjust target FPS
- **Projection Settings**: Toggle fullscreen mode, show/hide projection window
- **Performance Monitoring**: View real-time statistics and FPS graphs

## Architecture

### Core Components

1. **DXGIHook**: Microsoft Detours-based hooking system
   - Intercepts `IDXGISwapChain::Present()`
   - Intercepts `IDXGISwapChain::ResizeBuffers()`
   - Thread-safe hook management

2. **FrameCapture**: High-performance frame capturing
   - Shared texture creation for GPU-to-GPU transfer
   - Frame queue with proper synchronization
   - Performance metrics collection

3. **ProjectionWindow**: Borderless display window
   - DirectX 11 rendering pipeline
   - Fullscreen/windowed mode support
   - DWM integration for seamless display

4. **UI**: ImGui-based control interface
   - Real-time configuration controls
   - Performance monitoring dashboard
   - Tabbed interface design

### Thread Safety

- All shared resources protected with mutexes
- Lock-free frame queue for minimal latency
- Atomic operations for performance counters

### Performance Optimizations

- GPU-to-GPU texture copying (no CPU readback)
- Shared texture resources to minimize memory usage
- Frame dropping when queue is full (no blocking)
- VSync-aware presentation timing

## Technical Details

### Hooking Mechanism

The system uses Microsoft Detours to intercept DirectX function calls:

```cpp
// Hook installation
DetourTransactionBegin();
DetourUpdateThread(GetCurrentThread());
DetourAttach(&OriginalPresent, HookedPresent);
DetourTransactionCommit();
```

### Frame Capture Pipeline

1. **Interception**: `Present()` call intercepted
2. **Texture Extraction**: Back buffer extracted from swap chain
3. **GPU Copy**: Texture copied to shared resource
4. **Queue Management**: Frame added to lock-free queue
5. **Projection**: Latest frame rendered to projection window

### Resource Management

- Automatic cleanup on application shutdown
- Resource recreation on resolution changes
- Memory leak prevention with RAII patterns
- Exception safety throughout codebase

## Compatibility

### Tested Configurations

- **FiveM**: Latest stable versions
- **Graphics**: NVIDIA GTX/RTX series, AMD RX series
- **Windows**: Windows 10 version 1903+, Windows 11
- **DirectX**: DirectX 11 feature level 11.0+

### Known Limitations

- Requires DirectX 11 (no DirectX 12 support yet)
- Windows-only (no Linux/Mac support)
- May not work with some anti-cheat systems

## Troubleshooting

### Common Issues

1. **"Failed to initialize DirectX 11"**
   - Ensure graphics drivers are up to date
   - Verify DirectX 11 is available on system

2. **"Failed to setup DirectX hooks"**
   - Run as administrator if necessary
   - Check anti-virus isn't blocking the application

3. **Low capture FPS**
   - Reduce target FPS in settings
   - Close other graphics-intensive applications

### Performance Tips

- Use dedicated GPU (not integrated graphics)
- Close unnecessary background applications
- Ensure adequate VRAM availability
- Use fullscreen mode for best performance

## Development

### Building from Source

Requirements:
- Visual Studio 2019/2022 with C++20 support
- Windows 10 SDK (latest)
- CMake 3.16+

### Code Structure

```
src/
├── FiveMCapture.h/cpp    # Main application class
├── DXGIHook.h/cpp        # DirectX hooking system
├── FrameCapture.h/cpp    # Frame capture pipeline
├── ProjectionWindow.h/cpp # Display window management
└── UI.h/cpp              # ImGui interface
```

### Contributing

1. Fork the repository
2. Create feature branch
3. Implement changes with tests
4. Submit pull request

## License

This project is licensed under the MIT License - see LICENSE file for details.

## Disclaimer

This software is for educational and legitimate use only. Users are responsible for ensuring compliance with FiveM terms of service and applicable laws. The developers are not responsible for any misuse of this software.
