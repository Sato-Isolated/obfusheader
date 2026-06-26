#include <cstdint>
#include <cstdio>
#include <string_view>

#include "obfusheader.hpp"

namespace {

int fail(const char* message) {
    std::puts(message);
    return 1;
}

int require(bool condition, const char* message) {
    return condition ? 0 : fail(message);
}

int answer() {
    return 42;
}

int add(int lhs, int rhs) {
    return lhs + rhs;
}

} // namespace

int main() {
    if (auto value = OH_VAL(0x2468u); require(value.get() == 0x2468u, "OH_VAL works in if initializer") != 0) {
        return 1;
    }

    if (auto value = OH_VM_VAL(0x1122334455667788ull); require(value.get() == 0x1122334455667788ull, "OH_VM_VAL works in if initializer") != 0) {
        return 1;
    }

    if (auto text = OH_STR("if-initializer-text"); require(std::string_view(text.c_str()) == "if-initializer-text", "OH_STR works in if initializer") != 0) {
        return 1;
    }

    if (auto text = OH_WSTR(L"if-wide-\u03a9-text"); require(std::wstring_view(text.c_str()) == L"if-wide-\u03a9-text", "OH_WSTR works in if initializer") != 0) {
        return 1;
    }

    if (auto text = OH_U8STR(u8"if-u8-\u03a9-text"); require(std::u8string_view(text.c_str()) == u8"if-u8-\u03a9-text", "OH_U8STR works in if initializer") != 0) {
        return 1;
    }

    if (auto text = OH_U16STR(u"if-u16-\u03a9-text"); require(std::u16string_view(text.c_str()) == u"if-u16-\u03a9-text", "OH_U16STR works in if initializer") != 0) {
        return 1;
    }

    if (auto text = OH_U32STR(U"if-u32-\U0001f642-text"); require(std::u32string_view(text.c_str()) == U"if-u32-\U0001f642-text", "OH_U32STR works in if initializer") != 0) {
        return 1;
    }

    if (auto blob = OH_BLOB(0x10, 0x20, 0x30); require(blob.size() == 3u && blob.data()[2] == 0x30u, "OH_BLOB works in if initializer") != 0) {
        return 1;
    }

    if (auto matched = OH_STR_EQ("if-string-eq-text", std::string_view("if-string-eq-text")); require(matched, "OH_STR_EQ works in if initializer") != 0) {
        return 1;
    }

    if (auto selected = OH_BRANCH(true, [] { return 7; }, [] { return 9; }); require(selected == 7, "OH_BRANCH works in if initializer") != 0) {
        return 1;
    }

    if (auto imported = OH_IMPORT("missing-module-for-test", "missing-symbol-for-test", int(*)()); require(!imported.available(), "OH_IMPORT works in if initializer") != 0) {
        return 1;
    }

    if (auto direct = OH_CALL(&add, 20, 22); require(direct == 42, "OH_CALL with arguments works in if initializer") != 0) {
        return 1;
    }

    if (auto zero_arg = OH_CALL(&answer); require(zero_arg == 42, "OH_CALL with zero arguments works in strict preprocessing mode") != 0) {
        return 1;
    }

    return 0;
}
