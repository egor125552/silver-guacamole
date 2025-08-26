@echo off
echo =======================================
echo  BUILDING STEALTH ACTION GAME
echo =======================================

REM Set the path to your MinGW bin directory if it's not in your system's PATH
REM SET MINGW_PATH=C:\msys64\mingw64\bin
REM SET PATH=%MINGW_PATH%;%PATH%

echo --- Deleting old build directory...
if exist build (
    rmdir /s /q build
)

echo --- Creating build directory...
mkdir build
if not exist build (
    echo Failed to create build directory.
    goto :error
)
cd build

echo --- Configuring project with CMake...
REM The "MinGW Makefiles" generator must be specified on Windows.
cmake .. -G "MinGW Makefiles"
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    goto :error
)

echo --- Compiling project...
REM Using mingw32-make. You can add -j8 or -j16 to speed up compilation on multi-core CPUs.
mingw32-make
if %errorlevel% neq 0 (
    echo Compilation failed.
    goto :error
)

echo.
echo =======================================
echo  BUILD SUCCESSFUL!
echo =======================================
echo Executable is in the build/ directory.
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
