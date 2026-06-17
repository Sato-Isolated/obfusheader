#ifndef OBFUSHEADER_DETAIL_VM_HPP
#define OBFUSHEADER_DETAIL_VM_HPP

#include "anti_analysis.hpp"
#include "config.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace oh {

namespace detail {

enum class vm_opcode : unsigned char {
    load,
    xor_key,
    xor_junk,
    add_key,
    add_junk,
    sub_junk,
    flip,
    rol,
    ror,
    end
};

template <std::size_t Bytes>
struct vm_program {
    std::array<vm_opcode, Bytes * 8u + 1u> opcodes{};
    std::array<unsigned char, Bytes * 6u> operands{};
};

template <typename T, bool IsEnum = std::is_enum_v<T>>
struct vm_storage {
    using type = T;
};

template <typename T>
struct vm_storage<T, true> {
    using type = std::underlying_type_t<T>;
};

template <typename T>
using vm_storage_type = typename vm_storage<T>::type;

template <std::uint64_t Seed, std::size_t Bytes>
constexpr vm_program<Bytes> make_vm_program() {
    vm_program<Bytes> program{};
    std::size_t op_index = 0;
    std::size_t operand_index = 0;

    for (std::size_t i = 0; i < Bytes; ++i) {
        program.opcodes[op_index++] = vm_opcode::load;
        program.opcodes[op_index++] = vm_opcode::xor_key;
        program.operands[operand_index++] = detail::key_byte(Seed, i + Bytes);
        program.opcodes[op_index++] = vm_opcode::add_junk;
        program.operands[operand_index++] = detail::key_byte(Seed, i + Bytes * 3u);
        program.opcodes[op_index++] = vm_opcode::sub_junk;
        program.operands[operand_index++] = detail::key_byte(Seed, i + Bytes * 3u);
        if ((detail::stream(Seed, i + Bytes * 5u) & 1u) != 0u) {
            program.opcodes[op_index++] = vm_opcode::flip;
            program.opcodes[op_index++] = vm_opcode::flip;
        } else {
            program.opcodes[op_index++] = vm_opcode::xor_junk;
            program.operands[operand_index++] = 0u;
            program.opcodes[op_index++] = vm_opcode::xor_junk;
            program.operands[operand_index++] = 0u;
        }
        program.opcodes[op_index++] = (detail::stream(Seed, i) & 1u) != 0u ? vm_opcode::rol : vm_opcode::ror;
        program.operands[operand_index++] = static_cast<unsigned char>((detail::stream(Seed, i + Bytes) % 7u) + 1u);
        program.opcodes[op_index++] = vm_opcode::add_key;
    }

    program.opcodes[op_index] = vm_opcode::end;
    return program;
}

} // namespace detail

template <typename T, std::uint64_t Seed>
class virtualized_value {
public:
    static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>, "OH_VM_VAL supports arithmetic and enum values");

    constexpr explicit virtualized_value(T value) {
        const auto storage = static_cast<storage_type>(value);
        const auto bytes = std::bit_cast<byte_array>(storage);
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            encrypted_[i] = encrypt_byte(bytes[i], i);
        }
    }

    T get() const {
        byte_array bytes;
        const auto program = detail::make_vm_program<Seed, sizeof(storage_type)>();
        const auto suspicious = anti_analysis::detail::analysis_detected();

        std::size_t byte_index = 0;
        std::size_t operand_index = 0;
        unsigned char accumulator = 0;

        for (const auto opcode : program.opcodes) {
            switch (opcode) {
            case detail::vm_opcode::load:
                accumulator = encrypted_[byte_index];
                break;
            case detail::vm_opcode::xor_key:
                accumulator = static_cast<unsigned char>(accumulator ^ detail::runtime_barrier(program.operands[operand_index++]));
                break;
            case detail::vm_opcode::xor_junk:
                accumulator = static_cast<unsigned char>(accumulator ^ detail::runtime_barrier(program.operands[operand_index++]));
                break;
            case detail::vm_opcode::add_key:
                accumulator = static_cast<unsigned char>(accumulator + detail::runtime_barrier(detail::key_byte(Seed, byte_index)));
                if (suspicious && !std::is_same_v<storage_type, bool>) {
                    accumulator = static_cast<unsigned char>(accumulator ^ detail::runtime_barrier(detail::key_byte(Seed, byte_index + 97u)));
                }
                bytes[byte_index++] = accumulator;
                break;
            case detail::vm_opcode::add_junk:
                accumulator = static_cast<unsigned char>(accumulator + detail::runtime_barrier(program.operands[operand_index++]));
                break;
            case detail::vm_opcode::sub_junk:
                accumulator = static_cast<unsigned char>(accumulator - detail::runtime_barrier(program.operands[operand_index++]));
                break;
            case detail::vm_opcode::flip:
                accumulator = static_cast<unsigned char>(~accumulator);
                break;
            case detail::vm_opcode::rol:
                accumulator = detail::rotl_byte(accumulator, detail::runtime_barrier(program.operands[operand_index++]));
                break;
            case detail::vm_opcode::ror:
                accumulator = detail::rotr_byte(accumulator, detail::runtime_barrier(program.operands[operand_index++]));
                break;
            case detail::vm_opcode::end:
                if constexpr (std::is_same_v<storage_type, bool>) {
                    if (suspicious) {
                        return cast_out(!std::bit_cast<storage_type>(bytes));
                    }
                }
                return cast_out(std::bit_cast<storage_type>(bytes));
            }
        }

        if constexpr (std::is_same_v<storage_type, bool>) {
            if (suspicious) {
                return cast_out(!std::bit_cast<storage_type>(bytes));
            }
        }
        return cast_out(std::bit_cast<storage_type>(bytes));
    }

    operator T() const {
        return get();
    }

private:
    using storage_type = detail::vm_storage_type<T>;
    using byte_array = std::array<unsigned char, sizeof(storage_type)>;

    static constexpr unsigned char encrypt_byte(unsigned char value, std::size_t index) {
        auto encoded = static_cast<unsigned char>(value - detail::key_byte(Seed, index));
        const auto rotate = static_cast<unsigned char>((detail::stream(Seed, index + sizeof(storage_type)) % 7u) + 1u);
        if ((detail::stream(Seed, index) & 1u) != 0u) {
            encoded = detail::rotr_byte(encoded, rotate);
        } else {
            encoded = detail::rotl_byte(encoded, rotate);
        }
        return static_cast<unsigned char>(encoded ^ detail::key_byte(Seed, index + sizeof(storage_type)));
    }

    static constexpr T cast_out(storage_type value) {
        if constexpr (std::is_enum_v<T>) {
            return static_cast<T>(value);
        } else {
            return value;
        }
    }

    byte_array encrypted_;
};

template <std::uint64_t Seed, typename T>
constexpr auto make_virtualized_value(T value) {
    return virtualized_value<T, Seed>(value);
}

} // namespace oh

#endif
