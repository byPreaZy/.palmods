# Mods UE4SS Lua

Tous les mods UE4SS Lua inclus dans PalTrainerUltra pour Palworld v1.0.2.

## Sommaire des touches

| Touche | Mod | Action |
|--------|-----|--------|
| F1 | PalCheatMenu | Ouvrir/fermer le menu |
| F2 | PalSpawner | Ouvrir/fermer le menu de spawn |
| F3 | PalCheatMenu | Toggle stamina infinie |
| F4 | PalCheatMenu | Toggle faim/soif illimitées |
| F5 | PalCheatMenu | Toggle sans dégâts de chute |
| F6 | PalCheatMenu | Toggle mode vol |
| F7 | PalCheatMenu | Toggle no-clip |
| F8 | PalCheatMenu | Toggle invisibilité |
| F9 | PalCheatMenu | Toggle super vitesse |
| F10 | PalCaptureRate | Toggle taux de capture 100% |
| F11 | PalCheatMenu | Toggle munitions illimitées |
| F12 | PalFastTravelAnywhere | Toggle fast travel partout |
| INS | PalCheatMenu | Toggle God Mode |
| NUM_LOCK | PalCheatMenu | Toggle super saut |
| PAUSE | PalCheatMenu | Activer/désactiver tous les cheats |
| CAPS_LOCK | PalAutoLoot | Toggle ramassage auto |
| SCROLL_LOCK | PalWeight | Toggle poids infini |
| END | PalInstantBreed | Toggle reproduction instantanée |
| PAGE_UP | PalInstantFish | Toggle pêche instantanée |
| PAGE_DOWN | PalInstantHatch | Toggle éclosion instantanée |
| HOME | PalInspectPal | Afficher infos du Pal visé |
| PRINT_SCREEN | PalCaptureCounter | Afficher compteur de captures |
| HELP | PalModManager | Ouvrir le gestionnaire de mods |
| NumPad0 | PalMiniMap | Afficher/cacher la minimap |
| NumPad1 | PalMiniMap | Mode déplacement |
| NumPad2 | PalMiniMap | Menu filtres |
| NumPad3 | PalMiniMap | Calibrer POI |
| NumPad4 | PalNoBuildingRestrictions | Toggle restrictions construction |
| NumPad5 | PalInfiniteDurability | Toggle durabilité infinie |
| NumPad6 | PalInfiniteAmmo | Toggle munitions infinies |
| NumPad7 | PalExpBoost | Toggle boost XP |
| NumPad8 | PalNoStaminaFly | Toggle vol sans stamina |
| NumPad9 | PalBaseExpansion | Toggle expansion base |
| NumPad+ | PalMiniMap | Zoom + |
| NumPad- | PalMiniMap | Zoom - |
| NumPad* | PalMiniMap | Super zoom |
| NumPad/ | PalMiniMap | Toggle rotation |

## Mods existants

### PalCheatMenu
- **Touche** : F1 (menu), INS (god mode), F3-F9, F11, NUM_LOCK, PAUSE
- **Fichier** : `mods/PalCheatMenu/`
- **Description** : Menu de triches complet avec 35+ cheats : God Mode, stamina infinie, vol, no-clip, super vitesse, super saut, munitions illimitées, capture 100%, spawn d'items, etc.
- **Config** : `Scripts/config.lua` — tous les keybinds, valeurs de gameplay, limites de sliders, cheats actifs par défaut

### PalMiniMap
- **Touche** : NumPad0 (toggle), NumPad1 (move), NumPad2 (filtres), NumPad3 (calibrer), NumPad+/- (zoom), NumPad* (super zoom), NumPad/ (rotation)
- **Fichier** : `mods/PalMiniMap/Prototype/`
- **Description** : Minimap ImGui avec carte, POIs, Pals, terrain heightmap, détection temps réel, filtres par type
- **Config** : `Scripts/config.lua` — touches, affichage, filtres, couleurs, portées

### PalAutoLoot
- **Touche** : CAPS_LOCK
- **Fichier** : `mods/PalAutoLoot/`
- **Description** : Ramassage automatique des objets au sol dans un rayon configurable
- **Config** : `Scripts/config.lua` — rayon, intervalle, classes d'objets

### PalWeight
- **Touche** : SCROLL_LOCK
- **Fichier** : `mods/PalWeight/`
- **Description** : Poids d'inventaire illimité (force la valeur max)
- **Config** : `Scripts/config.lua` — poids max, intervalle

### PalInstantBreed
- **Touche** : END
- **Fichier** : `mods/PalInstantBreed/`
- **Description** : Reproduction instantanée à la ferme d'élevage
- **Config** : `Scripts/config.lua` — intervalle

### PalInstantFish
- **Touche** : PAGE_UP
- **Fichier** : `mods/PalInstantFish/`
- **Description** : Pêche instantanée / automatique
- **Config** : `Scripts/config.lua` — auto-reel, auto-hook, intervalle

### PalInstantHatch
- **Touche** : PAGE_DOWN
- **Fichier** : `mods/PalInstantHatch/`
- **Description** : Éclosion instantanée des œufs
- **Config** : `Scripts/config.lua` — intervalle

