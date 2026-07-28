#!/usr/bin/env python3
"""test_install.py — Vérifie que l'installation des mods Palworld est correcte.

Ce script :
  1. Détecte le dossier Palworld.
  2. Vérifie la présence de RE-UE4SS (UE4SS.dll).
  3. Vérifie la présence des dossiers de mods et de leurs fichiers clés.
  4. Vérifie la syntaxe Lua des fichiers copiés.
  5. Lit UE4SS.log pour détecter les erreurs de chargement des mods (s'il existe).
  6. Affiche un résumé OK/KO.
"""

import os
import platform
import re
import sys
from pathlib import Path

DEFAULT_WINDOWS_PALWORLD = r"C:\Program Files (x86)\Steam\steamapps\common\Palworld"
DEFAULT_WSL_PALWORLD = "/mnt/c/Program Files (x86)/Steam/steamapps/common/Palworld"


def parse_library_folders(vdf_path: Path) -> list[Path]:
    folders = []
    if not vdf_path.exists():
        return folders
    text = vdf_path.read_text(encoding="utf-8", errors="ignore")
    for m in re.finditer(r'"path"\s*"([^"]+)"', text):
        folders.append(Path(m.group(1).replace("\\\\", "\\").replace("//", "/")))
    return folders


def find_palworld() -> Path | None:
    env = os.environ.get("PALWORLD_DIR")
    if env:
        p = Path(env)
        if p.exists():
            return p

    system = platform.system()
    if system == "Windows":
        candidates = [Path(DEFAULT_WINDOWS_PALWORLD)]
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam") as key:
                steam_path, _ = winreg.QueryValueEx(key, "SteamPath")
                sp = Path(steam_path)
                candidates.append(sp / "steamapps" / "common" / "Palworld")
                for lib in parse_library_folders(sp / "steamapps" / "libraryfolders.vdf"):
                    candidates.append(lib / "common" / "Palworld")
        except (ImportError, OSError):
            pass
    else:
        candidates = [Path(DEFAULT_WSL_PALWORLD)]
        vdf = Path("/mnt/c/Program Files (x86)/Steam/steamapps/libraryfolders.vdf")
        if vdf.exists():
            for lib in parse_library_folders(vdf):
                candidates.append(lib / "common" / "Palworld")

    for c in candidates:
        if c.exists() and (c / "Pal" / "Binaries" / "Win64").exists():
            return c
    return None


def check_lua_syntax(file_path: Path) -> list[str]:
    """Vérifie rapidement la correspondance des crochets/accolades/parenthèses."""
    text = file_path.read_text(encoding="utf-8", errors="ignore")
    # Supprime les commentaires -- et --[[ ]]
    clean = re.sub(r"--\[\[.*?\]\]", "", text, flags=re.DOTALL)
    clean = re.sub(r"--[^\n]*", "", clean)
    in_string = False
    string_char = None
    escape = False
    stack = []
    errors = []
    for i, c in enumerate(clean):
        if in_string:
            if escape:
                escape = False
                continue
            if c == "\\":
                escape = True
                continue
            if c == string_char:
                in_string = False
                string_char = None
            continue
        if c in ('"', "'"):
            in_string = True
            string_char = c
            continue
        if c in "([{":
            stack.append(c)
        elif c in ")]}":
            if not stack:
                errors.append(f"{file_path}:{i}: fermeture sans ouverture '{c}'")
            else:
                o = stack.pop()
                if (o == "(" and c != ")") or (o == "[" and c != "]") or (o == "{" and c != "}"):
                    errors.append(f"{file_path}:{i}: mismatch '{o}' vs '{c}'")
    if stack:
        errors.append(f"{file_path}: ouvertures non fermees {stack[-5:]}")
    return errors


def main():
    pal = find_palworld()
    if not pal:
        print("[test] ERREUR : Impossible de trouver Palworld. Definissez PALWORLD_DIR.")
        sys.exit(1)

    print(f"[test] Palworld detecte : {pal}")

    bin_dir = pal / "Pal" / "Binaries" / "Win64"
    # Layout actuel : Win64/ue4ss/UE4SS.dll
    mods_dir = bin_dir / "ue4ss" / "Mods"
    checks = []

    # 1. UE4SS
    ue4ss_dll = bin_dir / "ue4ss" / "UE4SS.dll"
    # Fallback legacy
    legacy_dll = bin_dir / "UE4SS.dll"
    if not ue4ss_dll.exists() and legacy_dll.exists():
        ue4ss_dll = legacy_dll
        mods_dir = bin_dir / "Mods"
    if ue4ss_dll.exists():
        checks.append(("UE4SS.dll", True, str(ue4ss_dll)))
    else:
        checks.append(("UE4SS.dll", False, f"{ue4ss_dll} manquant"))

    # 2. Mods : detection automatique de tous les dossiers contenant Info.json + main.lua
    required_mods: dict[str, list[str]] = {}
    if mods_dir.exists():
        for mod_path in sorted(mods_dir.iterdir()):
            if not mod_path.is_dir():
                continue
            if (mod_path / "Info.json").exists() and (mod_path / "Scripts" / "main.lua").exists():
                files = ["Info.json", "Scripts/main.lua", "Scripts/config.lua"]
                if mod_path.name == "PalMiniMapPrototype":
                    files.append("Scripts/assets_data.lua")
                required_mods[mod_path.name] = files

    lua_errors = []
    for mod, files in required_mods.items():
        mod_path = mods_dir / mod
        if not mod_path.exists():
            checks.append((f"{mod} dossier", False, f"{mod_path} manquant"))
            continue
        checks.append((f"{mod} dossier", True, str(mod_path)))
        for rel in files:
            fp = mod_path / rel
            if fp.exists():
                checks.append((f"{mod}/{rel}", True, str(fp)))
                if rel.endswith(".lua"):
                    errs = check_lua_syntax(fp)
                    if errs:
                        lua_errors.extend(errs)
            else:
                checks.append((f"{mod}/{rel}", False, f"{fp} manquant"))

    # 3. Logs UE4SS
    log_file = bin_dir / "UE4SS.log"
    log_errors = []
    if log_file.exists():
        text = log_file.read_text(encoding="utf-8", errors="ignore")
        # Recherche d'erreurs Lua ou de chargement des mods
        for line in text.splitlines():
            low = line.lower()
            if any(k in low for k in ["error", "failed", "lua", "panicked", "fatal"]):
                if any(m in line for m in ["PalCheatMenu", "PalMiniMap", "assets_data"]):
                    log_errors.append(line.strip())

    # Résumé
    print("\n[test] === Résumé des vérifications ===")
    ok = 0
    ko = 0
    for name, status, detail in checks:
        symbol = "OK" if status else "KO"
        if status:
            ok += 1
        else:
            ko += 1
        print(f"[{symbol}] {name:40s} {detail}")

    if lua_errors:
        print("\n[test] === Erreurs de syntaxe Lua ===")
        for e in lua_errors[:20]:
            print(f"  ! {e}")

    if log_errors:
        print("\n[test] === Erreurs dans UE4SS.log liées aux mods ===")
        for e in log_errors[:20]:
            print(f"  ! {e}")

    print(f"\n[test] Resultat : {ok} OK, {ko} KO")

    if ko == 0 and not lua_errors:
        print("\n[test] Installation OK.")
        print("[test] Etapes manuelles : lancez Palworld.")
        print("[test] F1 : menu cheat | M : minimap | F10 : PalModManager")
        print("[test] Config hors jeu : python config_manager.py")
    else:
        print("\n[test] Installation incomplete ou invalide.")
        sys.exit(1)


if __name__ == "__main__":
    main()
