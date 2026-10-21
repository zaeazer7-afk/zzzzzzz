#!/usr/bin/env python3
"""
GTA5 Enhanced — full signature + offset scanner.
Scans GTA5.exe for every pattern and struct offset needed
for a complete mod menu. Outputs verified values for game.h.

Usage:
    py gta5_scanner.py
    py gta5_scanner.py "D:\path\to\GTA5.exe"
"""

import struct
import sys
import os
import time

DEFAULT_PATH = r"C:\Program Files\Rockstar Games\Grand Theft Auto V Enhanced\GTA5.exe"


# ============================================================
# SIGNATURE DEFINITIONS
# Each entry: (name, [(variant_name, pattern), ...])
# ============================================================

SIGNATURES = [
    # --- Core ---
    ("WORLD", [
        ("Primary",  "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9 74 52 8B 81"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9 74"),
        ("Alt 2",    "48 8B 05 ? ? ? ? 48 8B 48 08 48 8B 01"),
        ("Broad",    "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9"),
    ]),
    ("NATIVE_REG", [
        ("Primary",  "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ? 48 85 C0 75 0A"),
        ("Alt 1",    "48 8D 0D ? ? ? ? 48 8B 14 FA E8"),
        ("Alt 2",    "4C 8D 05 ? ? ? ? 48 8B 14 FA"),
        ("Broad",    "48 8D 0D ? ? ? ? 48 8B 14"),
    ]),
    ("SWAPCHAIN", [
        ("DX11",     "48 8B 0D ? ? ? ? 48 8B 01 44 8D 43 01 33 D2 FF 50 40"),
        ("DX12 v1",  "48 8B 0D ? ? ? ? 48 8B 01 33 D2 FF 50"),
        ("DX12 v2",  "48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 50"),
        ("DX12 v3",  "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 48"),
        ("Present",  "48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? 00 00"),
    ]),

    # --- Replay / Game State ---
    ("REPLAY_INTERFACE", [
        ("Primary",  "48 8B 0D ? ? ? ? 48 8B D7 48 8B 5C 24 ? 48 8B 6C 24"),
        ("Alt 1",    "48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 48 8B 01 FF 50"),
        ("Broad",    "48 8B 0D ? ? ? ? 48 8B D7 48 8B"),
    ]),
    ("GLOBAL_TABLE", [
        ("Primary",  "4C 8D 05 ? ? ? ? 4D 8B 08 4D 85 C9 74 11"),
        ("Alt 1",    "48 8D 15 ? ? ? ? 4C 8B 05 ? ? ? ? 4C 8B 0C"),
        ("Broad",    "4C 8D 05 ? ? ? ? 4D 8B 08 4D 85 C9"),
    ]),
    ("SCRIPT_HANDLER", [
        ("Primary",  "48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 90 ? ? 00 00 48 8B C8"),
        ("Alt 1",    "48 8B 0D ? ? ? ? 48 85 C9 74 ? E8 ? ? ? ? 48 8B"),
        ("Broad",    "48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 90"),
    ]),
    ("SCRIPT_THREAD_COUNT", [
        ("Primary",  "FF 0D ? ? ? ? 48 8B 0D ? ? ? ? 48 85 C9"),
    ]),

    # --- Camera ---
    ("VIEWPORT", [
        ("Primary",  "48 8B 15 ? ? ? ? 48 8D 2D ? ? ? ? 48 8B CD"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 48 8B 98 ? ? 00 00 EB"),
        ("Broad",    "48 8B 15 ? ? ? ? 48 8D 2D"),
    ]),
    ("CAMERA", [
        ("Primary",  "48 8B 05 ? ? ? ? 48 8B 98 ? 00 00 00 48 85 DB"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 48 8B 90 ? 00 00 00 F3 0F"),
        ("Broad",    "48 8B 05 ? ? ? ? 48 8B 98 ? 00 00 00"),
    ]),

    # --- Pools ---
    ("PED_POOL", [
        ("Primary",  "48 8B 05 ? ? ? ? 41 0F BF C8 0F BF 40 10"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 0F BF 48 10 66 3B"),
    ]),
    ("VEHICLE_POOL", [
        ("Primary",  "48 8B 05 ? ? ? ? F3 0F 59 F6 48 8B 08"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 48 8B 48 ? EB ? 48 8B 46"),
    ]),
    ("OBJECT_POOL", [
        ("Primary",  "48 8B 05 ? ? ? ? 8B 78 10 48 8B 0D"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 0F BF 48 10 44 3B"),
    ]),
    ("PICKUP_POOL", [
        ("Primary",  "48 8B 05 ? ? ? ? 48 8B 0C C8 EB ? 48 8B 0C"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 48 63 D1 48 8B 0C D0"),
    ]),

    # --- Blips ---
    ("BLIP_LIST", [
        ("Primary",  "4C 8D 05 ? ? ? ? 0F B7 C1 48 8D 14 80"),
        ("Alt 1",    "48 8D 0D ? ? ? ? 0F B7 C2 48 8D 14 80"),
        ("Broad",    "4C 8D 05 ? ? ? ? 0F B7 C1"),
    ]),

    # --- Network ---
    ("NETWORK_PLAYER_MGR", [
        ("Primary",  "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B D8 48 85 C0 74 ? 48 8B 00"),
        ("Alt 1",    "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 84 C0"),
        ("Broad",    "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B D8 48 85 C0"),
    ]),
    ("NETWORK_INFO", [
        ("Primary",  "48 8B 0D ? ? ? ? 48 8B D3 48 8B 01 FF 90 ? ? 00 00 33"),
        ("Alt 1",    "48 8B 0D ? ? ? ? 44 8D 43 ? 48 8B D6"),
    ]),
    ("NETWORK_OBJ_MGR", [
        ("Primary",  "48 8B 0D ? ? ? ? 45 33 C0 48 8B D7 48 8B 01"),
        ("Alt 1",    "48 8B 0D ? ? ? ? 48 8B D3 48 8B 01 FF 50"),
    ]),
    ("FRIEND_LIST", [
        ("Primary",  "48 8B 0D ? ? ? ? E8 ? ? ? ? 84 C0 74 ? 48 8B 0D ? ? ? ? 48 8B 01"),
    ]),

    # --- Weather / Clock ---
    ("WEATHER", [
        ("Primary",  "48 8D 0D ? ? ? ? 0F 29 44 24 ? 66 44 0F 7F 44 24"),
        ("Alt 1",    "48 8D 0D ? ? ? ? E8 ? ? ? ? 8B 05 ? ? ? ? 49"),
    ]),
    ("CLOCK", [
        ("Primary",  "48 8B 0D ? ? ? ? 8B 44 24 ? 89 01 8B 44 24 ? 89 41 04"),
        ("Alt 1",    "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? B2 01"),
    ]),

    # --- Input ---
    ("PAD", [
        ("Primary",  "48 8B 05 ? ? ? ? 48 8B 70 ? EB ? 48 8B 36 48 85 F6 75"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 48 8D 1C F0 48 8B 0B"),
    ]),

    # --- Weapon ---
    ("WEAPON_INFO_MANAGER", [
        ("Primary",  "48 8B 05 ? ? ? ? 48 8B 48 ? 48 63 C2 48 8D 04 40"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 44 8B CE 48 8B 48"),
    ]),

    # --- Waypoint ---
    ("WAYPOINT", [
        ("Primary",  "48 8D 05 ? ? ? ? 48 69 C9 ? ? 00 00 48 03 C8 F3 0F 10"),
        ("Alt 1",    "48 8D 05 ? ? ? ? 48 8D 0C ? F3 0F 10"),
    ]),

    # --- Handling ---
    ("HANDLING_DATA", [
        ("Primary",  "48 8B 05 ? ? ? ? 48 8B 0C C8 0F B7 81"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 48 8B 0C C8 EB"),
    ]),

    # --- Model Info ---
    ("MODEL_INFO", [
        ("Primary",  "0F B7 05 ? ? ? ? 45 33 C9 4C 8B DA 66 85 C0"),
        ("Alt 1",    "48 8B 05 ? ? ? ? 48 8B 14 C8 48 85 D2 74 ? 8B 42"),
    ]),

    # --- Entity ---
    ("IS_ENTITY_VISIBLE", [
        ("Primary",  "48 89 5C 24 ? 57 48 83 EC ? 48 8B 01 48 8B F9 FF 90 ? ? 00 00 84 C0 74"),
    ]),
    ("SET_ENTITY_VISIBLE", [
        ("Primary",  "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 8B FA 48 8B D9 E8 ? ? ? ? 48 8B"),
    ]),

    # --- Ped ---
    ("GET_PED_NEARBY", [
        ("Primary",  "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 83 3D ? ? ? ? ? 48 8B F1"),
    ]),
    ("BONE_MATRIX", [
        ("Primary",  "E8 ? ? ? ? 48 8B 47 ? 44 0F 28 C0"),
        ("Alt 1",    "E8 ? ? ? ? 44 0F 28 C0 F3 44 0F 10 ? ? F3 44"),
    ]),

    # --- Vehicle ---
    ("FIX_VEHICLE", [
        ("Primary",  "80 A1 ? ? 00 00 ? 48 8B 89 ? ? 00 00 48 85 C9 74"),
    ]),
]


# ============================================================
# OFFSET DEFINITIONS
# (name, [(pattern, known_offset_or_None, description), ...])
# ============================================================

STRUCT_OFFSETS = {
    # --- CPed ---
    "CPed": [
        ("Ped_Health", [
            ("F3 0F 10 81 80 02 00 00", 0x280, "Health float"),
            ("F3 0F 10 81 ? 02 00 00", None, "Health float (wildcard)"),
        ]),
        ("Ped_MaxHealth", [
            ("F3 0F 10 81 84 02 00 00", 0x284, "MaxHealth float"),
            ("F3 0F 10 89 84 02 00 00", 0x284, "MaxHealth float alt"),
        ]),
        ("Ped_Armor", [
            ("F3 0F 10 81 88 02 00 00", 0x288, "Armor float (+8 from health)"),
            ("F3 0F 10 81 ? 02 00 00", None, "Armor float (wildcard)"),
        ]),
        ("Ped_Vehicle", [
            ("48 8B 89 30 10 00 00", 0x1030, "CurrentVehicle ptr"),
            ("48 8B 89 ? 10 00 00", None, "CurrentVehicle ptr (wildcard)"),
            ("48 8B 89 ? ? 00 00 48 85 C9 74", None, "CurrentVehicle ptr (broad)"),
        ]),
        ("Ped_Ragdoll", [
            ("80 B9 ? ? 00 00 00 48 8B F9 74", None, "Ragdoll flag"),
        ]),
        ("Ped_PlayerInfo", [
            ("48 8B 89 ? ? 00 00 48 85 C9 0F 84 ? ? 00 00 48 8B 41", None, "PlayerInfo ptr"),
        ]),
        ("Ped_WeaponMgr", [
            ("48 8B 89 ? ? 00 00 48 85 C9 74 ? 48 8B 41 ? 48 85 C0 74 ? E8", None, "WeaponManager ptr"),
            ("48 8B B1 ? ? 00 00 48 8B FA", None, "WeaponManager ptr alt"),
        ]),
        ("Ped_Drawhandler", [
            ("48 8B 89 ? ? 00 00 E8 ? ? ? ? 48 8B D0 48 8B CE E8", None, "DrawHandler ptr"),
        ]),
        ("Ped_Invisible", [
            ("0F BA B1 ? ? 00 00 ? 0F BA B1 ? ? 00 00 ? 0F BA B1", None, "Invisible bitfield"),
        ]),
        ("Ped_Seatbelt", [
            ("C0 E8 ? 24 01 48 8B 89 ? ? 00 00", None, "Seatbelt check"),
        ]),
    ],

    # --- CPlayerInfo ---
    "CPlayerInfo": [
        ("PI_WantedLevel", [
            ("89 91 88 08 00 00", 0x888, "WantedLevel write"),
            ("8B 81 88 08 00 00", 0x888, "WantedLevel read"),
            ("83 B9 88 08 00 00 00", 0x888, "WantedLevel compare"),
            ("89 91 ? ? 00 00", None, "WantedLevel (wildcard)"),
        ]),
        ("PI_WantedMax", [
            ("89 91 8C 08 00 00", 0x88C, "WantedMax write"),
            ("C7 81 8C 08 00 00 05 00 00 00", 0x88C, "WantedMax set to 5"),
            ("8B 81 8C 08 00 00", 0x88C, "WantedMax read"),
        ]),
        ("PI_FrameFlags", [
            ("83 89 ? ? 00 00 ? 48 8B 81", None, "FrameFlags or"),
            ("09 B1 ? ? 00 00", None, "FrameFlags or alt"),
        ]),
        ("PI_Stamina", [
            ("F3 0F 10 81 ? ? 00 00 F3 0F 5C 05", None, "Stamina float"),
        ]),
        ("PI_RunSpeed", [
            ("F3 0F 10 89 ? ? 00 00 0F 2F CA 73", None, "RunSpeed float"),
            ("F3 0F 11 81 ? ? 00 00 48 83 C4", None, "RunSpeed write"),
        ]),
        ("PI_SwimSpeed", [
            ("F3 0F 10 81 ? ? 00 00 F3 0F 59 05 ? ? ? ? F3 0F 58", None, "SwimSpeed float"),
        ]),
    ],

    # --- CNavigation ---
    "CNavigation": [
        ("Nav_PosX", [
            ("F3 0F 10 40 50", 0x50, "Position X"),
            ("F3 0F 10 41 50", 0x50, "Position X alt"),
        ]),
        ("Nav_PosY", [
            ("F3 0F 10 40 54", 0x54, "Position Y"),
        ]),
        ("Nav_PosZ", [
            ("F3 0F 10 40 58", 0x58, "Position Z"),
        ]),
        ("Nav_Heading", [
            ("F3 0F 10 41 ? F3 0F 10 51 ? F3 0F 10 69 ? 0F 28 E1", None, "Heading float"),
        ]),
        ("Nav_RotMatrix", [
            ("0F 28 01 0F 29 03 0F 28 49 10 0F 29 4B 10", 0x00, "Rotation 4x4 copy"),
        ]),
    ],

    # --- CVehicle ---
    "CVehicle": [
        ("Veh_Health", [
            ("F3 0F 10 B3 80 02 00 00", 0x280, "Vehicle health"),
            ("F3 0F 10 81 ? 03 00 00", None, "Vehicle health alt"),
        ]),
        ("Veh_HealthBody", [
            ("F3 0F 10 89 ? 09 00 00", None, "Body health"),
            ("F3 0F 10 B3 ? 09 00 00", None, "Body health alt"),
        ]),
        ("Veh_HealthEngine", [
            ("F3 0F 10 81 ? 09 00 00 F3 0F 5D 05", None, "Engine health"),
        ]),
        ("Veh_HealthPetrol", [
            ("F3 0F 10 81 ? 09 00 00 0F 2F C6", None, "Petrol tank health"),
        ]),
        ("Veh_Speed", [
            ("F3 0F 10 89 ? ? 00 00 F3 0F 59 C9 F3 0F 58 C1", None, "Speed float"),
            ("F3 0F 10 81 ? ? 00 00 0F 2F C1 76", None, "Speed compare"),
        ]),
        ("Veh_Handling", [
            ("48 8B 89 ? ? 00 00 48 85 C9 74 ? 0F 10 41", None, "Handling data ptr"),
            ("48 8B 93 ? ? 00 00 48 85 D2 74 ? 48 8B 82", None, "Handling data ptr alt"),
        ]),
        ("Veh_GodMode", [
            ("0F BA B3 ? ? 00 00 ? E8 ? ? ? ? 84 C0", None, "God mode bitfield"),
        ]),
        ("Veh_Gravity", [
            ("F3 0F 10 81 ? ? 00 00 F3 0F 59 05 ? ? ? ? F3 0F 58 81 ? ? 00 00", None, "Gravity float"),
        ]),
        ("Veh_CurrentGear", [
            ("0F B7 89 ? ? 00 00 66 89 4C 24", None, "Current gear"),
            ("66 89 86 ? ? 00 00 0F B6 86", None, "Current gear write"),
        ]),
        ("Veh_NextGear", [
            ("0F B7 81 ? ? 00 00 66 3B 81", None, "Next gear"),
        ]),
        ("Veh_HighGear", [
            ("0F B6 81 ? ? 00 00 3C ? 73", None, "Highest gear"),
        ]),
        ("Veh_RPM", [
            ("F3 0F 10 81 ? ? 00 00 F3 0F 58 81 ? ? 00 00 F3 0F 11 81 ? ? 00 00 C3", None, "RPM float"),
        ]),
        ("Veh_Turbo", [
            ("F3 0F 10 89 ? ? 00 00 0F 28 C1 F3 0F 5C 05", None, "Turbo float"),
        ]),
        ("Veh_IsStolen", [
            ("80 B9 ? ? 00 00 00 74 ? 48 8B D1", None, "IsStolen flag"),
        ]),
        ("Veh_WheelCount", [
            ("8B 81 ? ? 00 00 85 C0 74 ? 48 8B 89", None, "Wheel count"),
        ]),
        ("Veh_DirtLevel", [
            ("F3 0F 10 81 ? ? 00 00 F3 0F 5C C1 0F 2F C2", None, "Dirt level float"),
        ]),
        ("Veh_LockStatus", [
            ("8B 81 ? ? 00 00 3B C2 74", None, "Door lock status"),
        ]),
    ],

    # --- CWeaponManager ---
    "CWeaponManager": [
        ("WM_CurrentWeapon", [
            ("48 8B 41 ? 48 85 C0 74 ? 48 8B 88 ? 00 00 00 48 85 C9", None, "Current weapon ptr"),
        ]),
        ("WM_WeaponInfo", [
            ("48 8B 47 ? 48 85 C0 74 ? 0F 10 40", None, "Weapon info ptr"),
        ]),
    ],

    # --- CWeaponInfo ---
    "CWeaponInfo": [
        ("WI_Damage", [
            ("F3 0F 10 81 ? ? 00 00 48 8B 5C 24 ? 48 83 C4 ? 5F C3", None, "Damage float"),
        ]),
        ("WI_Spread", [
            ("F3 0F 10 81 ? ? 00 00 0F 2F C1 72", None, "Spread float"),
        ]),
        ("WI_Range", [
            ("F3 0F 10 89 ? ? 00 00 0F 2F C8 73 ? F3 0F 10 89", None, "Range float"),
        ]),
    ],

    # --- CNetGamePlayer ---
    "CNetGamePlayer": [
        ("NGP_PlayerID", [
            ("0F B6 41 ? 3C FF 74", None, "Player ID byte"),
            ("0F B6 71 ? 40 3A F5", None, "Player ID byte alt"),
        ]),
        ("NGP_Name", [
            ("48 8D 41 ? 48 8B 5C 24 ? 48 83 C4 ? 5F C3", None, "Name offset from player"),
        ]),
    ],
}


# ============================================================
# SCANNER ENGINE
# ============================================================

def read_exe(path: str) -> bytes:
    size = os.path.getsize(path)
    print(f"[*] Reading {path} ({size / 1024 / 1024:.1f} MB)")
    with open(path, "rb") as f:
        return f.read()


def pattern_to_list(pattern: str) -> list[int | None]:
    result = []
    for t in pattern.strip().split():
        if t in ("?", "??"):
            result.append(None)
        else:
            result.append(int(t, 16))
    return result


def scan_pattern(data: bytes, pattern: str) -> list[int]:
    pat = pattern_to_list(pattern)
    results = []
    pat_len = len(pat)
    data_len = len(data)
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


def resolve_rip(data: bytes, offset: int, instr_len: int = 7) -> int:
    disp = struct.unpack_from("<i", data, offset + instr_len - 4)[0]
    return offset + instr_len + disp


def format_bytes(data: bytes, offset: int, length: int = 20) -> str:
    end = min(offset + length, len(data))
    return " ".join(f"{b:02X}" for b in data[offset:end])


def extract_offset_from_pattern(data: bytes, pattern: str, hit: int) -> int | None:
    """Extract the wildcard offset value from a pattern match."""
    tokens = pattern.strip().split()
    try:
        idx = tokens.index("?")
    except ValueError:
        return None

    # Count consecutive wildcards
    wc_count = 0
    for t in tokens[idx:]:
        if t in ("?", "??"):
            wc_count += 1
        else:
            break

    if wc_count == 1:
        return data[hit + idx]
    elif wc_count == 2:
        return struct.unpack_from("<H", data, hit + idx)[0]
    elif wc_count == 4:
        return struct.unpack_from("<I", data, hit + idx)[0]
    else:
        return struct.unpack_from("<I", data, hit + idx)[0]


# ============================================================
# OUTPUT
# ============================================================

def scan_signatures(data: bytes) -> dict:
    results = {}
    print("\n" + "=" * 70)
    print(" SIGNATURES")
    print("=" * 70)

    for name, variants in SIGNATURES:
        print(f"\n  {name}:")
        found = False
        for vname, pat in variants:
            hits = scan_pattern(data, pat)
            if hits:
                off = hits[0]
                rip = resolve_rip(data, off)
                print(f"    [{vname}] FOUND — {len(hits)} hit(s)")
                print(f"      File offset:  0x{off:X}")
                print(f"      RIP target:   0x{rip:X}")
                print(f"      Bytes:        {format_bytes(data, off, 24)}")
                matched_pat = format_bytes(data, off, len(pat.split()))
                # Rebuild with wildcards preserved
                toks = pat.split()
                rebuilt = []
                for i, t in enumerate(toks):
                    if t in ("?", "??"):
                        rebuilt.append("?")
                    else:
                        rebuilt.append(f"{data[off + i]:02X}")
                print(f"      Pattern:      \"{' '.join(rebuilt)}\"")
                results[name] = {
                    "offset": off,
                    "rip": rip,
                    "pattern": " ".join(rebuilt),
                    "variant": vname,
                    "hits": len(hits),
                }
                found = True
                break
        if not found:
            print(f"    [!] NOT FOUND — all variants missed")
            results[name] = None

    return results


def scan_offsets(data: bytes) -> dict:
    results = {}
    print("\n" + "=" * 70)
    print(" STRUCT OFFSETS")
    print("=" * 70)

    for struct_name, fields in STRUCT_OFFSETS.items():
        print(f"\n  --- {struct_name} ---")
        struct_results = {}

        for field_name, variants in fields:
            print(f"\n    {field_name}:")
            found = False

            for pat, known_off, desc in variants:
                hits = scan_pattern(data, pat)
                if hits:
                    off = hits[0]
                    if known_off is not None:
                        print(f"      [{desc}] CONFIRMED — 0x{known_off:X} ({len(hits)} hits)")
                        struct_results[field_name] = known_off
                    else:
                        extracted = extract_offset_from_pattern(data, pat, off)
                        if extracted is not None:
                            print(f"      [{desc}] FOUND — 0x{extracted:X} ({len(hits)} hits)")
                            struct_results[field_name] = extracted
                        else:
                            print(f"      [{desc}] HIT but could not extract offset ({len(hits)} hits)")
                    print(f"        At: 0x{off:X} -> {format_bytes(data, off, 14)}")
                    found = True
                    break

            if not found:
                print(f"      [!] NOT FOUND")
                struct_results[field_name] = None

        results[struct_name] = struct_results

    return results


def print_summary(sig_results: dict, off_results: dict) -> None:
    print("\n" + "=" * 70)
    print(" SUMMARY — COPY INTO game.h")
    print("=" * 70)

    # Signatures
    print("\n// ---- Signatures ----")
    for name, data in sig_results.items():
        if data:
            print(f'#define SIG_{name} "{data["pattern"]}"  // {data["variant"]}, {data["hits"]} hits')
        else:
            print(f'#define SIG_{name} ""  // NOT FOUND — needs manual RE')

    # Offsets
    for struct_name, fields in off_results.items():
        print(f"\n// ---- {struct_name} ----")
        for field_name, value in fields.items():
            if value is not None:
                print(f"#define {field_name} 0x{value:X}")
            else:
                print(f"#define {field_name} 0x0  // NOT FOUND — FILL")


def main():
    if len(sys.argv) > 1:
        path = sys.argv[1]
    else:
        path = DEFAULT_PATH

    if not os.path.exists(path):
        print(f"[!] File not found: {path}")
        print(f"    Usage: py {sys.argv[0]} \"C:\\path\\to\\GTA5.exe\"")
        input("\nPress Enter to exit...")
        sys.exit(1)

    data = read_exe(path)
    print(f"[*] Scanning {len(data):,} bytes for {len(SIGNATURES)} signatures + "
          f"{sum(len(f) for f in STRUCT_OFFSETS.values())} offsets...")

    t0 = time.time()

    sig_results = scan_signatures(data)
    off_results = scan_offsets(data)

    elapsed = time.time() - t0
    print(f"\n[*] Scan completed in {elapsed:.1f}s")

    print_summary(sig_results, off_results)

    # Count results
    sig_found = sum(1 for v in sig_results.values() if v)
    sig_total = len(sig_results)
    off_found = sum(1 for s in off_results.values() for v in s.values() if v is not None)
    off_total = sum(len(s) for s in off_results.values())

    print(f"\n[*] Signatures: {sig_found}/{sig_total} found")
    print(f"[*] Offsets:    {off_found}/{off_total} found")

    if sig_found < sig_total or off_found < off_total:
        print(f"\n[!] {sig_total - sig_found + off_total - off_found} items need manual verification")
        print(f"    Use IDA/Ghidra + ReClass.NET for NOT FOUND entries")

    input("\nPress Enter to exit...")


if __name__ == "__main__":
    main()
