@echo off
echo =======================================
echo  BUILDING STEALTH ACTION GAME FOR WINDOWS
echo =======================================
echo.

REM !!! IMPORTANT !!!
REM Set the path to your local SFML installation directory here.
REM This is the directory that contains the 'lib', 'include', etc. subdirectories.
SET SFML_INSTALL_PATH=C:\dev\SFML-3.0.1

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
REM We pass the path to SFML's cmake config files directly to CMake.
cmake .. -G "MinGW Makefiles" -DSFML_DIR="%SFML_INSTALL_PATH%/lib/cmake/SFML"
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed. Check the SFML_INSTALL_PATH variable in this script.
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
