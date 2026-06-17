#ifndef OBFUSHEADER_DETAIL_CALL_HPP
#define OBFUSHEADER_DETAIL_CALL_HPP

#include "anti_analysis.hpp"
#include "config.hpp"

#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace oh {

namespace detail {

template <std::uint64_t Seed, typename Fn>
Fn decode_callable(std::uintptr_t encoded) {
    const auto mask = static_cast<std::uintptr_t>(stream(Seed, 13) ^ stream(Seed, 29));
    return reinterpret_cast<Fn>(encoded ^ mask);
}

template <std::uint64_t Seed, typename Fn>
std::uintptr_t encode_callable(Fn function) {
    const auto mask = static_cast<std::uintptr_t>(stream(Seed, 13) ^ stream(Seed, 29));
    return reinterpret_cast<std::uintptr_t>(function) ^ mask;
}

template <std::uint64_t Seed, typename R>
R poison_return() {
    if constexpr (std::is_void_v<R>) {
        return;
    } else if constexpr (std::is_same_v<R, bool>) {
        return true;
    } else if constexpr (std::is_enum_v<R>) {
        using storage_type = std::underlying_type_t<R>;
        return static_cast<R>(static_cast<storage_type>(key_byte(Seed, 53) | 1u));
    } else if constexpr (std::is_integral_v<R>) {
        return static_cast<R>(key_byte(Seed, 53) | 1u);
    } else if constexpr (std::is_floating_point_v<R>) {
        return static_cast<R>(static_cast<unsigned>(key_byte(Seed, 53) | 1u));
    } else if constexpr (std::is_pointer_v<R>) {
        return nullptr;
    } else {
        static_assert(std::is_default_constructible_v<R>, "OH_CALL poisoned non-scalar returns must be default constructible");
        return R{};
    }
}

} // namespace detail

template <std::uint64_t Seed, typename Fn, typename... Args>
decltype(auto) call(Fn function, Args&&... args) {
    static_assert(std::is_pointer_v<Fn> && std::is_function_v<std::remove_pointer_t<Fn>>, "OH_CALL expects a function pointer");

    using result_type = std::invoke_result_t<Fn, Args...>;
    const auto encoded = detail::encode_callable<Seed>(function);
    volatile auto selector = static_cast<unsigned>((detail::stream(Seed, 7) ^ detail::stream(Seed, 9)) % 3u);
    const std::uintptr_t table[3] = {
        encoded,
        encoded ^ static_cast<std::uintptr_t>(selector & 0u),
        encoded
    };
    auto masked = detail::decode_callable<Seed, Fn>(table[selector % 3u]);

    if (anti_analysis::detail::analysis_detected()) {
        if constexpr (std::is_void_v<result_type>) {
            return;
        } else {
            return detail::poison_return<Seed, result_type>();
        }
    }

    if constexpr (std::is_void_v<result_type>) {
        std::invoke(masked, std::forward<Args>(args)...);
    } else {
        return std::invoke(masked, std::forward<Args>(args)...);
    }
}

} // namespace oh

#endif
