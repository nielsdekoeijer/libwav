#pragma once

#include <type_traits>
#include <variant>

namespace Wav::Internal {

// Helper function for printing header chunk ids for debug purposes
// Variadic template helper to ensure all input STL containers are of equal size
template <typename First, typename... Args>
bool allSizeEqual(const First& first, Args&&... args)
{
    return ((first.size() == ((Args&&)args).size()) && ...);
}

// Variadic template helper to get the size of the first STL container
template <typename First, typename... Args>
std::size_t getSize(const First& first, Args&&... args)
{
    return first.size();
}

// Variadic template helpers to provide a pattern-matching-like API when using std::variant
template <typename... Ts>
struct Overload : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

} // namespace Wav::Internal
