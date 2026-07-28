#!/usr/bin/env python3
"""Convert palworld-server-manager 1.0 POI data to PalTrainerUltra unified format."""

import json
import os

MAPDATA_DIR = os.path.join(os.path.dirname(__file__), "..", "web", "mapdata")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "web", "mapObjects.json")

def boss_name(character_id, spawner_id):
    if character_id and character_id != "None":
        n = character_id.replace("BOSS_", "").replace("_", " ")
        # CamelCase to spaces
        result = []
        for i, c in enumerate(n):
            if i > 0 and c.isupper() and n[i-1].islower():
                result.append(" ")
            result.append(c)
        n = "".join(result).strip()
        if n:
            return n
    n = spawner_id.replace("BOSS_", "").replace("REGION_", "").replace("_", " ")
    result = []
    for i, c in enumerate(n):
        if i > 0 and c.isupper() and n[i-1].islower():
            result.append(" ")
        result.append(c)
    n = "".join(result).strip()
    return n if n else "Boss"

def main():
    pois = []

    # 1. Fast travel points
    with open(os.path.join(MAPDATA_DIR, "fast_travel.json"), encoding="utf-8") as f:
        ft_data = json.load(f)
    for guid, p in ft_data.items():
        pois.append({
            "id": p.get("id", guid),
            "label": p.get("localized_name", "Fast Travel Point"),
            "type": "fastTravelPoint",
            "location": {
                "X": float(p["x"]),
                "Y": float(p["y"]),
                "Z": float(p.get("z", 0))
            }
        })

    # 2. Map objects (dungeons, etc.)
    with open(os.path.join(MAPDATA_DIR, "map_objects.json"), encoding="utf-8") as f:
        obj_data = json.load(f)
    type_map = {
        "dungeon": "dungeon",
        "egg": "egg",
        "treasure": "treasure",
        "enemyCamp": "enemyCamp",
        "oilrig": "oilrig",
    }
    for i, o in enumerate(obj_data):
        raw_type = o.get("type", "unknown")
        poi_type = type_map.get(raw_type, raw_type)
        pois.append({
            "id": f"obj_{i}",
            "label": poi_type.capitalize(),
            "type": poi_type,
            "location": {
                "X": float(o["x"]),
                "Y": float(o["y"]),
                "Z": 0.0
            }
        })

    # 3. Bosses
    with open(os.path.join(MAPDATA_DIR, "bosses.json"), encoding="utf-8") as f:
        boss_data = json.load(f)
    for i, (key, b) in enumerate(boss_data.items()):
        name = boss_name(b.get("character_id", ""), b.get("spawner_id", ""))
        level = b.get("level", 0)
        label = f"{name} (Lv {level})" if level else name
        pois.append({
            "id": f"boss_{i}",
            "label": label,
            "type": "strongEnemy",
            "location": {
                "X": float(b["x"]),
                "Y": float(b["y"]),
                "Z": float(b.get("z", 0))
            }
        })

    # 4. Effigies (relics)
    with open(os.path.join(MAPDATA_DIR, "effigies.json"), encoding="utf-8") as f:
        eff_data = json.load(f)
    for guid, p in eff_data.items():
        pois.append({
            "id": f"relic_{guid[:8]}",
            "label": "Relic",
            "type": "treasure",
            "location": {
                "X": float(p["x"]),
                "Y": float(p["y"]),
                "Z": float(p.get("z", 0))
            }
        })

    # Write output
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(pois, f, ensure_ascii=False, indent=2)

    # Stats
    type_counts = {}
    for p in pois:
        t = p["type"]
        type_counts[t] = type_counts.get(t, 0) + 1

    print(f"Generated {len(pois)} POIs -> {OUTPUT_PATH}")
    for t, c in sorted(type_counts.items()):
        print(f"  {t}: {c}")

    # Verify some known points
    main_count = 0
    tree_count = 0
    for p in pois:
        x = p["location"]["X"]
        y = p["location"]["Y"]
        if 347351.5 <= x <= 689148.5 and -818197 <= y <= -476400:
            tree_count += 1
        elif -1099400 <= x <= 349400 and -724400 <= y <= 724400:
            main_count += 1
    print(f"\nArea distribution: MainMap={main_count}, Tree={tree_count}")

    # Check WorldTree_A fast travel point
    for p in pois:
        if p["id"] == "WorldTree_A":
            print(f"\nWorldTree_A: X={p['location']['X']} Y={p['location']['Y']} (should be ~512112, -510663)")

if __name__ == "__main__":
    main()
