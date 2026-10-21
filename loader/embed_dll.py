#!/usr/bin/env python3
"""
Reads trident.dll, XOR-encrypts it, writes payload.hpp
with the encrypted bytes as a constexpr array.

Usage: python embed_dll.py <path-to-trident.dll> [output.hpp]
"""

import sys, os, struct, secrets

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <dll_path> [output.hpp]")
        sys.exit(1)

    dll_path = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) > 2 else "payload.hpp"

    with open(dll_path, "rb") as f:
        raw = f.read()

    key = secrets.token_bytes(32)
    enc = bytearray(len(raw))
    for i in range(len(raw)):
        enc[i] = raw[i] ^ key[i % 32]

    with open(out_path, "w") as f:
        f.write("#pragma once\n")
        f.write("#include <cstdint>\n\n")
        f.write(f"inline constexpr uint32_t g_PayloadSize = {len(raw)}u;\n\n")

        f.write("inline constexpr uint8_t g_PayloadKey[32] = {\n    ")
        for i, b in enumerate(key):
            f.write(f"0x{b:02X}")
            if i < 31: f.write(", ")
            if (i + 1) % 16 == 0 and i < 31: f.write("\n    ")
        f.write("\n};\n\n")

        f.write("inline const uint8_t g_PayloadData[] = {\n    ")
        for i, b in enumerate(enc):
            f.write(f"0x{b:02X}")
            if i < len(enc) - 1: f.write(",")
            if (i + 1) % 24 == 0: f.write("\n    ")
        f.write("\n};\n")

    print(f"Embedded {len(raw)} bytes ({len(raw)/1024:.1f} KB) -> {out_path}")
    print(f"XOR key: {key.hex()}")

if __name__ == "__main__":
    main()
