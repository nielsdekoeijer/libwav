#pragma once

#include "Variadic.hpp"

#include <iostream>

namespace Wav::Internal {

template <typename K, typename T>
requires(
    (std::is_same<K, double>::value or std::is_same<K, float>::value) and
    (std::is_same<T, double>::value or std::is_same<T, float>::value))
inline T convert(K x)
{
    std::cout << "converting floating point" << std::endl;
    return static_cast<T>(x);
}

template <typename K, typename T>
requires((std::is_same<K, uint8_t>::value) and (std::is_same<T, double>::value or std::is_same<T, float>::value))
inline T convert(K x)
{
    std::cout << "converting u8" << std::endl;
    // Convert uint8_t to float in the range [0, 1]
    return (static_cast<T>(x) - 128.0f) / 127.0f;
}

template <typename K, typename T>
requires((std::is_same<K, int16_t>::value) and (std::is_same<T, double>::value or std::is_same<T, float>::value))
inline T convert(K x)
{
    std::cout << "converting s16" << std::endl;
    // Convert int16_t to float in the range [-1, 1]
    return static_cast<T>(x) / 32767.0f;
}

template <typename K, typename T>
inline T convert(K x)
{
    static_assert(std::is_same_v<K, void> && std::is_same_v<T, void>, "Invalid conversion");
    return x;
}

// Deinterleaving in two template functions
// TODO: casting interface for different types
// TODO: container of containers support
template <typename K, typename T>
inline void deinterleave(K& interleaved, T& x, std::size_t& i, std::size_t j)
{
    using K_t = std::remove_reference<decltype(interleaved[0])>::type;
    using T_t = std::remove_reference<decltype(x[0])>::type;

    std::cout << interleaved[i] << std::endl;

    std::cout << x[j] << std::endl;

    x[j] = convert<K_t, T_t>(interleaved[i++]);

    std::cout << x[j] << std::endl;
}

template <typename K, typename... T>
void deinterleave(K& interleaved, T&... x)
{
    std::size_t i = 0;
    for (std::size_t j = 0; j < getSize(x...); j++) {
        (deinterleave(interleaved, x, i, j), ...);
    }
}

// Interleaving in two template functions
// TODO: casting interface for different types
// TODO: container of containers support
template <typename K, typename T>
inline void interleave(K& interleaved, T& x, std::size_t& i, std::size_t j)
{
    interleaved[i++] = x[j];
}

template <typename K, typename... T>
void interleave(K& interleaved, T&... x)
{
    std::size_t i = 0;
    for (std::size_t j = 0; j < getSize(x...); j++) {
        (interleave(interleaved, x, i, j), ...);
    }
}

} // namespace Wav::Internal
