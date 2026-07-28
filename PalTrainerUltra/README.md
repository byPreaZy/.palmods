# PalTrainerUltra

> Trainer, minimap et outils pour Palworld 1.0 — injection DLL, overlay ImGui, carte interactive temps réel.

[![Build & Release](https://github.com/byPreaZy/.palmods/actions/workflows/build.yml/badge.svg)](https://github.com/byPreaZy/.palmods/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/byPreaZy/.palmods)](https://github.com/byPreaZy/.palmods/releases)
[![Palworld](https://img.shields.io/badge/Palworld-1.0-green.svg)](https://palworld.game)

## Quickstart

1. **Télécharger** la dernière release depuis [Releases](../../releases) (`PalTrainerUltra-*.zip`)
2. **Extraire** l'archive dans un dossier (ex: `C:\PalTrainerUltra\`)
3. **Lancer `Launch.bat`** (ou `PalTrainerLauncher.exe`) en tant qu'administrateur
4. **Lancer Palworld** — le launcher détecte le jeu automatiquement
5. **Cliquer sur "JOUER"** — injection DLL + overlay en un clic
6. **Optionnel**: utiliser les boutons du launcher pour la minimap, carte web, outils

## ⚠️ Windows Defender / Antivirus

PalTrainerUltra utilise des techniques d'injection DLL (`CreateRemoteThread`, `WriteProcessMemory`) qui sont détectées comme suspectes par les antivirus. **C'est un faux positif** — l'outil est open source et n'est pas un malware.

### Solution automatique
Lancer `Launch.bat` ajoute automatiquement le dossier aux exclusions de Windows Defender.

### Solution manuelle
Si Windows Defender bloque l'exécution :
1. Ouvrir **Sécurité Windows** → **Protection contre les virus et menaces**
2. **Gérer les paramètres** → **Ajouter ou supprimer des exclusions**
3. **Ajouter une exclusion** → **Dossier** → sélectionner le dossier `PalTrainerUltra`
4. Relancer `Launch.bat`

Ou en PowerShell (admin) :
```powershell
Add-MpPreference -ExclusionPath "C:\PalTrainerUltra"
```

## Composants

| Fichier | Rôle | Comment lancer |
|---------|------|----------------|
| **PalTrainerLauncher.exe** | Launcher centralisé — détecte Palworld, injecte, lance les outils | `Launch.bat` ou double-clic (admin) |
| **Launch.bat** | Script de lancement rapide du launcher | Clic droit → Exécuter en tant qu'admin |
| **PalTrainerCore.dll** | DLL injectée dans Palworld — exécute les cheats en temps réel | Injectée via le launcher ou `inject.bat` |
| **PalTrainerInjector.exe** | Injecteur autonome (CreateRemoteThread) | `inject.bat` ou `PalTrainerInjector.exe PalTrainerCore.dll` |
| **PalTrainerOverlay.exe** | Overlay ImGui complet: cheats, minimap, téléport, spawns, ESP | Via le launcher ou double-clic (après injection) |
| **PalTrainerUltra.exe** | Alias de l'overlay (même exécutable, nom plus convivial) | Double-clic (après injection) |
| **PalTrainerMiniMap.exe** | Minimap autonome (sans cheats, lecture mémoire seule) | Via le launcher ou double-clic (Palworld doit tourner) |
| **PalOffsetScanner.exe** | Scanner d'offsets automatique (GWorld, GObjects, ProcessEvent) | Lancer si offsets manquants/obsolètes |
| **inject.bat** | Script d'injection DLL (admin requis) | Clic droit → Exécuter en tant qu'admin |
| **start_map.bat** | Démarre le serveur web de carte interactive | Double-clic, ouvre `http://localhost:8765` |
| **server/server.py** | Serveur web local (carte, POIs, spawns, téléport) | `python server/server.py` |
| **web/** | Interface web interactive (HTML/JS) | Servie par `server.py` |
| **data/pal_spawns.json** | Données de spawn des Pals (positions, niveaux, jour/nuit) | Chargé automatiquement |
| **data/pal_database.json** | Base de données des Pals (types, breeding power, travaux) | Utilisé par le breeding calculator |
| **scripts/breeding_calculator.py** | Calculateur de reproduction (child = moyenne breeding power) | `python scripts/breeding_calculator.py parent1 parent2` |
| **scripts/save_editor.py** | Éditeur de sauvegarde (niveau, argent, points tech) | `python scripts/save_editor.py --info` |
| **runtime_offsets.json** | Offsets dynamiques générés par le scanner | Généré automatiquement |

## Cheats disponibles

### Cheats de base (toggle)
- **Santé infinie** — PV du joueur verrouillés au max
- **Endurance infinie** — Stamina illimitée
- **Faim illimitée** — Satiété verrouillée
- **Santé mentale max** — SAN verrouillée
- **Pas de crime** — Désactive le système de wanted
- **Santé Pals infinie** — PV de tous les Pals au max
- **Capture 100%** — Toutes les captures réussissent
- **Matériaux de craft illimités** — Pas de consommation
- **Matériaux de construction illimités** — Pas de consommation
- **Santé base illimitée** — PV de la base verrouillés
- **Argent illimité** — Ajoute automatiquement de l'argent
- **Effigies illimitées** — Ajoute des effigies Lifmunk

### Cheats avancés (parité FLiNG)
- **Bouclier infini** — Maintient le bouclier au maximum
- **Mode furtif** — Réduit la détection des ennemis
- **Taux de drop 100%** — Force le taux de drop au maximum
- **Nourriture non périssable** — Empêche la décomposition
- **XP infini** — Ajoute automatiquement de l'XP
- **One Hit Kill** — Met les PV des ennemis à 1
- **Cooldown compétences instant** — Supprime les cooldowns des Pals
- **Stats de base illimitées** — Max talents, support, vitesse de craft

### Cheats Palworld 1.0
- **Débloquer World Tree** — Débloque le contenu World Tree
- **Débloquer Awakening** — Débloque le donjon Awakening
- **Débloquer tours (boss)** — Débloque tous les rematchs de boss de tour

### Valeurs configurables
- **Multiplicateur de dégâts** — 1x à 99999x
- **Taux de régénération PV** — Vitesse de heal
- **Taux de satiété** — Vitesse de faim
- **Points de technologie** — Définir une valeur exacte
- **Points de technologie antique** — Définir une valeur exacte
- **Points de stats** — Points à distribuer
- **Niveau du joueur** — Définir le niveau
- **XP du joueur** — Définir l'XP
- **Rang du joueur** — Définir le rang
- **Montant d'effigies** — Nombre d'effigies Lifmunk
- **Montant d'objets** — Taille des stacks d'objets
- **Heure du monde** — Définir l'heure (0-23)
- **Vitesse du temps** — Multiplicateur (1x = normal)
- **Vitesse de déplacement** — Multiplicateur de vitesse
- **Hauteur de saut** — Multiplicateur de saut
- **Probabilité de Pal rare** — Taux d'apparition de Pals rares
- **Poids d'inventaire** — Capacité de transport
- **Vitesse de travail** — Multiplicateur de vitesse de craft
- **Multiplicateur de capture** — Force de capture

### Actions ponctuelles
- **Téléporter** — Clic droit sur la minimap (via commands.json → DLL → SetComponentLocation)
- **Déverrouiller voyages rapides** — Active tous les fast travel
- **Dégager la météo** — Nettoie la météo

## Minimap

### Fonctionnalités
- Carte interactive avec zoom et pan
- Position du joueur en temps réel (ReadProcessMemory)
- POIs (voyages rapides, tours, donjons, camps, etc.)
- Spawns de Pals (jour/nuit, alpha, niveau)
- Détection temps réel des Pals proches
- Détection temps réel des entités (coffres, oeufs, effigies, fruits de compétence)
- ESP avancé (tableau triable, lignes, noms, code couleur par distance)
- Favoris avec téléport
- Téléportation par clic droit

### Raccourcis clavier
- **Insert** — Afficher/cacher la minimap
- **F1** — Suivre le joueur
- **F2/F3** — Zoom +/-
- **F4** — Toujours au-dessus
- **F5** — Afficher les Pals
- **Clic droit** — Téléporter à la position cliquée
- **Shift+Clic droit** — Téléporter vers un POI/Pal

## Compilation

### Prérequis
- **MinGW-w64** (g++ 13+)
- **windres** (inclus avec MinGW)
- **make** (mingw32-make ou make)
- **Python 3** (pour les scripts et le serveur web)
- **ffmpeg** (pour la conversion d'icônes, optionnel)

### Build
```bash
make all
```

### Targets individuelles
```bash
make PalTrainerCore.dll       # DLL du trainer
make PalTrainerInjector.exe   # Injecteur
make PalOffsetScanner.exe     # Scanner d'offsets
make PalTrainerOverlay.exe    # Overlay complet
make PalTrainerMiniMap.exe    # Minimap autonome
make PalTrainerLauncher.exe   # Launcher centralisé
```

### Packaging
```bash
bash scripts/package.sh       # Crée dist/ + archive zip (nécessite `make all` au préalable)
```

## Structure du projet

```
src/trainer/        DLL du trainer (core, cheat, engine, injector)
src/overlay/        Overlay ImGui + minimap (overlay.cpp, imgui/)
src/launcher/       Launcher centralisé (launcher.cpp)
src/tools/          Offset scanner
scripts/            Scripts utilitaires (packaging, conversion, injection, breeding, save editor)
web/                Interface web interactive
server/             Serveur web local
data/               Données de jeu (pal_spawns.json, pal_database.json)
assets/             Icônes BMP + textures de carte (.rgba)
docs/               Documentation (ARCHITECTURE.md, CHEATS.md, MINIMAP.md)
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md) — Flux de données, structure du code
- [Cheats](docs/CHEATS.md) — Liste complète des cheats et hotkeys
- [Minimap](docs/MINIMAP.md) — Hotkeys, téléport, spawns, pals, filtres, ESP

## CI/CD

Le workflow GitHub Actions (`.github/workflows/build.yml` à la racine du repo):
1. **Build** — Compile tous les composants sur `windows-latest` avec MinGW-w64
2. **Package** — Crée `dist/` avec exécutables, données, assets, docs
3. **Upload** — Publie les artifacts de build
4. **Release** — Crée une release GitHub automatique sur tag `v*`

### Déclencher une release
```bash
git tag v1.0
git push origin v1.0
```
La CI build tous les composants, crée l'archive `PalTrainerUltra-*.zip` et publie la Release GitHub automatiquement avec les assets de carte.

## Scripts utilitaires

### Breeding calculator
```bash
# Calculer le child de deux parents
python scripts/breeding_calculator.py Lamball Cattiva

# Trouver les parents d'un Pal
python scripts/breeding_calculator.py --reverse Anubis

# Lister tous les Pals
python scripts/breeding_calculator.py --list
```

### Save editor
```bash
# Info sur la sauvegarde
python scripts/save_editor.py --info

# Modifier le niveau
python scripts/save_editor.py --set-level 50

# Modifier l'argent
python scripts/save_editor.py --set-money 999999

# Backup
python scripts/save_editor.py --backup
```

## Disclaimer

Outil éducatif pour usage en single-player et serveurs privés. Ne pas utiliser sur des serveurs publics avec anti-cheat.
