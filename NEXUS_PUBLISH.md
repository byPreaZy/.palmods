# Texte pret a copier sur Nexus Mods

## Titre

Palworld Mods ULTRA MAX — Cheat Menu + Super Minimap (Lua, no .pak needed)

## Summary

Pack de deux mods Lua pour **Palworld 1.0 (Steam)** utilisant **RE-UE4SS**.
Aucun Unreal Engine ni `.pak` requis : copiez les dossiers dans `Pal/Binaries/Win64/ue4ss/Mods/` et jouez.

- **PalCheatMenu ULTRA MAX** : menu de triches ImGui complet avec onglets, sous-onglets, presets PvP/Builder, 35+ cheats, spawn d'items et +niveaux.
- **PalMiniMapPrototype ULTRA MAX** : super minimap avec rendu terrain/heightmap, selection des Pals, fleches de couleurs multijoueur, filtres et zooms.

## Description

### PalCheatMenu ULTRA MAX

- **Joueur** : God Mode, endurance infinie, faim/soif infinies, pas de degats de chute, fly, no clip, invisibilite, super vitesse (x1-x20), super saut (500-20 000).
- **Combat** : munitions infinies, pas de rechargement, one-hit kill, no recoil, no spread, rapid fire, taux de capture max.
- **Inventaire / Craft** : objets infinis, craft instantane, ignorer couts, dupliquer items, durabilite infinie.
- **Pals** : Pal God Mode, sanity infinie, eclosion instantanee, stats max, travail ultra rapide, pas faim.
- **Monde** : +XP (jusqu'a 10 M), +points tech, +niveaux, spawn d'items, freeze time, regler l'heure, unlock fast travel, teleportation, meteo claire, reveler carte.
- **UI** : fenetre ImGui avec onglets, sous-onglets, sliders, boutons "Tout activer / Tout desactiver", presets PvP et Builder.

### PalMiniMapPrototype ULTRA MAX

- **Rendu terrain** : heightmap/satellite/topographique avec **teinte biome** et interpolation par echantillons d'entites pour plus de precision.
- **Donnees publiques Palworld 1.0** : `assets_data.lua` avec **289 Pals** (Paldeck complet + variantes), types elementaires et biomes, **3600+ POI** (7 tours, 137 fast-travel, 170+ donjons, 18 sealed realms, 31+ bounty targets, 28 skill fruit trees, 28+ merchants, 140 relics/statues, 604+ lifmunk effigies, 2941+ resource nodes : charbon, soufre, quartz, chromite, etc.).
- **Pals colores par type** : chaque Pal affiche avec la couleur de son type elementaire.
- **Points d'interet (POI)** : tours de faction, fast-travel, sanctuaires, alpha boss et donjons.
- **Calibration in-game** : enregistre la position exacte du joueur pour remplacer les coordonnees d'un POI (touche `Insert`).
- **Police open-source Noto Sans** integree pour un rendu texte plus propre.
- **Selection des Pals** : menu pour choisir quels Pals afficher/masquer individuellement.
- **Multijoueur** : autres joueurs affiches avec fleches de couleurs indiquant leur orientation.
- Filtres : Pals (amicaux/hostiles), coffres, oeufs, donjons, fast travel, ressources.
- Zoom normal 0.1-10 + super zoom, rotation camera / nord fixe, grille, boussole, coordonnees + biome actuel.
- Menus ImGui de filtres, selection des Pals et calibration.

## Installation

1. Installez **RE-UE4SS** dans `Pal/Binaries/Win64` (dossier `ue4ss/` + `dwmapi.dll`).
2. Téléchargez et décompressez l'archive.
3. Copiez les dossiers `PalCheatMenu` et `PalMiniMapPrototype` dans :
   `Pal/Binaries/Win64/ue4ss/Mods/`
4. Lancez Palworld.
5. Appuyez sur **F1** pour le menu de triches, **M** pour la minimap.

## Raccourcis principaux

- F1 : menu cheat
- F2-F10 / F11-F12 : cheats joueur et combat
- Insert, Delete, PageUp, PageDown, Home, End : cheats avances
- R, S, F, D, V, C, W, H, L, P : no recoil, no spread, rapid fire, durabilite, reveler map, capture, Pals rapides, Pals pas faim, +niveaux, spawn item
- NumPad 0-9, NumPad . : Pals, monde, progression, meteo
- M : minimap
- L : deplacer/redimensionner la minimap
- Numpad +/-/*// : zoom, super zoom, rotation
- N : menu filtres minimap

## Avertissements

- Utilisez en **solo** ou sur un serveur dont vous êtes administrateur.
- Les cheats peuvent provoquer des instabilités après les mises à jour du jeu.
- Fourni à des fins éducatives et de divertissement personnel.

## Permissions

- Pas de réupload sans crédit.
- Pas de vente.
- Modifiez `config.lua` à votre convenance.

## Tags recommandes

Palworld, UE4SS, Cheat, Menu, Minimap, HUD, Lua, MAX

## Version

1.0.0
