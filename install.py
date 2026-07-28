#!/usr/bin/env python3
"""Installe PalCheatMenu, PalMiniMapPrototype et RE-UE4SS pour Palworld."""

import argparse
import datetime
import os
import re
import shutil
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile
import json
from pathlib import Path
from urllib.request import Request, urlopen
from urllib.error import URLError


UE4SS_RELEASE_TAGS = [
    "https://api.github.com/repos/UE4SS-RE/RE-UE4SS/releases/tags/experimental-latest",
    "https://api.github.com/repos/UE4SS-RE/RE-UE4SS/releases/tags/experimental",
    "https://api.github.com/repos/UE4SS-RE/RE-UE4SS/releases/latest",
]



# Palworld-compatible UE4SS version to prefer (Git SHA c838a8ac)
PALWORLD_UE4SS_VERSION = "v3.0.1-1012-gc838a8ac"

# MemberVariableLayout.ini override for Palworld 0.4.15+ UEnum crash fix
MEMBER_LAYOUT_INI = """[UEnum]
; Total Size: 0x70
; FString                             Size: 0x0010
CppType = 0x30
; TArray<TTuple<FName,__int64>,TSizedDefaultAllocator<32> > Size: 0x0010
Names = 0x48
; UEnum::ECppForm                     Size: 0x0004
CppForm = 0x58
; EEnumFlags                          Size: 0x0004
EnumFlags_Internal = 0x5C
; std::function<void(int32)>*         Size: 0x0008
EnumDisplayNameFn = 0x60
; FName                               Size: 0x0008
EnumPackage = 0x68
UEP_TotalSize = 0x70
"""

class Tee:
    """Redirige stdout/stderr vers la console ET vers un fichier de log."""
    def __init__(self, file, stream):
        self.file = file
        self.stream = stream

    def write(self, data):
        self.stream.write(data)
        self.file.write(data)
        self.file.flush()

    def flush(self):
        self.stream.flush()
        self.file.flush()

    def isatty(self):
        return self.stream.isatty()


def setup_logging(project_dir: Path | None = None):
    """Cree install-report.txt a cote du script et duplique stdout/stderr dedans."""
    if project_dir is None:
        project_dir = Path(__file__).parent
    report = project_dir / "install-report.txt"
    report.write_text(
        f"Palworld Mods ULTRA MAX - Rapport d'installation\nDate: {datetime.datetime.now()}\n\n",
        encoding="utf-8"
    )
    handle = open(report, "a", encoding="utf-8")
    sys.stdout = Tee(handle, sys.__stdout__)
    sys.stderr = Tee(handle, sys.__stderr__)


def log(msg: str):
    print(f"[install] {msg}")


def error(msg: str):
    print(f"[ERREUR] {msg}", file=sys.stderr)
    sys.exit(1)


def is_program_files(path: Path) -> bool:
    """Detecte si le dossier cible est dans Program Files (protégé)."""
    try:
        return "program files" in str(path.resolve()).lower()
    except Exception:
        return False


def parse_library_folders(vdf_path: Path) -> list[Path]:
    folders = []
    if not vdf_path.exists():
        return folders
    text = vdf_path.read_text(encoding="utf-8", errors="ignore")
    for m in re.finditer(r'"path"\s*"([^"]+)"', text):
        folders.append(Path(m.group(1).replace("\\\\", "\\").replace("//", "/")))
    return folders


def _common_palworld_paths() -> list[Path]:
    """Chemins Steam courants sur tous les disques disponibles."""
    candidates: list[Path] = []
    system = os.name
    if system == "nt":
        import string
        for letter in string.ascii_uppercase:
            d = Path(f"{letter}:/")
            if not d.exists():
                continue
            candidates.extend([
                d / "Steam" / "steamapps" / "common" / "Palworld",
                d / "SteamLibrary" / "steamapps" / "common" / "Palworld",
                d / "Program Files (x86)" / "Steam" / "steamapps" / "common" / "Palworld",
                d / "Program Files" / "Steam" / "steamapps" / "common" / "Palworld",
                d / "Jeux" / "Steam" / "steamapps" / "common" / "Palworld",
                d / "Games" / "Steam" / "steamapps" / "common" / "Palworld",
            ])
            candidates.extend([
                d / "Steam" / "steamapps" / "libraryfolders.vdf",
                d / "SteamLibrary" / "steamapps" / "libraryfolders.vdf",
                d / "Program Files (x86)" / "Steam" / "steamapps" / "libraryfolders.vdf",
            ])
    else:
        candidates.append(Path("/mnt/c/Program Files (x86)/Steam/steamapps/common/Palworld"))
        candidates.append(Path("/mnt/c/Program Files/Steam/steamapps/common/Palworld"))
        candidates.append(Path("/mnt/d/Steam/steamapps/common/Palworld"))
        candidates.append(Path("/mnt/d/SteamLibrary/steamapps/common/Palworld"))
    return candidates


