#!/usr/bin/env python3
"""uninstall.py — Désinstallateur Palworld Mods ULTRA MAX.

Supprime les mods PalCheatMenu et PalMiniMapPrototype du dossier Palworld.
Peut aussi supprimer UE4SS et ses fichiers si --remove-ue4ss est passé.
"""

import argparse
import os
import platform
import shutil
import sys
from pathlib import Path


UE4SS_BASE_MODS = {
    "CheatManagerEnablerMod",
    "ConsoleCommandsMod",
    "BPML_GenericFunctions",
    "ConsoleEnablerMod",
    "LineTraceMod",
    "Keybinds",
    "BPModLoaderMod",
    "SplitScreenMod",
    "shared",
}

DEFAULT_WINDOWS_PALWORLD = r"C:\Program Files (x86)\Steam\steamapps\common\Palworld"
DEFAULT_WSL_PALWORLD = "/mnt/c/Program Files (x86)/Steam/steamapps/common/Palworld"

UE4SS_FILES = [
    "UE4SS.dll",
    "UE4SS.pdb",
    "UE4SS-console.dll",
    "UE4SS-console.pdb",
    "UE4SS-settings.ini",
    "UE4SS.log",
    "UE4SS-errors.log",
    "dwmapi.dll",
    "xinput1_3.dll",
    "cpp2il_out",
]


def log(msg: str):
    print(f"[uninstall] {msg}")


def find_palworld_env() -> Path | None:
    p = os.environ.get("PALWORLD_DIR")
    if p:
        c = Path(p)
        if c.exists():
            return c
    return None


def find_palworld_windows() -> Path | None:
    candidates = [Path(DEFAULT_WINDOWS_PALWORLD)]
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam") as key:
            steam_path, _ = winreg.QueryValueEx(key, "SteamPath")
            candidates.append(Path(steam_path) / "steamapps" / "common" / "Palworld")
    except Exception:
        pass
    for c in candidates:
        if c.exists() and (c / "Pal" / "Binaries" / "Win64").exists():
            return c
    return None


def find_palworld_wsl() -> Path | None:
    c = Path(DEFAULT_WSL_PALWORLD)
    if c.exists() and (c / "Pal" / "Binaries" / "Win64").exists():
        return c
    return None


def find_palworld() -> Path:
    pal = find_palworld_env()
    if pal:
        return pal
    if platform.system() == "Windows":
        pal = find_palworld_windows()
    else:
        pal = find_palworld_wsl()
    if pal:
        return pal
    print("[uninstall] Impossible de trouver Palworld. Definissez PALWORLD_DIR ou utilisez --palworld-dir.")
    sys.exit(1)


def find_mods_to_remove(mods_dir: Path) -> list[Path]:
    """Detecte tous les mods Pal* crees par ce projet, en ignorant les mods UE4SS de base."""
    if not mods_dir.exists():
        return []
    result = []
    for item in mods_dir.iterdir():
        if not item.is_dir():
            continue
        if item.name in UE4SS_BASE_MODS:
            continue
        if item.name.startswith("Pal") and (item / "Info.json").exists():
            result.append(item)
    return result


def main():
    parser = argparse.ArgumentParser(description="Desinstallateur Palworld Mods ULTRA MAX")
    parser.add_argument("--palworld-dir", type=Path, help="Chemin du dossier Palworld")
    parser.add_argument("--remove-ue4ss", action="store_true", help="Supprimer aussi UE4SS et ses dependances")
    parser.add_argument("--dry-run", action="store_true", help="Afficher ce qui serait supprime sans rien effacer")
    args = parser.parse_args()

    palworld_dir = args.palworld_dir or find_palworld()
    log(f"Dossier Palworld : {palworld_dir}")

    if not palworld_dir.exists():
        log("ERREUR : Le dossier Palworld n'existe pas.")
        sys.exit(1)

    binaries_dir = palworld_dir / "Pal" / "Binaries" / "Win64"
    new_dir = binaries_dir / "ue4ss"
    if (new_dir / "UE4SS.dll").exists() or (new_dir / "Mods").exists():
        ue4ss_dir = new_dir
    else:
        ue4ss_dir = binaries_dir
    mods_dir = ue4ss_dir / "Mods"

    mods_to_remove = find_mods_to_remove(mods_dir)
    removed_any = bool(mods_to_remove) or args.remove_ue4ss

    if args.dry_run:
        log("[dry-run] Mode simulation active.")

    for mod_path in mods_to_remove:
        if args.dry_run:
            log(f"[dry-run] Supprimerait {mod_path}")
        else:
            log(f"Suppression de {mod_path} ...")
            shutil.rmtree(mod_path)

    # Supprimer aussi les dossiers du trainer (ancien + actuel)
    for trainer_name in ("PalTrainerApp", "PalTrainerUltra"):
        trainer_dir = palworld_dir / trainer_name
        if trainer_dir.exists():
            removed_any = True
            if args.dry_run:
                log(f"[dry-run] Supprimerait {trainer_dir}")
            else:
                log(f"Suppression de {trainer_dir} ...")
                shutil.rmtree(trainer_dir)
                log(f"Supprime : {trainer_dir}")

    if args.remove_ue4ss:
        log("Suppression de UE4SS ...")
        for name in UE4SS_FILES:
            target = binaries_dir / name
            if target.exists():
                if args.dry_run:
                    log(f"[dry-run] Supprimerait {target}")
                else:
                    if target.is_dir():
                        shutil.rmtree(target)
                    else:
                        target.unlink()
                    log(f"Supprime : {target}")
        # Supprimer aussi le dossier Mods s'il est vide
        if mods_dir.exists() and not any(mods_dir.iterdir()):
            if args.dry_run:
                log(f"[dry-run] Supprimerait {mods_dir} (vide)")
            else:
                mods_dir.rmdir()
                log(f"Supprime : {mods_dir}")

    if removed_any or args.remove_ue4ss:
        log("Desinstallation terminee." if not args.dry_run else "[dry-run] Simulation terminee.")
    else:
        log("Rien a desinstaller.")


if __name__ == "__main__":
    main()
