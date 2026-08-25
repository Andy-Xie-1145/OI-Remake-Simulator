@echo off
setlocal enabledelayedexpansion
title OI Simulator Test Build (MinGW)

echo ========================================
echo OI Simulator Test Runner Build (MinGW)
echo ========================================
echo.

set GXX=g++
where %GXX% >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] g++ not found in PATH.
    echo Please install MinGW and ensure g++ is in your PATH.
    pause
    exit /b 1
)

echo [INFO] Found g++
%GXX% --version | findstr /c:"g++"
echo.

set OUTPUT_DIR=output
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

set SOURCES=test_main.cpp test_types.cpp test_game.cpp test_contest.cpp test_engine.cpp test_features.cpp
set INCLUDES=-I.. -I../imgui

set CXXFLAGS=-std=c++17 -Wall -Wextra -O0 -g -static -static-libgcc -static-libstdc++

echo [INFO] Compiling test runner...
%GXX% %CXXFLAGS% %INCLUDES% %SOURCES% -o "%OUTPUT_DIR%\test_runner.exe" 2>&1

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo [INFO] Build succeeded.
echo.
echo [INFO] Running tests...
echo.

"%OUTPUT_DIR%\test_runner.exe" --use-colour yes

echo.
echo [INFO] Tests completed.
pause