def _is_valid_palworld(p: Path) -> bool:
    return p.exists() and (p / "Pal" / "Binaries" / "Win64").exists()


def _search_palworld_exe() -> Path | None:
    """Recherche rapide de Palworld.exe sur tous les disques, profondeur limitee."""
    if os.name != "nt":
        return None
    import string
    exe_names = ["Palworld.exe", "Palworld-Win64-Shipping.exe"]
    for letter in string.ascii_uppercase:
        drive = Path(f"{letter}:/")
        if not drive.exists():
            continue
        for base in [drive / "Steam", drive / "SteamLibrary", drive / "Jeux", drive / "Games", drive / "Program Files (x86)" / "Steam"]:
            if not base.exists():
                continue
            try:
                for exe in exe_names:
                    found = _find_in_depth(base, exe, max_depth=8)
                    if found:
                        # Remonter de 4 niveaux depuis Pal/Binaries/Win64/exe
                        p = found.parent
                        for _ in range(4):
                            p = p.parent
                        if _is_valid_palworld(p):
                            return p
            except (PermissionError, OSError):
                pass
    return None


def _find_in_depth(start: Path, name: str, max_depth: int) -> Path | None:
    """Recherche un fichier par nom avec profondeur max."""
    for root, dirs, files in os.walk(start):
        depth = root.count(os.sep) - str(start).count(os.sep)
        if depth > max_depth:
            del dirs[:]
            continue
        if name in files:
            return Path(root) / name
    return None


def find_palworld() -> Path:
    """Tente de localiser automatiquement le dossier Palworld, avec prompt si echec."""
    env = os.environ.get("PALWORLD_DIR")
    if env:
        p = Path(env.strip().strip('"'))
        if _is_valid_palworld(p):
            log(f"Palworld trouve via PALWORLD_DIR : {p}")
            return p
        log(f"PALWORLD_DIR fourni mais invalide : {p}")

    candidates: list[Path] = []

    if os.name == "nt":
        # 1. Registry Steam
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam") as key:
                steam_path, _ = winreg.QueryValueEx(key, "SteamPath")
            log(f"SteamPath trouve : {steam_path}")
            sp = Path(steam_path.replace("//", "/").replace("/", os.sep))
            candidates.append(sp / "steamapps" / "common" / "Palworld")
            for lib in parse_library_folders(sp / "steamapps" / "libraryfolders.vdf"):
                candidates.append(lib / "common" / "Palworld")
        except (ImportError, OSError) as e:
            log(f"Impossible de lire la registry Steam : {e}")

    # 2. Chemins communs et VDF supplementaires
    candidates.extend(_common_palworld_paths())

    # 3. VDF supplementaires decouverts dans les chemins communs
    for c in list(candidates):
        if c.suffix == ".vdf":
            for lib in parse_library_folders(c):
                candidates.append(lib / "common" / "Palworld")

    # 4. Verification
    for c in candidates:
        if c.suffix == ".vdf":
            continue
        if _is_valid_palworld(c):
            log(f"Palworld trouve : {c}")
            return c

    # 5. Recherche par exe
    found = _search_palworld_exe()
    if found:
        log(f"Palworld trouve via Palworld.exe : {found}")
        return found

    # 6. Prompt utilisateur
    log("Palworld non trouve automatiquement.")
    try:
        raw = input(r"Entrez le chemin complet de Palworld (ex: E:\Steam\steamapps\common\Palworld) : ")
    except EOFError:
        raw = ""
    p = Path(raw.strip().strip('"'))
    if _is_valid_palworld(p):
        log(f"Palworld utilise depuis la saisie : {p}")
        return p

    error("Chemin fourni invalide. Passez --palworld-dir ou definissez PALWORLD_DIR.")