### PalInspectPal
- **Touche** : HOME
- **Fichier** : `mods/PalInspectPal/`
- **Description** : Affiche les informations du Pal visé (nom, niveau, HP, stats)
- **Config** : `Scripts/config.lua` — taille fenêtre, rayon scan, intervalle

### PalCaptureCounter
- **Touche** : PRINT_SCREEN
- **Fichier** : `mods/PalCaptureCounter/`
- **Description** : Affiche le nombre de captures en temps réel
- **Config** : `Scripts/config.lua` — position overlay, rayon scan

### PalQuickDrop
- **Touche** : PRINT
- **Fichier** : `mods/PalQuickDrop/`
- **Description** : Dépôt rapide d'objets
- **Config** : `Scripts/config.lua` — itemId, quantité, slot

### PalQuickStack
- **Touche** : EXECUTE
- **Fichier** : `mods/PalQuickStack/`
- **Description** : Transfert rapide de stack vers un conteneur
- **Config** : `Scripts/config.lua` — itemId, quantité, slot

### PalModManager
- **Touche** : HELP
- **Fichier** : `mods/PalModManager/`
- **Description** : Fenêtre de configuration unifiée pour les mods Palworld
- **Config** : `Scripts/config.lua` — liste des mods gérés, taille fenêtre

## Nouveaux mods (v1.0.2)

### PalSpawner
- **Touche** : F2
- **Fichier** : `mods/PalSpawner/`
- **Description** : Menu ImGui complet pour spawner des Pals avec stats et passives custom
- **Fonctionnalités** :
  - Liste de 129 Pals avec noms internes, types, dex#
  - Recherche filtrable par nom/type
  - Niveau (1-100), HP, attaque, défense, stamina, genre
  - 4 slots de compétences passives parmi 115 passives
  - Spawn près du joueur ou ajout à la Palbox
- **Fichiers** :
  - `Scripts/main.lua` — logique de spawn, menu ImGui
  - `Scripts/config.lua` — touches, valeurs par défaut, limites
  - `Scripts/pal_list.lua` — base de données des Pals (nom, internalName, type)
  - `Scripts/passives.lua` — base de données des 115 passives

### PalNoBuildingRestrictions
- **Touche** : NumPad4
- **Fichier** : `mods/PalNoBuildingRestrictions/`
- **Description** : Supprime les restrictions de construction (angles, collisions)
- **Config** : `Scripts/config.lua` — angle marchable, application au démarrage

### PalInfiniteDurability
- **Touche** : NumPad5
- **Fichier** : `mods/PalInfiniteDurability/`
- **Description** : Durabilité infinie pour les armes et outils
- **Config** : `Scripts/config.lua` — intervalle de vérification

### PalInfiniteAmmo
- **Touche** : NumPad6
- **Fichier** : `mods/PalInfiniteAmmo/`
- **Description** : Munitions infinies pour toutes les armes à feu
- **Config** : `Scripts/config.lua` — intervalle de vérification

### PalExpBoost
- **Touche** : NumPad7
- **Fichier** : `mods/PalExpBoost/`
- **Description** : Multiplicateur d'XP configurable pour le joueur et les Pals
- **Config** : `Scripts/config.lua` — multiplicateur XP joueur, multiplicateur XP Pals

### PalNoStaminaFly
- **Touche** : NumPad8
- **Fichier** : `mods/PalNoStaminaFly/`
- **Description** : Vol sans consommation de stamina (FlyHover_SP, FlyHorizon_SP, etc. à 0)
- **Config** : `Scripts/config.lua` — toggle

### PalCaptureRate
- **Touche** : F10
- **Fichier** : `mods/PalCaptureRate/`
- **Description** : Taux de capture 100% ou configurable
- **Config** : `Scripts/config.lua` — taux de capture, probabilité Pal rare

### PalBaseExpansion
- **Touche** : NumPad9
- **Fichier** : `mods/PalBaseExpansion/`
- **Description** : Agrandit le rayon de construction de la Palbox (AreaRange)
- **Config** : `Scripts/config.lua` — AreaRange, application aux bases existantes

### PalFastTravelAnywhere
- **Touche** : F12
- **Fichier** : `mods/PalFastTravelAnywhere/`
- **Description** : Fast travel depuis n'importe où + déblocage de tous les points
- **Config** : `Scripts/config.lua` — déblocage au démarrage

## Structure d'un mod UE4SS

```
mods/<ModName>/
  Info.json              — métadonnées (nom, version, auteur, dépendances)
  Scripts/
    main.lua             — script principal
    config.lua           — configuration (keybinds, valeurs)
    <additional>.lua     — données ou modules supplémentaires
```

## Installation

1. Onglet **"Mods UE4SS"** dans PalTrainerUltra
2. Cliquer **"Installer UE4SS"** (installe le framework)
3. Sélectionner les mods désirés → **"Installer"**
4. Ou **"Tout installer"** pour installer tous les mods d'un coup
5. Les mods sont copiés vers `<Palworld>/Pal/Binaries/Win64/ue4ss/Mods/<ModName>/`

## Compatibilité

- **Palworld** : v1.0.2 (Steam)
- **UE4SS** : version compatible Palworld v1.0.2
- **Mode** : Single-player et serveurs privés (multi nécessite admin)
- **ImGui** : requis pour les mods avec menu (PalCheatMenu, PalSpawner, PalMiniMap, PalModManager)
