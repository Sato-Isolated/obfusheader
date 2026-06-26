#ifndef OBFUSHEADER_V2_HPP
#define OBFUSHEADER_V2_HPP

#include "obfusheader/detail/anti_analysis.hpp"
#include "obfusheader/detail/branch.hpp"
#include "obfusheader/detail/call.hpp"
#include "obfusheader/detail/config.hpp"
#include "obfusheader/detail/encrypted_blob.hpp"
#include "obfusheader/detail/encrypted_string.hpp"
#include "obfusheader/detail/encrypted_value.hpp"
#include "obfusheader/detail/import.hpp"
#include "obfusheader/detail/vm.hpp"

#define OH_DETAIL_SEED ::oh::detail::seed_from_context(__FILE__, __LINE__, __COUNTER__)

#define OH_STR(value) (::oh::make_string<OH_DETAIL_SEED, ::oh::fixed_string{value}>())
#define OH_WSTR(value) (::oh::make_wide_string<OH_DETAIL_SEED, ::oh::fixed_string{value}>())
#define OH_U8STR(value) (::oh::make_u8string<OH_DETAIL_SEED, ::oh::fixed_string{value}>())
#define OH_U16STR(value) (::oh::make_u16string<OH_DETAIL_SEED, ::oh::fixed_string{value}>())
#define OH_U32STR(value) (::oh::make_u32string<OH_DETAIL_SEED, ::oh::fixed_string{value}>())
#define OH_BLOB(...) (::oh::make_blob<OH_DETAIL_SEED>(::oh::detail::blob_bytes<__VA_ARGS__>{}))
#define OH_STR_EQ(value, candidate) (::oh::string_equals<OH_DETAIL_SEED, ::oh::fixed_string{value}>((candidate)))
#define OH_VAL(value) (::oh::make_value<OH_DETAIL_SEED>(value))
#define OH_VM_VAL(value) (::oh::make_virtualized_value<OH_DETAIL_SEED>(value))
#if defined(_MSC_VER) && defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
#define OH_CALL(function_ptr, ...) (::oh::call<OH_DETAIL_SEED>((function_ptr), __VA_ARGS__))
#else
#define OH_CALL(function_ptr, ...) (::oh::call<OH_DETAIL_SEED>((function_ptr) __VA_OPT__(,) __VA_ARGS__))
#endif
#define OH_BRANCH(condition, on_true, on_false) (::oh::branch<OH_DETAIL_SEED>((condition), (on_true), (on_false)))
#define OH_IMPORT(module_name, symbol_name, signature) (::oh::import<signature>(OH_STR(module_name), OH_STR(symbol_name)))
#define OH_PRESET(name) ::oh::preset::name

#endif
