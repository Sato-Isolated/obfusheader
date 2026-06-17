#ifndef OBFUSHEADER_DETAIL_ENCRYPTED_STRING_HPP
#define OBFUSHEADER_DETAIL_ENCRYPTED_STRING_HPP

#include "anti_analysis.hpp"
#include "config.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace oh {

template <std::size_t Size>
struct fixed_string {
    char value[Size]{};

    consteval fixed_string(const char (&text)[Size]) {
        for (std::size_t i = 0; i < Size; ++i) {
            value[i] = text[i];
        }
    }
};

template <typename CharT, std::size_t Size, std::uint64_t Seed>
class encrypted_string {
public:
    constexpr explicit encrypted_string(const CharT (&value)[Size]) {
        for (std::size_t i = 0; i < Size; ++i) {
            encrypted_[i] = crypt(value[i], i);
            decrypted_buffer_[i] = CharT{};
        }
    }

    const CharT* c_str() {
        decrypt();
        return decrypted_buffer_.data();
    }

    std::basic_string_view<CharT> view() {
        decrypt();
        return std::basic_string_view<CharT>(decrypted_buffer_.data(), Size == 0 ? 0 : Size - 1);
    }

    void clear() {
        for (std::size_t i = 0; i < decrypted_buffer_.size(); ++i) {
            volatile CharT* item = &decrypted_buffer_[i];
            *item = CharT{};
        }
        cleared_ = true;
    }

private:
    static constexpr CharT crypt(CharT value, std::size_t index) {
        auto byte = static_cast<unsigned char>(value);
        const auto add = detail::key_byte(Seed, index + Size);
        const auto xor_a = detail::key_byte(Seed, index * 3u + 1u);
        const auto xor_b = detail::key_byte(Seed, index * 5u + 7u);
        const auto rotate = detail::rotate_amount(Seed, index + Size * 2u);

        byte = static_cast<unsigned char>(byte + add);
        byte = static_cast<unsigned char>(byte ^ xor_a);
        byte = detail::rotl_byte(byte, rotate);
        byte = static_cast<unsigned char>(byte + static_cast<unsigned char>(index * 17u));
        byte = static_cast<unsigned char>(byte ^ xor_b);
        return static_cast<CharT>(byte);
    }

    static constexpr CharT decrypt_char(CharT value, std::size_t index) {
        auto byte = static_cast<unsigned char>(value);
        const auto add = detail::key_byte(Seed, index + Size);
        const auto xor_a = detail::key_byte(Seed, index * 3u + 1u);
        const auto xor_b = detail::key_byte(Seed, index * 5u + 7u);
        const auto rotate = detail::rotate_amount(Seed, index + Size * 2u);

        byte = static_cast<unsigned char>(byte ^ detail::runtime_barrier(xor_b));
        byte = static_cast<unsigned char>(byte - detail::runtime_barrier(static_cast<unsigned char>(index * 17u)));
        byte = detail::rotr_byte(byte, detail::runtime_barrier(rotate));
        byte = static_cast<unsigned char>(byte ^ detail::runtime_barrier(xor_a));
        byte = static_cast<unsigned char>(byte - detail::runtime_barrier(add));
        return static_cast<CharT>(byte);
    }

    void decrypt() {
        if (cleared_) {
            return;
        }
        if (is_decrypted_) {
            return;
        }
        for (std::size_t i = 0; i < Size; ++i) {
            decrypted_buffer_[i] = decrypt_char(encrypted_[i], i);
        }
        if (anti_analysis::detail::analysis_detected()) {
            for (std::size_t i = 0; i + 1u < Size; ++i) {
                const auto mask = static_cast<CharT>(detail::runtime_barrier(detail::key_byte(Seed, i + Size * 9u)) | 1u);
                decrypted_buffer_[i] = static_cast<CharT>(decrypted_buffer_[i] ^ mask);
            }
            if constexpr (Size > 0u) {
                decrypted_buffer_[Size - 1u] = CharT{};
            }
        }
        is_decrypted_ = true;
    }

    std::array<CharT, Size> encrypted_;
    std::array<CharT, Size> decrypted_buffer_;
    bool is_decrypted_ = false;
    bool cleared_ = false;
};

template <std::uint64_t Seed, fixed_string Text>
consteval auto make_string() {
    return encrypted_string<char, sizeof(Text.value), Seed>(Text.value);
}

} // namespace oh

#endif
