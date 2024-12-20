#pragma once

#include <optional>

#include "Format.hpp"
#include "IO.hpp"

namespace Wav {

// Infer properties
// TODO: We currently break after the data field. I'm not sure that this is correct, really. Its
// just general laziness for now.
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
    auto descriptorHeader = Internal::DescriptorHeader{};
    Internal::readDescriptorHeader(stream, descriptorHeader);

    // now, we read various headers.
    // there can be a bunch, most we dont give a fuck about
    // source: https://www.recordingblogs.com/wiki/wave-file-format
    // we require at least the format chunk and the data chunk though :-)

    // read the format header
    std::optional<Internal::DataFormat> format;
    bool foundFMT = false;
    bool foundDATA = false;
    while (true) {
        Internal::RIFFHeader riff;
        if (!stream.read(reinterpret_cast<char*>(&riff), Internal::getSizeBytes(riff))) {
            if (stream.eof()) {
                break; // End of file reached
            } else {
                throw std::runtime_error("error reading from file");
            }
        }

        if (riff.chunkId == Internal::FMT0 or riff.chunkId == Internal::FMT1) {
            auto formatChunk = Internal::getFormat(riff.chunkSize);
            std::visit(
                Internal::overloaded{
                    [&stream, &descriptor, &format](Internal::FormatChunk40&& formatChunk) {
                        stream.read(reinterpret_cast<char*>(&formatChunk), Internal::getSizeBytes(formatChunk));
                        descriptor.channelCount = formatChunk.channelCount;
                        descriptor.sampleRate = formatChunk.sampleRate;
                        if (formatChunk.format != 0xFFFE) {
                            throw std::runtime_error(
                                "Invalid format id for WAVE_FORMAT_EXTENSIBLE, expected 0xFFFE got " +
                                std::to_string(formatChunk.format));
                        }
                        std::size_t formatCode = *reinterpret_cast<const uint16_t*>(formatChunk.subFormat);
                        std::size_t sampleBits = formatChunk.sampleBits;
                        format = Internal::getDataFormat(formatCode, sampleBits);
                    },
                    [&stream, &descriptor, &format](auto&& formatChunk) {
                        stream.read(reinterpret_cast<char*>(&formatChunk), Internal::getSizeBytes(formatChunk));
                        descriptor.channelCount = formatChunk.channelCount;
                        descriptor.sampleRate = formatChunk.sampleRate;
                        std::size_t formatCode = formatChunk.format;
                        std::size_t sampleBits = formatChunk.sampleBits;
                        format = Internal::getDataFormat(formatCode, sampleBits);
                    },
                },
                std::move(formatChunk));
            foundFMT = true;
            continue;
        }

        if (riff.chunkId == Internal::DATA) {
            if (!format.has_value()) {
                throw std::runtime_error("got 'DATA' chunk before 'fmt ' chunk");
            }
            std::visit(
                [&stream, &descriptor, &riff](auto&& format) {
                    descriptor.sampleCount = 8 * riff.chunkSize / (descriptor.channelCount * (format.sampleBits));
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

} // namespace Wav
