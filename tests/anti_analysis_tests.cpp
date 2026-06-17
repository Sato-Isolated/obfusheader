#define OH_FORCE_ANALYSIS_DETECTED

#include <cstdint>
#include <cstdio>
#include <string_view>

#include "obfusheader.hpp"

namespace {

int true_calls = 0;
int false_calls = 0;

int add(int lhs, int rhs) {
    return lhs + rhs;
}

int fail(const char* message) {
    std::puts(message);
    return 1;
}

int require(bool condition, const char* message) {
    return condition ? 0 : fail(message);
}

} // namespace

int main() {
    auto secret = OH_STR("analysis-sensitive-text");
    if (require(std::string_view(secret.c_str()) != "analysis-sensitive-text", "OH_STR poisons text under forced analysis") != 0) {
        return 1;
    }

    const auto value = OH_VAL(0x44556677u);
    if (require(value.get() != 0x44556677u, "OH_VAL poisons scalar under forced analysis") != 0) {
        return 1;
    }

    const auto vm_value = OH_VM_VAL(0x1122334455667788ull);
    if (require(vm_value.get() != 0x1122334455667788ull, "OH_VM_VAL poisons scalar under forced analysis") != 0) {
        return 1;
    }

    const auto bool_value = OH_VAL(true);
    if (require(!bool_value.get(), "OH_VAL poisons bool under forced analysis") != 0) {
        return 1;
    }

    const auto vm_bool_value = OH_VM_VAL(true);
    if (require(!vm_bool_value.get(), "OH_VM_VAL poisons bool under forced analysis") != 0) {
        return 1;
    }

    const auto called = OH_CALL(&add, 20, 22);
    if (require(called != 42, "OH_CALL poisons return under forced analysis") != 0) {
        return 1;
    }

    auto branch_value = OH_BRANCH(true,
        [] {
            ++true_calls;
            return 7;
        },
        [] {
            ++false_calls;
            return 9;
        });
    if (require(branch_value == 9, "OH_BRANCH selects poisoned branch under forced analysis") != 0) {
        return 1;
    }
    if (require(true_calls == 0 && false_calls == 1, "OH_BRANCH evaluates only one branch under forced analysis") != 0) {
        return 1;
    }

    auto import = OH_IMPORT("missing-module-for-test", "missing-symbol-for-test", int(*)());
    if (require(!import.available(), "OH_IMPORT remains unavailable under forced analysis") != 0) {
        return 1;
    }

    return 0;
}
