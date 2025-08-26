@echo off
setlocal

echo =================================================
echo  SBORKA I TESTIROVANIE "Stealth Action Game" (WINDOWS)
echo =================================================
echo.

REM --- Proverka zavisimostey ---
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [OSHIBKA] Komanda 'cmake' ne naydena. Pozhaluysta, dobav'te CMake v peremennuyu sredy PATH.
    goto :error
)
echo CMake nayden.

REM --- Podgotovka direktori sborki ---
echo.
echo --- Podgotovka direktori sborki...
if exist build (
    echo Udalenie staroy direktori sborki...
    rmdir /s /q build
)
mkdir build
cd build

REM --- Konfiguratsiya proekta cherez CMake ---
echo.
echo --- Konfiguratsiya proekta s pomoshch'yu CMake...
REM Predpolagaetsya, chto pol'zovatel' uzhe vypolnil komandu 'vcpkg integrate install'.
REM V etom sluchae CMake avtomaticheski naydet nuzhnyy toolchain-fayl ot vcpkg.
cmake ..
if %errorlevel% neq 0 (
    echo [OSHIBKA] Oshibka konfiguratsii CMake.
    echo.
    echo Pozhaluysta, ubedites', chto vy odin raz vypolnili komandu 'vcpkg integrate install'.
    echo Takzhe ubedites', chto vy ustanovili vse neobkhodimye biblioteki:
    echo vcpkg install sfml openal-soft fmt libogg libvorbis gtest --triplet x64-windows
    goto :error
)

REM --- Sborka proekta ---
echo.
echo --- Kompilyatsiya proekta (Release)...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [OSHIBKA] Oshibka kompilyatsii.
    goto :error
)

REM --- Zapusk testov ---
echo.
echo --- Zapusk testov...
REM CTest zapustit vse naydennye testy. Flazhok --output-on-failure ochen' polezen.
ctest --config Release --output-on-failure
if %errorlevel% neq 0 (
    echo [PREDUPREZHDENIE] Odin ili neskol'ko testov provalilis'. Smotrite vyvod vyshe.
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
