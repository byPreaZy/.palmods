# Architecture

## Vue d'ensemble

```
PalTrainerUltra.exe (mode launcher)
  ├── Détection Palworld (polling)
  ├── Scan offsets → runtime_offsets.json
  ├── Injection PalTrainerCore.dll → Palworld.exe
  ├── Mod Manager UE4SS (scan, install, uninstall, enable/disable)
  └── Lance: Minimap (--minimap), Overlay (--overlay), Carte web, Scanner

Palworld.exe ←──injection─── PalTrainerUltra.exe (launcher mode)
     │
     ▼
PalTrainerCore.dll
  (cheat thread)
     │
     ▼
paltrainer.json ←──lecture─── PalTrainerUltra.exe (--overlay)
commands.json                       (ImGui overlay)
     │                               │
     ▼                               ├── Minimap (D3D11)
  Cheat execution                    ├── POIs (mapObjects.json)
  (memory write)                     ├── Pal Spawns (pal_spawns.json)
                                      ├── Player position (RPM)
                                      └── Teleport (commands.json → DLL)

PalTrainerUltra.exe --minimap = version autonome (lecture mémoire seule, pas de cheats)

Web (server.py) ←── HTTP ──→ Browser (index.html)
  Sert: carte, POIs, spawns, cheats, téléport
```

## Structure du code source

### src/trainer/ — DLL du trainer
- `core.cpp` — Thread travailleur, lit `paltrainer.json` et `commands.json`, exécute les cheats
- `cheat.cpp/.hpp` — Logique des cheats (God Mode, infinite stamina, etc.)
- `engine.cpp/.hpp` — Lecture/écriture mémoire du jeu (HP, position, stats)
- `sdk.hpp` — Structures SDK Unreal Engine (UWorld, APalCharacter, etc.)
- `types.hpp/.h` — Types de base (FVector, FRotator, etc.)
- `offsets.h` — Offsets statiques (peuvent être surchargés par `runtime_offsets.json`)

### src/overlay/ — Application unifiée (ImGui + D3D11)
- `overlay.cpp` — Point d'entrée unique, fenêtre ImGui, rendu D3D11, launcher, trainer, mod manager, modes runtime
- `overlay_minimap.inl` — Inclu par overlay.cpp; toute la logique minimap (POIs, spawns, pals, téléport, filtres)
- `imgui/` — ImGui vendored (version officielle)

### Mod Manager UE4SS (intégré au launcher)
- `FindPalworldPath()` — détection via process PID ou registry Steam
- `ScanAvailableMods()` — scanne `mods/` pour les dossiers avec `Info.json`
- `InstallMod()` / `UninstallMod()` — copie/suppression récursive + `enabled.txt` + `mods.txt`
- `ToggleMod()` — active/désactive un mod via `enabled.txt` et `mods.txt`
- `InstallUE4SSFromBundle()` — installe UE4SS depuis le bundle `UE4SS/` local
- UI : tab "Mods UE4SS" dans le launcher avec liste, badges de statut, boutons d'action

### src/tools/ — Outils
- `offset_scanner.cpp` — Scanner d'offsets AOB (GWorld, GObjects, FName, ProcessEvent)

### scripts/ — Scripts utilitaires
- `package.sh` — Packaging dist/ + archive zip (inclut mods/ et UE4SS/ depuis la racine du projet)
- `download_assets.sh` — Téléchargement des gros fichiers (.rgba, .webp) depuis GitHub Releases
- `convert_pois.py` — Conversion des données POI vers mapObjects.json
- `convert_textures.py` — Conversion des textures .webp → .rgba
- `extract_spawns.py` — Extraction des données de spawn depuis les fichiers du jeu
- `breeding_calculator.py` — Calculateur de reproduction (breeding power)
- `save_editor.py` — Éditeur de sauvegarde (niveau, argent, points tech)
- `start_map.bat` — Démarre le serveur web local (port 8765)

## Flux de données

1. **Offset Scanner** → `runtime_offsets.json` (offsets dynamiques)
2. **Launcher** → injecte `PalTrainerCore.dll` dans Palworld
3. **DLL** → lit `paltrainer.json` (cheats activés) + `commands.json` (commandes)
4. **Overlay** → lit position joueur via ReadProcessMemory, affiche minimap + UI
5. **Web** → sert la carte interactive, communique avec l'overlay via HTTP
6. **Mod Manager** → scanne `mods/`, installe/désinstalle dans `<Palworld>/Pal/Binaries/Win64/ue4ss/Mods/`

## Compilation

```bash
make all  # Compile tout depuis la racine
```

Targets individuelles:
- `make PalTrainerUltra.exe` — Application unifiée (launcher + overlay + minimap)
- `make PalTrainerCore.dll` — DLL du trainer
- `make PalOffsetScanner.exe` — Scanner d'offsets