def get_ue4ss_dirs(binaries_dir: Path):
    """Detecte le layout UE4SS : nouveau Win64/ue4ss/ ou legacy Win64/."""
    new_dir = binaries_dir / "ue4ss"
    legacy_dll = binaries_dir / "UE4SS.dll"
    if (new_dir / "UE4SS.dll").exists() or (new_dir / "Mods").exists():
        return new_dir, new_dir / "Mods"
    if legacy_dll.exists():
        return binaries_dir, binaries_dir / "Mods"
    # Nouvelle installation : forcer le layout ue4ss/
    return new_dir, new_dir / "Mods"


def download(url: str, dest: Path, timeout: int = 120):
    """Telecharge un fichier avec une barre de progression minimaliste."""
    dest.parent.mkdir(parents=True, exist_ok=True)
    req = Request(url, headers={"User-Agent": "PalworldMods-Installer/1.0"})
    with urlopen(req, timeout=timeout) as resp:
        total = int(resp.headers.get("Content-Length", 1))
        with dest.open("wb") as f:
            chunk_size = 256 * 1024
            downloaded = 0
            while True:
                chunk = resp.read(chunk_size)
                if not chunk:
                    break
                f.write(chunk)
                downloaded += len(chunk)
                if total > 1:
                    pct = downloaded * 100 // total
                    print(f"\rTelechargement: {pct}%", end="", flush=True)
    print()


def _ue4ss_sort_key(name: str):
    """Cles de tri pour choisir le zip UE4SS le plus recent."""
    # Exemples attendus :
    #   UE4SS_v3.0.1-1012-gc838a8ac.zip  (build experimental)
    #   UE4SS_v3.0.1.zip                  (release stable, pas de build)
    m = re.search(r"UE4SS_v([\d.]+)-(\d+)-g[0-9a-f]{7,}\.zip$", name)
    if m:
        version = tuple(int(x) for x in m.group(1).split(".") if x.isdigit())
        build = int(m.group(2))
        return (version, build, True)
    m = re.search(r"UE4SS_v([\d.]+)\.zip$", name)
    if m:
        version = tuple(int(x) for x in m.group(1).split(".") if x.isdigit())
        return (version, -1, False)
    return (0, -1, False)


def _candidate_assets(data):
    """Filtre les assets d'une release UE4SS."""
    for asset in data.get("assets", []):
        name = asset.get("name", "")
        url = asset.get("browser_download_url", "")
        if not name.endswith(".zip"):
            continue
        if any(k in name.lower() for k in ("symbols", "dev", "customgame", "mapgen", "xinput")):
            continue
        if not re.match(r"UE4SS[-_]v", name):
            continue
        if url:
            yield name, url


def resolve_ue4ss_download_info() -> tuple[str | None, str | None]:
    """Interroge GitHub et retourne le zip UE4SS standard le plus recent."""
    best = None
    best_name = None
    best_url = None
    for tag_url in UE4SS_RELEASE_TAGS:
        try:
            req = Request(tag_url, headers={"User-Agent": "PalworldMods-Installer/1.0"})
            with urlopen(req, timeout=30) as resp:
                data = json.loads(resp.read().decode("utf-8"))
            for name, url in _candidate_assets(data):
                key = _ue4ss_sort_key(name)
                if best is None or key > best:
                    best = key
                    best_name, best_url = name, url
        except Exception as e:
            log(f"API GitHub indisponible ({tag_url}) : {e}")
    return (best_name, best_url)


def _install_ue4ss_from_bundle(bundle_dir: Path, binaries_dir: Path, ue4ss_dll: Path):
    """Copie le bundle local dans le dossier Win64."""
    log("UE4SS bundle local detecte, copie en cours ...")
    binaries_dir.mkdir(parents=True, exist_ok=True)
    for item in bundle_dir.iterdir():
        if item.name in (".version", "__MACOSX", ".DS_Store"):
            continue
        dest = binaries_dir / item.name
        if dest.exists():
            if dest.is_dir():
                shutil.rmtree(dest)
            else:
                dest.unlink()
        if item.is_dir():
            shutil.copytree(item, dest)
        else:
            shutil.copy2(item, dest)
    if not ue4ss_dll.exists():
        found = list(binaries_dir.rglob("UE4SS.dll"))
        if found:
            error(f"UE4SS.dll est dans un sous-dossier inattendu : {found[0]}")
        else:
            error(f"UE4SS.dll introuvable apres copie du bundle. Contenu de {binaries_dir}: {list(binaries_dir.iterdir())}")
    log("RE-UE4SS installe depuis le bundle local.")


