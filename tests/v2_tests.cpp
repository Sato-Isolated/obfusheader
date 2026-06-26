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

    auto wide_secret = OH_WSTR(L"wide-\u03a9-text");
    if (require(wide_secret.view() == L"wide-\u03a9-text", "OH_WSTR decrypts wide string view") != 0) {
        return 1;
    }
    if (require(std::wstring_view(wide_secret.c_str()) == L"wide-\u03a9-text", "OH_WSTR decrypts wide c_str") != 0) {
        return 1;
    }
    wide_secret.clear();
    if (require(wide_secret.c_str()[0] == L'\0', "OH_WSTR clears decrypted bytes") != 0) {
        return 1;
    }

    auto u8_secret = OH_U8STR(u8"u8-\u03a9-text");
    if (require(u8_secret.view() == u8"u8-\u03a9-text", "OH_U8STR decrypts UTF-8 string view") != 0) {
        return 1;
    }
    if (require(std::u8string_view(u8_secret.c_str()) == u8"u8-\u03a9-text", "OH_U8STR decrypts UTF-8 c_str") != 0) {
        return 1;
    }
    u8_secret.clear();
    if (require(u8_secret.c_str()[0] == u8'\0', "OH_U8STR clears decrypted bytes") != 0) {
        return 1;
    }

    auto u16_secret = OH_U16STR(u"u16-\u03a9-text");
    if (require(u16_secret.view() == u"u16-\u03a9-text", "OH_U16STR decrypts UTF-16 string view") != 0) {
        return 1;
    }
    if (require(std::u16string_view(u16_secret.c_str()) == u"u16-\u03a9-text", "OH_U16STR decrypts UTF-16 c_str") != 0) {
        return 1;
    }
    u16_secret.clear();
    if (require(u16_secret.c_str()[0] == u'\0', "OH_U16STR clears decrypted bytes") != 0) {
        return 1;
    }

    auto u32_secret = OH_U32STR(U"u32-\U0001f642-text");
    if (require(u32_secret.view() == U"u32-\U0001f642-text", "OH_U32STR decrypts UTF-32 string view") != 0) {
        return 1;
    }
    if (require(std::u32string_view(u32_secret.c_str()) == U"u32-\U0001f642-text", "OH_U32STR decrypts UTF-32 c_str") != 0) {
        return 1;
    }
    u32_secret.clear();
    if (require(u32_secret.c_str()[0] == U'\0', "OH_U32STR clears decrypted bytes") != 0) {
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
    if (require(import.invoke_or(1234) == 1234, "OH_IMPORT invoke_or returns fallback for missing symbols") != 0) {
        return 1;
    }

    using missing_void_t = void (*)(int*);
    int untouched = 77;
    auto missing_void = OH_IMPORT("missing-module-for-test", "missing-void-symbol-for-test", missing_void_t);
    if (require(!missing_void.invoke_if(&untouched), "OH_IMPORT invoke_if reports missing void symbols") != 0) {
        return 1;
    }
    if (require(untouched == 77, "OH_IMPORT invoke_if does not call missing void symbols") != 0) {
        return 1;
    }

    static_assert(oh::current_preset == oh::preset::balanced);
    return 0;
}
