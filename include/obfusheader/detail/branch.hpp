#ifndef OBFUSHEADER_DETAIL_BRANCH_HPP
#define OBFUSHEADER_DETAIL_BRANCH_HPP

#include "anti_analysis.hpp"
#include "config.hpp"

#include <utility>

namespace oh {

template <std::uint64_t Seed, typename Then, typename Else>
decltype(auto) branch(bool condition, Then&& on_true, Else&& on_false) {
    const auto poisoned = anti_analysis::detail::analysis_detected();
    const auto masked_condition = poisoned ? !condition : condition;
    const auto guard = static_cast<bool>((detail::stream(Seed, 0) | 1u) && masked_condition);
    if (guard) {
        return std::forward<Then>(on_true)();
    }
    return std::forward<Else>(on_false)();
}

} // namespace oh

#endif