def _install_ue4ss_from_download(ue4ss_url: str, binaries_dir: Path, ue4ss_dll: Path):
    """Telecharge et installe RE-UE4SS depuis GitHub."""
    log(f"URL retenue : {ue4ss_url}")
    with tempfile.TemporaryDirectory() as tmp:
        zip_path = Path(tmp) / "UE4SS.zip"
        try:
            download(ue4ss_url, zip_path)
        except URLError as e:
            raise RuntimeError(f"Impossible de telecharger UE4SS : {e}\nURL : {ue4ss_url}\nTelechargez-le manuellement.")
        extract_tmp = Path(tmp) / "extracted"
        extract_tmp.mkdir()
        with zipfile.ZipFile(zip_path, "r") as z:
            z.extractall(extract_tmp)
        ue4ss_dll_files = list(extract_tmp.rglob("UE4SS.dll"))
        if not ue4ss_dll_files:
            raise RuntimeError("UE4SS.dll introuvable dans le zip telecharge.")
        extracted_root = ue4ss_dll_files[0].parent
        ue4ss_dir = ue4ss_dll.parent
        ue4ss_dir.mkdir(parents=True, exist_ok=True)
        for item in extracted_root.iterdir():
            if item.name in ("__MACOSX", ".DS_Store"):
                continue
            dest = ue4ss_dir / item.name
            if dest.exists():
                if dest.is_dir():
                    shutil.rmtree(dest)
                else:
                    dest.unlink()
            shutil.move(str(item), str(dest))
        for proxy_name in ["dwmapi.dll", "xinput1_3.dll"]:
            proxies = list(extract_tmp.rglob(proxy_name))
            if proxies:
                proxy_dest = binaries_dir / proxy_name
                if not proxy_dest.exists():
                    shutil.copy2(str(proxies[0]), str(proxy_dest))
                    log(f"Proxy DLL copie : {proxy_name}")


def apply_palworld_member_layout(ue4ss_dir: Path, force: bool = False, dry: bool = False):
    """Injecte le MemberVariableLayout.ini Palworld 0.4.15+ si absent."""
    layout_path = ue4ss_dir / "MemberVariableLayout.ini"
    if layout_path.exists() and not force:
        return
    if dry:
        log(f"[dry-run] MemberVariableLayout.ini serait ecrit dans {{layout_path}}")
        return
    ue4ss_dir.mkdir(parents=True, exist_ok=True)
    layout_path.write_text(MEMBER_LAYOUT_INI, encoding="utf-8")
    log(f"MemberVariableLayout.ini Palworld ecrit dans {{layout_path}}")


