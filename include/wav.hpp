#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>

namespace Wav {
namespace internal {
    // variadic template helpers
    template <typename First, typename... Args>
    bool allSizeEqual(const First& first, Args&&... args)
    {
        return ((first.size() == ((Args &&) args).size()) && ...);
    }

    template <typename First, typename... Args>
    std::size_t getSize(const First& first, Args&&... args)
    {
        return first.size();
    }

    template <typename... Ts> struct Overload : Ts... {
        using Ts::operator()...;
    };

    struct RIFFHeader {
        uint32_t chunkId;
        uint32_t chunkSize;
    };

    struct DescriptorHeader {
        RIFFHeader riff;
        uint32_t format;
    };

    struct FormatHeader {
        RIFFHeader riff;
        uint16_t format;
        uint16_t channelCount;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t sampleBits;
    };

    struct DataHeader {
        RIFFHeader riff;
    };

    template <typename T, uint16_t B> struct Format {
        using SampleType = T;
        static constexpr uint16_t sampleBits = B;
    };

    using U8LE = Format<uint8_t, 8>;
    using S16LE = Format<int16_t, 16>;
    using F32 = Format<float, 32>;
    using F64 = Format<double, 64>;
    using DataFormat = std::variant<U8LE, S16LE, F32, F64>;

    static DataFormat getDataFormat(uint16_t format, uint16_t sampleBits)
    {
        switch (format) {
        case 1:
            switch (sampleBits) {
            case 8:
                return U8LE {};
                break;
            case 16:
                return S16LE {};
                break;
            default:
                throw std::runtime_error("unsupported sample bits " + std::to_string(sampleBits)
                    + " for format with code " + std::to_string(format));
            }
        case 3:
            switch (sampleBits) {
            case 32:
                return F32 {};
                break;
            case 64:
                return F64 {};
                break;
            default:
                throw std::runtime_error("unsupported sample bits " + std::to_string(sampleBits)
                    + " for format with code " + std::to_string(format));
            }
        default:
            throw std::runtime_error("unsupported format with code " + std::to_string(format));
        }
    }
}

struct FileDescriptor {
    std::size_t sampleRate;
    std::size_t sampleCount;
    std::size_t channelCount;
    std::size_t dataOffset;
    internal::DataFormat format;
};

static void infer(const std::string& path, FileDescriptor& descriptor)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to open file at" + path);
    }

    // read the descriptor header
    auto descriptorHeader = internal::DescriptorHeader {};
    file.read(reinterpret_cast<char*>(&descriptorHeader), sizeof(descriptorHeader));

    const uint32_t RIFF = ('F' << 24) | ('F' << 16) | ('I' << 8) | 'R';
    if (descriptorHeader.riff.chunkId != RIFF) {
        throw std::runtime_error("header invalid chunk id, expected 'RIFF'");
    }

    const uint32_t WAVE = ('E' << 24) | ('V' << 16) | ('A' << 8) | 'W';
    if (descriptorHeader.format != WAVE) {
        throw std::runtime_error("Header invalid chunk id, expected 'WAVE'");
    }

    // read the format header
    auto formatHeader = internal::FormatHeader {};
    file.read(reinterpret_cast<char*>(&formatHeader), sizeof(formatHeader));
    const uint32_t FMT1 = (00 << 24) | ('t' << 16) | ('m' << 8) | 'f';
    const uint32_t FMT0 = (32 << 24) | ('t' << 16) | ('m' << 8) | 'f';
    if (!(formatHeader.riff.chunkId == FMT0 or formatHeader.riff.chunkId == FMT1)) {
        throw std::runtime_error("Header invalid chunk id, expected 'fmt '");
    }
    descriptor.channelCount = formatHeader.channelCount;
    descriptor.sampleRate = formatHeader.sampleRate;
    auto format = internal::getDataFormat(formatHeader.format, formatHeader.sampleBits);

    // read the data header
    auto dataHeader = internal::DataHeader {};
    file.read(reinterpret_cast<char*>(&dataHeader), sizeof(dataHeader));
    const uint32_t DATA = ('a' << 24) | ('t' << 16) | ('a' << 8) | 'd';
    if (dataHeader.riff.chunkId != DATA) {
        throw std::runtime_error("Header invalid chunk id, expected 'data'");
    }
    descriptor.sampleCount
        = 8 * dataHeader.riff.chunkSize / (descriptor.channelCount * (formatHeader.sampleBits));
    descriptor.dataOffset = file.tellg();
    file.close();
}

template <typename... T> void read(const std::string& path, T&... x)
{
    if (!internal::allSizeEqual(x...)) {
        throw std::runtime_error("input containers unequally sized");
    }

    FileDescriptor descriptor;
    infer(path, descriptor);
    std::size_t channelCount = sizeof...(x);
    if (descriptor.channelCount != channelCount) {
        throw std::runtime_error("provided " + std::to_string(channelCount)
            + " input containers, file contains " + std::to_string(descriptor.channelCount)
            + " channels");
    }

    std::size_t sampleBytes
        = std::visit([](auto&& format) { return format.sampleBits / 8; }, descriptor.format);
    std::size_t sampleCount = descriptor.sampleCount;
    if (internal::getSize(x...) < descriptor.sampleCount) {
        sampleCount = internal::getSize(x...);
    }
    std::size_t byteCount = sampleCount * channelCount * sampleBytes;
    auto mem = std::unique_ptr<char[]>(new char[byteCount]);

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to open file at" + path);
    }

    // move to the data block + read
    file.seekg(descriptor.dataOffset);
    file.read(mem.get(), byteCount);
    std::visit(
        [mem = std::move(mem)](auto&& format) {
            // get interleaved buffer of SampleType
            auto buff = reinterpret_cast<
                const typename std::remove_reference_t<decltype(format)>::SampleType*>(mem.get());
        },
        descriptor.format);
    // recursiveChannelRead(channelCount, payload, x...);
}

template <typename... T> void write(const std::string& path, const std::size_t rate, T&... x) { }
}
