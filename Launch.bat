@echo off
cd /d "%~dp0"

if not exist "PalTrainerUltra.exe" (
    echo ERREUR: PalTrainerUltra.exe introuvable !
    echo Compilez d'abord avec: make all
    pause
    exit /b 1
)

REM Ajouter le dossier aux exclusions Windows Defender (silencieux si deja exclu)
powershell -Command "Add-MpPreference -ExclusionPath '%~dp0' -ErrorAction SilentlyContinue" 2>nul

PalTrainerUltra.exe