def install_ue4ss(palworld_dir: Path, skip: bool = False, dry: bool = False, offline: bool = False, force: bool = False):
    """Installe RE-UE4SS : telecharge la derniere release par defaut, bundle local en fallback."""
    binaries_dir = palworld_dir / "Pal" / "Binaries" / "Win64"
    ue4ss_dir, _ = get_ue4ss_dirs(binaries_dir)
    ue4ss_dll = ue4ss_dir / "UE4SS.dll"

    if skip:
        log("RE-UE4SS ignore (--skip-ue4ss).")
        return

    # Nettoyer l'ancien proxy xinput1_3.dll qui fait crasher avec UE4SS 3.x
    old_xinput = binaries_dir / "xinput1_3.dll"
    if old_xinput.exists():
        if dry:
            log(f"[dry-run] {old_xinput.name} serait renomme en .bak")
        else:
            old_xinput.replace(old_xinput.with_suffix(old_xinput.suffix + ".bak"))
            log(f"{old_xinput.name} renomme (ancien loader incompatible).")

    if ue4ss_dll.exists() and not force:
        msg = "RE-UE4SS est deja installe. Utilisez --force-ue4ss pour forcer la reinstallation."
        if dry:
            log(f"[dry-run] {msg}")
        else:
            log(msg)
        apply_palworld_member_layout(ue4ss_dir, force=False, dry=dry)
        return

    if dry:
        log("[dry-run] RE-UE4SS serait telecharge/installe.")
        return

    source_dir = Path(__file__).parent
    bundle_dir = source_dir / "UE4SS"
    version_file = bundle_dir / ".version"

    # Preferer le bundle local s'il correspond a la version voulue (evite une mauvaise build GitHub)
    if not force and not offline and bundle_dir.exists() and version_file.exists():
        local_version = version_file.read_text(encoding="utf-8").strip()
        if PALWORLD_UE4SS_VERSION in local_version:
            log(f"Bundle local UE4SS correspondant detecte ({local_version}). Utilisation du bundle.")
            if dry:
                log("[dry-run] RE-UE4SS serait installe depuis le bundle local.")
            else:
                _install_ue4ss_from_bundle(bundle_dir, binaries_dir, ue4ss_dll)
                apply_palworld_member_layout(ue4ss_dir, force=True, dry=dry)
                log("RE-UE4SS installe depuis le bundle local.")
            return

    latest_name, latest_url = resolve_ue4ss_download_info()

    if not offline and latest_url:
        try:
            log(f"Telechargement de la derniere build UE4SS : {latest_name} ...")
            _install_ue4ss_from_download(latest_url, binaries_dir, ue4ss_dll)
            log(f"RE-UE4SS installe depuis le telechargement ({latest_name}).")
            apply_palworld_member_layout(ue4ss_dir, force=True, dry=dry)
            return
        except Exception as e:
            log(f"Telechargement echoue : {e}")
            if not bundle_dir.exists():
                error("Telechargement impossible et aucun bundle local.")
    else:
        log("Mode offline active ou aucune URL distante trouvee.")
        if not bundle_dir.exists():
            error("Mode offline sans bundle local.")

    if bundle_dir.exists():
        log("Utilisation du bundle local en fallback ...")
        _install_ue4ss_from_bundle(bundle_dir, binaries_dir, ue4ss_dll)

    if not ue4ss_dll.exists():
        error(f"L'installation d'UE4SS n'a pas produit {ue4ss_dll}.")
    apply_palworld_member_layout(ue4ss_dir, force=True, dry=dry)
    log("RE-UE4SS installe.")


def _timestamp() -> str:
    return datetime.datetime.now().strftime("%Y%m%d-%H%M%S")


def install_ue4ss_settings(source_dir: Path, palworld_dir: Path, dry: bool = False):
    """Ecrit UE4SS-settings.ini de maniere propre, sans BOM, sans doublons."""
    binaries_dir = palworld_dir / "Pal" / "Binaries" / "Win64"
    ue4ss_dir, _ = get_ue4ss_dirs(binaries_dir)
    settings_src = source_dir / "UE4SS-settings.ini"
    settings_dest = ue4ss_dir / "UE4SS-settings.ini"

    if settings_dest.exists():
        backup = settings_dest.with_suffix(settings_dest.suffix + f".bak.{_timestamp()}")
        if dry:
            log(f"[dry-run] Sauvegarde de {settings_dest} vers {backup}")
        else:
            shutil.copy2(settings_dest, backup)
            log(f"Sauvegarde de {settings_dest} vers {backup}")

    if dry:
        log(f"[dry-run] Ecriture de {settings_dest}")
        return

    settings_dest.parent.mkdir(parents=True, exist_ok=True)

    if settings_src.exists():
        text = settings_src.read_text(encoding="utf-8", errors="ignore")
    else:
        text = """[Debug]
ConsoleEnabled = 1
GuiConsoleEnabled = 0
GuiConsoleVisible = 0
GraphicsAPI = dx11
RenderMode = GameViewportClientTick
DebugGUIFontScaling = 1.0

[General]
UseCache = 1
InvalidateCacheIfDLLDiffers = 1
bUseUObjectArrayCache = false

[Hooks]
HookEngineTick = 1
HookGameViewportClientTick = 1
"""

    text = text.lstrip("\ufeff")
    settings_dest.write_text(text, encoding="utf-8")
    log("UE4SS-settings.ini ecrit.")

def _parse_lua_table(text: str) -> dict[str, str]:
    """Extrait les paires cle = valeur (brutes) d'un simple `return { ... }`."""
    match = re.search(r'return\s*\{(.*)\}', text, re.DOTALL)
    if not match:
        return {}
    body = match.group(1)
    pairs: dict[str, str] = {}
    for line in body.splitlines():
        line = line.split('--')[0].strip()
        if not line or line == ',':
            continue
        m = re.match(r'^(\w+)\s*=\s*(.+?)\s*,?\s*$', line)
        if m:
            pairs[m.group(1)] = m.group(2).strip()
    return pairs


