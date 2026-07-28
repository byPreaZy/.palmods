#!/usr/bin/env bash
# uninstall.sh — Desinstallateur Palworld Mods ULTRA MAX pour WSL/Linux

set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"

PALWORLD_DIR="${PALWORLD_DIR:-/mnt/c/Program Files (x86)/Steam/steamapps/common/Palworld}"
REMOVE_UE4SS=0
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case $1 in
    --remove-ue4ss) REMOVE_UE4SS=1 ; shift ;;
    --palworld-dir) PALWORLD_DIR="$2" ; shift 2 ;;
    --dry-run) DRY_RUN=1 ; shift ;;
    *) PALWORLD_DIR="$1" ; shift ;;
  esac
done

BIN_DIR="$PALWORLD_DIR/Pal/Binaries/Win64"
if [ -d "$BIN_DIR/ue4ss" ] || [ -f "$BIN_DIR/ue4ss/UE4SS.dll" ]; then
    UE4SS_DIR="$BIN_DIR/ue4ss"
else
    UE4SS_DIR="$BIN_DIR"
fi
MODS_DIR="$UE4SS_DIR/Mods"

UE4SS_BASE_MODS="CheatManagerEnablerMod|ConsoleCommandsMod|BPML_GenericFunctions|ConsoleEnablerMod|LineTraceMod|Keybinds|BPModLoaderMod|SplitScreenMod|shared"

echo "[uninstall] Dossier Palworld : $PALWORLD_DIR"

if [ "$DRY_RUN" -eq 1 ]; then
  echo "[uninstall] [dry-run] Mode simulation active."
fi

removed_any=0
if [ -d "$MODS_DIR" ]; then
  for mod_dir in "$MODS_DIR"/Pal*/; do
    [ -d "$mod_dir" ] || continue
    name=$(basename "$mod_dir")
    case "$name" in
      PalCheatMenu|PalMiniMapPrototype|Pal*)
        if echo "$name" | grep -Eq "^($UE4SS_BASE_MODS)$"; then
          continue
        fi
        if [ "$DRY_RUN" -eq 1 ]; then
          echo "[uninstall] [dry-run] Supprimerait $mod_dir"
        else
          echo "[uninstall] Suppression de $mod_dir ..."
          rm -rf "$mod_dir"
        fi
        removed_any=1
        ;;
    esac
  done
fi

if [ "$REMOVE_UE4SS" -eq 1 ]; then
  echo "[uninstall] Suppression de UE4SS ..."
  for f in UE4SS.dll UE4SS.pdb UE4SS-console.dll UE4SS-console.pdb UE4SS-settings.ini UE4SS.log UE4SS-errors.log dwmapi.dll xinput1_3.dll cpp2il_out; do
    target="$BIN_DIR/$f"
    if [ -e "$target" ]; then
      if [ "$DRY_RUN" -eq 1 ]; then
        echo "[uninstall] [dry-run] Supprimerait $target"
      else
        rm -rf "$target"
        echo "[uninstall] Supprime : $target"
      fi
    fi
  done
  if [ -d "$MODS_DIR" ] && [ -z "$(ls -A "$MODS_DIR" 2>/dev/null)" ]; then
    if [ "$DRY_RUN" -eq 1 ]; then
      echo "[uninstall] [dry-run] Supprimerait $MODS_DIR (vide)"
    else
      rmdir "$MODS_DIR"
      echo "[uninstall] Supprime : $MODS_DIR"
    fi
  fi
fi

# Nettoyer les dossiers du trainer (ancien + actuel)
for trainer_dir in "$PALWORLD_DIR/PalTrainerApp" "$PALWORLD_DIR/PalTrainerUltra"; do
  if [ -d "$trainer_dir" ]; then
    if [ "$DRY_RUN" -eq 1 ]; then
      echo "[uninstall] [dry-run] Supprimerait $trainer_dir"
    else
      echo "[uninstall] Suppression de $trainer_dir ..."
      rm -rf "$trainer_dir"
    fi
    removed_any=1
  fi
done

if [ "$removed_any" -eq 1 ] || [ "$REMOVE_UE4SS" -eq 1 ]; then
  if [ "$DRY_RUN" -eq 1 ]; then
    echo "[uninstall] [dry-run] Simulation terminee."
  else
    echo "[uninstall] Desinstallation terminee."
  fi
else
  echo "[uninstall] Rien a desinstaller."
fi
