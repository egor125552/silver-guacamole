@echo off
echo =======================================
echo  BUILDING STEALTH ACTION GAME FOR WINDOWS
echo =======================================
echo.

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
REM The "MinGW Makefiles" generator must be specified on Windows.
cmake .. -G "MinGW Makefiles"
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed.
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
