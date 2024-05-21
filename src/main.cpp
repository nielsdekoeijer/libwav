#include "wav.hpp"
#include <iostream>
#include <vector>

// test program that uses wav infer to see what a file is all about...
int main(int argv, char** argc)
{
    Wav::FileDescriptor descriptor;
    if (argv > 1) {
        Wav::infer(argc[1], descriptor);
        std::cout << "Sample Rate: " << descriptor.sampleRate << std::endl;
        std::cout << "Sample Count: " << descriptor.sampleCount << " samples" << std::endl;
        std::cout << "Sample Count: "
                  << float(descriptor.sampleCount) * (1.0 / float(descriptor.sampleRate))
                  << " seconds" << std::endl;
        std::cout << "Channel Count: " << descriptor.channelCount << std::endl;
        std::cout << "File offset: " << descriptor.dataOffset << std::endl;

        std::cout << "attempting to read..." << std::endl;
        auto x = std::vector<float>(200);
        auto y = std::vector<float>(200);
        Wav::read(argc[1], x, y);
        for (std::size_t i = 0; i < 200; i++) {
            std::cout << x[i] << std::endl;
            std::cout << y[i] << std::endl;
        }
        Wav::write("out.wav", descriptor.sampleRate, x, y);
        std::cout << "attempting to read OK" << std::endl;
    }
}
