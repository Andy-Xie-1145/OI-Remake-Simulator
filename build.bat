@echo off
REM OI Simulator GUI Build Script

echo ========================================
echo OI Simulator GUI Build Script
echo ========================================
echo.

REM Check VS 2022 Community
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    goto :build
)

REM Check VS 2022 Professional
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    goto :build
)

REM Check VS 2019 Community
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    goto :build
)

REM Check VS 2019 Professional
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
    goto :build
)

echo [ERROR] Visual Studio not found!
echo Please install Visual Studio 2019 or later.
pause
exit /b 1

:build
echo.
echo [INFO] Compiling...
echo.

if not exist "output" mkdir output

set SOURCES=main.cpp imgui\imgui.cpp imgui\imgui_draw.cpp imgui\imgui_tables.cpp imgui\imgui_widgets.cpp imgui\backends\imgui_impl_win32.cpp imgui\backends\imgui_impl_dx11.cpp

cl /EHsc /std:c++17 /O2 /W3 /DUNICODE /D_UNICODE /I"imgui" /I"imgui/backends" %SOURCES% /Fe:output\oi_simulator_gui.exe /link d3d11.lib d3dcompiler.lib dxgi.lib user32.lib gdi32.lib

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

del *.obj 2>nul

pause
