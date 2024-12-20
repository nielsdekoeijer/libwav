#pragma once

#include <variant>

namespace Wav::Internal {
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

// Variant for the format chunks, so that they can be iterated over using pattern matching
using FormatChunk = std::variant<FormatChunk16, FormatChunk18, FormatChunk40>;

// Variant for the supported chunks, so that they can be iterated over using pattern matching
using SupportedChunk = std::variant<RIFFHeader, DescriptorHeader, FormatChunk16, FormatChunk18, FormatChunk40>;

// Gets the raw bytes per struct.
// TODO: Naively, we could have used sizeof in place of this. Unfortunately, sizeof gives us the
// PADDED size; the compiler adds padding to make cache nice... This leads to use essentially
// fucking up the cursor for reading. Hence, I have this for now. But a smaller solution may be possible.
static std::size_t getSizeBytes(SupportedChunk chunk)
{
    return std::visit(
        overloaded{
            [](RIFFHeader chunk) { return 8; },
            [](DescriptorHeader chunk) { return 12; },
            [](FormatChunk16 chunk) { return 16; },
            [](FormatChunk18 chunk) { return 18; },
            [](FormatChunk40 chunk) { return 40; },
        },
        chunk);
}

} // namespace Wav::Internal
