#pragma once

#include <string>

// Declares a helper function to read a whole file into a string.
// The definition is in test_utils.cpp.
std::string readFileContents(const std::string& path);

// Declares a helper function to count occurrences of a substring.
// The definition is in test_utils.cpp.
int countOccurrences(const std::string& text, const std::string& sub);
