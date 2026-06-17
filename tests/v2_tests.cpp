#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>

#include "obfusheader.hpp"

namespace {

int add(int lhs, int rhs) {
    return lhs + rhs;
}

int out_value = 0;

void set_out(int value) {
    out_value = value;
}

enum class sample_mode : std::uint32_t {
    enabled = 0x13572468u
};

int fail(const char* message) {
    std::puts(message);
    return 1;
}

int require(bool condition, const char* message) {
    return condition ? 0 : fail(message);
}

} // namespace

int main() {
    auto secret = OH_STR("correct horse battery staple");
    if (require(std::string_view(secret.c_str()) == "correct horse battery staple", "OH_STR decrypts string") != 0) {
        return 1;
    }

    secret.clear();
    if (require(secret.c_str()[0] == '\0', "OH_STR clears decrypted bytes") != 0) {
        return 1;
    }

    const auto number = OH_VAL(0x12345678u);
    if (require(number.get() == 0x12345678u, "OH_VAL decrypts unsigned scalar") != 0) {
        return 1;
    }

    const auto vm_number = OH_VM_VAL(0x89abcdefu);
    if (require(vm_number.get() == 0x89abcdefu, "OH_VM_VAL interprets unsigned scalar") != 0) {
        return 1;
    }

    const auto vm_wide = OH_VM_VAL(0x1122334455667788ull);
    if (require(vm_wide.get() == 0x1122334455667788ull, "OH_VM_VAL interprets 64-bit scalar") != 0) {
        return 1;
    }

    const auto vm_signed = OH_VM_VAL(-12345);
    if (require(vm_signed.get() == -12345, "OH_VM_VAL interprets signed scalar") != 0) {
        return 1;
    }

    const auto vm_bool = OH_VM_VAL(true);
    if (require(vm_bool.get(), "OH_VM_VAL interprets bool scalar") != 0) {
        return 1;
    }

    const auto vm_enum = OH_VM_VAL(sample_mode::enabled);
    if (require(vm_enum.get() == sample_mode::enabled, "OH_VM_VAL interprets enum scalar") != 0) {
        return 1;
    }

    if (require(OH_CALL(&add, 20, 22) == 42, "OH_CALL returns direct-call result") != 0) {
        return 1;
    }

    OH_CALL(&set_out, 73);
    if (require(out_value == 73, "OH_CALL handles void functions") != 0) {
        return 1;
    }

    auto branch_value = OH_BRANCH(true, [] { return 7; }, [] { return 9; });
    if (require(branch_value == 7, "OH_BRANCH selects true branch") != 0) {
        return 1;
    }

    auto import = OH_IMPORT("missing-module-for-test", "missing-symbol-for-test", int(*)());
    if (require(!import.available(), "OH_IMPORT reports missing symbols without crashing") != 0) {
        return 1;
    }

    static_assert(oh::current_preset == oh::preset::balanced);
    return 0;
}
