#!/usr/bin/env python3
"""
GTA5 Enhanced signature + offset scanner.
Scans GTA5.exe for WORLD, NATIVE_REG, SWAPCHAIN patterns
and dumps results for updating game.h.

Usage:
    py gta5_scanner.py
    py gta5_scanner.py "D:\path\to\GTA5.exe"
"""

import struct
import sys
import os
import re

DEFAULT_PATH = r"C:\Program Files\Rockstar Games\Grand Theft Auto V Enhanced\GTA5.exe"


def read_exe(path: str) -> bytes:
    size = os.path.getsize(path)
    print(f"[*] Reading {path} ({size / 1024 / 1024:.1f} MB)")
    with open(path, "rb") as f:
        return f.read()


def pattern_to_bytes(pattern: str) -> tuple[list[int | None], str]:
    """Convert an IDA-style pattern string to a list of bytes (None = wildcard)."""
    tokens = pattern.strip().split()
    result = []
    for t in tokens:
        if t == "?" or t == "??":
            result.append(None)
        else:
            result.append(int(t, 16))
    return result


def scan_pattern(data: bytes, pattern: str, label: str = "") -> list[int]:
    """Scan binary data for a byte pattern. Returns list of offsets."""
    pat = pattern_to_bytes(pattern)
    results = []
    pat_len = len(pat)
    data_len = len(data)

    # Build first-byte index for speed
    first = pat[0]
    i = 0
    while i < data_len - pat_len:
        if first is not None and data[i] != first:
            i += 1
            continue

        match = True
        for j in range(pat_len):
            if pat[j] is not None and data[i + j] != pat[j]:
                match = False
                break
        if match:
            results.append(i)
        i += 1

    return results


def resolve_rip_relative(data: bytes, offset: int, instr_len: int = 7) -> int:
    """Resolve a RIP-relative address (e.g., from 48 8B 05 xx xx xx xx)."""
    disp = struct.unpack_from("<i", data, offset + instr_len - 4)[0]
    return offset + instr_len + disp


def format_pattern(data: bytes, offset: int, length: int = 20) -> str:
    """Format bytes at offset as a pattern string."""
    end = min(offset + length, len(data))
    return " ".join(f"{b:02X}" for b in data[offset:end])


def scan_world(data: bytes) -> None:
    print("\n" + "=" * 60)
    print("WORLD (CPedFactory)")
    print("=" * 60)

    # Known Legacy pattern
    patterns = [
        ("Legacy", "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9 74 52 8B 81"),
        ("Alt 1", "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9 74"),
        ("Alt 2", "48 8B 05 ? ? ? ? 48 8B 48 08 48 8B 01"),
        ("Alt 3", "48 8B 05 ? ? ? ? 33 D2 48 8B 48 08"),
        ("Broad",  "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9"),
    ]

    for name, pat in patterns:
        results = scan_pattern(data, pat, name)
        if results:
            print(f"\n  [{name}] Found {len(results)} match(es):")
            for off in results[:5]:
                rip = resolve_rip_relative(data, off)
                print(f"    Offset: 0x{off:X}")
                print(f"    RIP target: 0x{rip:X}")
                print(f"    Bytes: {format_pattern(data, off, 24)}")
                print(f"    Pattern for game.h: \"{format_pattern(data, off, 18)}\"")
            if len(results) > 5:
                print(f"    ... and {len(results) - 5} more")
            return

    print("  [!] No matches found for any WORLD pattern variant")


def scan_native_reg(data: bytes) -> None:
    print("\n" + "=" * 60)
    print("NATIVE_REG (Native Registration)")
    print("=" * 60)

    patterns = [
        ("Legacy", "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ? 48 85 C0 75 0A"),
        ("Alt 1",  "48 8D 0D ? ? ? ? 48 8B 14 FA E8"),
        ("Alt 2",  "48 8D 0D ? ? ? ? 48 8B 14 ? E8 ? ? ? ? 48 85 C0"),
        ("Alt 3",  "4C 8D 05 ? ? ? ? 48 8B 14 FA"),
        ("Broad",  "48 8D 0D ? ? ? ? 48 8B 14"),
    ]

    for name, pat in patterns:
        results = scan_pattern(data, pat, name)
        if results:
            print(f"\n  [{name}] Found {len(results)} match(es):")
            for off in results[:5]:
                rip = resolve_rip_relative(data, off)
                print(f"    Offset: 0x{off:X}")
                print(f"    RIP target: 0x{rip:X}")
                print(f"    Bytes: {format_pattern(data, off, 24)}")
                print(f"    Pattern for game.h: \"{format_pattern(data, off, 20)}\"")
            if len(results) > 5:
                print(f"    ... and {len(results) - 5} more")
            return

    print("  [!] No matches found for any NATIVE_REG pattern variant")


def scan_swapchain(data: bytes) -> None:
    print("\n" + "=" * 60)
    print("SWAPCHAIN (IDXGISwapChain)")
    print("=" * 60)

    patterns = [
        ("DX11 Legacy", "48 8B 0D ? ? ? ? 48 8B 01 44 8D 43 01 33 D2 FF 50 40"),
        ("DX12 Alt 1",  "48 8B 0D ? ? ? ? 48 8B 01 33 D2 FF 50"),
        ("DX12 Alt 2",  "48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 50"),
        ("DX12 Alt 3",  "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 48"),
        ("DX12 Present", "48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? 00 00"),
        ("Broad",       "48 8B 0D ? ? ? ? 48 8B 01 44 8D 43"),
    ]

    for name, pat in patterns:
        results = scan_pattern(data, pat, name)
        if results:
            print(f"\n  [{name}] Found {len(results)} match(es):")
            for off in results[:5]:
                rip = resolve_rip_relative(data, off)
                print(f"    Offset: 0x{off:X}")
                print(f"    RIP target: 0x{rip:X}")
                print(f"    Bytes: {format_pattern(data, off, 24)}")
                print(f"    Pattern for game.h: \"{format_pattern(data, off, 18)}\"")
            if len(results) > 5:
                print(f"    ... and {len(results) - 5} more")
            return

    print("  [!] No matches found for any SWAPCHAIN pattern variant")


