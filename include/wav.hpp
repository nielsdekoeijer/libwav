#pragma once

#include <fstream>
#include <istream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <iostream>

// TODO: use of static? Whats the implication again? I don't remember
// TODO: cleanup read / infer
// TODO: cleanup write

namespace Wav {
namespace internal {
    // Supported RIFF header chunk ids, others are ignored
    const uint32_t FMT1 = (00 << 24) | ('t' << 16) | ('m' << 8) | 'f';
    const uint32_t FMT0 = (32 << 24) | ('t' << 16) | ('m' << 8) | 'f';
    const uint32_t DATA = ('a' << 24) | ('t' << 16) | ('a' << 8) | 'd';
    const uint32_t RIFF = ('F' << 24) | ('F' << 16) | ('I' << 8) | 'R';
    const uint32_t WAVE = ('E' << 24) | ('V' << 16) | ('A' << 8) | 'W';

    // Helper function for printing header chunk ids for debug purposes
    static std::string idString(uint32_t id)
    {
        char chars[4];
        chars[3] = (id >> 24) & 0xFF;
        chars[2] = (id >> 16) & 0xFF;
        chars[1] = (id >> 8) & 0xFF;
        chars[0] = (id)&0xFF;
        return std::string(chars, 4);
    }

    // Variadic template helper to ensure all input STL containers are of equal size
    template <typename First, typename... Args>
    bool allSizeEqual(const First& first, Args&&... args)
    {
        return ((first.size() == ((Args &&) args).size()) && ...);
    }

    // Variadic template helper to get the size of the first STL container
    template <typename First, typename... Args>
    std::size_t getSize(const First& first, Args&&... args)
    {
        return first.size();
    }

    // Variadic template helpers to provide a pattern-matching-like API when using std::variant
    template <typename... Ts> struct Overload : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts> struct overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    // Deinterleaving in two template functions
    // TODO: casting interface for different types
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

    // Interleaving in two template functions
    // TODO: casting interface for different types

    template <typename K, typename T>
    inline void interleave(K& interleaved, T& x, std::size_t& i, std::size_t j)
    {
        interleaved[i++] = x[j];
    }

    template <typename K, typename... T> void interleave(K& interleaved, T&... x)
    {
        std::size_t i = 0;
        for (std::size_t j = 0; j < getSize(x...); j++) {
            (interleave(interleaved, x, i, j), ...);
        }
    }

    // Structs for representing RIFF and WAV headers
    // source: https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
    // TODO: somewhat verbose for the different format chunks. potential DRY violations that can be
    // optimized out

    struct DescriptorHeader {
        uint32_t chunkId;
        uint32_t chunkSize;
        uint32_t format;
    };

    struct RIFFHeader {
        uint32_t chunkId;
        uint32_t chunkSize;
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
        uint16_t extensionSize;
        uint16_t validBitsPerSample;
        uint32_t channelMask;
        char subFormat[16];
    };

    using FormatChunk = std::variant<FormatChunk16, FormatChunk18, FormatChunk40>;
    using SupportedChunk
        = std::variant<RIFFHeader, DescriptorHeader, FormatChunk16, FormatChunk18, FormatChunk40>;

    // Generic type that wraps information about supported formats
    template <typename _SampleType, std::size_t _sampleBits, bool _isPCM> struct Format {
        using SampleType = _SampleType;
        static constexpr uint16_t sampleBits = _sampleBits;
        static constexpr bool isPCM = _isPCM;
    };

    // Aliases + variants for different supported formats
    using U8LE = Format<uint8_t, 8, true>;
    using S16LE = Format<int16_t, 16, true>;
    using F32 = Format<float, 32, false>;
    using F64 = Format<double, 64, false>;
    using DataFormat = std::variant<F32, U8LE, S16LE, F64>;

    // Aliases + variants for different supported formats
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

