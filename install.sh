#!/bin/bash
# install.sh — Installateur Palworld Mods ULTRA MAX (WSL/Linux)
# Usage : ./install.sh [PALWORLD_DIR] [--dry-run]

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEFAULT_PALWORLD_WSL="/mnt/c/Program Files (x86)/Steam/steamapps/common/Palworld"
PALWORLD_DIR="${PALWORLD_DIR:-$DEFAULT_PALWORLD_WSL}"
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case $1 in
    --dry-run) DRY_RUN=1 ; shift ;;
    --palworld-dir) PALWORLD_DIR="$2" ; shift 2 ;;
    *) if [ -z "$PALWORLD_DIR" ] || [ "$PALWORLD_DIR" = "$DEFAULT_PALWORLD_WSL" ]; then PALWORLD_DIR="$1"; fi ; shift ;;
  esac
done

echo "============================================"
echo " Installateur Palworld Mods ULTRA MAX"
echo " Dossier : $PROJECT_DIR"
echo " Jeu     : $PALWORLD_DIR"
echo "============================================"

if [ ! -d "$PALWORLD_DIR" ]; then
    echo "ERREUR : Le dossier Palworld n'a pas ete trouve : $PALWORLD_DIR"
    echo "Definissez PALWORLD_DIR ou passez le chemin en argument."
    exit 1
fi

# Essayer install.py si Python est disponible
PY=""
if command -v python3 >/dev/null 2>&1; then
    PY="python3"
elif command -v python >/dev/null 2>&1; then
    PY="python"
fi

if [ -n "$PY" ]; then
    echo "Python detecte ($PY), lancement de install.py ..."
    PY_ARGS=("--palworld-dir" "$PALWORLD_DIR")
    if [ "$DRY_RUN" -eq 1 ]; then
        PY_ARGS+=("--dry-run")
        echo "[dry-run] Simulation, aucun fichier ne sera modifie."
    fi
    if "$PY" "$PROJECT_DIR/install.py" "${PY_ARGS[@]}"; then
        echo "Installation terminee."
        exit 0
    fi
    echo "install.py a echoue, utilisation du mode autonome."
fi

echo "Mode autonome : installation depuis le bundle local ..."

BIN_DIR="$PALWORLD_DIR/Pal/Binaries/Win64"
UE4SS_DIR="$BIN_DIR/ue4ss"
MODS_DIR="$UE4SS_DIR/Mods"
UE4SS_DLL="$UE4SS_DIR/UE4SS.dll"
BUNDLE_DIR="$PROJECT_DIR/UE4SS"

mkdir -p "$BIN_DIR"

if [ ! -f "$UE4SS_DLL" ]; then
    if [ ! -d "$BUNDLE_DIR" ]; then
        echo "ERREUR : Bundle UE4SS local manquant ($BUNDLE_DIR)."
        exit 1
    fi
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "[dry-run] Copie du bundle UE4SS local simulee."
    else
        echo "Copie du bundle UE4SS local ..."
        cp -r "$BUNDLE_DIR"/* "$BIN_DIR/"
        if [ ! -f "$UE4SS_DLL" ]; then
            echo "ERREUR : UE4SS.dll n'a pas ete installe. Contenu de $UE4SS_DIR :"
            ls -R "$UE4SS_DIR" 2>/dev/null || echo "(dossier vide/inexistant)"
            exit 1
        fi
        echo "UE4SS installe."
    fi
else
    echo "UE4SS deja present."
fi

# Parametres UE4SS
SETTINGS_SRC="$PROJECT_DIR/UE4SS-settings.ini"
SETTINGS_DEST="$UE4SS_DIR/UE4SS-settings.ini"
if [ -f "$SETTINGS_SRC" ]; then
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "[dry-run] Copie de $SETTINGS_SRC vers $SETTINGS_DEST simulee."
    else
        if [ -f "$SETTINGS_DEST" ]; then
            timestamp=$(date +%Y%m%d-%H%M%S)
            cp -f "$SETTINGS_DEST" "$SETTINGS_DEST.$timestamp.bak"
            echo "Sauvegarde de $SETTINGS_DEST vers $SETTINGS_DEST.$timestamp.bak"
        fi
        cp -f "$SETTINGS_SRC" "$SETTINGS_DEST"
        echo "UE4SS-settings.ini copie."
    fi
fi

if [ "$DRY_RUN" -eq 1 ]; then
    echo "[dry-run] Creation de $MODS_DIR simulee."
else
    mkdir -p "$MODS_DIR"
fi

enable_mod_in_mods_txt() {
    local mod_name="$1"
    local mods_txt="$MODS_DIR/mods.txt"
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "[dry-run] $mod_name serait active dans mods.txt"
        return
    fi
    if [ ! -f "$mods_txt" ]; then
        touch "$mods_txt"
    fi
    if grep -iE "^[[:space:]]*${mod_name}[[:space:]]*:" "$mods_txt" > /dev/null; then
        sed -i -E "s/^[[:space:]]*${mod_name}[[:space:]]*:[[:space:]]*.*/${mod_name} : 1/i" "$mods_txt"
    else
        echo "${mod_name} : 1" >> "$mods_txt"
    fi
    echo "${mod_name} active dans mods.txt"
}

install_mod() {
    local src_path="$1"
    local mod_name="$2"
    if [ ! -d "$src_path" ]; then
        echo "AVERTISSEMENT : $src_path non trouve, ignore."
        return
    fi
    if [ ! -f "$src_path/Info.json" ]; then
        echo "AVERTISSEMENT : $src_path/Info.json manquant, ignore."
        return
    fi
    echo "Installation de $mod_name ..."
    local user_config=""
    if [ -d "$MODS_DIR/$mod_name" ] && [ -f "$MODS_DIR/$mod_name/Scripts/config.lua" ]; then
        user_config=$(cat "$MODS_DIR/$mod_name/Scripts/config.lua")
    fi
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "[dry-run] Copie de $src_path vers $MODS_DIR/$mod_name simulee."
    else
        rm -rf "$MODS_DIR/$mod_name"
        cp -r "$src_path" "$MODS_DIR/$mod_name"
        if [ ! -f "$MODS_DIR/$mod_name/Info.json" ]; then
            echo "ERREUR : $mod_name n'a pas ete copie correctement."
            exit 1
        fi
        touch "$MODS_DIR/$mod_name/enabled.txt"
        if [ -n "$user_config" ] && [ -f "$MODS_DIR/$mod_name/Scripts/config.lua" ]; then
            echo "$user_config" > "$MODS_DIR/$mod_name/Scripts/config.lua.user.bak"
        fi
    fi
    enable_mod_in_mods_txt "$mod_name"
}

# Installe tous les mods Pal* detectes
for d in "$PROJECT_DIR"/Pal*/; do
    [ -d "$d" ] || continue
    name=$(basename "$d")
    [ "$name" = "PalMiniMap" ] && continue
    install_mod "$d" "$name"
done

# Cas special dev : PalMiniMap/Prototype
if [ -f "$PROJECT_DIR/PalMiniMap/Prototype/Info.json" ]; then
    install_mod "$PROJECT_DIR/PalMiniMap/Prototype" "PalMiniMapPrototype"
fi

if [ "$DRY_RUN" -eq 1 ]; then
    echo "[dry-run] Simulation terminee. Aucun fichier n'a ete modifie."
else
    echo "Installation terminee."
    echo "Lancez Palworld."
fi
echo "F1     : menu de triches"
echo "INS    : God Mode"
echo "NUMPAD0: minimap"
echo "HELP   : gestionnaire de mods"
echo "python config_manager.py : configuration hors jeu"
