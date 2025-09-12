cmake_minimum_required(VERSION 3.20)

project(pick6 VERSION 1.0.0 LANGUAGES CXX)

# Target Windows 10+ x64 only
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_GENERATOR_PLATFORM x64)

# C++17 for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(NOT WIN32)
    message(WARNING "This project is designed for Windows x64. Building on other platforms for syntax checking only.")
    # Add dummy libraries for non-Windows platforms
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX_BUILD")
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Main executable
if(WIN32)
    add_executable(pick6
        src/main.cpp
        src/application.cpp
        src/application.h
        src/monitor_manager.cpp
        src/monitor_manager.h
        src/injection_manager.cpp
        src/injection_manager.h
        src/frame_projector.cpp
        src/frame_projector.h
        src/shared_texture.cpp
        src/shared_texture.h
        src/shared_memory.cpp
        src/shared_memory.h
        src/resource.rc
    )
else()
    # Simplified Linux build for syntax checking - just main with stubs
    add_executable(pick6
        src/main.cpp
    )
endif()

# Hook DLL (will be embedded into main executable)
if(WIN32)
    add_library(pick6_hook SHARED
        src/hook/hook_main.cpp
        src/hook/d3d11_hook.cpp
        src/hook/d3d11_hook.h
        src/hook/frame_capture.cpp
        src/hook/frame_capture.h
        src/shared_texture.cpp
        src/shared_texture.h
        src/shared_memory.cpp
        src/shared_memory.h
    )
endif()

# Link Windows libraries for main executable
if(WIN32)
    target_link_libraries(pick6 PRIVATE
        user32
        kernel32
        gdi32
        d3d11
        dxgi
        comctl32
        shell32
        advapi32
    )
endif()

# Link libraries for hook DLL
if(WIN32)
    target_link_libraries(pick6_hook PRIVATE
        d3d11
        dxgi
        kernel32
        user32
    )
endif()

# Embed hook DLL as resource in main executable
if(WIN32)
    add_custom_command(TARGET pick6_hook POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pick6_hook> ${CMAKE_CURRENT_BINARY_DIR}/pick6_hook.dll
        COMMENT "Copying hook DLL for embedding"
    )

    # Generate resource file to embed the hook DLL
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hook_resource.rc
        DEPENDS pick6_hook
        COMMAND ${CMAKE_COMMAND} -E echo "HOOK_DLL RCDATA \"${CMAKE_CURRENT_BINARY_DIR}/pick6_hook.dll\"" > ${CMAKE_CURRENT_BINARY_DIR}/hook_resource.rc
        COMMENT "Generating resource file for hook DLL"
    )

    target_sources(pick6 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/hook_resource.rc)
endif()

# Set subsystem to Windows (GUI application)
if(WIN32)
    set_target_properties(pick6 PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "pick6"
    )

    set_target_properties(pick6_hook PROPERTIES
        OUTPUT_NAME "pick6_hook"
    )
else()
    set_target_properties(pick6 PROPERTIES
        OUTPUT_NAME "pick6"
    )
endif()

# Compiler flags for optimization and debugging
target_compile_definitions(pick6 PRIVATE
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
    NOMINMAX
)

if(WIN32)
    target_compile_definitions(pick6_hook PRIVATE
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
        NOMINMAX
    )
endif()

# Enable parallel compilation
if(MSVC)
    target_compile_options(pick6 PRIVATE /MP)
    if(WIN32)
        target_compile_options(pick6_hook PRIVATE /MP)
    endif()
endif()