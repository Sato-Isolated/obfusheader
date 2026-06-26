from pathlib import Path
import sys


TEXT_ENCODINGS = (
    "utf-8",
    "utf-16le",
    "utf-16be",
    "utf-32le",
    "utf-32be",
)


def marker_variants(value: str):
    if value.startswith("hex:"):
        return [("hex", bytes.fromhex(value[4:]))]
    return [(encoding, value.encode(encoding)) for encoding in TEXT_ENCODINGS]


def preview(data: bytes, offset: int, size: int) -> str:
    return data[offset:offset + min(size, 32)].hex(" ")


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: scan_binary.py <binary> <marker> [marker...]", file=sys.stderr)
        return 2

    data = Path(sys.argv[1]).read_bytes()
    failures = []
    for marker_text in sys.argv[2:]:
        try:
            variants = marker_variants(marker_text)
        except ValueError as error:
            print(f"invalid marker {marker_text!r}: {error}", file=sys.stderr)
            return 2

        for encoding, marker in variants:
            offset = data.find(marker)
            if offset != -1:
                failures.append(
                    f"{marker_text} found as {encoding} at offset {offset}: {preview(data, offset, len(marker))}"
                )

    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1

    print("binary scan ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
