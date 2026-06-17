#define OH_CONFIG_PRESET ::oh::preset::strong

#include <cstdint>
#include <cstdio>

#include "obfusheader.hpp"

namespace {

int fail(const char* message) {
    std::puts(message);
    return 1;
}

int require(bool condition, const char* message) {
    return condition ? 0 : fail(message);
}

} // namespace

int main() {
    static_assert(oh::current_preset == oh::preset::strong);

    const auto value = OH_VAL(0xa5b6c7d8u);
    if (require(value.get() == 0xa5b6c7d8u, "OH_VAL uses the strong preset value path") != 0) {
        return 1;
    }

    const auto signed_value = OH_VAL(-67890);
    if (require(signed_value.get() == -67890, "OH_VAL strong preset handles signed scalar") != 0) {
        return 1;
    }

    return 0;
}
