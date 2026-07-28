@echo off
REM install.bat - Installateur Palworld Mods ULTRA MAX (unique point d entree Windows)
REM Double-cliquez ce fichier apres avoir decompresse le zip.
REM Tout est journalise dans install-report.txt du dossier courant.

setlocal EnableDelayedExpansion

set "PROJECT_DIR=%~dp0"
set "LOG=%PROJECT_DIR%install-report.txt"
set "PYTHON_CMD="
set "DRY_RUN="
set "PALWORLD_ARG="

REM Parsing minimal des arguments
:parse_args
if "%~1"=="" goto :args_done
if /I "%~1"=="--dry-run" (
    set "DRY_RUN=--dry-run"
) else if not defined PALWORLD_ARG (
    set "PALWORLD_ARG=%~1"
)
shift
goto :parse_args
:args_done

echo ============================================
echo  Installateur Palworld Mods ULTRA MAX
echo  Dossier : %PROJECT_DIR%
echo  Date    : %date% %time%
if defined DRY_RUN echo  Mode    : SIMULATION ^(dry-run^)
echo ============================================

REM Detection de Python
echo Detection de Python en cours ...
for %%c in (py python python3) do (
    where %%c >nul 2>nul
    if !errorlevel! == 0 (
        %%c --version >nul 2>nul
        if !errorlevel! == 0 (
            set "PYTHON_CMD=%%c"
            goto :python_ok
        )
    )
)

REM Aucun Python trouve
echo ERREUR : Python fonctionnel non trouve.
echo.
echo Installez Python depuis https://www.python.org
echo   ou depuis le Microsoft Store, puis relancez install.bat.
echo.
(echo ERREUR : Python fonctionnel non trouve) > "%LOG%"
(echo Installez Python depuis https://www.python.org ou le Microsoft Store.) >> "%LOG%"
goto :end_error

:python_ok
echo Python detecte : %PYTHON_CMD%

REM Lancement de install.py
if defined PALWORLD_ARG (
    echo Lancement de install.py --palworld-dir "%PALWORLD_ARG%" ...
    %PYTHON_CMD% "%PROJECT_DIR%install.py" %DRY_RUN% --palworld-dir "%PALWORLD_ARG%"
) else if defined PALWORLD_DIR (
    echo Lancement de install.py --palworld-dir "%PALWORLD_DIR%" ...
    %PYTHON_CMD% "%PROJECT_DIR%install.py" %DRY_RUN% --palworld-dir "%PALWORLD_DIR%"
) else (
    echo Lancement de install.py ...
    %PYTHON_CMD% "%PROJECT_DIR%install.py" %DRY_RUN%
)
set "EXIT_CODE=%errorlevel%"

if %EXIT_CODE%==0 (
    (echo.) >> "%LOG%"
    (echo [install.bat] Installation terminee avec succes.) >> "%LOG%"
    (echo [install.bat] Lancez Palworld. F1 : menu de triches ^| INS : God Mode ^| NUMPAD0 : minimap ^| HELP : PalModManager.) >> "%LOG%"
) else (
    (echo.) >> "%LOG%"
    (echo [install.bat] ERREUR : install.py a retourne le code %EXIT_CODE%.) >> "%LOG%"
    (echo [install.bat] Consultez le rapport ci-dessus pour le detail.) >> "%LOG%"
)

goto :end

:end_error
set "EXIT_CODE=1"

:end
if exist "%LOG%" (
    echo.
    echo --- Rapport d installation ---
    type "%LOG%"
    echo --- Fin du rapport ---
)
echo.
echo Appuyez sur une touche pour fermer ...
pause >nul
exit /b %EXIT_CODE%
