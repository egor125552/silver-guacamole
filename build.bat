@echo off
echo =======================================
echo  BUILDING STEALTH ACTION GAME FOR WINDOWS
echo =======================================
echo.

REM !!! IMPORTANT !!!
REM Set the path to the root of your vcpkg installation directory.
REM This is the directory that contains 'installed', 'scripts', etc.
SET VCPKG_ROOT=C:\dev\vcpkg

REM Set the path to your MinGW bin directory if it's not in your system's PATH
REM Example: SET MINGW_PATH=C:\msys64\mingw64\bin
REM SET PATH=%MINGW_PATH%;%PATH%

echo --- Deleting old build directory...
if exist build (
    rmdir /s /q build
    if errorlevel 1 (
        echo Failed to delete build directory. It might be in use.
        goto :error
    )
)

echo --- Creating build directory...
mkdir build
if not exist build (
    echo Failed to create build directory.
    goto :error
)
cd build

echo --- Configuring project with CMake...
REM We point CMake to the vcpkg toolchain file.
REM This automatically finds all libraries installed with vcpkg.
SET VCPKG_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%"
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed.
    echo Check that VCPKG_ROOT is set correctly in this script.
    echo Also ensure you have run 'vcpkg integrate install'.
    goto :error
)

echo --- Compiling project with mingw32-make...
REM You can add -j options to speed up compilation, e.g., mingw32-make -j8
mingw32-make
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Compilation failed.
    goto :error
)

echo.
echo =======================================
echo  BUILD SUCCESSFUL!
echo =======================================
echo Executable is in the 'build' directory.
cd ..
goto :end

:error
echo.
echo =======================================
echo  BUILD FAILED
echo =======================================
cd ..

:end
echo.
pause
