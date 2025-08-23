#include "SoundEngine.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

int main() {
    try {
        SoundEngine engine;
        engine.run();
    } catch (const std::exception& e) {
        std::string errorMessage = "КРИТИЧЕСКАЯ ОШИБКА: " + std::string(e.what());
        logError(errorMessage);
        #ifdef _WIN32
        MessageBoxA(NULL, e.what(), "Критическая ошибка", MB_OK | MB_ICONERROR);
        #endif
        return 1;
    }
    return 0;
}