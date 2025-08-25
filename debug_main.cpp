#include "SoundEngine.h"
#include <iostream>

int main() {
    try {
        std::cout << "DEBUG: Creating SoundEngine..." << std::endl;
        SoundEngine engine;
        std::cout << "DEBUG: SoundEngine created." << std::endl;
        std::cout << "DEBUG: Calling run()..." << std::endl;
        engine.run();
        std::cout << "DEBUG: run() finished." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "DEBUG: Exception caught in main: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