def _merge_lua_configs(fresh_text: str, backup_values: dict[str, str]) -> str:
    """Gardre la structure de fresh_text mais remplace les cles presentes dans backup_values."""
    merged = fresh_text
    for key, val in backup_values.items():
        escaped = re.escape(key)
        pattern = re.compile(rf'^(\s*{escaped}\s*=\s*)(.+?)(\s*)(,?)(\s*)$', re.MULTILINE)

        def repl(m, v=val):
            return f"{m.group(1)}{v}{m.group(3)}{m.group(4)}{m.group(5)}"

        merged = pattern.sub(repl, merged)
    return merged


def ensure_mod_enabled_in_mods_txt(mods_dir: Path, mod_name: str, dry: bool = False):
    """Ajoute ou active un mod dans mods.txt d'UE4SS."""
    mods_txt = mods_dir / "mods.txt"
    if not mods_txt.exists():
        if dry:
            log(f"[dry-run] Creation de {mods_txt}")
            return
        mods_txt.write_text("", encoding="utf-8-sig")
    try:
        text = mods_txt.read_text(encoding="utf-8-sig", errors="ignore")
        lines = text.splitlines()
        pattern = re.compile(rf"^\s*{re.escape(mod_name)}\s*:\s*(\d)", re.IGNORECASE)
        found = False
        for i, line in enumerate(lines):
            if pattern.match(line):
                lines[i] = f"{mod_name} : 1"
                found = True
                break
        if not found:
            lines.append(f"{mod_name} : 1")
        if dry:
            log(f"[dry-run] {mod_name} serait active dans mods.txt")
        else:
            mods_txt.write_text("\n".join(lines) + "\n", encoding="utf-8-sig")
            log(f"{mod_name} activé dans mods.txt")
    except Exception as e:
        log(f"Impossible de mettre a jour mods.txt pour {mod_name} : {e}")


def install_mod(source_dir: Path, mod_rel: str, mods_dir: Path, mod_name: str | None = None, dry: bool = False):
    """Copie un mod depuis le zip source vers le dossier Mods cible."""
    src = source_dir / mod_rel
    if not src.exists():
        log(f"Source non trouvée : {src}")
        return
    name = mod_name or src.name
    dest = mods_dir / name

    user_config: str | None = None
    if dest.exists():
        cfg_path = dest / "Scripts" / "config.lua"
        if cfg_path.exists():
            user_config = cfg_path.read_text(encoding="utf-8-sig", errors="ignore")
        if dry:
            log(f"[dry-run] Suppression de {dest}")
        else:
            shutil.rmtree(dest)

    if dry:
        log(f"[dry-run] Copie de {src} vers {dest}")
    else:
        shutil.copytree(src, dest)
        enabled = dest / "enabled.txt"
        enabled.write_text("", encoding="utf-8-sig")

        # Sauvegarde de l'ancienne config et fusion avec la nouvelle
        fresh_cfg = dest / "Scripts" / "config.lua"
        if fresh_cfg.exists() and user_config:
            backup_cfg = fresh_cfg.with_name("config.lua.user.bak")
            backup_cfg.write_text(user_config, encoding="utf-8-sig")
            backup_values = _parse_lua_table(user_config)
            if backup_values:
                fresh_text = fresh_cfg.read_text(encoding="utf-8-sig", errors="ignore")
                merged = _merge_lua_configs(fresh_text, backup_values)
                fresh_cfg.write_text(merged, encoding="utf-8-sig")

    ensure_mod_enabled_in_mods_txt(mods_dir, name, dry=dry)
    if dry:
        log(f"[dry-run] {name} serait installe.")
    else:
        log(f"{name} installé.")


def find_mod_source_dirs(source_dir: Path) -> dict[str, str]:
    """Detecte automatiquement tous les mods Lua valides dans le dossier source/mods."""
    mod_map: dict[str, str] = {}
    mods_src = source_dir / "mods"

    # Special : PalMiniMap a sa source dans mods/PalMiniMap/Prototype
    prototype_src = mods_src / "PalMiniMap" / "Prototype"
    if (prototype_src / "Info.json").exists():
        mod_map["PalMiniMapPrototype"] = str(prototype_src.relative_to(source_dir))
    elif (mods_src / "PalMiniMapPrototype" / "Info.json").exists():
        mod_map["PalMiniMapPrototype"] = "mods/PalMiniMapPrototype"

    if not mods_src.exists():
        return mod_map

    # Tous les dossiers Pal* contenant Info.json + main.lua
    for d in sorted(mods_src.iterdir()):
        if not d.is_dir():
            continue
        if d.name == "PalMiniMap":
            continue  # deja gere
        if (d / "Info.json").exists() and (d / "Scripts" / "main.lua").exists():
            mod_map[d.name] = f"mods/{d.name}"

    return mod_map


