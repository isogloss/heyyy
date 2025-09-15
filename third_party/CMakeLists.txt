# Microsoft Detours
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Detours")
    message(STATUS "Detours not found, will be downloaded during build")
endif()

# Dear ImGui
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_dx11.cpp
    imgui/backends/imgui_impl_win32.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC imgui imgui/backends)
target_link_libraries(imgui d3d11 dxgi)

# Detours library (will be linked externally)
add_library(detours INTERFACE)
target_link_libraries(detours INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Detours/lib.X64/detours.lib")
target_include_directories(detours INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Detours/include")