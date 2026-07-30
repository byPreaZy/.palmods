@echo off
REM PalTrainerUltra — démarre le serveur web local de carte + trainer
set PALTRAINER_DATA_DIR=%~dp0
set PALTRAINER_PORT=8765
python server\server.py
pause
