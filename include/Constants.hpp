#pragma once

#include <cstdint>
#include <string>

namespace Wav::Internal {

// Supported RIFF header chunk ids, others are ignored
const uint32_t FMT1 = (00 << 24) | ('t' << 16) | ('m' << 8) | 'f';
const uint32_t FMT0 = (32 << 24) | ('t' << 16) | ('m' << 8) | 'f';
const uint32_t DATA = ('a' << 24) | ('t' << 16) | ('a' << 8) | 'd';
const uint32_t RIFF = ('F' << 24) | ('F' << 16) | ('I' << 8) | 'R';
const uint32_t WAVE = ('E' << 24) | ('V' << 16) | ('A' << 8) | 'W';

static std::string idString(uint32_t id)
{
    char chars[4];
    chars[3] = (id >> 24) & 0xFF;
    chars[2] = (id >> 16) & 0xFF;
    chars[1] = (id >> 8) & 0xFF;
    chars[0] = (id) & 0xFF;
    return std::string(chars, 4);
}

} // namespace Wav::Internal
