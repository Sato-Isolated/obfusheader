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

    auto wide_secret = OH_WSTR(L"analysis-wide-\u03a9-text");
    if (require(std::wstring_view(wide_secret.c_str()) != L"analysis-wide-\u03a9-text", "OH_WSTR poisons wide text under forced analysis") != 0) {
        return 1;
    }

    auto u8_secret = OH_U8STR(u8"analysis-u8-\u03a9-text");
    if (require(std::u8string_view(u8_secret.c_str()) != u8"analysis-u8-\u03a9-text", "OH_U8STR poisons UTF-8 text under forced analysis") != 0) {
        return 1;
    }

    auto u16_secret = OH_U16STR(u"analysis-u16-\u03a9-text");
    if (require(std::u16string_view(u16_secret.c_str()) != u"analysis-u16-\u03a9-text", "OH_U16STR poisons UTF-16 text under forced analysis") != 0) {
        return 1;
    }

    auto u32_secret = OH_U32STR(U"analysis-u32-\U0001f642-text");
    if (require(std::u32string_view(u32_secret.c_str()) != U"analysis-u32-\U0001f642-text", "OH_U32STR poisons UTF-32 text under forced analysis") != 0) {
        return 1;
    }

    auto blob = OH_BLOB(0xde, 0xad, 0xbe, 0xef);
    const auto blob_view = blob.view();
    if (require(blob_view[0] != 0xdeu
            && blob_view[1] != 0xadu
            && blob_view[2] != 0xbeu
            && blob_view[3] != 0xefu,
            "OH_BLOB poisons bytes under forced analysis") != 0) {
        return 1;
    }

    if (require(!OH_STR_EQ("analysis-string-eq-secret", "analysis-string-eq-secret"), "OH_STR_EQ rejects matches under forced analysis") != 0) {
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
