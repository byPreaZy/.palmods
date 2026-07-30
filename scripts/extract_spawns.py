#!/usr/bin/env python3
"""Extract Pal spawn data from DT_PaldexDistributionData and convert to pal_spawns.json format.

This script processes the raw spawn point cloud data from Palworld's game files
and converts it into the flat JSON format used by PalTrainerUltra's minimap.

Usage:
    python scripts/extract_spawns.py [--input PATH] [--output PATH]

Input:  Path to DT_PaldexDistributionData.json (dumped from game .pak via UE4SS or palworld-pal-extractor)
Output: Path to pal_spawns.json (default: data/pal_spawns.json)

The script:
1. Reads the raw distribution data (day/night spawn points per Pal)
2. Bins points onto a 50,000-unit X/Y grid
3. Averages each cluster's position and records density
4. Outputs in the flat format: [{palName, x, y, z, levelMin, levelMax, isDay, isNight, isAlpha, density}]
"""

import json
import os
import sys
import math
from collections import defaultdict

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(SCRIPT_DIR, "..")

DEFAULT_INPUT = os.path.join(ROOT, "data", "DT_PaldexDistributionData.json")
DEFAULT_OUTPUT = os.path.join(ROOT, "data", "pal_spawns.json")

GRID_SIZE = 50000  # Bin size in game units (cm)


def parse_args():
    args = {"input": DEFAULT_INPUT, "output": DEFAULT_OUTPUT}
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == "--input" and i + 1 < len(sys.argv):
            args["input"] = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == "--output" and i + 1 < len(sys.argv):
            args["output"] = sys.argv[i + 1]
            i += 2
        else:
            i += 1
    return args


def bin_points(points, grid_size=GRID_SIZE):
    """Bin a list of (x, y, z) points into grid cells, returning averaged clusters."""
    if not points:
        return []

    cells = defaultdict(list)
    for x, y, z in points:
        gx = int(x) // grid_size
        gy = int(y) // grid_size
        cells[(gx, gy)].append((x, y, z))

    clusters = []
    for (gx, gy), pts in cells.items():
        avg_x = sum(p[0] for p in pts) / len(pts)
        avg_y = sum(p[1] for p in pts) / len(pts)
        avg_z = sum(p[2] for p in pts) / len(pts)
        clusters.append({
            "x": round(avg_x),
            "y": round(avg_y),
            "z": round(avg_z),
            "density": len(pts)
        })

    clusters.sort(key=lambda c: c["density"], reverse=True)
    return clusters


def extract_spawns(input_path, output_path):
    if not os.path.exists(input_path):
        print(f"Error: Input file not found: {input_path}")
        print(f"\nTo get this file, you need to dump DT_PaldexDistributionData from Palworld's .pak files.")
        print(f"Options:")
        print(f"  1. Use UE4SS to dump the data table")
        print(f"  2. Use palworld-pal-extractor: https://github.com/mivashinko88-netizen/palworld-pal-extractor")
        print(f"  3. Use blaynem/paldex: https://github.com/blaynem/paldex")
        return False

    with open(input_path, encoding="utf-8") as f:
        data = json.load(f)

    spawns = []

    # The data can be in different formats depending on the source:
    # Format 1: { "PalName": { "spawnLocations": { "day": [[x,y,z],...], "night": [[x,y,z],...] } } }
    # Format 2: { "PalName": { "day": [[x,y,z],...], "night": [[x,y,z],...] } }
    # Format 3: [{ "name": "PalName", "spawnLocations": { "day": [...], "night": [...] }, ... }]

    if isinstance(data, dict):
        items = data.items()
    elif isinstance(data, list):
        items = [(item.get("name", item.get("palName", "Unknown")), item) for item in data]
    else:
        print("Error: Unrecognized data format")
        return False

    for pal_name, pal_data in items:
        if not isinstance(pal_data, dict):
            continue

        # Find spawn locations
        spawn_loc = pal_data.get("spawnLocations", pal_data)

        # Determine level range
        level_min = pal_data.get("levelMin", pal_data.get("minLevel", 1))
        level_max = pal_data.get("levelMax", pal_data.get("maxLevel", 100))
        is_alpha = pal_data.get("isBoss", pal_data.get("isAlpha", False))

        # Extract day/night points
        day_points = []
        night_points = []

        if isinstance(spawn_loc, dict):
            day_raw = spawn_loc.get("day", spawn_loc.get("Day", []))
            night_raw = spawn_loc.get("night", spawn_loc.get("Night", []))

            for p in day_raw:
                if isinstance(p, (list, tuple)) and len(p) >= 3:
                    day_points.append((float(p[0]), float(p[1]), float(p[2])))
                elif isinstance(p, dict):
                    day_points.append((float(p.get("x", 0)), float(p.get("y", 0)), float(p.get("z", 0))))

            for p in night_raw:
                if isinstance(p, (list, tuple)) and len(p) >= 3:
                    night_points.append((float(p[0]), float(p[1]), float(p[2])))
                elif isinstance(p, dict):
                    night_points.append((float(p.get("x", 0)), float(p.get("y", 0)), float(p.get("z", 0))))

        # Bin and cluster
        day_clusters = bin_points(day_points)
        night_clusters = bin_points(night_points)

        # Generate spawn entries
        for cluster in day_clusters:
            spawns.append({
                "palName": pal_name,
                "x": cluster["x"],
                "y": cluster["y"],
                "z": cluster["z"],
                "levelMin": level_min,
                "levelMax": level_max,
                "isDay": True,
                "isNight": False,
                "isAlpha": is_alpha,
                "density": cluster["density"]
            })

        for cluster in night_clusters:
            # Check if this cluster overlaps with a day cluster (same position)
            overlaps = any(
                abs(cluster["x"] - dc["x"]) < GRID_SIZE and
                abs(cluster["y"] - dc["y"]) < GRID_SIZE
                for dc in day_clusters
            )
            if overlaps:
                # Mark as both day and night
                for s in spawns:
                    if s["palName"] == pal_name and abs(s["x"] - cluster["x"]) < GRID_SIZE and abs(s["y"] - cluster["y"]) < GRID_SIZE:
                        s["isNight"] = True
                        break
            else:
                spawns.append({
                    "palName": pal_name,
                    "x": cluster["x"],
                    "y": cluster["y"],
                    "z": cluster["z"],
                    "levelMin": level_min,
                    "levelMax": level_max,
                    "isDay": False,
                    "isNight": True,
                    "isAlpha": is_alpha,
                    "density": cluster["density"]
                })

    # Write output
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(spawns, f, ensure_ascii=False, indent=2)

    # Stats
    pal_names = set(s["palName"] for s in spawns)
    print(f"Extracted {len(spawns)} spawn points for {len(pal_names)} Pals -> {output_path}")

    type_counts = {"day": 0, "night": 0, "alpha": 0}
    for s in spawns:
        if s["isDay"]:
            type_counts["day"] += 1
        if s["isNight"]:
            type_counts["night"] += 1
        if s["isAlpha"]:
            type_counts["alpha"] += 1

    print(f"  Day spawns: {type_counts['day']}")
    print(f"  Night spawns: {type_counts['night']}")
    print(f"  Alpha spawns: {type_counts['alpha']}")

    return True


if __name__ == "__main__":
    args = parse_args()
    success = extract_spawns(args["input"], args["output"])
    sys.exit(0 if success else 1)
