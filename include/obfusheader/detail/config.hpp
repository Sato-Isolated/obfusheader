#ifndef OBFUSHEADER_DETAIL_CONFIG_HPP
#define OBFUSHEADER_DETAIL_CONFIG_HPP

#include <cstdint>
#include <cstddef>

#ifndef OH_USER_SEED
#define OH_USER_SEED 0
#endif

namespace oh {

enum class preset {
    light,
    balanced,
    strong
};

#ifndef OH_CONFIG_PRESET
inline constexpr preset current_preset = preset::balanced;
#else
inline constexpr preset current_preset = OH_CONFIG_PRESET;
#endif

namespace detail {

consteval std::uint64_t fnv1a(const char* value) {
    std::uint64_t hash = 14695981039346656037ull;
    for (std::size_t i = 0; value[i] != '\0'; ++i) {
        hash ^= static_cast<unsigned char>(value[i]);
        hash *= 1099511628211ull;
    }
    return hash;
}

constexpr std::uint64_t mix(std::uint64_t value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ull;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebull;
    value ^= value >> 31;
    return value;
}

consteval std::uint64_t seed_from_context(const char* file, int line, int counter) {
    return mix(fnv1a(file) ^ (static_cast<std::uint64_t>(line) << 32) ^ static_cast<std::uint64_t>(counter) ^ static_cast<std::uint64_t>(OH_USER_SEED));
}

constexpr std::uint64_t stream(std::uint64_t seed, std::size_t index) {
    return mix(seed + 0x9e3779b97f4a7c15ull * (index + 1));
}

constexpr unsigned char key_byte(std::uint64_t seed, std::size_t index) {
    return static_cast<unsigned char>((stream(seed, index) >> ((index % 8u) * 8u)) & 0xffu);
}

constexpr unsigned char rotl_byte(unsigned char value, unsigned char amount) {
    const auto shift = static_cast<unsigned char>(amount & 7u);
    return static_cast<unsigned char>((value << shift) | (value >> ((8u - shift) & 7u)));
}

constexpr unsigned char rotr_byte(unsigned char value, unsigned char amount) {
    const auto shift = static_cast<unsigned char>(amount & 7u);
    return static_cast<unsigned char>((value >> shift) | (value << ((8u - shift) & 7u)));
}

constexpr unsigned char rotate_amount(std::uint64_t seed, std::size_t index) {
    return static_cast<unsigned char>((stream(seed, index) % 7u) + 1u);
}

inline unsigned char runtime_barrier(unsigned char value) {
    volatile unsigned char guarded = value;
    return guarded;
}

} // namespace detail
} // namespace oh

#endif
