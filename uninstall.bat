@echo off
REM uninstall.bat — Désinstallateur Palworld Mods ULTRA MAX

setlocal

cd /d "%~dp0"

echo ============================================
echo  Désinstallateur Palworld Mods ULTRA MAX
echo ============================================
echo.

where powershell >nul 2>nul
if %errorlevel%==0 (
    echo PowerShell detecte, lancement de uninstall.ps1 ...
    powershell -ExecutionPolicy Bypass -File "%~dp0uninstall.ps1"
) else (
    echo PowerShell non detecte, utilisation de uninstall.py ...
    python "%~dp0uninstall.py"
    if %errorlevel%==0 goto :end
    py "%~dp0uninstall.py"
    if %errorlevel%==0 goto :end
    echo.
    echo ERREUR : Python et PowerShell non detectes.
    echo Utilisez uninstall.py manuellement avec Python.
)

:end
echo.
echo Appuyez sur une touche pour fermer ...
pause >nul
