cmake_minimum_required(VERSION 3.15)
project(OI_Simulator_GUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows specific
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# ImGui source files
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx11.cpp
)

# Game source files
set(GAME_SOURCES
    main.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)

# Create executable
add_executable(oi_simulator_gui WIN32 ${GAME_SOURCES} ${IMGUI_SOURCES})

# Link libraries for Windows
if(WIN32)
    target_link_libraries(oi_simulator_gui d3d11 d3dcompiler dxgi)
endif()

# Output directory
set_target_properties(oi_simulator_gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output
)
