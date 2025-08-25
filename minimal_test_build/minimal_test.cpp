#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
    std::cout << "--- Minimal SFML Test ---" << std::endl;
    std::cout << "Attempting to create sf::RenderWindow..." << std::endl;

    try {
        sf::RenderWindow window(sf::VideoMode({200, 200}), "Minimal Test");
        std::cout << "SUCCESS: sf::RenderWindow created." << std::endl;

        // We don't need a loop, just to see if creation works.
        // But let's close it cleanly.
        window.close();
        std::cout << "Window closed." << std::endl;

    } catch (const sf::Exception& e) {
        std::cerr << "ERROR: SFML Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Generic exception caught: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Test finished successfully." << std::endl;
    return 0;
}
