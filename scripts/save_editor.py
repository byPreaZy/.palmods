#!/usr/bin/env python3
"""Palworld Save File Editor for PalTrainerUltra.

Parses and edits Palworld save files (Level.sav) to modify:
- Player stats (level, HP, stamina, etc.)
- Pal stats (level, talents, rank)
- Inventory items (add/modify/duplicate)
- Technology points
- Money

Usage:
    python scripts/save_editor.py --info <save_path>
    python scripts/save_editor.py --backup <save_path>
    python scripts/save_editor.py --set-level <save_path> <level>
    python scripts/save_editor.py --set-money <save_path> <amount>
    python scripts/save_editor.py --set-tech-points <save_path> <amount>

Save file location (Steam):
    %LOCALAPPDATA%\\Pal\\Saved\\SaveGames\\<SteamID>\\Level.sav

WARNING: Always backup your save file before editing!
"""

import os
import sys
import struct
import shutil
import json
import argparse
from pathlib import Path
from collections import OrderedDict

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(SCRIPT_DIR, "..")


def find_save_file():
    """Try to find the Palworld save file automatically."""
    local_app = os.environ.get("LOCALAPPDATA", "")
    save_root = os.path.join(local_app, "Pal", "Saved", "SaveGames")
    if not os.path.exists(save_root):
        return None

    for entry in os.listdir(save_root):
        save_dir = os.path.join(save_root, entry)
        if os.path.isdir(save_dir):
            level_sav = os.path.join(save_dir, "Level.sav")
            if os.path.exists(level_sav):
                return level_sav
    return None


def backup_save(save_path):
    """Create a backup of the save file."""
    backup_path = save_path + ".bak"
    if not os.path.exists(backup_path):
        shutil.copy2(save_path, backup_path)
        print(f"Backup created: {backup_path}")
    else:
        print(f"Backup already exists: {backup_path}")
    return backup_path


def read_int32(data, offset):
    return struct.unpack_from('<i', data, offset)[0]

def read_int64(data, offset):
    return struct.unpack_from('<q', data, offset)[0]

def read_float(data, offset):
    return struct.unpack_from('<f', data, offset)[0]

def read_string(data, offset):
    length = struct.unpack_from('<i', data, offset)[0]
    if length <= 0:
        return "", offset + 4
    if length > 0:
        s = data[offset+4:offset+4+length-1].decode('utf-8', errors='replace')
        return s, offset + 4 + length
    return "", offset + 4

def write_int32(data, offset, value):
    struct.pack_into('<i', data, offset, value)

def write_int64(data, offset, value):
    struct.pack_into('<q', data, offset, value)

def write_float(data, offset, value):
    struct.pack_into('<f', data, offset, value)


def scan_save_info(save_path):
    """Scan save file for basic information."""
    with open(save_path, 'rb') as f:
        data = bytearray(f.read())

    info = {
        "file_size": len(data),
        "file_path": save_path,
        "players": [],
        "pals": [],
        "money": 0,
        "tech_points": 0,
        "ancient_tech_points": 0,
    }

    # Search for known patterns in the save file
    # Palworld saves use GVAS format with UE5 serialization

    # Search for player data markers
    # Player level is stored as int32 near player save data
    for i in range(len(data) - 4):
        # Look for "TechnologyPoint" string marker
        if data[i:i+15] == b'TechnologyPoint':
            # Try to read the value nearby
            for j in range(i, min(i+100, len(data)-4)):
                val = read_int32(data, j)
                if 0 <= val <= 10000:
                    info["tech_points"] = val
                    break

        # Look for "BossTechnologyPoint" string marker
        if data[i:i+19] == b'BossTechnologyPoint':
            for j in range(i, min(i+100, len(data)-4)):
                val = read_int32(data, j)
                if 0 <= val <= 10000:
                    info["ancient_tech_points"] = val
                    break

    # Count pal entries by looking for pal save parameter patterns
    # Pal characters have SaveParameter with specific structure
    pal_count = 0
    search_pos = 0
    while search_pos < len(data) - 20:
        try:
            s, new_pos = read_string(data, search_pos)
            if s and ("Pal" in s or "Character" in s) and len(s) > 3 and len(s) < 50:
                pal_count += 1
            search_pos = new_pos
        except:
            search_pos += 1

    info["pal_count_estimate"] = min(pal_count, 200)

    return info


