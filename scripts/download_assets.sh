#!/usr/bin/env bash
# Télécharge les gros fichiers d'assets (.rgba, .webp) depuis un GitHub Release.
# Utilisé par package.sh et la CI/CD pour obtenir les textures de carte.
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)

# Configuration: remplacer OWNER/REPO par le dépôt GitHub réel
GITHUB_REPO="${GITHUB_REPO:-byPreaZy/.palmods}"
ASSETS_DIR="$ROOT/assets/maps"
WEB_MAP_DIR="$ROOT/web/assets/map"

mkdir -p "$ASSETS_DIR" "$WEB_MAP_DIR"

# Liste des fichiers à télécharger depuis le dernier release
RGBA_FILES=(
    "map_2048.rgba"
    "map_tree_2048.rgba"
)

WEBP_FILES=(
    "palworld-map.webp"
    "palworld-treemap.webp"
    "T_WorldMap_85.webp"
    "T_icon_compass_Bounty.webp"
    "T_icon_compass_EnemyCamp.webp"
    "T_icon_compass_FTtower.webp"
    "T_icon_compass_Oilrig.webp"
    "T_icon_compass_camp.webp"
    "T_icon_compass_dungeon.webp"
    "T_icon_compass_egg.webp"
    "T_icon_compass_tower.webp"
    "T_icon_compass_treasure.webp"
    "T_icon_enemy_strong.webp"
)

echo "Téléchargement des assets de carte depuis GitHub Releases..."

# Essayer de récupérer l'URL du dernier release
API_URL="https://api.github.com/repos/${GITHUB_REPO}/releases/latest"
AUTH_HEADER=""
if [ -n "$GITHUB_TOKEN" ]; then
    AUTH_HEADER="-H \"Authorization: Bearer ${GITHUB_TOKEN}\""
fi
DOWNLOAD_BASE=$(eval curl -s $AUTH_HEADER \"$API_URL\" | grep -o '"browser_download_url": *"[^"]*"' | sed 's/.*"browser_download_url": *"//;s/"$//')

if [ -z "$DOWNLOAD_BASE" ]; then
    echo "Avertissement: Impossible de récupérer l'URL du release GitHub."
    echo "Les fichiers .rgba et .webp doivent être présents localement."
    echo "Si vous avez les fichiers webp source, exécutez: python scripts/convert_textures.py"
    exit 0
fi

# Télécharger les fichiers .rgba
for f in "${RGBA_FILES[@]}"; do
    if [ ! -f "$ASSETS_DIR/$f" ]; then
        echo "Téléchargement: $f"
        # Chercher l'asset correspondant dans les URLs du release
        url=$(echo "$DOWNLOAD_BASE" | grep "$f" | head -1)
        if [ -n "$url" ]; then
            curl -L -o "$ASSETS_DIR/$f" "$url"
        else
            echo "  Non trouvé dans le release: $f"
        fi
    else
        echo "Déjà présent: $ASSETS_DIR/$f"
    fi
done

# Télécharger les fichiers .webp
for f in "${WEBP_FILES[@]}"; do
    if [ ! -f "$WEB_MAP_DIR/$f" ]; then
        echo "Téléchargement: $f"
        url=$(echo "$DOWNLOAD_BASE" | grep "$f" | head -1)
        if [ -n "$url" ]; then
            curl -L -o "$WEB_MAP_DIR/$f" "$url"
        else
            echo "  Non trouvé dans le release: $f"
        fi
    else
        echo "Déjà présent: $WEB_MAP_DIR/$f"
    fi
done

echo "Téléchargement des assets terminé."
