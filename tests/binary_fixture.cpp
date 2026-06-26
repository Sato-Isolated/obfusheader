#include <cstddef>
#include <cstdio>
#include <string_view>

#include "obfusheader.hpp"

namespace {

volatile unsigned value_sink = 0;

}

int main() {
    auto hidden = OH_STR("needle-v2-binary-scan-secret");
    auto hidden_wide = OH_WSTR(L"wide-binary-scan-secret");
    auto hidden_u8 = OH_U8STR(u8"u8-binary-scan-secret");
    auto hidden_u16 = OH_U16STR(u"u16-binary-scan-secret");
    auto hidden_u32 = OH_U32STR(U"u32-binary-scan-secret");
    auto hidden_blob = OH_BLOB(0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10);
    auto hidden_value = OH_VM_VAL(0x1234abcdu);
    const auto* narrow = hidden.c_str();
    const auto* wide = hidden_wide.c_str();
    const auto* u8 = hidden_u8.c_str();
    const auto* u16 = hidden_u16.c_str();
    const auto* u32 = hidden_u32.c_str();
    const auto blob_view = hidden_blob.view();

    const unsigned char encoded_eq_candidate[] = {
        0x26u, 0x21u, 0x27u, 0x78u, 0x30u, 0x24u, 0x78u, 0x37u, 0x3cu, 0x3bu,
        0x34u, 0x27u, 0x2cu, 0x78u, 0x26u, 0x36u, 0x34u, 0x3bu, 0x78u, 0x26u,
        0x30u, 0x36u, 0x27u, 0x30u, 0x21u
    };
    char eq_candidate[sizeof(encoded_eq_candidate)]{};
    for (std::size_t i = 0; i < sizeof(encoded_eq_candidate); ++i) {
        eq_candidate[i] = static_cast<char>(encoded_eq_candidate[i] ^ 0x55u);
    }
    const auto eq_matched = OH_STR_EQ("str-eq-binary-scan-secret", std::string_view(eq_candidate, sizeof(eq_candidate)));

    std::puts(narrow);
    value_sink = static_cast<unsigned>(narrow[0])
        + static_cast<unsigned>(wide[0])
        + static_cast<unsigned>(u8[0])
        + static_cast<unsigned>(u16[0])
        + static_cast<unsigned>(u32[0]);
    for (const auto byte : blob_view) {
        value_sink += byte;
    }
    value_sink += hidden_value.get();
    value_sink += eq_matched ? 1u : 0u;
    hidden.clear();
    hidden_wide.clear();
    hidden_u8.clear();
    hidden_u16.clear();
    hidden_u32.clear();
    hidden_blob.clear();
    return value_sink == 0u || !eq_matched ? 1 : 0;
}
