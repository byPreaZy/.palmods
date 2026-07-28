# uninstall.ps1 — Désinstallateur Palworld Mods ULTRA MAX (PowerShell)
# Supprime les mods du dossier Palworld. Peut aussi supprimer UE4SS avec -RemoveUE4SS.

param(
    [string]$PalworldDir = "",
    [switch]$RemoveUE4SS,
    [switch]$DryRun
)

$UE4SS_BASE_MODS = @(
    "CheatManagerEnablerMod",
    "ConsoleCommandsMod",
    "BPML_GenericFunctions",
    "ConsoleEnablerMod",
    "LineTraceMod",
    "Keybinds",
    "BPModLoaderMod",
    "SplitScreenMod",
    "shared"
)

$ErrorActionPreference = "Stop"

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

try {
    if ([string]::IsNullOrWhiteSpace($PalworldDir)) {
        $PalworldDir = $env:PALWORLD_DIR
    }
    if ([string]::IsNullOrWhiteSpace($PalworldDir)) {
        # Detection rapide
        $drives = Get-CimInstance -ClassName Win32_LogicalDisk | Where-Object { $_.DriveType -eq 3 } | Select-Object -ExpandProperty DeviceID
        $commonPaths = @(
            "\Steam\steamapps\common\Palworld",
            "\SteamLibrary\steamapps\common\Palworld",
            "\Program Files (x86)\Steam\steamapps\common\Palworld",
            "\Program Files\Steam\steamapps\common\Palworld"
        )
        foreach ($drive in $drives) {
            foreach ($rel in $commonPaths) {
                $candidate = "$drive$rel"
                if (Test-Path (Join-Path $candidate "Pal\Binaries\Win64")) {
                    $PalworldDir = $candidate
                    break
                }
            }
            if (-not [string]::IsNullOrWhiteSpace($PalworldDir)) { break }
        }
    }

    if ([string]::IsNullOrWhiteSpace($PalworldDir)) {
        $PalworldDir = Read-Host "Entrez le chemin complet de Palworld"
    }

    $BinDir = Join-Path $PalworldDir "Pal\Binaries\Win64"
    $UE4SSDir = Join-Path $BinDir "ue4ss"
    if (-not (Test-Path (Join-Path $UE4SSDir "UE4SS.dll")) -and -not (Test-Path (Join-Path $UE4SSDir "Mods"))) {
        $UE4SSDir = $BinDir
    }
    $ModsDir = Join-Path $UE4SSDir "Mods"

    if (-not (Test-Path $PalworldDir)) {
        throw "Le dossier Palworld n'existe pas : $PalworldDir"
    }

    Write-Host "[uninstall] Dossier Palworld : $PalworldDir"

    if ($DryRun) {
        Write-Host "[uninstall] [dry-run] Mode simulation active."
    }

    $removedAny = $false
    if (Test-Path $ModsDir) {
        foreach ($modDir in Get-ChildItem -Path $ModsDir -Directory) {
            if ($UE4SS_BASE_MODS -contains $modDir.Name) {
                continue
            }
            if (-not $modDir.Name.StartsWith("Pal")) {
                continue
            }
            if (Test-Path (Join-Path $modDir.FullName "Info.json")) {
                if ($DryRun) {
                    Write-Host "[uninstall] [dry-run] Supprimerait $($modDir.FullName)"
                } else {
                    Write-Host "[uninstall] Suppression de $($modDir.FullName) ..."
                    Remove-Item -Recurse -Force $modDir.FullName
                }
                $removedAny = $true
            }
        }
    }

    # Nettoyer les dossiers du trainer (ancien + actuel)
    $trainerDirs = @("PalTrainerApp", "PalTrainerUltra")
    foreach ($trainerName in $trainerDirs) {
        $trainerDir = Join-Path $PalworldDir $trainerName
        if (Test-Path $trainerDir) {
            if ($DryRun) {
                Write-Host "[uninstall] [dry-run] Supprimerait $trainerDir"
            } else {
                Write-Host "[uninstall] Suppression de $trainerDir ..."
                Remove-Item -Recurse -Force $trainerDir
            }
            $removedAny = $true
        }
    }

    if ($RemoveUE4SS) {
        Write-Host "[uninstall] Suppression de UE4SS ..."
        $ue4ssFiles = @(
            "UE4SS.dll",
            "UE4SS.pdb",
            "UE4SS-console.dll",
            "UE4SS-console.pdb",
            "UE4SS-settings.ini",
            "UE4SS.log",
            "UE4SS-errors.log",
            "dwmapi.dll",
            "xinput1_3.dll",
            "cpp2il_out"
        )
        foreach ($name in $ue4ssFiles) {
            $target = Join-Path $BinDir $name
            if (Test-Path $target) {
                if ($DryRun) {
                    Write-Host "[uninstall] [dry-run] Supprimerait $target"
                } else {
                    Remove-Item -Recurse -Force $target
                    Write-Host "[uninstall] Supprime : $target"
                }
            }
        }
        if (Test-Path $ModsDir -and -not (Get-ChildItem $ModsDir)) {
            if ($DryRun) {
                Write-Host "[uninstall] [dry-run] Supprimerait $ModsDir (vide)"
            } else {
                Remove-Item $ModsDir
                Write-Host "[uninstall] Supprime : $ModsDir"
            }
        }
    }

    if ($DryRun) {
        Write-Host "[uninstall] [dry-run] Simulation terminee."
    } else {
        Write-Host "[uninstall] Desinstallation terminee."
    }
}
catch {
    Write-Host "[uninstall] ERREUR FATALE : $_"
    Read-Host "Appuyez sur Entree pour fermer"
    exit 1
}

Read-Host "Appuyez sur Entree pour fermer"
