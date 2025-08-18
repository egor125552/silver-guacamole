#include <iostream>
#include <fstream>
#include <iomanip>

int main() {
    std::ifstream file("sound.wav", std::ios::binary);
    if (!file) {
        std::cerr << "Не удалось открыть файл sound.wav" << std::endl;
        return -1;
    }

    // Читаем и выводим первые 64 байта для анализа
    char buffer[64];
    file.read(buffer, 64);
    
    std::cout << "Первые 64 байта файла:" << std::endl;
    for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) std::cout << std::endl << std::setw(3) << i << ": ";
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)(unsigned char)buffer[i] << " ";
    }
    std::cout << std::endl << std::endl;
    
    // Показываем ASCII представление
    std::cout << "ASCII представление:" << std::endl;
    for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) std::cout << std::endl << std::setw(3) << i << ": ";
        char c = buffer[i];
        if (c >= 32 && c <= 126) std::cout << c;
        else std::cout << ".";
    }
    std::cout << std::endl;
    
    // Проверяем сигнатуры
    if (buffer[0] == 'R' && buffer[1] == 'I' && buffer[2] == 'F' && buffer[3] == 'F') {
        std::cout << std::endl << "Файл имеет RIFF сигнатуру - правильно" << std::endl;
    } else {
        std::cout << std::endl << "Файл НЕ имеет RIFF сигнатуру" << std::endl;
    }
    
    if (buffer[8] == 'W' && buffer[9] == 'A' && buffer[10] == 'V' && buffer[11] == 'E') {
        std::cout << "Файл имеет WAVE сигнатуру - правильно" << std::endl;
    } else {
        std::cout << "Файл НЕ имеет WAVE сигнатуру" << std::endl;
    }
    
    return 0;
}