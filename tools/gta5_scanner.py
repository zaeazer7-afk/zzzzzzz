#!/usr/bin/env python3
r"""
GTA5 Enhanced — signature + offset scanner.
Uses patterns sourced from YimMenuV2 (verified for GTA5_Enhanced.exe).

Usage:
    py gta5_scanner.py
    py gta5_scanner.py "D:\path\to\GTA5_Enhanced.exe"
"""

import struct
import sys
import os
import time

DEFAULT_PATH = r"C:\Program Files\Rockstar Games\Grand Theft Auto V Enhanced\GTA5_Enhanced.exe"


# ============================================================
# SIGNATURE DEFINITIONS — from YimMenuV2 Pointers.cpp
# Each: (name, [(variant_name, pattern), ...])
# ============================================================

SIGNATURES = [
    # --- Rendering ---
    ("SWAPCHAIN", [
        ("YimV2",    "72 C7 EB 02 31 C0 8B 0D"),
    ]),
    ("WNDPROC", [
        ("YimV2",    "3D 85 00 00 00 0F 87 2D 02 00 00"),
    ]),
    ("SCREEN_RES", [
        ("YimV2",    "75 39 0F 57 C0 F3 0F 2A 05"),
    ]),

    # --- Version ---
    ("VERSION", [
        ("YimV2",    "4C 8D 0D ? ? ? ? 48 8D 5C 24 ? 48 89 D9 48 89 FA"),
    ]),

    # --- Script ---
    ("SCRIPT_THREADS", [
        ("YimV2",    "48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97"),
    ]),
    ("INIT_NATIVE_TABLES", [
        ("YimV2",    "EB 2A 0F 1F 40 00 48 8B 54 17 10"),
    ]),
    ("RUN_SCRIPT_THREADS", [
        ("YimV2",    "BE 40 5D C6 00"),
    ]),
    ("SCRIPT_GLOBALS", [
        ("YimV2",    "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8"),
    ]),
    ("SCRIPT_PROGRAMS", [
        ("YimV2",    "48 C7 84 C8 D8 00 00 00 00 00 00 00"),
    ]),
    ("SCRIPT_VM", [
        ("YimV2",    "49 63 41 1C"),
    ]),

    # --- Handles / Entity ---
    ("HANDLES_AND_PTRS", [
        ("YimV2",    "0F 1F 84 00 00 00 00 00 89 F8 0F 28 FE 41"),
    ]),
    ("PED_FACTORY", [
        ("YimV2",    "C7 40 30 03 00 00 00 48 8B 0D"),
    ]),

    # --- Network ---
    ("GET_NET_PLAYER_FROM_PID", [
        ("YimV2",    "83 FB 20 74 2A 89 D9"),
    ]),
    ("IS_SESSION_STARTED", [
        ("YimV2",    "0F B6 05 ? ? ? ? 0A 05 ? ? ? ? 75 2A"),
    ]),
    ("ASSIGN_PHYSICAL_INDEX", [
        ("YimV2",    "41 8D 47 01 3C 20 0F 87 68 01 00 00"),
    ]),
    ("NETWORK_PLAYER_MGR_INIT", [
        ("YimV2",    "41 56 56 57 55 53 48 83 EC 30 4C 89 CF 45 89 C6 48 89 D3"),
    ]),
    ("NETWORK_PLAYER_MGR_SHUTDOWN", [
        ("YimV2",    "84 C0 0F 84 88 00 00 00 4C 89 F9 E8"),
    ]),
    ("NETWORK_PLAYER_MGR", [
        ("YimV2",    "75 0E 48 8B 05 ? ? ? ? 48 8B 88 F0 00 00 00"),
    ]),
    ("NETWORK_OBJ_MGR", [
        ("YimV2",    "41 83 7E FA 02 40 0F 9C C5 C1 E5 02"),
    ]),
    ("NETWORK_SESSION", [
        ("YimV2",    "49 C7 86 F8 00 00 00 00 00 00 00 48 8B 05"),
    ]),
    ("NETWORK_TIME", [
        ("YimV2",    "89 05 ? ? ? ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? E9"),
    ]),
    ("GAME_TIMER", [
        ("YimV2",    "3B 2D ? ? ? ? 76 ? 89 D9"),
    ]),
    ("REGION_CODE", [
        ("YimV2",    "4C 8D 05 ? ? ? ? 48 89 F1 48 89 FA E8 ? ? ? ? 84 C0 74 3D"),
    ]),

    # --- Weapon ---
    ("TRIGGER_WEAPON_DAMAGE_EVENT", [
        ("YimV2",    "E8 ? ? ? ? 66 90 FF C5"),
    ]),

    # --- Sync ---
    ("WRITE_NODE_DATA", [
        ("YimV2",    "48 8B 89 A8 00 00 00 48 8B 01 48 8B 40 10 48 FF E0"),
    ]),
    ("SHOULD_USE_NODE_CACHE", [
        ("YimV2",    "83 FA 20 74 1D 48 89 CE"),
    ]),
    ("IS_NODE_IN_SCOPE", [
        ("YimV2",    "41 83 F9 02 74 22 48 8B 06"),
    ]),
    ("WRITE_SYNC_TREE", [
        ("YimV2",    "4D 89 CD 45 89 C6 41 89 D7 48 89 CF 8B 05 ? ? ? ? 65 48 8B 0C 25 58 00 00 00"),
    ]),
    ("MIGRATE_OBJECT", [
        ("YimV2",    "48 8B 96 D0 00 00 00 4C 89 F9 41 B8 03 00 00 00"),
    ]),

    # --- Packets / Events ---
    ("QUEUE_PACKET", [
        ("YimV2",    "45 89 F1 E8 ? ? ? ? 84 C0 74 25"),
    ]),
    ("GET_NET_OBJECT_BY_ID", [
        ("YimV2",    "0F B7 4E 60 E8"),
    ]),
    ("REQUEST_CONTROL", [
        ("YimV2",    "74 0A 48 89 F9 E8 ? ? ? ? 31 F6 89 F0 48 83 C4 20"),
    ]),
    ("RECEIVE_NET_MESSAGE", [
        ("YimV2",    "48 81 C1 00 03 00 00 4C 89 E2"),
    ]),
    ("NET_EVENT_MGR", [
        ("YimV2",    "4C 8B 05 ? ? ? ? 44 0F B7 CA"),
    ]),
    ("SEND_EVENT_ACK", [
        ("YimV2",    "E8 ? ? ? ? 84 C0 75 ? 44 89 F5"),
    ]),

    # --- Pools ---
    ("PED_POOL", [
        ("YimV2",    "80 79 4B 00 0F 84 F5 00 00 00 48 89 F1"),
    ]),
    ("VEHICLE_POOL", [
        ("YimV2",    "48 83 78 18 0D"),
    ]),
    ("OBJECT_POOL", [
        ("YimV2",    "48 8B 04 0A C3 0F B6 05"),
    ]),

    # --- Bypass ---
    ("MODEL_SPAWN_BYPASS", [
        ("YimV2",    "E8 ? ? ? ? 48 8B 78 48"),
    ]),
    ("SPECTATE_PATCH", [
        ("YimV2",    "74 ? 66 83 FF 0D"),
    ]),
    ("WORLD_MODEL_SPAWN_BYPASS", [
        ("YimV2",    "4C 8B 2C 01 4D 85 ED 0F 84 ? ? ? ?"),
    ]),

    # --- BattlEye ---
    ("BE_DATA", [
        ("YimV2",    "48 C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? 48 89 C1 E8 ? ? ? ? E8 ? ? ? ? BD 0A 00 00 00"),
    ]),
    ("BE_STATUS_UPDATE_PATCH", [
        ("YimV2",    "C6 05 ? ? ? ? 00 84 C0 0F 84 ? ? ? ? E9"),
    ]),
    ("BE_SERVER_PROCESS_PLAYER_JOIN", [
        ("YimV2",    "48 89 10 48 89 48 10 89 F9"),
    ]),

    # --- Stats ---
    ("STATS_MGR", [
        ("YimV2",    "89 6C 24 28 48 8D 0D ? ? ? ? 48 8D"),
    ]),
    ("GET_PACKED_STAT_DATA", [
        ("YimV2",    "8D 81 37 FE FF FF"),
    ]),

    # --- Catalog / Shop ---
    ("NET_CATALOG", [
        ("YimV2",    "0F 82 55 FF FF FF 44 89 7C 24 30"),
    ]),
    ("TRANSACTION_MGR", [
        ("YimV2",    "48 8B 05 ? ? ? ? 80 78 39 00 74 2D"),
    ]),
    ("GET_ACTIVE_BASKET", [
        ("YimV2",    "48 8B 40 10 81 7B 0C AE A0 A9 04"),
    ]),

    # --- HTTP / Network Session ---
    ("HTTP_START_REQUEST", [
        ("YimV2",    "56 57 48 83 EC 28 48 89 CE 8B 81 ? ? ? ? FF C8 83 F8 04 0F 87"),
    ]),
    ("JOIN_SESSION_BY_INFO", [
        ("YimV2",    "E8 ? ? ? ? 0F 10 87 ? ? ? ? 0F 11 86 ? ? ? ? 88 86 ? ? ? ? 84 C0"),
    ]),
    ("GET_SESSION_BY_GAMER_HANDLE", [
        ("YimV2",    "48 C7 84 24 80 00 00 00 10 00 00 08"),
    ]),

    # --- Queue / SigScan ---
    ("QUEUE_DEPENDENCY", [
        ("YimV2",    "0F 29 46 50 48 8D 05"),
    ]),

    # --- Metric ---
    ("PREPARE_METRIC_FOR_SENDING", [
        ("YimV2",    "41 56 56 57 55 53 48 83 EC ? 4C 89 CB 4C 89 C6"),
    ]),

    # --- Net Array ---
    ("WRITE_NET_ARRAY_DATA", [
        ("YimV2",    "0F 84 06 03 00 00 0F B6 83"),
    ]),
    ("NET_ARRAY_CACHED_DATA_PATCH", [
        ("YimV2",    "0F 84 64 FE FF FF FF"),
    ]),

    # --- Aim Assist ---
    ("SHOULD_NOT_TARGET_ENTITY_PATCH", [
        ("YimV2",    "F6 80 A9 14 00 00 01"),
    ]),
    ("GET_ASSISTED_AIM_TYPE_PATCH", [
        ("YimV2",    "FF E0 48 8D 86"),
    ]),
    ("GET_LOCK_ON_POS_PATCH", [
        ("YimV2",    "0F 29 74 24 ? 48 89 D6 48 89 CF 48 8B 05"),
    ]),
    ("SHOULD_ALLOW_DRIVER_LOCK_ON_PATCH", [
        ("YimV2",    "75 ? 45 89 C7 49 89 CE"),
    ]),
    ("ASSISTED_AIM_SHOULD_RELEASE_ENTITY", [
        ("YimV2",    "80 7F 28 04 75 6A"),
    ]),
    ("ASSISTED_AIM_FIND_NEW_TARGET", [
        ("YimV2",    "0F 84 C9 00 00 00 48 89 CE 48 89 F9"),
    ]),

    # --- Pause / Misc ---
    ("ALLOW_PAUSING_IN_SESSION_PATCH", [
        ("YimV2",    "80 88 ? ? ? ? ? EB ? E8"),
    ]),
    ("GET_POOL_TYPE", [
        ("YimV2",    "BA CE 94 A6 ED E8"),
    ]),
    ("SET_JOIN_REQUEST_POOL_TYPE_PATCH", [
        ("YimV2",    "89 86 ? ? ? ? E8 ? ? ? ? 89 C0"),
    ]),
    ("HANDLE_JOIN_REQUEST_IGNORE_POOL_PATCH", [
        ("YimV2",    "41 83 FF 05 0F 84 ? ? ? ? 8B 84 F7"),
    ]),

    # --- Stats MP ---
    ("STATS_MP_CHARACTER_MAPPING", [
        ("YimV2",    "48 8D 0D ? ? ? ? 89 F2 0F 28 74 24 ? 48 83 C4 38"),
    ]),
    ("SC_MEMBERSHIP", [
        ("YimV2",    "48 8D 15 ? ? ? ? 41 B8 18 02 00 00 E8"),
    ]),

    # --- Format ---
    ("FORMAT_INT_CALLER_1", [
        ("YimV2",    "48 89 35 ? ? ? ? 48 8B 74 24"),
    ]),
    ("FORMAT_INT_CALLER_2", [
        ("YimV2",    "48 B8 20 73 69 7A 65 3D 27 32 48 89 84 24"),
    ]),

    # --- DLC ---
    ("GET_DLC_HASH", [
        ("YimV2",    "31 D2 E8 ? ? ? ? 3B 84"),
    ]),

    # --- Game Data ---
    ("GAME_DATA_HASH", [
        ("YimV2",    "48 8D 3D ? ? ? ? 69 C9"),
    ]),

    # --- Anti-cheat ---
    ("ANTICHEAT_INITIALIZED_HASH", [
        ("YimV2",    "89 9E C8 00 00 00 48 8B 0D ? ? ? ? 48 85 C9 74 46"),
    ]),
    ("ANTICHEAT_CONTEXT", [
        ("YimV2",    "48 8D BB 70 0A 00 00 4C 8D 35 ? ? ? ? 66 90"),
    ]),
    ("GET_ANTICHEAT_INITIALIZED_HASH_2", [
        ("YimV2",    "89 9E E8 00 00 00 89 C2 E8 ? ? ? ? 69"),
    ]),

    # --- UI ---
    ("ABILITY_BAR_PATCH", [
        ("YimV2",    "75 39 48 85 F6 74 1A 48 89 F1 E8"),
    ]),

    # --- Matchmaking ---
    ("MATCHMAKING_ADVERTISE", [
        ("YimV2",    "C7 47 30 01 00 00 00 E9 92"),
    ]),
    ("MATCHMAKING_UPDATE", [
        ("YimV2",    "C7 47 30 02 00 00 00 EB 7A"),
    ]),
    ("MATCHMAKING_UNADVERTISE", [
        ("YimV2",    "C7 86 C8 01 00 00 04 00 00 00"),
    ]),
    ("MATCHMAKING_SESSION_DETAIL_SEND_RESPONSE", [
        ("YimV2",    "48 B8 01 00 00 00 0D 00 00 00"),
    ]),

    # --- Skeleton ---
    ("GAME_SKELETON_UPDATE", [
        ("YimV2",    "56 48 83 EC 20 48 8B 81 40 01 00 00 48 85 C0"),
    ]),
]


