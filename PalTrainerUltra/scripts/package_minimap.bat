@echo off
REM package_minimap.bat — Build et packaging Minimap-Only pour partage
setlocal enabledelayedexpansion

set ROOT=%~dp0..
set DIST=%ROOT%\dist-minimap
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%

echo ============================================
echo  Build + Packaging Minimap-Only
echo ============================================
echo.

REM Build
echo [1/4] Compilation de PalTrainerMiniMap.exe...
cd /d %ROOT%
mingw32-make clean >nul 2>&1
mingw32-make PalTrainerMiniMap.exe 2>&1 | findstr /i "error" >nul
if %errorlevel% equ 0 (
    echo ERREUR : Build echoue.
    mingw32-make PalTrainerMiniMap.exe 2>&1 | more
    pause
    exit /b 1
)
echo Build OK.

REM Preparer le dossier
echo [2/4] Preparation du dossier dist-minimap...
if exist "%DIST%" rmdir /s /q "%DIST%"
mkdir "%DIST%"
mkdir "%DIST%\assets\maps"
mkdir "%DIST%\assets\icons"
mkdir "%DIST%\data"

REM Copier les fichiers
echo [3/4] Copie des fichiers...
copy "%ROOT%\PalTrainerMiniMap.exe" "%DIST%\" >nul
copy "%ROOT%\src\trainer\runtime_offsets.json" "%DIST%\" >nul
copy "%ROOT%\scripts\Setup-Defender.bat" "%DIST%\" >nul
copy "%ROOT%\scripts\Lancer-Minimap.bat" "%DIST%\" >nul
copy "%ROOT%\src\overlay\palmods.ico" "%DIST%\" >nul 2>nul

REM Textures (toutes les resolutions)
copy "%ROOT%\assets\maps\map_2048.rgba" "%DIST%\assets\maps\" >nul
copy "%ROOT%\assets\maps\map_tree_2048.rgba" "%DIST%\assets\maps\" >nul
copy "%ROOT%\assets\maps\map_4096.rgba" "%DIST%\assets\maps\" >nul 2>nul
copy "%ROOT%\assets\maps\map_tree_4096.rgba" "%DIST%\assets\maps\" >nul 2>nul
copy "%ROOT%\assets\maps\map_8192.rgba" "%DIST%\assets\maps\" >nul 2>nul
copy "%ROOT%\assets\maps\map_tree_8192.rgba" "%DIST%\assets\maps\" >nul 2>nul

REM Icones
copy "%ROOT%\assets\icons\*.bmp" "%DIST%\assets\icons\" >nul 2>nul

REM Data
copy "%ROOT%\data\pal_database.json" "%DIST%\data\" >nul 2>nul
copy "%ROOT%\data\pal_spawns.json" "%DIST%\data\" >nul 2>nul

REM Verifications
if not exist "%DIST%\PalTrainerMiniMap.exe" (
    echo ERREUR : PalTrainerMiniMap.exe manquant !
    pause
    exit /b 1
)
if not exist "%DIST%\assets\maps\map_2048.rgba" (
    echo ERREUR : map_2048.rgba manquant !
    pause
    exit /b 1
)

REM Zip
echo [4/4] Creation du zip...
set VERSION=v1.0.2
set ZIP=%ROOT%\PalTrainerMiniMap-%VERSION%.zip
if exist "%ZIP%" del "%ZIP%"

powershell -Command "Compress-Archive -Path '%DIST%\*' -DestinationPath '%ZIP%' -Force"

echo.
echo ============================================
echo  Zip cree : %ZIP%
echo ============================================

REM Afficher la taille
for %%I in ("%ZIP%") do echo Taille : %%~zI octets

echo.
echo Vous pouvez maintenant partager ce zip.
echo Les utilisateurs doivent :
echo   1. Extraire le zip
echo   2. Lancer Setup-Defender.bat (admin)
echo   3. Lancer Lancer-Minimap.bat
echo.
pause
