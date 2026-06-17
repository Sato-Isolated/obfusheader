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
- aggressive-by-default value, string, call, branch, and import hardening
- anti-analysis poisoning across public primitives, with an explicit opt-out
- release-build binary scanning for protected strings and scalar byte markers

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

    auto sum = OH_CALL(&add, value.get(), 10);
    auto selected = OH_BRANCH(sum > 0,
        [] { return 1; },
        [] { return 0; });

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
```

`OH_STR` decrypts into an internal buffer when `c_str()` or `view()` is called.
Call `clear()` when the plaintext is no longer needed.

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
```

On Windows this uses `GetModuleHandleA`, `LoadLibraryA`, and `GetProcAddress`.
On Linux it uses `dlopen` and `dlsym`. Unsupported platforms return an empty
symbol wrapper.

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
- `strong` preset behavior
- forced anti-analysis poisoning behavior
- release binary scanning for protected string cleartext
- release binary scanning for protected little-endian scalar bytes
- release binary scanning for protected import names
- seeded build variation through `OH_USER_SEED`

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
- prefer `OH_VM_VAL` or the `strong` preset only where the additional runtime
  cost is justified

## Requirements

- C++20 or newer
- MSVC 2022 tested on Windows
- Linux support for core headers; dynamic imports require `dl`
- CMake 3.20+ for the provided test project
