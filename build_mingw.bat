@echo off
REM OI Simulator GUI Build Script (MinGW-w64)

echo ========================================
echo OI Simulator GUI Build Script (MinGW)
echo ========================================
echo.

REM Check for g++
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] g++ not found!
    echo.
    echo Please install MinGW-w64:
    echo   1. Download from: https://github.com/niXman/mingw-builds-binaries/releases
    echo   2. Or use MSYS2: pacman -S mingw-w64-x86_64-gcc
    echo   3. Add bin folder to PATH
    pause
    exit /b 1
)

echo [INFO] Found g++
g++ --version
echo.

if not exist "output" mkdir output

echo [INFO] Compiling...

set CXXFLAGS=-std=c++17 -O2 -mwindows -DUNICODE -D_UNICODE
set INCLUDES=-I"imgui" -I"imgui/backends"
set SOURCES=main.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_win32.cpp imgui/backends/imgui_impl_dx11.cpp
set LIBS=-ld3d11 -ld3dcompiler -ldxgi -luser32 -lgdi32 -limm32 -ldwmapi

g++ %CXXFLAGS% %INCLUDES% %SOURCES% -o output/oi_simulator_gui.exe %LIBS%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo [SUCCESS] Build complete!
    echo Output: output\oi_simulator_gui.exe
    echo ========================================
) else (
    echo.
    echo [ERROR] Build failed!
)

pause
