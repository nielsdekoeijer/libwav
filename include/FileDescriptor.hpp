#pragma once

#include "Format.hpp"

namespace Wav {

struct FileDescriptor {
    std::size_t sampleRate;
    std::size_t sampleCount;
    std::size_t channelCount;
    std::size_t dataOffset;
    Internal::DataFormat format;
};

} // namespace Wav
