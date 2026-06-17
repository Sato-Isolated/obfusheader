#ifndef OBFUSHEADER_DETAIL_IMPORT_HPP
#define OBFUSHEADER_DETAIL_IMPORT_HPP

#include "anti_analysis.hpp"
#include "encrypted_string.hpp"

#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#endif

namespace oh {

template <typename Signature>
class imported_symbol {
public:
    constexpr imported_symbol() = default;
    explicit imported_symbol(Signature pointer) : pointer_(pointer) {}

    bool available() const {
        return pointer_ != nullptr;
    }

    explicit operator bool() const {
        return available();
    }

    template <typename... Args>
    decltype(auto) operator()(Args&&... args) const {
        return pointer_(std::forward<Args>(args)...);
    }

    Signature get() const {
        return pointer_;
    }

private:
    Signature pointer_ = nullptr;
};

template <typename Signature, typename ModuleName, typename SymbolName>
imported_symbol<Signature> import(ModuleName module_name, SymbolName symbol_name) {
    if (anti_analysis::detail::analysis_detected()) {
        module_name.clear();
        symbol_name.clear();
        return imported_symbol<Signature>();
    }
#if defined(_WIN32)
    auto module = GetModuleHandleA(module_name.c_str());
    if (module == nullptr) {
        module = LoadLibraryA(module_name.c_str());
    }
    module_name.clear();
    if (module == nullptr) {
        symbol_name.clear();
        return imported_symbol<Signature>();
    }
    auto pointer = reinterpret_cast<Signature>(GetProcAddress(module, symbol_name.c_str()));
    symbol_name.clear();
    return imported_symbol<Signature>(pointer);
#elif defined(__linux__)
    void* module = dlopen(module_name.c_str(), RTLD_LAZY);
    module_name.clear();
    if (module == nullptr) {
        symbol_name.clear();
        return imported_symbol<Signature>();
    }
    auto pointer = reinterpret_cast<Signature>(dlsym(module, symbol_name.c_str()));
    symbol_name.clear();
    return imported_symbol<Signature>(pointer);
#else
    (void)module_name;
    (void)symbol_name;
    return imported_symbol<Signature>();
#endif
}

} // namespace oh

#endif
