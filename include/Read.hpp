#pragma once

#include <memory>
#include <vector>

#include "Format.hpp"
#include "Variadic.hpp"

namespace Wav {
template <typename... T>
void read(std::istream& stream, T&... x)
{
    // ensure the stream is in binary mode if necessary
    stream.seekg(0, std::ios::end);
    std::streampos length = stream.tellg();
    stream.seekg(0, std::ios::beg);
    if (length == -1) {
        throw std::runtime_error("failed to determine stream length");
    }

    if (!Internal::allSizeEqual(x...)) {
        throw std::runtime_error("input containers unequally sized");
    }

    FileDescriptor descriptor;
    infer(stream, descriptor);
    std::size_t channelCount = sizeof...(x);
    if (descriptor.channelCount != channelCount) {
        throw std::runtime_error(
            "provided " + std::to_string(channelCount) + " input containers, file contains " +
            std::to_string(descriptor.channelCount) + " channels");
    }

    std::size_t sampleBytes = std::visit([](auto&& format) { return format.sampleBits / 8; }, descriptor.format);
    std::size_t sampleCount = descriptor.sampleCount;
    if (Internal::getSize(x...) < descriptor.sampleCount) {
        sampleCount = Internal::getSize(x...);
    }
    std::size_t byteCount = sampleCount * channelCount * sampleBytes;
    auto mem = std::unique_ptr<char[]>(new char[byteCount]);

    // move to the data block + read
    stream.seekg(descriptor.dataOffset);
    stream.read(mem.get(), byteCount);
    std::visit(
        [mem = std::move(mem), sampleCount, channelCount, &x...](auto&& format) {
            // get interleaved buffer of SampleType
            using SampleType = typename std::remove_reference_t<decltype(format)>::SampleType;
            SampleType* data = reinterpret_cast<SampleType*>(mem.get());
            std::vector<SampleType> interleaved = std::vector<SampleType>(data, data + channelCount * sampleCount);
            Internal::deinterleave(interleaved, x...);
        },
        descriptor.format);
}

// Helper, usually what you'd do
template <typename... T>
void read(std::string& path, T&... x)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("failed to open file at " + std::string(path));
    }
    read(stream, x...);
}

} // namespace Wav