def scan_ped_offsets(data: bytes) -> None:
    print("\n" + "=" * 60)
    print("PED STRUCT OFFSETS")
    print("=" * 60)

    # CPed::m_fHealth — usually accessed via movss xmm0, [rcx+offset]
    # Look for health-related patterns
    offset_patterns = {
        "Ped_Health": [
            ("F3 0F 10 81 80 02 00 00", 0x280, "movss xmm0, [rcx+280h]"),
            ("F3 0F 10 81 ? 02 00 00", None, "movss xmm0, [rcx+2xxh]"),
        ],
        "Ped_Armor": [
            ("F3 0F 10 81 84 02 00 00", 0x284, "movss xmm0, [rcx+284h]"),
            ("F3 0F 10 81 ? 02 00 00", None, "movss xmm0, [rcx+2xxh] (armor after health)"),
        ],
        "Ped_Vehicle (GetVehicle)": [
            ("48 8B 89 30 10 00 00", 0x1030, "mov rcx, [rcx+1030h]"),
            ("48 8B 89 ? 10 00 00", None, "mov rcx, [rcx+10xxh]"),
            ("48 8B 89 ? ? 00 00 48 85 C9 74", None, "mov rcx, [rcx+????h]; test rcx,rcx"),
        ],
    }

    for name, variants in offset_patterns.items():
        print(f"\n  {name}:")
        for pat, expected, desc in variants:
            results = scan_pattern(data, pat)
            if results:
                # Try to extract the offset from the instruction
                off = results[0]
                if pat.count("?") > 0:
                    # Extract the wildcard byte(s) to determine actual offset
                    tokens = pat.split()
                    offset_bytes = []
                    for i, t in enumerate(tokens):
                        if t == "?":
                            offset_bytes.append((i, data[off + i]))

                    if len(offset_bytes) >= 1:
                        # Reconstruct the offset
                        idx = tokens.index("?")
                        actual_offset = struct.unpack_from("<I", data, off + idx)[0]
                        print(f"    [{desc}] {len(results)} hits, actual offset: 0x{actual_offset:X}")
                        print(f"      First at: 0x{off:X} -> {format_pattern(data, off, 12)}")
                elif expected:
                    print(f"    [{desc}] {len(results)} hits — offset confirmed: 0x{expected:X}")
                    print(f"      First at: 0x{off:X} -> {format_pattern(data, off, 12)}")
                break
        else:
            print(f"    [!] No pattern matched")


def scan_player_info_offsets(data: bytes) -> None:
    print("\n" + "=" * 60)
    print("PLAYER INFO OFFSETS")
    print("=" * 60)

    patterns = {
        "WantedLevel": [
            ("89 91 88 08 00 00", 0x888, "mov [rcx+888h], edx"),
            ("89 ? 88 08 00 00", 0x888, "mov [rcx+888h], reg"),
            ("8B 81 88 08 00 00", 0x888, "mov eax, [rcx+888h]"),
            ("83 B9 88 08 00 00 00", 0x888, "cmp dword [rcx+888h], 0"),
            ("89 91 ? ? 00 00", None, "mov [rcx+????h], edx (wanted)"),
        ],
        "WantedMax": [
            ("89 91 8C 08 00 00", 0x88C, "mov [rcx+88Ch], edx"),
            ("8B 81 8C 08 00 00", 0x88C, "mov eax, [rcx+88Ch]"),
            ("C7 81 8C 08 00 00 05 00 00 00", 0x88C, "mov dword [rcx+88Ch], 5"),
        ],
    }

    for name, variants in patterns.items():
        print(f"\n  PI_{name}:")
        for pat, expected, desc in variants:
            results = scan_pattern(data, pat)
            if results:
                off = results[0]
                if expected:
                    print(f"    [{desc}] {len(results)} hits — offset confirmed: 0x{expected:X}")
                else:
                    # Extract offset from wildcard
                    tokens = pat.split()
                    idx = tokens.index("?")
                    actual = struct.unpack_from("<I", data, off + idx)[0]
                    print(f"    [{desc}] {len(results)} hits — actual offset: 0x{actual:X}")
                print(f"      First at: 0x{off:X} -> {format_pattern(data, off, 12)}")
                break
        else:
            print(f"    [!] No pattern matched")


def main():
    if len(sys.argv) > 1:
        path = sys.argv[1]
    else:
        path = DEFAULT_PATH

    if not os.path.exists(path):
        print(f"[!] File not found: {path}")
        print(f"    Usage: py {sys.argv[0]} \"C:\\path\\to\\GTA5.exe\"")
        sys.exit(1)

    data = read_exe(path)

    print(f"[*] Scanning {len(data)} bytes...")

    scan_world(data)
    scan_native_reg(data)
    scan_swapchain(data)
    scan_ped_offsets(data)
    scan_player_info_offsets(data)

    print("\n" + "=" * 60)
    print("SCAN COMPLETE")
    print("=" * 60)
    print("\nCopy the patterns and offsets above into game.h")
    print("Replace any FILL comments with the confirmed values.")

    input("\nPress Enter to exit...")


if __name__ == "__main__":
    main()
