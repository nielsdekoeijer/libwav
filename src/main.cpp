#include "wav.hpp"
#include <fstream>
#include <iostream>
#include <vector>

// test program that uses wav infer to see what a file is all about...
int main(int argv, char** argc)
{
    Wav::FileDescriptor descriptor;
    if (argv > 1) {
        std::ifstream file(argc[1], std::ios::binary);
        if (!file) {
            throw std::runtime_error("failed to open file at " + std::string(argc[1]));
        }
        Wav::infer(file, descriptor);
        std::cout << "Sample Rate: " << descriptor.sampleRate << std::endl;
        std::cout << "Sample Count: " << descriptor.sampleCount << " samples" << std::endl;
        std::cout << "Sample Count: " << float(descriptor.sampleCount) * (1.0 / float(descriptor.sampleRate)) << " seconds"
                  << std::endl;
        std::cout << "Channel Count: " << descriptor.channelCount << std::endl;
        std::cout << "File offset: " << descriptor.dataOffset << std::endl;

        std::cout << "attempting to read..." << std::endl;
        auto x = std::vector<float>(50000);
        auto y = std::vector<float>(50000);
        Wav::read(file, x, y);
        std::ofstream ofile("out.wav", std::ios::binary);
        if (!ofile) {
            throw std::runtime_error("failed to open file at " + std::string(argc[1]));
        }
        for (std::size_t i = 0; i < 50000; i++) {
            std::cout << x[i] << std::endl;
            std::cout << y[i] << std::endl;
        }

        Wav::write(ofile, descriptor.sampleRate, x, y);
    }
}
