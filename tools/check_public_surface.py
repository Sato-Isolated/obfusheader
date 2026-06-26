from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
UMBRELLA = ROOT / "include" / "obfusheader.hpp"

required_macros = {
    "OH_STR",
    "OH_WSTR",
    "OH_U8STR",
    "OH_U16STR",
    "OH_U32STR",
    "OH_VAL",
    "OH_VM_VAL",
    "OH_CALL",
    "OH_BRANCH",
    "OH_IMPORT",
    "OH_PRESET",
}

text = UMBRELLA.read_text(encoding="utf-8")
defined = set(re.findall(r"^#define\s+(OH_[A-Z0-9_]+)", text, re.MULTILINE))
missing = sorted(required_macros - defined)

if missing:
    print("missing public macros: " + ", ".join(missing), file=sys.stderr)
    raise SystemExit(1)

for header in (ROOT / "include" / "obfusheader" / "detail").glob("*.hpp"):
    content = header.read_text(encoding="utf-8")
    if "#ifndef " not in content or "#define " not in content:
        print(f"{header.relative_to(ROOT)} is missing an include guard", file=sys.stderr)
        raise SystemExit(1)

print("public surface ok")
