#!/usr/bin/env python3

import argparse
import pathlib
import sys

def write_byte_array(out: pathlib.Path, symbol: str, data: bytes) -> None:
    with out.open("a", encoding="utf-8") as file:
        file.write(f"static const unsigned char {symbol}[] = {{\n")

        for index, byte in enumerate(data):
            if index % 16 == 0:
                file.write("    ")

            file.write(f"0x{byte:02x},")

            if index % 16 == 15:
                file.write("\n")

        file.write("\n};\n\n")

def main() -> int:
    parser = argparse.ArgumentParser(description="Embed MecchaCheatV payload into injector")
    parser.add_argument("--dll", required=True)
    parser.add_argument("--out-cpp", required=True)
    parser.add_argument("--out-hpp", required=True)
    args = parser.parse_args()

    files = [
        ("xv_meca.dll", "kDllData", pathlib.Path(args.dll)),
    ]

    for _, _, path in files:
        if not path.is_file():
            print(f"error: missing file {path}", file=sys.stderr)
            return 1

    out_cpp = pathlib.Path(args.out_cpp)
    out_hpp = pathlib.Path(args.out_hpp)
    out_cpp.parent.mkdir(parents=True, exist_ok=True)

    out_hpp.write_text(
        "#pragma once\n\n"
        "#include <cstddef>\n\n"
        "namespace EmbeddedPayload\n"
        "{\n"
        "\tstruct FileEntry\n"
        "\t{\n"
        "\t\tconst char* Name = nullptr;\n"
        "\t\tconst unsigned char* Data = nullptr;\n"
        "\t\tsize_t Size = 0;\n"
        "\t};\n\n"
        "\tauto GetFiles() -> const FileEntry*;\n"
        "\tauto GetFileCount() -> size_t;\n"
        "}\n",
        encoding="utf-8",
    )

    out_cpp.write_text(
        '#include "embedded_payload.hpp"\n\n'
        "namespace EmbeddedPayload\n"
        "{\n",
        encoding="utf-8",
    )

    entries = []

    for file_name, symbol, path in files:
        data = path.read_bytes()
        write_byte_array(out_cpp, symbol, data)
        entries.append((file_name, symbol, len(data)))

    with out_cpp.open("a", encoding="utf-8") as file:
        file.write("\tstatic const FileEntry kFiles[] =\n\t{\n")

        for file_name, symbol, size in entries:
            file.write(f'\t\t{{ "{file_name}" , {symbol} , {size} }},\n')

        file.write("\t};\n\n")
        file.write("\tauto GetFiles() -> const FileEntry*\n\t{\n")
        file.write("\t\treturn kFiles;\n")
        file.write("\t}\n\n")
        file.write("\tauto GetFileCount() -> size_t\n\t{\n")
        file.write(f"\t\treturn {len(entries)};\n")
        file.write("\t}\n")
        file.write("}\n")

    total_size = sum(size for _, _, size in entries)
    print(f"embedded {len(entries)} files ({total_size} bytes)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
