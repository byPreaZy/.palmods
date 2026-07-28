# PalMiniMap ULTRA MAX — Super minimap HUD pour Palworld

Deux versions sont fournies : un **prototype Lua ULTRA MAX** (solution principale, immediat, sans UE) et un **LogicMod** (optionnel/avance, necessite Unreal Engine 5.1).

## 1. Prototype Lua ULTRA MAX (solution principale)

Lancez-le des maintenant avec RE-UE4SS. Il affiche une **super minimap** en surimpression du HUD de Palworld 1.0 (Steam).

### Objectif

- **Rendu terrain** : heightmap/satellite/topographique de l'environnement autour du joueur, avec **teinte biome** et interpolation par echantillons d'entites pour plus de precision.
- **Donnees publiques Palworld 1.0** : fichier `assets_data.lua` avec **289 Pals** (Paldeck complet + variantes), types elementaires et biomes, **3600+ POI** (7 tours, 137 fast-travel, 170+ donjons, 18 sealed realms, 31+ bounty targets, 28 skill fruit trees, 28+ merchants, 140 relics/statues, 604+ lifmunk effigies, 2941+ resource nodes : charbon, soufre, quartz, chromite, etc.).
- **Pals colores par type** : chaque Pal prend la couleur de son type elementaire (Feu, Eau, Glace, Electrik, etc.).
- **Points d'interet (POI)** : tours de faction, fast-travel, sanctuaires, alpha boss et donjons affiches comme marqueurs supplementaires (coordonnees publiques).
- **Calibration in-game** : enregistre la position exacte du joueur pour remplacer les coordonnees d'un POI (touche `Insert`, sauvegarde dans `user_poi_overrides.lua`).
- **Police open-source Noto Sans** integree pour un rendu texte plus propre.
- **Selection des Pals** : menu pour choisir quels Pals afficher/masquer individuellement par espece.
- **Multijoueur** : autres joueurs affichés avec **fleches de couleurs** indiquant leur orientation.
- Affichage de : Pals amicaux/hostiles, coffres, oeufs, donjons, points de voyage rapide, ressources.
- Zoom normal + super zoom, rotation camera / nord fixe, grille, boussole, coordonnees + biome actuel.
- Menu de filtres ImGui, menu selecteur de Pals et menu de calibration, configuration complete via `Prototype/Scripts/config.lua`.

### Touches par defaut

- **M** : afficher/masquer
- **L** : mode deplacement/redimensionnement
- **Numpad + / -** : zoom
- **Numpad \*** : super zoom
- **Numpad /** : basculer rotation camera / nord
- **N** : menu filtres
- **Insert** : ouvrir/valider calibration POI

### Installation du prototype

1. Copiez `PalMiniMap/Prototype/` dans `Pal/Binaries/Win64/Mods/PalMiniMap/`.
2. Lancez Palworld.
3. Appuyez sur **M**.

### Configuration

Editez `Prototype/Scripts/config.lua` pour regler touches, opacite, zoom, filtres, couleurs, portees, resolution/portee du terrain et les couleurs biome.

## 2. LogicMod (optionnel / avance)

Si vous avez acces a Unreal Engine 5.1, vous pouvez cuisiner un `.pak` plus integre. Sinon, ignorez cette section.

### Prérequis

- **Unreal Engine 5.1** (version exacte requise par Palworld).
- **Palworld Modding Kit** (skeleton UE 5.1 communautaire).
- **Visual Studio 2022** avec C++ / MSVC v143.
- **.NET 6**.
- **Wwise 2021.1.11** (prérequis du Modding Kit).

## 3. Structure du projet

```
PalMiniMap/
├── Prototype/           <- Version Lua MAX principale
│   ├── Info.json
│   └── Scripts/
│       ├── main.lua
│       └── config.lua
├── PalMiniMap.uproject  <- Piste LogicMod (optionnel)
├── Config/
│   └── FilterPlugin.ini
├── build_pak.bat
└── build_pak.sh
```

## 4. Build LogicMod (optionnel)

Si vous disposez d'UE 5.1 + Palworld Modding Kit :
1. Ouvrir le **Palworld Modding Kit** (`Pal.uproject`).
2. Créer `Content/Mods/PalMiniMap/ModActor` (Blueprint Actor).
3. Créer `WBP_MiniMap` avec image de fond, marqueur joueur, canvas d'icones, slider opacite, bouton filtres.
4. `PrimaryAssetLabel` avec `Chunk ID` unique (ex: 201) et `Always Cook`.
5. Package Project puis renommer le `.pak` en `PalMiniMap_P.pak`.
6. Copier dans `Pal/Content/Paks/LogicMods/`.
7. Utilisez `build_pak.bat` ou `build_pak.sh` si configures.

## 5. Publication Nexus Mods

Pour publier sans UE :
- Incluez le dossier `Prototype/` comme mod Lua `PalMiniMap`.
- Le nom `PalMiniMap` dans `Info.json` suffit ; pas besoin de `.pak`.
