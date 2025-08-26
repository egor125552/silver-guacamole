#include "test_utils.h"
#include <fstream>
#include <streambuf>

// Definition for the helper function to read a whole file into a string.
std::string readFileContents(const std::string& path) {
    std::ifstream t(path);
    if (!t) return "";
    return std::string((std::istreambuf_iterator<char>(t)),
                       std::istreambuf_iterator<char>());
}

// Definition for the helper function to count occurrences of a substring.
int countOccurrences(const std::string& text, const std::string& sub) {
    int count = 0;
    size_t pos = 0;
    while ((pos = text.find(sub, pos)) != std::string::npos) {
        count++;
        pos += sub.length();
    }
    return count;
}
