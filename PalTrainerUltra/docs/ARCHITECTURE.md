# Architecture

## Vue d'ensemble

```
PalTrainerLauncher.exe
  ├── Détection Palworld (polling)
  ├── Scan offsets → runtime_offsets.json
  ├── Injection PalTrainerCore.dll → Palworld.exe
  └── Lance: Overlay, Minimap, Carte web, Scripts

Palworld.exe ←──injection─── PalTrainerInjector.exe (ou Launcher)
     │
     ▼
PalTrainerCore.dll
  (cheat thread)
     │
     ▼
paltrainer.json ←──lecture─── PalTrainerOverlay.exe
commands.json                       (ImGui overlay)
     │                               │
     ▼                               ├── Minimap (D3D11)
  Cheat execution                    ├── POIs (mapObjects.json)
  (memory write)                     ├── Pal Spawns (pal_spawns.json)
                                      ├── Player position (RPM)
                                      └── Teleport (commands.json → DLL)

PalTrainerMiniMap.exe = version autonome (lecture mémoire seule, pas de cheats)

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
- `injector.cpp` — Source de `PalTrainerInjector.exe` (CreateRemoteThread)

### src/overlay/ — Overlay ImGui + Minimap
- `overlay.cpp` — Point d'entrée principal, fenêtre ImGui, rendu D3D11, chargement des cheats
- `overlay_minimap.inl` — Inclu par overlay.cpp; toute la logique minimap (POIs, spawns, pals, téléport, filtres)
- `minimap.cpp` — Point d'entrée alternatif pour `PalTrainerMiniMap.exe` (définit `PALTRAINER_MINIMAP_ONLY`)
- `imgui/` — ImGui vendored (version officielle)

### src/tools/ — Outils
- `offset_scanner.cpp` — Scanner d'offsets AOB (GWorld, GObjects, FName, ProcessEvent)

### src/launcher/ — Launcher centralisé
- `launcher.cpp` — Application ImGui autonome qui détecte Palworld, injecte la DLL, et lance les composants (overlay, minimap, carte web, scripts)

### scripts/ — Scripts utilitaires
- `package.sh` — Packaging dist/ + archive zip (la compilation est faite séparément par `make all`)
- `download_assets.sh` — Téléchargement des gros fichiers (.rgba, .webp) depuis GitHub Releases
- `convert_pois.py` — Conversion des données POI vers mapObjects.json
- `convert_textures.py` — Conversion des textures .webp → .rgba
- `extract_spawns.py` — Extraction des données de spawn depuis les fichiers du jeu
- `breeding_calculator.py` — Calculateur de reproduction (breeding power)
- `save_editor.py` — Éditeur de sauvegarde (niveau, argent, points tech)
- `inject.bat` — Injection DLL (admin requis)
- `start_map.bat` — Démarre le serveur web local (port 8765)

## Flux de données

1. **Offset Scanner** → `runtime_offsets.json` (offsets dynamiques)
2. **Injector** → injecte `PalTrainerCore.dll` dans Palworld
3. **DLL** → lit `paltrainer.json` (cheats activés) + `commands.json` (commandes)
4. **Overlay** → lit position joueur via ReadProcessMemory, affiche minimap + UI
5. **Web** → sert la carte interactive, communique avec l'overlay via HTTP

## Compilation

```bash
make all  # Compile tout depuis la racine
```

Targets individuelles:
- `make PalTrainerCore.dll` — DLL du trainer
- `make PalTrainerInjector.exe` — Injecteur
- `make PalOffsetScanner.exe` — Scanner d'offsets
- `make PalTrainerOverlay.exe` — Overlay complet
- `make PalTrainerMiniMap.exe` — Minimap autonome
- `make PalTrainerLauncher.exe` — Launcher centralisé
