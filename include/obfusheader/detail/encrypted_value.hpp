#ifndef OBFUSHEADER_DETAIL_ENCRYPTED_VALUE_HPP
#define OBFUSHEADER_DETAIL_ENCRYPTED_VALUE_HPP

#include "anti_analysis.hpp"
#include "config.hpp"
#include "vm.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <type_traits>

namespace oh {

template <typename T, std::uint64_t Seed>
class encrypted_value {
public:
    static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>, "OH_VAL supports arithmetic and enum values");

    constexpr explicit encrypted_value(T value) {
        const auto bytes = std::bit_cast<byte_array>(value);
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            encrypted_[i] = encrypt_byte(bytes[i], i);
        }
    }

    T get() const {
        byte_array bytes{};
        const auto suspicious = anti_analysis::detail::analysis_detected();
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            bytes[i] = decrypt_byte(encrypted_[i], i);
            if (suspicious && !std::is_same_v<T, bool>) {
                bytes[i] = static_cast<unsigned char>(bytes[i] ^ detail::runtime_barrier(detail::key_byte(Seed, i + 41u)));
            }
        }
        if constexpr (std::is_same_v<T, bool>) {
            if (suspicious) {
                return !std::bit_cast<T>(bytes);
            }
        }
        return std::bit_cast<T>(bytes);
    }

    operator T() const {
        return get();
    }

private:
    using byte_array = std::array<unsigned char, sizeof(T)>;

    static constexpr unsigned char encrypt_byte(unsigned char value, std::size_t index) {
        auto byte = static_cast<unsigned char>(value + detail::key_byte(Seed, index + 3u));
        byte = detail::rotl_byte(byte, detail::rotate_amount(Seed, index + 11u));
        byte = static_cast<unsigned char>(byte ^ detail::key_byte(Seed, index + 19u));
        byte = static_cast<unsigned char>(byte + detail::key_byte(Seed, index + 29u));
        return byte;
    }

    static unsigned char decrypt_byte(unsigned char value, std::size_t index) {
        auto byte = static_cast<unsigned char>(value - detail::runtime_barrier(detail::key_byte(Seed, index + 29u)));
        byte = static_cast<unsigned char>(byte ^ detail::runtime_barrier(detail::key_byte(Seed, index + 19u)));
        byte = detail::rotr_byte(byte, detail::runtime_barrier(detail::rotate_amount(Seed, index + 11u)));
        byte = static_cast<unsigned char>(byte - detail::runtime_barrier(detail::key_byte(Seed, index + 3u)));
        return byte;
    }

    byte_array encrypted_{};
};

template <std::uint64_t Seed, typename T>
constexpr auto make_value(T value) {
    if constexpr (current_preset == preset::strong) {
        return virtualized_value<T, Seed>(value);
    } else {
        return encrypted_value<T, Seed>(value);
    }
}

} // namespace oh

#endif
