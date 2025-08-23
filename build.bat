@echo off
REM This script builds the StealthActionGame project using CMake and MinGW (G++).
REM It requires vcpkg to be installed at C:\dev\vcpkg.

ECHO =======================================
ECHO  BUILDING STEALTH ACTION GAME
ECHO =======================================

REM Set the path to your vcpkg installation
SET VCPKG_ROOT=C:\dev\vcpkg
SET CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

REM Check if the toolchain file exists
IF NOT EXIST "%CMAKE_TOOLCHAIN_FILE%" (
    ECHO ERROR: vcpkg toolchain file not found at "%CMAKE_TOOLCHAIN_FILE%"
    ECHO Please make sure vcpkg is installed at "%VCPKG_ROOT%"
    PAUSE
    EXIT /B 1
)

REM Clean up previous build
IF EXIST build (
    ECHO "--- Removing previous build directory..."
    RMDIR /S /Q build
)

ECHO "--- Creating build directory..."
MKDIR build
CD build

ECHO "--- Configuring project with CMake..."
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-mingw-static

REM Check if CMake configuration was successful
IF %ERRORLEVEL% NEQ 0 (
    ECHO "CMake configuration failed."
    PAUSE
    EXIT /B 1
)

ECHO "--- Compiling project..."
cmake --build .

REM Check if compilation was successful
IF %ERRORLEVEL% NEQ 0 (
    ECHO "Compilation failed."
    PAUSE
    EXIT /B 1
)

ECHO "--- Build successful!"
ECHO The executable is located in the 'build' directory.
ECHO You will need to copy 'config.ini' and the 'sounds' folder next to the .exe to run it.
PAUSE
