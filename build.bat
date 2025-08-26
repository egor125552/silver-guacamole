@echo off
setlocal

echo =================================================
echo  SBORKA I TESTIROVANIE "Stealth Action Game" (WINDOWS - MinGW)
echo =================================================
echo.

REM --- Configuration ---
REM This script is specifically configured for a MinGW g++ environment
REM managed by vcpkg.

REM Set the path to the root of your vcpkg installation.
REM The script will try to find it, but you can set it manually if needed.
if not defined VCPKG_ROOT (
    echo [INFO] VCPKG_ROOT not set. Assuming default location C:\dev\vcpkg
    set "VCPKG_ROOT=C:\dev\vcpkg"
)

REM --- Prerequisite Check ---
set "VCPKG_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if not exist "%VCPKG_TOOLCHAIN_FILE%" (
    echo [OSHIBKA] Vcpkg toolchain file not found at "%VCPKG_TOOLCHAIN_FILE%"
    echo           Please make sure VCPKG_ROOT is set correctly.
    goto :error
)
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [OSHIBKA] 'cmake' not found. Please add CMake to your system PATH.
    goto :error
)
where mingw32-make >nul 2>nul
if %errorlevel% neq 0 (
    echo [OSHIBKA] 'mingw32-make' not found. Please ensure MinGW\bin is in your system PATH.
    goto :error
)

echo Using vcpkg root: %VCPKG_ROOT%
echo.

REM --- Build Steps ---
echo --- Preparing Build Directory...
if exist build rmdir /s /q build
mkdir build
cd build

echo.
echo --- Running CMake Configuration for MinGW...
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET=x64-mingw-static
if %errorlevel% neq 0 (
    echo [OSHIBKA] CMake configuration failed.
    echo           Please check your vcpkg installation and that you have run:
    echo           vcpkg install sfml openal-soft fmt libogg libvorbis gtest --triplet x64-mingw-static
    goto :error
)

echo.
echo --- Compiling project with MinGW Make...
mingw32-make
if %errorlevel% neq 0 (
    echo [OSHIBKA] Compilation failed.
    goto :error
)

echo.
echo --- Running tests...
ctest --output-on-failure
if %errorlevel% neq 0 (
    echo [WARNING] One or more tests failed.
    goto :error_but_built
)

echo.
echo =======================================
echo  SBORKA I TESTY USPESHNO ZAVERSHENY!
echo =======================================
cd ..
goto :end

:error_but_built
echo.
echo =======================================
echo  SBORKA PROSHLA USPESHNO, NO TESTY PROVALILIS'.
echo =======================================
cd ..
goto :end

:error
echo.
echo =======================================
echo           SBORKA PROVALILAS'
echo =======================================
if exist build (
    cd ..
)

:end
echo.
pause
