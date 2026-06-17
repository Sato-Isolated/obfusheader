from pathlib import Path
import sys


def parse_marker(value: str) -> bytes:
    if value.startswith("hex:"):
        return bytes.fromhex(value[4:])
    return value.encode("utf-8")


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: scan_binary.py <binary> <marker> [marker...]", file=sys.stderr)
        return 2

    data = Path(sys.argv[1]).read_bytes()
    failures = []
    for marker_text in sys.argv[2:]:
        marker = parse_marker(marker_text)
        offset = data.find(marker)
        if offset != -1:
            failures.append(f"{marker_text} found at offset {offset}")

    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1

    print("binary scan ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
