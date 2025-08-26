#include <gtest/gtest.h>
#include "../SoundEngine.h"
#include <fstream>
#include <string>
#include <streambuf>

// Helper function to read a whole file into a string
std::string readFileContents(const std::string& path) {
    std::ifstream t(path);
    if (!t) return "";
    return std::string((std::istreambuf_iterator<char>(t)),
                       std::istreambuf_iterator<char>());
}

TEST(ReverbTest, EFXInitialization) {
    // 1. Clear the log file before the test
    std::ofstream log_file("error_log.txt", std::ofstream::out | std::ofstream::trunc);
    log_file.close();

    // 2. Instantiate SoundEngine to trigger initialization
    // We need to run this in a virtual environment for audio to init.
    // This test won't be run directly, but via a script that uses xvfb-run.
    // For now, we just create the object.
    {
        SoundEngine engine;
    }

    // 3. Read the log file
    std::string log_contents = readFileContents("error_log.txt");

    // 4. Assert that the expected log messages are present
    EXPECT_NE(log_contents.find("DEBUG_LOG: EFX extension found. Loading function pointers..."), std::string::npos)
        << "EFX extension check message not found in log.";

    EXPECT_NE(log_contents.find("DEBUG_LOG: EFX reverb effect created and configured."), std::string::npos)
        << "EFX effect creation message not found in log.";

    EXPECT_NE(log_contents.find("DEBUG_LOG: OpenAL initialized successfully."), std::string::npos)
        << "OpenAL success message not found in log.";
}
