#!/usr/bin/env bash
# release_to_github.sh — Cree une GitHub Release et uploade le zip des mods.
#
# Usage :
#   GITHUB_TOKEN="<your-github-token>" ./release_to_github.sh v1.0
#   GITHUB_TOKEN="<your-github-token>" PRERELEASE=true ./release_to_github.sh v1.0
#   GITHUB_TOKEN="<your-github-token>" ZIP_PATH=./release/... ./release_to_github.sh
#
# Le script :
#   1. Verifie le token GitHub.
#   2. Genere le zip avec package_for_release.sh.
#   3. Recupere OWNER/REPO depuis le remote git.
#   4. Cree la release via l'API GitHub.
#   5. Upload le zip en tant qu'asset.

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
RELEASE_VERSION="${RELEASE_VERSION:-1.0.0}"
DEFAULT_ZIP="$PROJECT_DIR/release/PalworldMods-v${RELEASE_VERSION}.zip"
ZIP_PATH="${ZIP_PATH:-$DEFAULT_ZIP}"

echo "============================================"
echo " GitHub Release Automation"
echo " Project : $PROJECT_DIR"
echo "============================================"

if [ -z "${GITHUB_TOKEN:-}" ]; then
    echo "ERREUR : definissez la variable d'environnement GITHUB_TOKEN."
    echo "Exemple : GITHUB_TOKEN=<your-github-token> ./release_to_github.sh v1.0"
    exit 1
fi

TAG="${1:-${TAG:-}}"
if [ -z "$TAG" ]; then
    read -r -p "Nom du tag (ex: v1.0) : " TAG
fi
if [ -z "$TAG" ]; then
    echo "ERREUR : un tag est requis."
    exit 1
fi

PRERELEASE="${PRERELEASE:-false}"
DRAFT="${DRAFT:-false}"
RELEASE_NAME="Palworld Mods & Trainer ${TAG}"
RELEASE_BODY="Release ${TAG} - Lua UE4SS mods + PalTrainerUltra C++ trainer overlay with minimap, ESP, and 50+ cheats."

echo ""
echo "Tag      : $TAG"
echo "Version  : $RELEASE_VERSION"
echo "Pre-release : $PRERELEASE"
echo "Draft    : $DRAFT"
echo "Zip      : $ZIP_PATH"
echo ""

# 1. Packaging
echo "[release] Generation du zip ..."
if [ ! -x "$PROJECT_DIR/package_for_release.sh" ]; then
    chmod +x "$PROJECT_DIR/package_for_release.sh"
fi
"$PROJECT_DIR/package_for_release.sh"

if [ ! -f "$ZIP_PATH" ]; then
    echo "ERREUR : le zip n'a pas ete genere : $ZIP_PATH"
    exit 1
fi

# 2. Recuperation OWNER/REPO depuis le remote git origin
REMOTE_URL="$(git -C "$PROJECT_DIR" remote get-url origin 2>/dev/null || true)"
if [ -z "$REMOTE_URL" ]; then
    echo "ERREUR : aucun remote 'origin' configure."
    exit 1
fi

if [[ "$REMOTE_URL" =~ github\.com[/:]([^/]+)/(.+)\.git$ ]]; then
    OWNER="${BASH_REMATCH[1]}"
    REPO="${BASH_REMATCH[2]}"
elif [[ "$REMOTE_URL" =~ github\.com[/:]([^/]+)/(.+)$ ]]; then
    OWNER="${BASH_REMATCH[1]}"
    REPO="${BASH_REMATCH[2]}"
else
    echo "ERREUR : impossible de parser le remote GitHub : $REMOTE_URL"
    exit 1
fi

API_BASE="https://api.github.com/repos/${OWNER}/${REPO}"
echo "[release] Repo detecte : ${OWNER}/${REPO}"

# 3. Creation de la release
echo "[release] Creation de la release $TAG ..."
RELEASE_PAYLOAD=$(cat <<EOF
{
  "tag_name": "$TAG",
  "target_commitish": "master",
  "name": "$RELEASE_NAME",
  "body": "$RELEASE_BODY",
  "draft": $DRAFT,
  "prerelease": $PRERELEASE
}
EOF
)

CREATE_RESPONSE=$(curl -s -X POST \
    -H "Authorization: token $GITHUB_TOKEN" \
    -H "Accept: application/vnd.github+json" \
    -H "Content-Type: application/json" \
    -d "$RELEASE_PAYLOAD" \
    "$API_BASE/releases")

UPLOAD_URL=$(echo "$CREATE_RESPONSE" | sed -n 's/.*"upload_url"[[:space:]]*:[[:space:]]*"\([^"]*\){?name,label}".*/\1/p')
if [ -z "$UPLOAD_URL" ]; then
    echo "ERREUR : echec de creation de la release."
    echo "Reponse : $CREATE_RESPONSE"
    exit 1
fi

HTML_URL=$(echo "$CREATE_RESPONSE" | sed -n 's/.*"html_url"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -n1)
echo "[release] Release creee : ${HTML_URL:-$API_BASE/releases/tag/$TAG}"

# 4. Upload du zip
echo "[release] Upload de $(basename "$ZIP_PATH") ..."
UPLOAD_RESPONSE=$(curl -s -X POST \
    -H "Authorization: token $GITHUB_TOKEN" \
    -H "Accept: application/vnd.github+json" \
    -H "Content-Type: application/zip" \
    --data-binary @"$ZIP_PATH" \
    "${UPLOAD_URL}?name=$(basename "$ZIP_PATH")")

if echo "$UPLOAD_RESPONSE" | grep -q '"browser_download_url"'; then
    DOWNLOAD_URL=$(echo "$UPLOAD_RESPONSE" | sed -n 's/.*"browser_download_url"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')
    echo "[release] Asset uploade avec succes : $DOWNLOAD_URL"
else
    echo "ERREUR : echec de l'upload du zip."
    echo "Reponse : $UPLOAD_RESPONSE"
    exit 1
fi

echo ""
echo "[release] Termine. Release : ${HTML_URL:-$API_BASE/releases/tag/$TAG}"
