# Obfusheader

Obfusheader is a C++20/23 header-only obfuscation library for localized,
typed protection of constants, calls, branches, and optional dynamic imports.
It is designed to be small enough to drop into an application, while keeping the
public API explicit and testable.

The project focuses on:

- header-only integration through `include/obfusheader.hpp`
- typed primitives for localized protection
- per-call-site entropy from `__FILE__`, `__LINE__`, `__COUNTER__`, and
  optional `OH_USER_SEED`
- explicit narrow, wide, UTF-8, UTF-16, and UTF-32 string literal wrappers
- encrypted byte blobs and bounded protected string comparisons
- aggressive-by-default value, string, blob, call, branch, and import hardening
- anti-analysis poisoning across public primitives, with an explicit opt-out
- release-build binary scanning for protected strings across common encodings
  and scalar byte markers

## Quick Start

Copy or include the `include/` directory, then include the umbrella header:

```cpp
#include "obfusheader.hpp"

#include <cstdio>

int add(int lhs, int rhs) {
    return lhs + rhs;
}

int main() {
    auto message = OH_STR("hidden text");
    std::puts(message.c_str());
    message.clear();

    auto value = OH_VAL(0x12345678u);
    auto vm_value = OH_VM_VAL(0x89abcdefu);
    auto blob = OH_BLOB(0xde, 0xad, 0xbe, 0xef);

    auto sum = OH_CALL(&add, value.get(), 10);
    auto selected = OH_BRANCH(sum > 0,
        [] { return 1; },
        [] { return 0; });

    blob.clear();
    return selected == 1 && vm_value.get() == 0x89abcdefu ? 0 : 1;
}
```

Build with a C++20 compiler:

```powershell
cl /std:c++20 /O2 /Iinclude example.cpp
```

## Public API

| Macro | Purpose |
| --- | --- |
| `OH_STR("text")` | Stores a string literal encrypted and decrypts it on demand. |
| `OH_WSTR(L"text")` | Stores a `wchar_t` string literal encrypted and decrypts it on demand. |
| `OH_U8STR(u8"text")` | Stores a `char8_t` UTF-8 string literal encrypted and decrypts it on demand. |
| `OH_U16STR(u"text")` | Stores a `char16_t` UTF-16 string literal encrypted and decrypts it on demand. |
| `OH_U32STR(U"text")` | Stores a `char32_t` UTF-32 string literal encrypted and decrypts it on demand. |
| `OH_BLOB(bytes...)` | Stores one or more byte constants encrypted and decrypts them on demand. |
| `OH_STR_EQ("text", candidate)` | Compares a narrow protected string literal with a `std::string_view`-compatible candidate, then clears the protected buffer. |
| `OH_VAL(value)` | Stores an arithmetic or enum constant in an encrypted wrapper. |
| `OH_VM_VAL(value)` | Stores an arithmetic or enum constant behind a small seed-specialized bytecode interpreter. |
| `OH_CALL(fn, ...)` | Calls a function pointer through a small seeded indirection table. |
| `OH_BRANCH(cond, true_fn, false_fn)` | Selects between two callables through a seeded branch guard. |
| `OH_IMPORT(module, symbol, signature)` | Resolves an imported symbol by encrypted module and symbol names. |
| `OH_PRESET(name)` | Convenience macro for selecting `light`, `balanced`, or `strong`. |

### Strings

```cpp
auto secret = OH_STR("api-token-marker");
use(secret.c_str());
secret.clear();

auto wide = OH_WSTR(L"wide-marker");
auto utf8 = OH_U8STR(u8"utf8-marker");
auto utf16 = OH_U16STR(u"utf16-marker");
auto utf32 = OH_U32STR(U"utf32-marker");
```

String wrappers decrypt into an internal buffer when `c_str()` or `view()` is
called. Call `clear()` when the plaintext is no longer needed. The macro name
must match the literal character type; for example, use `OH_U16STR(u"text")`
for a UTF-16 literal instead of passing it to `OH_STR`.

### Blobs

```cpp
auto signature = OH_BLOB(0xde, 0xad, 0xbe, 0xef);
auto bytes = signature.view();

if (bytes.size() == 4 && bytes[0] == 0xde) {
    use_bytes(bytes.data(), bytes.size());
}

signature.clear();
```

`OH_BLOB` accepts at least one integer byte constant in the range `[0, 255]`.
It is useful for small byte markers, protocol constants, signatures, or compact
keys that should not sit in the binary as a contiguous cleartext byte sequence.
Like strings, blobs decrypt into an internal buffer and should be cleared after
use.

### Protected String Compare

```cpp
std::string_view user_input = read_token();

if (OH_STR_EQ("expected-token", user_input)) {
    accept();
}
```