    static std::size_t getSizeBytes(SupportedChunk chunk)
    {
        return std::visit(overloaded {
                              [](RIFFHeader chunk) { return 8; },
                              [](DescriptorHeader chunk) { return 12; },
                              [](FormatChunk16 chunk) { return 16; },
                              [](FormatChunk18 chunk) { return 18; },
                              [](FormatChunk40 chunk) { return 40; },
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

    static void readDescriptorHeader(std::istream& stream, DescriptorHeader& header)
    {
        stream.read(reinterpret_cast<char*>(&header), internal::getSizeBytes(header));

        if (header.chunkId != RIFF) {
            throw std::runtime_error("header invalid chunk id, expected 'RIFF', got "
                + internal::idString(header.chunkId));
        }

        if (header.format != WAVE) {
            throw std::runtime_error("Header invalid chunk id, expected 'WAVE', got "
                + internal::idString(header.format));
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

static void infer(std::istream& stream, FileDescriptor& descriptor)
{
    // ensure the stream is in binary mode if necessary
    stream.seekg(0, std::ios::end);
    std::streampos length = stream.tellg();
    stream.seekg(0, std::ios::beg);
    if (length == -1) {
        throw std::runtime_error("failed to determine stream length");
    }

    // read the descriptor header
    auto descriptorHeader = internal::DescriptorHeader {};
    internal::readDescriptorHeader(stream, descriptorHeader);

    // now, we read various headers.
    // there can be a bunch, most we dont give a fuck about
    // source: https://www.recordingblogs.com/wiki/wave-file-format
    // we require at least the format chunk and the data chunk though :-)

    // read the format header
    std::optional<internal::DataFormat> format;
    bool foundFMT = false;
    bool foundDATA = false;
    while (true) {
        internal::RIFFHeader riff;
        if (!stream.read(reinterpret_cast<char*>(&riff), internal::getSizeBytes(riff))) {
            if (stream.eof()) {
                break; // End of file reached
            } else {
                throw std::runtime_error("error reading from file");
            }
        }
        std::cout << "got " << internal::idString(riff.chunkId) << ", reading " << riff.chunkSize
                  << " bytes" << std::endl;

        if (riff.chunkId == internal::FMT0 or riff.chunkId == internal::FMT1) {
            auto formatChunk = internal::getFormatChunk(riff.chunkSize);
            std::visit(
                internal::overloaded {
                    [&stream, &descriptor, &format](internal::FormatChunk40&& formatChunk) {
                        stream.read(reinterpret_cast<char*>(&formatChunk),
                            internal::getSizeBytes(formatChunk));
                        descriptor.channelCount = formatChunk.channelCount;
                        descriptor.sampleRate = formatChunk.sampleRate;
                        if (formatChunk.format != 0xFFFE) {
                            throw std::runtime_error(
                                "Invalid format id for WAVE_FORMAT_EXTENSIBLE, expected 0xFFFE got "
                                + std::to_string(formatChunk.format));
                        }
                        std::size_t formatCode
                            = *reinterpret_cast<const uint16_t*>(formatChunk.subFormat);
                        std::size_t sampleBits = formatChunk.sampleBits;
                        format = internal::getDataFormat(formatCode, sampleBits);
                    },
                    [&stream, &descriptor, &format](auto&& formatChunk) {
                        stream.read(reinterpret_cast<char*>(&formatChunk),
                            internal::getSizeBytes(formatChunk));
                        descriptor.channelCount = formatChunk.channelCount;
                        descriptor.sampleRate = formatChunk.sampleRate;
                        std::size_t formatCode = formatChunk.format;
                        std::size_t sampleBits = formatChunk.sampleBits;
                        format = internal::getDataFormat(formatCode, sampleBits);
                    },
                },
                std::move(formatChunk));
            foundFMT = true;
            continue;
        }

        if (riff.chunkId == internal::DATA) {
            if (!format.has_value()) {
                throw std::runtime_error("got 'DATA' chunk before 'fmt ' chunk");
            }
            std::visit(
                [&stream, &descriptor, &riff](auto&& format) {
                    descriptor.sampleCount
                        = 8 * riff.chunkSize / (descriptor.channelCount * (format.sampleBits));
                    descriptor.dataOffset = stream.tellg();
                },
                format.value());
            foundDATA = true;
            break;
        }

        stream.seekg(riff.chunkSize, std::ios::cur);
    }

    if (!foundFMT) {
        throw std::runtime_error("failed to find 'fmt ' chunk");
    }
    if (!foundDATA) {
        throw std::runtime_error("failed to find 'DATA' chunk");
    }
}

static void infer(const std::string& path, FileDescriptor& descriptor)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("failed to open file at " + std::string(path));
    }
    infer(stream, descriptor);
}

template <typename... T> void read(std::istream& stream, T&... x)
{
    // ensure the stream is in binary mode if necessary
    stream.seekg(0, std::ios::end);
    std::streampos length = stream.tellg();
    stream.seekg(0, std::ios::beg);
    if (length == -1) {
        throw std::runtime_error("failed to determine stream length");
    }
    if (!internal::allSizeEqual(x...)) {
        throw std::runtime_error("input containers unequally sized");
    }
    FileDescriptor descriptor;
    infer(stream, descriptor);
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

    // move to the data block + read
    stream.seekg(descriptor.dataOffset);
    stream.read(mem.get(), byteCount);
    std::visit(
        [mem = std::move(mem), sampleCount, channelCount, &x...](auto&& format) {
            // get interleaved buffer of SampleType
            using SampleType = typename std::remove_reference_t<decltype(format)>::SampleType;
            SampleType* data = reinterpret_cast<SampleType*>(mem.get());
            std::vector<SampleType> interleaved
                = std::vector<SampleType>(data, data + channelCount * sampleCount);
            internal::deinterleave(interleaved, x...);
        },
        descriptor.format);
}

template <typename... T> void read(std::string& path, T&... x)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("failed to open file at " + std::string(path));
    }
    read(stream, x...);
}

template <typename... T> void write(std::ostream& stream, const std::size_t rate, T&... x)
{
    if (!internal::allSizeEqual(x...)) {
        throw std::runtime_error("input containers unequally sized");
    }

    std::size_t channelCount = sizeof...(x);
    std::size_t sampleCount = internal::getSize(x...);
    internal::DescriptorHeader descriptorHeader;
    internal::RIFFHeader formatHeader;
    internal::FormatChunk18 formatChunk;
    internal::RIFFHeader factHeader;
    internal::RIFFHeader dataHeader;
    auto mem = std::unique_ptr<float[]>(new float[channelCount * sampleCount]);
    descriptorHeader.chunkId = ('F' << 24) | ('F' << 16) | ('I' << 8) | 'R';
    descriptorHeader.chunkSize = 4 + (8 + getSizeBytes(formatChunk))
        + (8 + getSizeBytes(formatHeader) + getSizeBytes(formatChunk) + getSizeBytes(factHeader)
            + getSizeBytes(dataHeader) + sizeof(mem));
    descriptorHeader.format = ('E' << 24) | ('V' << 16) | ('A' << 8) | 'W';
    stream.write(
        reinterpret_cast<char*>(&descriptorHeader), internal::getSizeBytes(descriptorHeader));

    formatHeader.chunkId = (32 << 24) | ('t' << 16) | ('m' << 8) | 'f';
    formatHeader.chunkSize = getSizeBytes(formatChunk);
    stream.write(reinterpret_cast<char*>(&formatHeader), internal::getSizeBytes(formatHeader));

    formatChunk.format = 3;
    formatChunk.channelCount = channelCount;
    formatChunk.sampleRate = rate;
    formatChunk.byteRate = rate * channelCount * 4;
    formatChunk.blockAlign = channelCount * 4;
    formatChunk.sampleBits = 32;
    formatChunk.extensionSize = 0;
    stream.write(reinterpret_cast<char*>(&formatChunk), internal::getSizeBytes(formatChunk));

    factHeader.chunkId = ('t' << 24) | ('c' << 16) | ('a' << 8) | 'f';
    factHeader.chunkSize = 4;
    stream.write(reinterpret_cast<char*>(&factHeader), internal::getSizeBytes(factHeader));
    uint32_t dwSampleLength = channelCount * sampleCount;
    stream.write(reinterpret_cast<char*>(&dwSampleLength), 4);

    dataHeader.chunkId = ('a' << 24) | ('t' << 16) | ('a' << 8) | 'd';
    dataHeader.chunkSize = channelCount * sampleCount * 32 / 8;
    stream.write(reinterpret_cast<char*>(&dataHeader), internal::getSizeBytes(dataHeader));

    auto vec = std::vector<float>(mem.get(), mem.get() + channelCount * sampleCount);
    internal::interleave(vec, x...);
    stream.write(reinterpret_cast<char*>(vec.data()), channelCount * sampleCount * 4);
}

template <typename... T> void write(const std::string& path, const std::size_t rate, T&... x)
{
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("failed to open file at " + std::string(path));
    }
    write(stream, rate, x...);
}
}
