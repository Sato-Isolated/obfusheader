#ifndef OBFUSHEADER_DETAIL_ENCRYPTED_BLOB_HPP
#define OBFUSHEADER_DETAIL_ENCRYPTED_BLOB_HPP

#include "anti_analysis.hpp"
#include "config.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <type_traits>

namespace oh {

namespace detail {

template <auto... Bytes>
struct blob_bytes {};

template <typename>
inline constexpr bool blob_dependent_false = false;

template <auto Byte>
consteval unsigned char checked_blob_byte() {
    using byte_type = std::remove_cv_t<decltype(Byte)>;
    if constexpr (std::is_integral_v<byte_type> && !std::is_same_v<byte_type, bool>) {
        static_assert(Byte >= 0 && Byte <= 0xff, "OH_BLOB byte constants must be in the range [0, 255]");
        return static_cast<unsigned char>(Byte);
    } else {
        static_assert(blob_dependent_false<byte_type>, "OH_BLOB expects integer byte constants");
        return 0u;
    }
}

} // namespace detail

template <std::size_t Size, std::uint64_t Seed>
class encrypted_blob {
public:
    constexpr explicit encrypted_blob(const std::array<unsigned char, Size>& value) {
        for (std::size_t i = 0; i < Size; ++i) {
            encrypted_[i] = encrypt_byte(value[i], i);
            decrypted_buffer_[i] = 0u;
        }
    }

    const unsigned char* data() {
        decrypt();
        return decrypted_buffer_.data();
    }

    std::span<const unsigned char, Size> view() {
        decrypt();
        return std::span<const unsigned char, Size>(decrypted_buffer_.data(), decrypted_buffer_.size());
    }

    constexpr std::size_t size() const {
        return Size;
    }

    void clear() {
        for (std::size_t i = 0; i < decrypted_buffer_.size(); ++i) {
            volatile unsigned char* item = &decrypted_buffer_[i];
            *item = 0u;
        }
        is_decrypted_ = false;
        cleared_ = true;
    }

private:
    static constexpr unsigned char encrypt_byte(unsigned char value, std::size_t index) {
        auto byte = static_cast<unsigned char>(value + detail::key_byte(Seed, index + Size + 5u));
        byte = static_cast<unsigned char>(byte ^ detail::key_byte(Seed, index * 7u + 3u));
        byte = detail::rotl_byte(byte, detail::rotate_amount(Seed, index + Size * 3u + 11u));
        byte = static_cast<unsigned char>(byte + static_cast<unsigned char>(index * 23u));
        byte = static_cast<unsigned char>(byte ^ detail::key_byte(Seed, index * 13u + 17u));
        return byte;
    }

    static unsigned char decrypt_byte(unsigned char value, std::size_t index) {
        auto byte = static_cast<unsigned char>(value ^ detail::runtime_barrier(detail::key_byte(Seed, index * 13u + 17u)));
        byte = static_cast<unsigned char>(byte - detail::runtime_barrier(static_cast<unsigned char>(index * 23u)));
        byte = detail::rotr_byte(byte, detail::runtime_barrier(detail::rotate_amount(Seed, index + Size * 3u + 11u)));
        byte = static_cast<unsigned char>(byte ^ detail::runtime_barrier(detail::key_byte(Seed, index * 7u + 3u)));
        byte = static_cast<unsigned char>(byte - detail::runtime_barrier(detail::key_byte(Seed, index + Size + 5u)));
        return byte;
    }

    void decrypt() {
        if (cleared_) {
            return;
        }
        if (is_decrypted_) {
            return;
        }
        const auto suspicious = anti_analysis::detail::analysis_detected();
        for (std::size_t i = 0; i < Size; ++i) {
            auto byte = decrypt_byte(encrypted_[i], i);
            if (suspicious) {
                const auto mask = static_cast<unsigned char>(detail::runtime_barrier(detail::key_byte(Seed, i + Size * 9u)) | 1u);
                byte = static_cast<unsigned char>(byte ^ mask);
            }
            decrypted_buffer_[i] = byte;
        }
        is_decrypted_ = true;
    }

    std::array<unsigned char, Size> encrypted_;
    std::array<unsigned char, Size> decrypted_buffer_;
    bool is_decrypted_ = false;
    bool cleared_ = false;
};

template <std::uint64_t Seed, auto... Bytes>
consteval auto make_blob(detail::blob_bytes<Bytes...>) {
    static_assert(sizeof...(Bytes) > 0, "OH_BLOB expects at least one byte");
    return encrypted_blob<sizeof...(Bytes), Seed>(std::array<unsigned char, sizeof...(Bytes)>{detail::checked_blob_byte<Bytes>()...});
}

} // namespace oh

#endif
