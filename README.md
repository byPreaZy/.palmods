<div align="center">

# 🎮 Palworld Mods & Trainer — ULTRA MAX Edition

**Mods Lua UE4SS + Trainer C++ ImGui pour Palworld 1.0 (Steam)**

[![CI/CD](https://github.com/byPreaZy/.palmods/actions/workflows/build.yml/badge.svg)](https://github.com/byPreaZy/.palmods/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/byPreaZy/.palmods?label=Release&color=blue)](https://github.com/byPreaZy/.palmods/releases)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![Palworld](https://img.shields.io/badge/Palworld-1.0%20Steam-orange)](https://store.steampowered.com/app/1623730/Palworld/)
[![Platform](https://img.shields.io/badge/Platform-Windows-blue)](https://github.com/byPreaZy/.palmods)
[![Lua](https://img.shields.io/badge/Lua-UE4SS-purple)](https://github.com/UE4SS-RE/RE-UE4SS)
[![C++](https://img.shields.io/badge/C%2B%2B-17-red)](https://isocpp.org/)

</div>

---

## 🇫🇷 Français

Deux projets indépendants dans un seul dépôt pour **Palworld 1.0 (Steam)**.

### 📋 Projets

#### 1. Mods Lua UE4SS (`mods/`)

Mods utilisant **RE-UE4SS** (injection Lua). Aucun `.pak` ou Unreal Engine requis.

| Mod | Description | Touche |
|-----|-------------|--------|
| **PalCheatMenu** | Menu ImGui avec 35+ cheats (God Mode, vol, no clip, capture 100%, spawn d'items, etc.) | `F1` |
| **PalMiniMap** | Super minimap avec rendu terrain, POIs, sélection des Pals, multijoueur | `M` |
| **PalModManager** | Gestionnaire unifié pour activer/configurer tous les mods | `F10` |
| **PalWeight** | Poids d'inventaire augmenté | — |
| **PalAutoLoot** | Ramasse automatique des objets | — |
| **PalCaptureCounter** | Compteur de captures | — |
| **PalInspectPal** | Infos détaillées des Pals | — |
| **PalQuickDrop / PalQuickStack** | Raccourcis inventaire | — |
| **PalInstantFish / Hatch / Breed** | Actions instantanées | — |

**Installation**: `install.bat` (Windows) ou `./install.sh` (WSL/Linux) — détecte Palworld automatiquement, installe RE-UE4SS si nécessaire.

📖 [Guide d'installation détaillé](INSTALL.md)

#### 2. PalTrainerUltra (`PalTrainerUltra/`)

Trainer overlay **WeMod-like** en C++ — injection DLL, overlay ImGui, minimap interactive, carte web.

| Composant | Description |
|-----------|-------------|
| **PalTrainerLauncher.exe** | Launcher centralisé: détection Palworld, scan offsets, injection DLL, lancement des outils |
| **PalTrainerCore.dll** | DLL injectée: 50+ cheats en temps réel (God Mode, vitesse, vol, téléport, etc.) |
| **PalTrainerOverlay.exe** | Overlay ImGui complet avec minimap, ESP, spawns, favoris |
| **PalTrainerMiniMap.exe** | Minimap autonome (lecture mémoire seule, sans injection) |
| **PalOffsetScanner.exe** | Scanner d'offsets automatique |
| **Carte web** | Interface web interactive (`server/server.py` + `web/`) |

**Téléchargement**: [Releases](../../releases) — extraire le zip, lancer `Launch.bat` en admin.

📖 [Documentation PalTrainerUltra](PalTrainerUltra/README.md)

### 🚀 Installation rapide

<details>
<summary><b>Mods Lua</b></summary>

1. Télécharger et décompresser le zip
2. **Windows**: double-clic sur `install.bat`
3. **WSL/Linux**: `./install.sh`
4. Lancer Palworld — `F1` pour le menu, `M` pour la minimap
</details>

<details>
<summary><b>PalTrainerUltra</b></summary>

1. Télécharger la dernière [Release](../../releases) (`PalTrainerUltra-*.zip`)
2. Extraire dans un dossier
3. Lancer `Launch.bat` en administrateur
4. Lancer Palworld — le launcher détecte le jeu
5. Cliquer **JOUER** — injection + overlay automatique
</details>

### 🗂️ Structure du dépôt

```
.palmods/
├── mods/                    Mods Lua UE4SS
├── PalTrainerUltra/         Trainer C++ (DLL, overlay, minimap, launcher)
├── install.bat / .py / .sh  Installateurs mods
├── uninstall.bat / .py / .sh  Désinstallateurs
├── config_manager.py        Configurateur externe (Python/Tkinter)
├── test_install.py          Test d'installation
├── INSTALL.md               Guide d'installation détaillé (mods)
└── NEXUS_PUBLISH.md         Texte prêt pour Nexus Mods
```

### 🔧 CI/CD

Le workflow `.github/workflows/build.yml` compile PalTrainerUltra sur `windows-latest` (MinGW-w64) et crée une Release GitHub automatique sur tag `v*`.

### ⚠️ Avertissements

- Usage en **solo** ou sur serveur dont vous êtes administrateur
- Ne pas utiliser sur serveurs publics avec anti-cheat
- Projet éducatif et de divertissement personnel
- Les sauvegardes ne sont **jamais** touchées par les désinstallateurs

---

## 🇬🇧 English

Two independent projects in a single repository for **Palworld 1.0 (Steam)**.

### 📋 Projects

#### 1. Lua UE4SS Mods (`mods/`)

Mods using **RE-UE4SS** (Lua injection). No `.pak` or Unreal Engine required.

| Mod | Description | Key |
|-----|-------------|-----|
| **PalCheatMenu** | ImGui menu with 35+ cheats (God Mode, fly, no clip, 100% capture, item spawn, etc.) | `F1` |
| **PalMiniMap** | Super minimap with terrain rendering, POIs, Pal selection, multiplayer | `M` |
| **PalModManager** | Unified manager to enable/configure all mods | `F10` |
| **PalWeight** | Increased inventory weight | — |
| **PalAutoLoot** | Auto loot pickup | — |
| **PalCaptureCounter** | Capture counter | — |
| **PalInspectPal** | Detailed Pal info | — |
| **PalQuickDrop / QuickStack** | Inventory shortcuts | — |
| **PalInstantFish / Hatch / Breed** | Instant actions | — |

**Install**: `install.bat` (Windows) or `./install.sh` (WSL/Linux) — auto-detects Palworld, installs RE-UE4SS if needed.

📖 [Detailed install guide](INSTALL.md)

#### 2. PalTrainerUltra (`PalTrainerUltra/`)

**WeMod-like** C++ trainer overlay — DLL injection, ImGui overlay, interactive minimap, web map.

| Component | Description |
|-----------|-------------|
| **PalTrainerLauncher.exe** | Centralized launcher: Palworld detection, offset scan, DLL injection, tool launcher |
| **PalTrainerCore.dll** | Injected DLL: 50+ real-time cheats (God Mode, speed, fly, teleport, etc.) |
| **PalTrainerOverlay.exe** | Full ImGui overlay with minimap, ESP, spawns, favorites |
| **PalTrainerMiniMap.exe** | Standalone minimap (memory read-only, no injection) |
| **PalOffsetScanner.exe** | Automatic offset scanner |
| **Web map** | Interactive web interface (`server/server.py` + `web/`) |

**Download**: [Releases](../../releases) — extract the zip, run `Launch.bat` as admin.

📖 [PalTrainerUltra documentation](PalTrainerUltra/README.md)

### 🚀 Quick Start

<details>
<summary><b>Lua Mods</b></summary>

1. Download and extract the zip
2. **Windows**: double-click `install.bat`
3. **WSL/Linux**: `./install.sh`
4. Launch Palworld — `F1` for cheat menu, `M` for minimap
</details>

<details>
<summary><b>PalTrainerUltra</b></summary>

1. Download the latest [Release](../../releases) (`PalTrainerUltra-*.zip`)
2. Extract to a folder
3. Run `Launch.bat` as administrator
4. Launch Palworld — the launcher detects the game
5. Click **PLAY** — auto injection + overlay
</details>

### 🗂️ Repository Structure

```
.palmods/
├── mods/                    Lua UE4SS mods
├── PalTrainerUltra/         C++ trainer (DLL, overlay, minimap, launcher)
├── install.bat / .py / .sh  Mod installers
├── uninstall.bat / .py / .sh  Uninstallers
├── config_manager.py        External configurator (Python/Tkinter)
├── test_install.py          Installation test
├── INSTALL.md               Detailed install guide (mods)
└── NEXUS_PUBLISH.md         Ready-to-paste Nexus Mods text
```

### 🔧 CI/CD

The `.github/workflows/build.yml` workflow builds PalTrainerUltra on `windows-latest` (MinGW-w64) and creates a GitHub Release automatically on `v*` tags.

### ⚠️ Disclaimer

- Use in **single-player** or on a server you administer
- Do not use on public servers with anti-cheat
- Educational and personal entertainment project
- Save files are **never** touched by uninstallers

---

<div align="center">

**🎮 Made with ❤️ for the Palworld community**

[Report a bug](../../issues) · [Request a feature](../../issues) · [Contributing](CONTRIBUTING.md) · [Latest release](../../releases)

</div>
