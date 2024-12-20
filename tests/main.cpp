#define CATCH_CONFIG_MAIN 
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>

#include "Wav.hpp"

TEST_CASE("Binary read") {
    std::string filePath = "tests/files/48000Hz_32bit_float_1ch.wav";

    // Read file into binary array
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    REQUIRE(file.is_open());

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> binaryArray(size);
    REQUIRE(file.read(binaryArray.data(), size));

    // Read file into ifstream
    Wav::FileDescriptor descriptor;
    std::ifstream fileStream(filePath, std::ios::binary);
    Wav::infer(fileStream, descriptor);

    // Write to binary ostream
    std::ostringstream byteStream;
    auto data = std::vector<float>(descriptor.sampleCount);
    Wav::write(byteStream, descriptor.sampleRate, data);

    // Compare binary array to ostream content
    std::string output = byteStream.str();
    REQUIRE(output.size() == binaryArray.size());
    for(std::size_t i = 0; i < output.size(); i++) {
        REQUIRE(output[i] == binaryArray[i]);   
    }
}
