#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <cstdio>
#include <iostream>

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

    template <class... Ts> struct overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    // reading helpers + casting
    template <typename K, typename T>
    inline void deinterleave(K& interleaved, T& x, std::size_t& i, std::size_t j)
    {
        x[j] = interleaved[i++];
    }

    template <typename K, typename... T> void deinterleave(K& interleaved, T&... x)
    {
        std::size_t i = 0;
        for (std::size_t j = 0; j < getSize(x...); j++) {
            (deinterleave(interleaved, x, i, j), ...);
        }
    }

    // source: https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
    struct RIFFHeader {
        uint32_t chunkId;
        uint32_t chunkSize;
    };

    struct DescriptorHeader {
        uint32_t chunkId;
        uint32_t chunkSize;
        uint32_t format;
    };

    struct FormatChunk16 {
        uint16_t format;
        uint16_t channelCount;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t sampleBits;
    };

    struct FormatChunk18 {
        uint16_t format;
        uint16_t channelCount;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t sampleBits;
        uint16_t extensionSize;
    };

    struct FormatChunk40 {
        uint16_t format;
        uint16_t channelCount;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t sampleBits;
        uint16_t validBitsPerSample;
        uint32_t channelMask;
        char subFormat[16];
    };

    struct FactHeader {
        uint32_t chunkId;
        uint32_t chunkSize;
        uint32_t dwSampleLength;
    };

    struct DataHeader {
        uint32_t chunkId;
        uint32_t chunkSize;
    };

    using FormatChunk = std::variant<FormatChunk16, FormatChunk18, FormatChunk40>;
    using Chunk = std::variant<RIFFHeader, DescriptorHeader, FormatChunk16, FormatChunk18,
        FormatChunk40, FactHeader, DataHeader>;

    template <typename T, uint16_t B, bool P> struct Format {
        using SampleType = T;
        static constexpr uint16_t sampleBits = B;
        static constexpr bool isPCM = P;
    };
    using U8LE = Format<uint8_t, 8, true>;
    using S16LE = Format<int16_t, 16, true>;
    using F32 = Format<float, 32, false>;
    using F64 = Format<double, 64, false>;

    using DataFormat = std::variant<F32, U8LE, S16LE, F64>;

    static FormatChunk getFormatChunk(std::size_t chunkSize)
    {
        switch (chunkSize) {
        case 16:
            return FormatChunk16 {};
            break;
        case 18:
            return FormatChunk18 {};
            break;
        case 40:
            return FormatChunk40 {};
            break;
        default:
            throw std::runtime_error("unsupported format chunk size " + std::to_string(chunkSize));
        }
    }

    // (optional, non PCM) fact header
    static std::size_t getSizeBytes(Chunk chunk)
    {
        return std::visit(overloaded {
                              [](RIFFHeader chunk) { return 8; },
                              [](DescriptorHeader chunk) { return 12; },
                              [](FormatChunk16 chunk) { return 16; },
                              [](FormatChunk18 chunk) { return 18; },
                              [](FormatChunk40 chunk) { return 40; },
                              [](FactHeader chunk) { return 12; },
                              [](DataHeader chunk) { return 8; },
                          },
            chunk);
    }

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
            break;
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
            break;
        default:
            throw std::runtime_error("unsupported format with code " + std::to_string(format));
        }
        throw std::runtime_error("unreachable");
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
    file.read(reinterpret_cast<char*>(&descriptorHeader), internal::getSizeBytes(descriptorHeader));

    const uint32_t RIFF = ('F' << 24) | ('F' << 16) | ('I' << 8) | 'R';
    if (descriptorHeader.chunkId != RIFF) {
        throw std::runtime_error("header invalid chunk id, expected 'RIFF'");
    }

    const uint32_t WAVE = ('E' << 24) | ('V' << 16) | ('A' << 8) | 'W';
    if (descriptorHeader.format != WAVE) {
        throw std::runtime_error("Header invalid chunk id, expected 'WAVE'");
    }

    // read the format header
    auto formatRiff = internal::RIFFHeader {};
    file.read(reinterpret_cast<char*>(&formatRiff), internal::getSizeBytes(formatRiff));
    const uint32_t FMT1 = (00 << 24) | ('t' << 16) | ('m' << 8) | 'f';
    const uint32_t FMT0 = (32 << 24) | ('t' << 16) | ('m' << 8) | 'f';
    if (!(formatRiff.chunkId == FMT0 or formatRiff.chunkId == FMT1)) {
        throw std::runtime_error("Header invalid chunk id, expected 'fmt '");
    }
    auto formatChunk = internal::getFormatChunk(formatRiff.chunkSize);
    std::size_t formatCode, sampleBits;
    std::visit(
        [&file, &descriptor, &formatCode, &sampleBits](auto&& formatChunk) {
            file.read(reinterpret_cast<char*>(&formatChunk), internal::getSizeBytes(formatChunk));
            descriptor.channelCount = formatChunk.channelCount;
            descriptor.sampleRate = formatChunk.sampleRate;
            formatCode = formatChunk.format;
            sampleBits = formatChunk.sampleBits;
        },
        formatChunk);
    auto format = internal::getDataFormat(formatCode, sampleBits);

    // if format is not PCM, we get a fact chunk!
    std::visit(
        [&file](auto&& format) {
            if (!format.isPCM) {
                auto factHeader = internal::FactHeader {};
                file.read(reinterpret_cast<char*>(&factHeader), internal::getSizeBytes(factHeader));
                const uint32_t FACT = ('t' << 24) | ('c' << 16) | ('a' << 8) | 'f';
                if (factHeader.chunkId != FACT) {
                    throw std::runtime_error("Header invalid chunk id, expected 'FACT'"
                        + std::to_string(factHeader.chunkId));
                }
            }
        },
        format);

    // read the data header
    auto dataHeader = internal::DataHeader {};
    file.read(reinterpret_cast<char*>(&dataHeader), internal::getSizeBytes(dataHeader));
    const uint32_t DATA = ('a' << 24) | ('t' << 16) | ('a' << 8) | 'd';
    if (dataHeader.chunkId != DATA) {
        throw std::runtime_error("Header invalid chunk id, expected 'data'");
    }
    descriptor.sampleCount = 8 * dataHeader.chunkSize / (descriptor.channelCount * (sampleBits));
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
        [mem = std::move(mem), sampleCount, channelCount, &x...](auto&& format) {
            // get interleaved buffer of SampleType
            using SampleType = typename std::remove_reference_t<decltype(format)>::SampleType;
            SampleType* data = reinterpret_cast<SampleType*>(mem.get());
            std::vector<SampleType> interleaved = std::vector<SampleType>(data, data + channelCount * sampleCount);
            internal::deinterleave(interleaved, x...);
        },
        descriptor.format);
    // recursiveChannelRead(channelCount, payload, x...);
}

template <typename... T> void write(const std::string& path, const std::size_t rate, T&... x) { }
}