def install_mods(source_dir: Path, palworld_dir: Path, dry: bool = False):
    """Copie tous les mods Lua detectes dans le dossier Mods."""
    binaries_dir = palworld_dir / "Pal" / "Binaries" / "Win64"
    _, mods_dir = get_ue4ss_dirs(binaries_dir)
    if dry:
        log(f"[dry-run] Creation de {mods_dir} si besoin")
    else:
        mods_dir.mkdir(parents=True, exist_ok=True)

    mod_sources = find_mod_source_dirs(source_dir)
    if not mod_sources:
        log("Aucun mod trouve dans le dossier source.")
        return

    for mod_name, mod_rel in sorted(mod_sources.items()):
        install_mod(source_dir, mod_rel, mods_dir, mod_name, dry=dry)




def install_trainer(source_dir: Path, palworld_dir: Path, dry: bool = False):
    """Copie PalTrainerUltra/dist dans le dossier Palworld."""
    app_src = source_dir / "PalTrainerUltra" / "dist"
    app_dest = palworld_dir / "PalTrainerUltra"
    if app_src.exists():
        if dry:
            log(f"[dry-run] Copie de {app_src} vers {app_dest}")
        else:
            if app_dest.exists():
                shutil.rmtree(app_dest)
            shutil.copytree(app_src, app_dest)
            log(f"PalTrainerUltra copie vers {app_dest}")
    else:
        log("PalTrainerUltra/dist non trouve dans le dossier source.")



def preflight_check(palworld_dir: Path, dry: bool = False) -> bool:
    """Verifications avant installation."""
    if palworld_dir.name.lower() == "saved":
        error("Refuse de modifier le dossier Saved de Palworld (sauvegardes).")
    bin_dir = palworld_dir / "Pal" / "Binaries" / "Win64"
    if not bin_dir.exists():
        error(f"Dossier Palworld invalide : {palworld_dir}")
    if not os.access(bin_dir, os.W_OK):
        if dry:
            log(f"[dry-run] Dossier {bin_dir} semble non writable (verifiez les droits admin).")
        else:
            error(f"Le dossier {bin_dir} n'est pas accessible en ecriture. Lancez en administrateur.")
    if is_program_files(palworld_dir):
        log("ATTENTION : dossier Program Files detecte. Lancez l'installateur en administrateur.")
    return True


def main():
    setup_logging()
    parser = argparse.ArgumentParser(description="Installe les mods Palworld")
    parser.add_argument("--palworld-dir", type=Path, help="Chemin du dossier Palworld")
    parser.add_argument("--skip-ue4ss", action="store_true", help="Ne pas (re)installer UE4SS")
    parser.add_argument("--offline", action="store_true", help="Utiliser le bundle local UE4SS au lieu de telecharger")
    parser.add_argument("--force-ue4ss", action="store_true", help="Forcer la reinstallation/mise a jour d'UE4SS")
    parser.add_argument("--dry-run", action="store_true", help="Simuler l'installation sans ecrire sur le disque")
    args = parser.parse_args()

    if args.palworld_dir:
        palworld_dir = args.palworld_dir
    else:
        palworld_dir = find_palworld()

    preflight_check(palworld_dir, dry=args.dry_run)

    source_dir = Path(__file__).parent
    install_ue4ss(palworld_dir, skip=args.skip_ue4ss, dry=args.dry_run, offline=args.offline, force=args.force_ue4ss)
    install_ue4ss_settings(source_dir, palworld_dir, dry=args.dry_run)
    install_mods(source_dir, palworld_dir, dry=args.dry_run)
    install_trainer(source_dir, palworld_dir, dry=args.dry_run)

    if args.dry_run:
        log("[dry-run] Simulation terminee. Aucun fichier n'a ete modifie.")
    else:
        log("Installation terminee. Lancez Palworld.")
    log("Lancez PalTrainerUltra/PalTrainerUltra.exe en administrateur pour le menu WeMod.")
    log("Raccourcis clavier : INS overlay | F1 cheat menu | M minimap | F10 mod manager")


if __name__ == "__main__":
    main()
