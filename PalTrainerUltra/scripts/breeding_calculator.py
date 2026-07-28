#!/usr/bin/env python3
"""Pal breeding calculator for PalTrainerUltra.

Uses the breeding power system: child = closest Pal whose breeding power
matches the average of the two parents' breeding power.

Usage:
    python scripts/breeding_calculator.py parent1 parent2
    python scripts/breeding_calculator.py --list
    python scripts/breeding_calculator.py --reverse child

Examples:
    python scripts/breeding_calculator.py Lamball Cattiva
    python scripts/breeding_calculator.py --reverse Anubis
"""

import json
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(SCRIPT_DIR, "..")
DB_PATH = os.path.join(ROOT, "data", "pal_database.json")


def load_database():
    with open(DB_PATH, encoding="utf-8") as f:
        return json.load(f)


def find_pal(db, name):
    name_lower = name.lower()
    for pal in db:
        if pal["name"].lower() == name_lower:
            return pal
    return None


def calculate_child(db, parent1_name, parent2_name):
    p1 = find_pal(db, parent1_name)
    p2 = find_pal(db, parent2_name)
    if not p1:
        print(f"Error: Pal '{parent1_name}' not found in database.")
        return None
    if not p2:
        print(f"Error: Pal '{parent2_name}' not found in database.")
        return None

    avg_power = (p1["breedingPower"] + p2["breedingPower"]) / 2.0

    closest = None
    closest_diff = float("inf")
    for pal in db:
        if pal["name"] == p1["name"] or pal["name"] == p2["name"]:
            continue
        diff = abs(pal["breedingPower"] - avg_power)
        if diff < closest_diff:
            closest_diff = diff
            closest = pal

    return {
        "parent1": p1,
        "parent2": p2,
        "child": closest,
        "avg_power": avg_power,
        "power_diff": closest_diff
    }


def find_parents(db, child_name):
    child = find_pal(db, child_name)
    if not child:
        print(f"Error: Pal '{child_name}' not found in database.")
        return []

    target_power = child["breedingPower"]
    combos = []

    for i, p1 in enumerate(db):
        for p2 in db[i+1:]:
            avg = (p1["breedingPower"] + p2["breedingPower"]) / 2.0
            diff = abs(avg - target_power)
            if diff <= 50:
                combos.append({
                    "parent1": p1["name"],
                    "parent2": p2["name"],
                    "avg_power": avg,
                    "diff": diff
                })

    combos.sort(key=lambda c: c["diff"])
    return combos


def print_pal_info(pal):
    types = pal["type1"]
    if pal.get("type2"):
        types += f"/{pal['type2']}"
    works = ", ".join(pal.get("workSuitability", []))
    print(f"  {pal['name']} (#{pal['id']})")
    print(f"    Type: {types}")
    print(f"    Breeding Power: {pal['breedingPower']}")
    if works:
        print(f"    Work: {works}")


def main():
    db = load_database()

    if len(sys.argv) < 2:
        print(__doc__)
        return

    if sys.argv[1] == "--list":
        print(f"Pal Database ({len(db)} Pals):")
        print(f"{'ID':>4}  {'Name':<25} {'Type':<20} {'Power':>6}")
        print("-" * 60)
        for pal in db:
            types = pal["type1"]
            if pal.get("type2"):
                types += f"/{pal['type2']}"
            print(f"{pal['id']:>4}  {pal['name']:<25} {types:<20} {pal['breedingPower']:>6}")
        return

    if sys.argv[1] == "--reverse" and len(sys.argv) >= 3:
        child_name = sys.argv[2]
        combos = find_parents(db, child_name)
        if not combos:
            print(f"No parent combinations found for {child_name}.")
            return
        child = find_pal(db, child_name)
        print(f"Breeding combinations for: {child_name} (power={child['breedingPower']})")
        print(f"Found {len(combos)} combinations:\n")
        for i, c in enumerate(combos[:20]):
            print(f"  {i+1}. {c['parent1']} + {c['parent2']} (avg={c['avg_power']:.0f}, diff={c['diff']:.0f})")
        return

    if len(sys.argv) >= 3:
        result = calculate_child(db, sys.argv[1], sys.argv[2])
        if result:
            print(f"Breeding: {result['parent1']['name']} + {result['parent2']['name']}")
            print(f"Average Power: {result['avg_power']:.0f}")
            print(f"\nResult:")
            print_pal_info(result["child"])
            print(f"\nPower difference: {result['power_diff']:.0f}")
        return

    print(__doc__)


if __name__ == "__main__":
    main()
