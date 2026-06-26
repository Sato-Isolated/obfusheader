#include <cstdio>

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
    auto hidden_value = OH_VM_VAL(0x1234abcdu);
    const auto* narrow = hidden.c_str();
    const auto* wide = hidden_wide.c_str();
    const auto* u8 = hidden_u8.c_str();
    const auto* u16 = hidden_u16.c_str();
    const auto* u32 = hidden_u32.c_str();
    std::puts(narrow);
    value_sink = static_cast<unsigned>(narrow[0])
        + static_cast<unsigned>(wide[0])
        + static_cast<unsigned>(u8[0])
        + static_cast<unsigned>(u16[0])
        + static_cast<unsigned>(u32[0]);
    value_sink = hidden_value.get();
    hidden.clear();
    hidden_wide.clear();
    hidden_u8.clear();
    hidden_u16.clear();
    hidden_u32.clear();
    return value_sink == 0u ? 1 : 0;
}
