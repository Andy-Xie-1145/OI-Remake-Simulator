@echo off
REM OI 重开模拟器 GUI 版本编译脚本
REM 需要 Visual Studio 2019 或更高版本

echo ========================================
echo OI 重开模拟器 GUI 编译脚本
echo ========================================
echo.

REM 查找 Visual Studio
set "VS_PATH="
set "VS_EDITION="

REM 检查 VS 2022
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
    set "VS_EDITION=2022 Community"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
    set "VS_EDITION=2022 Professional"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    set "VS_EDITION=2022 Enterprise"
)

REM 检查 VS 2019
if "%VS_PATH%"=="" (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
        set "VS_EDITION=2019 Community"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional"
        set "VS_EDITION=2019 Professional"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise"
        set "VS_EDITION=2019 Enterprise"
    )
)

if "%VS_PATH%"=="" (
    echo [错误] 未找到 Visual Studio！
    echo 请安装 Visual Studio 2019 或更高版本。
    echo 下载地址: https://visualstudio.microsoft.com/downloads/
    pause
    exit /b 1
)

echo [信息] 找到 Visual Studio %VS_EDITION%
echo.

REM 设置 Visual Studio 环境
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

echo.
echo [信息] 开始编译...
echo.

REM 设置编译参数
set INCLUDE_DIRS=/I"imgui" /I"imgui/backends"
set LIBS=d3d11.lib d3dcompiler.lib dxgi.lib user32.lib gdi32.lib

set SOURCES=main.cpp ^
imgui/imgui.cpp ^
imgui/imgui_draw.cpp ^
imgui/imgui_tables.cpp ^
imgui/imgui_widgets.cpp ^
imgui/backends/imgui_impl_win32.cpp ^
imgui/backends/imgui_impl_dx11.cpp

REM 创建输出目录
if not exist "output" mkdir output

REM 编译
cl /EHsc /std:c++17 /O2 /W3 /DUNICODE /D_UNICODE %INCLUDE_DIRS% %SOURCES% /Fe:output/oi_simulator_gui.exe /link %LIBS%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo [成功] 编译完成！
    echo 输出文件: output\oi_simulator_gui.exe
    echo ========================================
) else (
    echo.
    echo [错误] 编译失败！请检查错误信息。
)

REM 清理中间文件
del *.obj 2>nul

pause