`OH_STR_EQ` currently targets narrow `char` string literals. The candidate must
be convertible to `std::string_view`, such as `const char*`, `std::string_view`,
or `std::string`. The comparison does not exit early on content mismatch and it
clears the protected string wrapper before returning. This reduces plaintext
exposure time; it is not a cryptographic authentication primitive or a formal
constant-time guarantee.

### Values

```cpp
auto plain_path = OH_VAL(0x12345678u);
auto vm_path = OH_VM_VAL(0x12345678u);

auto a = plain_path.get();
auto b = vm_path.get();
```

`OH_VAL` is the lighter default path. `OH_VM_VAL` routes recovery through a
small interpreter with seeded byte operations. Both support arithmetic types and
enums.

### Calls and Branches

```cpp
auto result = OH_CALL(&add, 20, 22);

auto branch = OH_BRANCH(result == 42,
    [] { return 7; },
    [] { return 9; });
```

`OH_CALL` and `OH_BRANCH` are intentionally narrow primitives. They are meant to
hide simple local intent without changing the C++ language model.

### Dynamic Imports

```cpp
using puts_t = int (*)(const char*);
auto imported_puts = OH_IMPORT("msvcrt.dll", "puts", puts_t);

if (imported_puts) {
    imported_puts("resolved dynamically");
}

auto safe_result = imported_puts.invoke_or(-1, "resolved dynamically");
```

On Windows this uses `GetModuleHandleA`, `LoadLibraryA`, and `GetProcAddress`.
On Linux it uses `dlopen` and `dlsym`; the library handle is intentionally
retained so the resolved symbol remains valid for the process lifetime.
Unsupported platforms return an empty symbol wrapper.

`OH_IMPORT` expects a function pointer signature. `operator()` is kept for
direct-call compatibility and assumes `available() == true`. For safer missing
symbol handling, use `invoke_or(fallback, args...)` with non-void functions or
`invoke_if(args...)` with void functions.

## Choosing a Primitive

| Need | Prefer | Notes |
| --- | --- | --- |
| Hide a short text literal | `OH_STR` or typed string variants | Call `clear()` after `c_str()` or `view()`. |
| Hide raw bytes | `OH_BLOB` | Keep blobs small and clear them after use. |
| Compare user input with a protected narrow literal | `OH_STR_EQ` | Keeps the protected literal lifetime short; not a password hashing substitute. |
| Hide an arithmetic or enum constant | `OH_VAL` | Lower runtime cost than the VM path. |
| Add stronger scalar recovery indirection | `OH_VM_VAL` or `strong` preset | Higher runtime cost, useful for selected constants. |
| Hide a local function-pointer call | `OH_CALL` | Function pointer only; does not change C++ call semantics. |
| Hide a simple local branch decision | `OH_BRANCH` | Evaluates only the selected callable. |
| Resolve a platform symbol lazily | `OH_IMPORT` | Resolve once, check `available()`, then reuse the wrapper. |

## Operational Notes

Plaintext lifetime:

- `OH_STR`, typed string wrappers, and `OH_BLOB` decrypt into mutable internal
  buffers.
- The plaintext remains available after `c_str()`, `view()`, or `data()` until
  `clear()` is called or the wrapper is destroyed.
- `OH_STR_EQ` creates a short-lived protected string wrapper and clears it
  before returning.

Thread-safety:

- Treat each wrapper object as mutable state.
- Do not share the same decrypted wrapper across threads without external
  synchronization.
- Prefer creating protected wrappers close to the use site instead of storing
  them globally.

Runtime cost:

- `OH_STR` and `OH_BLOB` are O(n) in the number of code units or bytes.
- `OH_VAL` is the light scalar path.
- `OH_VM_VAL` and the `strong` preset add interpreter overhead and should be
  reserved for constants where the extra cost is justified.
- `OH_IMPORT` performs platform symbol resolution; resolve once and reuse the
  returned wrapper where possible.

## Presets and Seeds

The default preset is `balanced`:

```cpp
static_assert(oh::current_preset == oh::preset::balanced);
```

Select another preset at compile time before including the header:

```cpp
#define OH_CONFIG_PRESET ::oh::preset::strong
#include "obfusheader.hpp"
```

Preset behavior:

| Preset | Behavior |
| --- | --- |
| `light` | Keeps the hardened typed wrappers with lower runtime cost. |
| `balanced` | Default aggressive portable behavior with anti-analysis poisoning. |
| `strong` | Routes `OH_VAL(value)` through the virtualized value path. |

Use `OH_USER_SEED` to vary generated binaries between builds:

```powershell
cl /std:c++20 /O2 /GL /DOH_USER_SEED=123 /Iinclude tests\binary_fixture.cpp
```

## Anti-Analysis

