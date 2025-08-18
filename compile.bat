@echo off
echo Компилирую Stealth Action...
g++ -std=c++17 -O2 -Wall -DSFML_STATIC -I. -I"C:\dev\vcpkg\installed\x64-mingw-static\include" -L"C:\dev\vcpkg\installed\x64-mingw-static\lib" main.cpp SoundEngine.cpp entities.cpp -o StealthAction.exe -lsfml-graphics-s -lsfml-window-s -lsfml-audio-s -lsfml-system-s -lopengl32 -lfreetype -lwinmm -lgdi32 -lopenal32 -lflac -lvorbisenc -lvorbisfile -lvorbis -logg -lpthread
echo.
echo Компиляция завершена.
echo Если ошибок не было, StealthAction.exe готов к запуску.
pause