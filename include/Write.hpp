#pragma once

#include <fstream>
#include <memory>

namespace Wav {
// Write a wav file to an N channel f32-pcm file
// TODO: This is unreadable garbage please change
template <typename... T>
void write(std::ostream& stream, const std::size_t rate, T&... x)
{
    if (!Internal::allSizeEqual(x...)) {
        throw std::runtime_error("input containers unequally sized");
    }

    std::size_t channelCount = sizeof...(x);
    std::size_t sampleCount = Internal::getSize(x...);
    Internal::DescriptorHeader descriptorHeader;
    Internal::RIFFHeader formatHeader;
    Internal::FormatChunk18 formatChunk;
    Internal::RIFFHeader factHeader;
    Internal::RIFFHeader dataHeader;
    auto mem = std::unique_ptr<float[]>(new float[channelCount * sampleCount]);
    descriptorHeader.chunkId = ('F' << 24) | ('F' << 16) | ('I' << 8) | 'R';
    descriptorHeader.chunkSize = 4 + (8 + getSizeBytes(formatChunk)) +
                                 (8 + getSizeBytes(formatHeader) + getSizeBytes(formatChunk) + getSizeBytes(factHeader) +
                                  getSizeBytes(dataHeader) + sizeof(mem));
    descriptorHeader.format = ('E' << 24) | ('V' << 16) | ('A' << 8) | 'W';
    stream.write(reinterpret_cast<char*>(&descriptorHeader), Internal::getSizeBytes(descriptorHeader));

    formatHeader.chunkId = (32 << 24) | ('t' << 16) | ('m' << 8) | 'f';
    formatHeader.chunkSize = getSizeBytes(formatChunk);
    stream.write(reinterpret_cast<char*>(&formatHeader), Internal::getSizeBytes(formatHeader));

    formatChunk.format = 3;
    formatChunk.channelCount = channelCount;
    formatChunk.sampleRate = rate;
    formatChunk.byteRate = rate * channelCount * 4;
    formatChunk.blockAlign = channelCount * 4;
    formatChunk.sampleBits = 32;
    formatChunk.extensionSize = 0;
    stream.write(reinterpret_cast<char*>(&formatChunk), Internal::getSizeBytes(formatChunk));

    factHeader.chunkId = ('t' << 24) | ('c' << 16) | ('a' << 8) | 'f';
    factHeader.chunkSize = 4;
    stream.write(reinterpret_cast<char*>(&factHeader), Internal::getSizeBytes(factHeader));
    uint32_t dwSampleLength = channelCount * sampleCount;
    stream.write(reinterpret_cast<char*>(&dwSampleLength), 4);

    dataHeader.chunkId = ('a' << 24) | ('t' << 16) | ('a' << 8) | 'd';
    dataHeader.chunkSize = channelCount * sampleCount * 32 / 8;
    stream.write(reinterpret_cast<char*>(&dataHeader), Internal::getSizeBytes(dataHeader));

    auto vec = std::vector<float>(mem.get(), mem.get() + channelCount * sampleCount);
    Internal::interleave(vec, x...);
    stream.write(reinterpret_cast<char*>(vec.data()), channelCount * sampleCount * 4);
}

template <typename... T>
void write(const std::string& path, const std::size_t rate, T&... x)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("failed to open file at " + std::string(path));
    }
    write(stream, rate, x...);
}

} // namespace Wav
