#ifndef OBFUSHEADER_DETAIL_ENCRYPTED_STRING_HPP
#define OBFUSHEADER_DETAIL_ENCRYPTED_STRING_HPP

#include "anti_analysis.hpp"
#include "config.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace oh {

template <typename CharT, std::size_t Size>
struct fixed_string {
    using value_type = CharT;
    static constexpr std::size_t size = Size;

    CharT value[Size]{};

    consteval fixed_string(const CharT (&text)[Size]) {
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
            const auto bytes = to_bytes(value[i]);
            for (std::size_t j = 0; j < bytes.size(); ++j) {
                const auto index = byte_index(i, j);
                encrypted_[index] = crypt_byte(bytes[j], index);
            }
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
    using char_bytes = std::array<unsigned char, sizeof(CharT)>;
    static constexpr std::size_t byte_size = Size * sizeof(CharT);

    static constexpr std::size_t byte_index(std::size_t code_unit, std::size_t byte) {
        return code_unit * sizeof(CharT) + byte;
    }

    static constexpr char_bytes to_bytes(CharT value) {
        return std::bit_cast<char_bytes>(value);
    }

    static constexpr CharT from_bytes(const char_bytes& value) {
        return std::bit_cast<CharT>(value);
    }

    static constexpr unsigned char crypt_byte(unsigned char value, std::size_t index) {
        auto byte = value;
        const auto add = detail::key_byte(Seed, index + byte_size);
        const auto xor_a = detail::key_byte(Seed, index * 3u + 1u);
        const auto xor_b = detail::key_byte(Seed, index * 5u + 7u);
        const auto rotate = detail::rotate_amount(Seed, index + byte_size * 2u);

        byte = static_cast<unsigned char>(byte + add);
        byte = static_cast<unsigned char>(byte ^ xor_a);
        byte = detail::rotl_byte(byte, rotate);
        byte = static_cast<unsigned char>(byte + static_cast<unsigned char>(index * 17u));
        byte = static_cast<unsigned char>(byte ^ xor_b);
        return byte;
    }

    static constexpr unsigned char decrypt_byte(unsigned char value, std::size_t index) {
        auto byte = value;
        const auto add = detail::key_byte(Seed, index + byte_size);
        const auto xor_a = detail::key_byte(Seed, index * 3u + 1u);
        const auto xor_b = detail::key_byte(Seed, index * 5u + 7u);
        const auto rotate = detail::rotate_amount(Seed, index + byte_size * 2u);

        byte = static_cast<unsigned char>(byte ^ detail::runtime_barrier(xor_b));
        byte = static_cast<unsigned char>(byte - detail::runtime_barrier(static_cast<unsigned char>(index * 17u)));
        byte = detail::rotr_byte(byte, detail::runtime_barrier(rotate));
        byte = static_cast<unsigned char>(byte ^ detail::runtime_barrier(xor_a));
        byte = static_cast<unsigned char>(byte - detail::runtime_barrier(add));
        return byte;
    }

    void decrypt() {
        if (cleared_) {
            return;
        }
        if (is_decrypted_) {
            return;
        }
        std::array<unsigned char, byte_size> decrypted_bytes{};
        for (std::size_t i = 0; i < byte_size; ++i) {
            decrypted_bytes[i] = decrypt_byte(encrypted_[i], i);
        }
        if (anti_analysis::detail::analysis_detected()) {
            const auto payload_bytes = Size == 0u ? 0u : (Size - 1u) * sizeof(CharT);
            for (std::size_t i = 0; i < payload_bytes; ++i) {
                const auto mask = static_cast<unsigned char>(detail::runtime_barrier(detail::key_byte(Seed, i + byte_size * 9u)) | 1u);
                decrypted_bytes[i] = static_cast<unsigned char>(decrypted_bytes[i] ^ mask);
            }
            if constexpr (Size > 0u) {
                for (std::size_t i = payload_bytes; i < byte_size; ++i) {
                    decrypted_bytes[i] = 0u;
                }
            }
        }
        for (std::size_t i = 0; i < Size; ++i) {
            char_bytes bytes{};
            for (std::size_t j = 0; j < bytes.size(); ++j) {
                bytes[j] = decrypted_bytes[byte_index(i, j)];
            }
            decrypted_buffer_[i] = from_bytes(bytes);
        }
        is_decrypted_ = true;
    }

    std::array<unsigned char, byte_size> encrypted_;
    std::array<CharT, Size> decrypted_buffer_;
    bool is_decrypted_ = false;
    bool cleared_ = false;
};

template <std::uint64_t Seed, fixed_string Text>
consteval auto make_checked_string() {
    using text_type = std::remove_cv_t<decltype(Text)>;
    using char_type = typename text_type::value_type;
    return encrypted_string<char_type, text_type::size, Seed>(Text.value);
}

template <typename Expected, std::uint64_t Seed, fixed_string Text>
consteval auto make_typed_string() {
    using text_type = std::remove_cv_t<decltype(Text)>;
    using char_type = typename text_type::value_type;
    static_assert(std::is_same_v<char_type, Expected>, "OH string macro requires a matching string literal character type");
    return make_checked_string<Seed, Text>();
}

template <std::uint64_t Seed, fixed_string Text>
consteval auto make_string() {
    return make_typed_string<char, Seed, Text>();
}

template <std::uint64_t Seed, fixed_string Text>
consteval auto make_wide_string() {
    return make_typed_string<wchar_t, Seed, Text>();
}

template <std::uint64_t Seed, fixed_string Text>
consteval auto make_u8string() {
    return make_typed_string<char8_t, Seed, Text>();
}

template <std::uint64_t Seed, fixed_string Text>
consteval auto make_u16string() {
    return make_typed_string<char16_t, Seed, Text>();
}

template <std::uint64_t Seed, fixed_string Text>
consteval auto make_u32string() {
    return make_typed_string<char32_t, Seed, Text>();
}

} // namespace oh

#endif