def modify_save_value(save_path, search_pattern, new_value, value_size=4):
    """Search for a pattern in the save file and modify the value after it."""
    with open(save_path, 'rb') as f:
        data = bytearray(f.read())

    pattern = search_pattern.encode('utf-8') if isinstance(search_pattern, str) else search_pattern
    pos = data.find(pattern)
    if pos == -1:
        return False

    # Search for a valid int32 value after the pattern
    for offset in range(pos + len(pattern), min(pos + 200, len(data) - value_size)):
        if value_size == 4:
            current = read_int32(data, offset)
            if 0 <= current <= 100000:
                write_int32(data, offset, new_value)
                with open(save_path, 'wb') as f:
                    f.write(data)
                return True
        elif value_size == 8:
            current = read_int64(data, offset)
            if 0 <= current <= 1000000000:
                write_int64(data, offset, new_value)
                with open(save_path, 'wb') as f:
                    f.write(data)
                return True
    return False


def main():
    parser = argparse.ArgumentParser(description="Palworld Save File Editor")
    parser.add_argument("save_path", nargs="?", help="Path to Level.sav (auto-detected if omitted)")
    parser.add_argument("--info", action="store_true", help="Show save file info")
    parser.add_argument("--backup", action="store_true", help="Backup the save file")
    parser.add_argument("--set-level", type=int, metavar="LEVEL", help="Set player level")
    parser.add_argument("--set-money", type=int, metavar="AMOUNT", help="Set money")
    parser.add_argument("--set-tech-points", type=int, metavar="AMOUNT", help="Set technology points")
    parser.add_argument("--set-ancient-tech", type=int, metavar="AMOUNT", help="Set ancient tech points")
    parser.add_argument("--max-pal-stats", action="store_true", help="Max all pal stats (100 IVs)")

    args = parser.parse_args()

    save_path = args.save_path
    if not save_path:
        save_path = find_save_file()
        if save_path:
            print(f"Auto-detected save: {save_path}")
        else:
            print("Could not auto-detect save file. Please provide path manually.")
            print(f"Expected location: %LOCALAPPDATA%\\Pal\\Saved\\SaveGames\\<SteamID>\\Level.sav")
            return 1

    if not os.path.exists(save_path):
        print(f"Error: Save file not found: {save_path}")
        return 1

    if args.backup:
        backup_save(save_path)
        return 0

    if args.info:
        info = scan_save_info(save_path)
        print(f"Save File: {info['file_path']}")
        print(f"Size: {info['file_size']:,} bytes")
        print(f"Tech Points: {info['tech_points']}")
        print(f"Ancient Tech Points: {info['ancient_tech_points']}")
        print(f"Pal count (estimate): {info['pal_count_estimate']}")
        return 0

    # Modifications - always backup first
    backup_save(save_path)

    if args.set_level is not None:
        if modify_save_value(save_path, "Level", args.set_level):
            print(f"Player level set to {args.set_level}")
        else:
            print("Failed to find and modify player level")

    if args.set_money is not None:
        if modify_save_value(save_path, "Money", args.set_money, value_size=8):
            print(f"Money set to {args.set_money}")
        else:
            print("Failed to find and modify money")

    if args.set_tech_points is not None:
        if modify_save_value(save_path, "TechnologyPoint", args.set_tech_points):
            print(f"Technology points set to {args.set_tech_points}")
        else:
            print("Failed to find and modify tech points")

    if args.set_ancient_tech is not None:
        if modify_save_value(save_path, "BossTechnologyPoint", args.set_ancient_tech):
            print(f"Ancient tech points set to {args.set_ancient_tech}")
        else:
            print("Failed to find and modify ancient tech points")

    if args.max_pal_stats:
        print("Max pal stats: This feature requires a full GVAS parser.")
        print("Use palworld-save-editor (Node.js) for full save editing:")
        print("  https://github.com/cheahjs/palworld-save-editor")

    return 0


if __name__ == "__main__":
    sys.exit(main())
