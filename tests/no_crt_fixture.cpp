#include "obfusheader.hpp"

namespace {

int answer() {
    return 42;
}

int add(int lhs, int rhs) {
    return lhs + rhs;
}

} // namespace

extern "C" int entry() {
    auto text = OH_STR("no-crt-text");
    const char* raw = text.c_str();
    const auto value = OH_VAL(0x12345678u);
    const auto vm_value = OH_VM_VAL(0xabcdef12u);
    const auto sum = OH_CALL(&add, 2, 3);
    const auto zero_arg = OH_CALL(&answer);
    const auto branch = OH_BRANCH(sum == 5, [] { return 7; }, [] { return 9; });

    using get_current_process_id_t = unsigned long (WINAPI*)();
    auto get_current_process_id = OH_IMPORT("kernel32.dll", "GetCurrentProcessId", get_current_process_id_t);

    const bool ok = raw[0] == 'n'
        && raw[1] == 'o'
        && raw[2] == '-'
        && raw[3] == 'c'
        && raw[4] == 'r'
        && raw[5] == 't'
        && raw[6] == '-'
        && raw[7] == 't'
        && raw[8] == 'e'
        && raw[9] == 'x'
        && raw[10] == 't'
        && raw[11] == '\0'
        && value.get() == 0x12345678u
        && vm_value.get() == 0xabcdef12u
        && sum == 5
        && zero_arg == 42
        && branch == 7
        && get_current_process_id
        && get_current_process_id() != 0ul;

    text.clear();
    return ok ? 0 : 1;
}
