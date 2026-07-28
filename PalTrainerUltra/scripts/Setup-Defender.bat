@echo off
REM Setup-Defender.bat — Ajoute le dossier courant aux exclusions Windows Defender
REM Doit etre execute en administrateur

:: Verifier si on est admin
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Demande d'elevation administrateur...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo ============================================
echo  Ajout des exclusions Windows Defender
echo  Dossier : %~dp0
echo ============================================
echo.

powershell -Command "Add-MpPreference -ExclusionPath '%~dp0' -ErrorAction SilentlyContinue"
powershell -Command "Add-MpPreference -ExclusionProcess 'PalTrainerMiniMap.exe' -ErrorAction SilentlyContinue"

if %errorlevel% neq 0 (
    echo ERREUR : Impossible d'ajouter les exclusions.
    echo Verifiez que Windows Defender est actif et reessayez.
    pause
    exit /b 1
)

echo.
echo Exclusions ajoutees avec succes !
echo Le dossier et les executables sont maintenant ignores par Defender.
echo.
echo Vous pouvez maintenant lancer Lancer-Minimap.bat
echo.
pause
