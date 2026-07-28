#!/usr/bin/env bash
# Packaging pour Palworld Mods ULTRA MAX
# Genere PalworldMods-vX.Y.Z.zip avec tous les mods, UE4SS bundle et PalTrainerUltra

set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
VERSION="${RELEASE_VERSION:-1.0.0}"
OUT_NAME="PalworldMods-v${VERSION}"
TMP_DIR="$ROOT/release/$OUT_NAME"
ZIP_PATH="$ROOT/release/${OUT_NAME}.zip"

echo "[Pack] Preparing release $OUT_NAME..."

rm -rf "$ROOT/release"
mkdir -p "$TMP_DIR"

# Mods Lua
cp -r "$ROOT/mods" "$TMP_DIR/mods"

# Docs
cp "$ROOT/README.md" "$TMP_DIR/README.md"
cp "$ROOT/NEXUS_PUBLISH.md" "$TMP_DIR/NEXUS_PUBLISH.md"
cp "$ROOT/INSTALL.md" "$TMP_DIR/INSTALL.md" 2>/dev/null || true

# UE4SS settings
cp "$ROOT/UE4SS-settings.ini" "$TMP_DIR/UE4SS-settings.ini"

# UE4SS bundle (experimental-latest) pour clic-and-play
cp -r "$ROOT/UE4SS" "$TMP_DIR/UE4SS"

# PalTrainerUltra (menu WeMod-like)
mkdir -p "$TMP_DIR/PalTrainerUltra"
cp -r "$ROOT/PalTrainerUltra/dist" "$TMP_DIR/PalTrainerUltra/dist"

# Installateurs automatiques
cp "$ROOT/install.py" "$TMP_DIR/install.py"
cp "$ROOT/install.bat" "$TMP_DIR/install.bat"
cp "$ROOT/install.sh" "$TMP_DIR/install.sh"
cp "$ROOT/test_install.py" "$TMP_DIR/test_install.py"

# Desinstallateurs
cp "$ROOT/uninstall.py" "$TMP_DIR/uninstall.py"
cp "$ROOT/uninstall.bat" "$TMP_DIR/uninstall.bat"
cp "$ROOT/uninstall.ps1" "$TMP_DIR/uninstall.ps1"
cp "$ROOT/uninstall.sh" "$TMP_DIR/uninstall.sh"

# Gestionnaire de configuration externe
cp "$ROOT/config_manager.py" "$TMP_DIR/config_manager.py" 2>/dev/null || true

chmod +x "$TMP_DIR/install.py" "$TMP_DIR/install.sh" "$TMP_DIR/uninstall.py" "$TMP_DIR/uninstall.sh" "$TMP_DIR/test_install.py" 2>/dev/null || true

# Zip
cd "$ROOT/release"
zip -r "${OUT_NAME}.zip" "$OUT_NAME"
rm -rf "$TMP_DIR"

echo "[Pack] Done: $ZIP_PATH"
