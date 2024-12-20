#pragma once

#include <fstream>
#include <stdexcept>
#include <variant>

#include "Header.hpp"

namespace Wav::Internal {

// Generic type that wraps information about supported formats
template <typename _SampleType, std::size_t _sampleBits, bool _isPCM>
struct Format {
    using SampleType = _SampleType;
    static constexpr uint16_t sampleBits = _sampleBits;
    static constexpr bool isPCM = _isPCM;
};

// Aliases for different supported formats
using U8LE = Format<uint8_t, 8, true>;
using S16LE = Format<int16_t, 16, true>;
using F32 = Format<float, 32, false>;
using F64 = Format<double, 64, false>;

// Variant for supported type
using DataFormat = std::variant<F32, U8LE, S16LE, F64>;

// Aliases + variants for different supported formats
static FormatChunk getFormat(std::size_t chunkSize)
{
    switch (chunkSize) {
    case 16:
        return FormatChunk16{};
        break;
    case 18:
        return FormatChunk18{};
        break;
    case 40:
        return FormatChunk40{};
        break;
    default:
        throw std::runtime_error("unsupported format chunk size " + std::to_string(chunkSize));
    }
}

// Map to variant based on format int and number of bits per sample
static DataFormat getDataFormat(uint16_t format, uint16_t sampleBits)
{
    switch (format) {
    case 1:
        switch (sampleBits) {
        case 8:
            return U8LE{};
            break;
        case 16:
            return S16LE{};
            break;
        default:
            throw std::runtime_error(
                "unsupported sample bits " + std::to_string(sampleBits) + " for format with code " + std::to_string(format));
        }
        break;
    case 3:
        switch (sampleBits) {
        case 32:
            return F32{};
            break;
        case 64:
            return F64{};
            break;
        default:
            throw std::runtime_error(
                "unsupported sample bits " + std::to_string(sampleBits) + " for format with code " + std::to_string(format));
        }
        break;
    default:
        throw std::runtime_error("unsupported format with code " + std::to_string(format));
    }
    throw std::runtime_error("unreachable");
}

} // namespace Wav::Internal
