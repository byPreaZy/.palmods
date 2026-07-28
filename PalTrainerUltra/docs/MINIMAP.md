# Minimap

## Hotkeys

| Touche | Action |
|--------|--------|
| F1 | Activer/désactiver le suivi du joueur |
| F2 | Zoom + |
| F3 | Zoom - |
| F4 | Toggle toujours au-dessus |
| F5 | Afficher/cacher les Pals |
| Insert | Afficher/cacher la minimap |
| Clic droit | Téléporter le joueur à la position cliquée |
| Shift+Clic droit | Téléporter vers un POI/Pal |

## Onglets

### Carte
- Sélection de la qualité de carte (2048/4096/8192)
- Toggle carte World Tree
- Compass avec position et orientation du joueur
- Auto-zoom sur le joueur

### Pals
- Liste des Pals à proximité avec niveau et distance
- Filtres par type (alpha, prédateur, normal)
- Téléport vers un pal (shift+clic droit sur la minimap)

### Spawns
- Affichage des zones de spawn de Pals sur la carte
- Recherche par nom de Pal
- Filtres: jour/nuit, alpha/normal, niveau min/max
- Sélection d'un Pal spécifique pour filtrer ses spawns
- Tooltips avec nom, niveau, heure, coordonnées

### Filtres
- Toggle par type de POI: fast travel, tower, donjon, oeuf, trésor, boss, camp ennemi, oilrig, base
- Filtrer les POIs collectés/non-collectés

### Favoris
- Sauvegarder des positions favorites
- Téléport vers un favori (clic droit)

## Téléportation

- **Clic droit** sur la minimap: téléporte le joueur à la position cliquée
- **Shift+clic droit** sur un POI/Pal: téléporte vers cet objet
- La téléportation écrit `teleportToX/Y/Z` dans `commands.json`, lu par la DLL injectée qui applique les coordonnées via `SetComponentLocation` (écriture mémoire dans le processus)
- Nécessite l'injection de la DLL (`PalTrainerCore.dll`) — la minimap autonome sans injection ne peut pas téléporter

## Détection temps réel

L'onglet **Filtres** permet d'activer la détection temps réel des entités du monde:

- **Coffres** (or) — coffres au trésor
- **Oeufs** (jaune) — oeufs de Pal
- **Effigies** (violet) — effigies Lifmunk
- **Skillfruits** (vert) — fruits de compétence

Le rayon de scan est ajustable (1000-20000 unités). Les entités sont scannées toutes les 2 secondes via ReadProcessMemory.

## ESP avancé

L'onglet **Pals** inclut un panneau ESP avec:

- **Tableau triable** des entités proches (type, niveau, distance, position)
- **Code couleur par distance**: vert (proche), jaune (moyen), rouge (loin)
- **Lignes ESP** reliant le joueur aux pals détectés
- **Noms ESP** affichant le niveau à côté de chaque pal
- Indicateurs **Alpha** et **Boss** avec couleurs dédiées

## Préférences

Les préférences de la minimap sont sauvegardées dans `minimap_prefs.json`:
- Qualité de carte
- Zoom level
- Position de la fenêtre
- Filtres activés
- Favoris
