@echo off
echo Compiling OI Simulator GUI...

set INCLUDE_DIRS=/I"imgui" /I"imgui/backends"
set LIBS=d3d11.lib d3dcompiler.lib dxgi.lib

set SOURCES=main.cpp ^
imgui/imgui.cpp ^
imgui/imgui_draw.cpp ^
imgui/imgui_tables.cpp ^
imgui/imgui_widgets.cpp ^
imgui/backends/imgui_impl_win32.cpp ^
imgui/backends/imgui_impl_dx11.cpp

cl /EHsc /std:c++17 /O2 /W3 %INCLUDE_DIRS% %SOURCES% /Fe:output/oi_simulator_gui.exe /link %LIBS%

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Output: output/oi_simulator_gui.exe
) else (
    echo Build failed!
)

pause