Anti-analysis probes are part of the default primitive paths. On Windows the
library checks local and remote debugger state. On Linux it checks
`/proc/self/status` for a tracer. If a probe detects analysis, primitives return
poisoned outputs instead of intentionally crashing:

- `OH_STR` decrypts to a wrong but still terminated buffer.
- `OH_BLOB` decrypts to wrong bytes.
- `OH_STR_EQ` returns false for protected literals that would otherwise match.
- `OH_VAL` and `OH_VM_VAL` return stable incorrect values.
- `OH_CALL` does not invoke the target and returns a poisoned value for
  non-void functions.
- `OH_BRANCH` selects the opposite callable without evaluating both branches.
- `OH_IMPORT` returns an unavailable symbol.

Use `OH_DISABLE_ANTI_ANALYSIS` when debugging or when a controlled CI
environment triggers false positives:

```powershell
cl /std:c++20 /DOH_DISABLE_ANTI_ANALYSIS /Iinclude app.cpp
```

Use `OH_FORCE_ANALYSIS_DETECTED` only in tests to force the poison paths.
The direct probe API remains available:

```cpp
auto probe = oh::anti_analysis::probe();
if (probe.detected()) {
    return 1;
}
```

## Verification

Preferred CMake flow on Windows:

```powershell
"C:\Program Files\CMake\bin\cmake.exe" -S . -B build-cmake-msvc -G "Visual Studio 17 2022" -A x64
"C:\Program Files\CMake\bin\cmake.exe" --build build-cmake-msvc --config Release
"C:\Program Files\CMake\bin\ctest.exe" --test-dir build-cmake-msvc -C Release --output-on-failure
```

Direct MSVC helper:

```powershell
powershell -ExecutionPolicy Bypass -File tools\verify_msvc.ps1
```

The verification suite covers:

- repository hygiene for the public header surface
- C++20 runtime behavior
- C++23 runtime behavior
- strict MSVC client syntax under `/permissive- /Zc:preprocessor`, including
  `if` statements with initializers and `OH_CALL(&fn)` with no call arguments
- explicit MSVC runtime coverage for both dynamic CRT (`/MD`) and static CRT
  (`/MT`) builds
- Windows no-default-CRT linking with a custom `entry` point, `/NODEFAULTLIB`,
  `/GS-`, `/EHsc-`, `/GR-`, and `kernel32.lib`
- `strong` preset behavior
- forced anti-analysis poisoning behavior
- release binary scanning for protected string cleartext across UTF-8,
  UTF-16LE, UTF-16BE, UTF-32LE, and UTF-32BE marker encodings
- release binary scanning for protected `OH_STR_EQ` literals
- release binary scanning for protected blob byte markers
- release binary scanning for protected little-endian scalar bytes
- release binary scanning for protected import names
- seeded build variation through `OH_USER_SEED`

The no-default-CRT fixture verifies that the core primitives can link and run in
a Windows binary with a custom entry point when the consuming code also avoids a
standard CRT startup path. It is not a guarantee that a normal `main` executable
or arbitrary user code can omit the CRT.

Run binary scans against optimized release builds. Debug builds can retain
compiler metadata, unoptimized constants, and diagnostics that are not
representative of the shipped binary.

## Project Layout

```text
include/
  obfusheader.hpp                 Public umbrella header
  obfusheader/detail/             Implementation modules
tests/
  v2_tests.cpp                    Runtime coverage for public primitives
  syntax_stability_tests.cpp      Strict client syntax and CRT runtime matrix coverage
  no_crt_fixture.cpp              Windows custom-entry no-default-CRT fixture
  strong_preset_tests.cpp         Strong preset coverage
  anti_analysis_tests.cpp         Forced anti-analysis poison coverage
  binary_fixture.cpp              Optimized binary scan fixture
tools/
  check_public_surface.py         Public surface hygiene check
  scan_binary.py                  String and hex byte marker scanner
  verify_msvc.ps1                 Direct MSVC verification flow
```

## Security Notes

Obfuscation raises reverse-engineering cost; it does not make secrets
unrecoverable. Treat these primitives as one layer in a broader protection
strategy:

- do not embed long-lived production secrets in client binaries
- validate behavior in optimized release builds, not only debug builds
- vary `OH_USER_SEED` between release lines when practical
- keep protected scopes small and explicit
- call `clear()` as soon as decrypted strings or blobs are no longer needed
- do not treat `OH_STR_EQ` as password storage, password hashing, or a remote
  authentication boundary
- prefer `OH_VM_VAL` or the `strong` preset only where the additional runtime
  cost is justified

## Requirements

- C++20 or newer
- MSVC 2022 tested on Windows
- Linux support for core headers; dynamic imports require `dl`
- CMake 3.20+ for the provided test project