# ============================================================
# STRUCT OFFSET DEFINITIONS — from YimMenuV2 types
# ============================================================

STRUCT_OFFSETS = {
    "fwEntity": [
        ("Entity_ModelInfo", [
            ("48 8B ? 20", 0x20, "CBaseModelInfo* at 0x20"),
        ]),
        ("Entity_Type", [
            ("FE", 0x28, "Type byte at 0x28"),
        ]),
        ("Entity_Navigation", [
            ("48 8B ? 30", 0x30, "CNavigation* at 0x30"),
        ]),
        ("Entity_DynComponent", [
            ("48 8B ? 40", 0x40, "fwDynamicEntityComponent* at 0x40"),
        ]),
        ("Entity_DrawData", [
            ("48 8B ? 48", 0x48, "fwDrawData* at 0x48"),
        ]),
        ("Entity_Transform", [
            ("0F 28 01 0F 29 03", 0x60, "matrix44 at 0x60"),
        ]),
        ("Entity_Position", [
            ("F3 0F 10 ? 90", 0x90, "Position from transform row[3] at 0x90"),
        ]),
    ],

    "CEntity": [
        ("CEntity_Flags3", [
            ("8B 81 C0 00 00 00", 0xC0, "flags_3 at 0xC0"),
            ("89 81 C0 00 00 00", 0xC0, "flags_3 write"),
        ]),
        ("CEntity_Flags4", [
            ("8B 81 C4 00 00 00", 0xC4, "flags_4 at 0xC4"),
            ("89 81 C4 00 00 00", 0xC4, "flags_4 write"),
        ]),
    ],

    "CDynamicEntity": [
        ("DynEnt_NetObject", [
            ("48 8B 81 D0 00 00 00", 0xD0, "netObject* at 0xD0"),
            ("48 8B 89 D0 00 00 00", 0xD0, "netObject* alt"),
        ]),
    ],

    "CPedFactory": [
        ("PedFactory_LocalPed", [
            ("48 8B 41 08", 0x08, "m_LocalPed at 0x08"),
            ("48 8B 49 08", 0x08, "m_LocalPed alt"),
        ]),
    ],

    "netPlayer": [
        ("NetPlayer_AccountId", [
            ("8B 41 08", 0x08, "m_AccountId at 0x08"),
        ]),
        ("NetPlayer_RockstarId", [
            ("48 8B 41 10", 0x10, "m_RockstarId at 0x10"),
        ]),
        ("NetPlayer_ActiveIndex", [
            ("0F B6 41 60", 0x60, "m_ActiveIndex at 0x60"),
        ]),
        ("NetPlayer_PlayerIndex", [
            ("0F B6 41 61", 0x61, "m_PlayerIndex at 0x61"),
        ]),
        ("NetPlayer_Flags", [
            ("F6 81 D0 00 00 00 01", 0xD0, "m_Flags at 0xD0"),
        ]),
    ],

    "CNetGamePlayer": [
        ("NGP_PlayerInfo", [
            ("48 8B 81 E8 00 00 00", 0xE8, "m_PlayerInfo at 0xE8"),
            ("48 8B 89 E8 00 00 00", 0xE8, "m_PlayerInfo alt"),
        ]),
    ],

    "PoolEncryption": [
        ("Pool_IsSet", [
            ("80 79 00 00", 0x00, "m_IsSet bool at 0x00"),
            ("80 39 00", 0x00, "m_IsSet alt"),
        ]),
        ("Pool_First", [
            ("48 8B 41 08", 0x08, "m_First at 0x08"),
        ]),
        ("Pool_Second", [
            ("48 8B 41 10", 0x10, "m_Second at 0x10"),
        ]),
    ],

    "rlGamerInfo": [
        ("GamerInfo_HostToken", [
            ("48 8B 81 C0 00 00 00", 0xC0, "m_HostToken at 0xC0"),
        ]),
        ("GamerInfo_Name", [
            ("48 8D 81 DC 00 00 00", 0xDC, "m_name at 0xDC"),
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


def pattern_to_list(pattern: str) -> list:
    result = []
    for t in pattern.strip().split():
        if t in ("?", "??"):
            result.append(None)
        else:
            result.append(int(t, 16))
    return result


def scan_pattern(data: bytes, pattern: str) -> list:
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


def extract_offset_from_pattern(data: bytes, pattern: str, hit: int):
    tokens = pattern.strip().split()
    try:
        idx = tokens.index("?")
    except ValueError:
        return None

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
    print("  SIGNATURES (YimMenuV2 patterns)")
    print("=" * 70)

    for name, variants in SIGNATURES:
        print(f"\n  {name}:")
        found = False
        for vname, pat in variants:
            hits = scan_pattern(data, pat)
            if hits:
                off = hits[0]
                print(f"    [{vname}] FOUND - {len(hits)} hit(s)")
                print(f"      File offset:  0x{off:X}")
                print(f"      Bytes:        {format_bytes(data, off, 24)}")

                toks = pat.split()
                rebuilt = []
                for i, t in enumerate(toks):
                    if t in ("?", "??"):
                        rebuilt.append("?")
                    else:
                        rebuilt.append(f"{data[off + i]:02X}")
                print(f"      Resolved:     \"{' '.join(rebuilt)}\"")

                # Try RIP resolve for patterns starting with common LEA/MOV opcodes
                try:
                    if data[off] in (0x48, 0x4C) and data[off+1] in (0x8B, 0x8D):
                        rip = resolve_rip(data, off)
                        print(f"      RIP target:   0x{rip:X}")
                    elif data[off+2] in (0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D):
                        rip = resolve_rip(data, off)
                        print(f"      RIP target:   0x{rip:X}")
                except Exception:
                    pass

                results[name] = {
                    "offset": off,
                    "pattern": " ".join(rebuilt),
                    "variant": vname,
                    "hits": len(hits),
                }
                found = True
                break
        if not found:
            print(f"    [!] NOT FOUND")
            results[name] = None

    return results


def scan_offsets(data: bytes) -> dict:
    results = {}
    print("\n" + "=" * 70)
    print("  STRUCT OFFSETS (YimMenuV2 types)")
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
                        print(f"      [{desc}] CONFIRMED - 0x{known_off:X} ({len(hits)} hits)")
                        struct_results[field_name] = known_off
                    else:
                        extracted = extract_offset_from_pattern(data, pat, off)
                        if extracted is not None:
                            print(f"      [{desc}] FOUND - 0x{extracted:X} ({len(hits)} hits)")
                            struct_results[field_name] = extracted
                        else:
                            print(f"      [{desc}] HIT but offset unclear ({len(hits)} hits)")
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
    print("  SUMMARY - COPY INTO game.h")
    print("=" * 70)

    print("\n// ---- Signatures (YimMenuV2) ----")
    for name, data in sig_results.items():
        if data:
            print(f'#define SIG_{name} "{data["pattern"]}"  // {data["hits"]} hits')
        else:
            print(f'// #define SIG_{name} ""  // NOT FOUND')

    for struct_name, fields in off_results.items():
        print(f"\n// ---- {struct_name} ----")
        for field_name, value in fields.items():
            if value is not None:
                print(f"#define {field_name} 0x{value:X}")
            else:
                print(f"// #define {field_name} 0x0  // NOT FOUND")


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
    total_sigs = len(SIGNATURES)
    total_offs = sum(len(f) for f in STRUCT_OFFSETS.values())
    print(f"[*] Scanning {len(data):,} bytes for {total_sigs} signatures + {total_offs} offsets...")

    t0 = time.time()

    sig_results = scan_signatures(data)
    off_results = scan_offsets(data)

    elapsed = time.time() - t0
    print(f"\n[*] Scan completed in {elapsed:.1f}s")

    print_summary(sig_results, off_results)

    sig_found = sum(1 for v in sig_results.values() if v)
    off_found = sum(1 for s in off_results.values() for v in s.values() if v is not None)

    print(f"\n[*] Signatures: {sig_found}/{total_sigs} found")
    print(f"[*] Offsets:    {off_found}/{total_offs} found")

    if sig_found < total_sigs or off_found < total_offs:
        missing = (total_sigs - sig_found) + (total_offs - off_found)
        print(f"\n[!] {missing} items not found - game may have updated")

    input("\nPress Enter to exit...")


if __name__ == "__main__":
    main()
