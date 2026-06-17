#include <cstdio>

#include "obfusheader.hpp"

namespace {

volatile unsigned value_sink = 0;

}

int main() {
    auto hidden = OH_STR("needle-v2-binary-scan-secret");
    auto hidden_value = OH_VM_VAL(0x1234abcdu);
    std::puts(hidden.c_str());
    value_sink = hidden_value.get();
    hidden.clear();
    return value_sink == 0u ? 1 : 0;
}
