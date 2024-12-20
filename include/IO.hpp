#pragma once

#include <istream>

namespace Wav::Internal {

// Helper to read and validate a description header.
static void readDescriptorHeader(std::istream& stream, DescriptorHeader& header)
{
    stream.read(reinterpret_cast<char*>(&header), Internal::getSizeBytes(header));

    if (header.chunkId != RIFF) {
        throw std::runtime_error("header invalid chunk id, expected 'RIFF', got " + Internal::idString(header.chunkId));
    }

    if (header.format != WAVE) {
        throw std::runtime_error("Header invalid chunk id, expected 'WAVE', got " + Internal::idString(header.format));
    }
}

} // namespace Wav::Internal
