# Cheats & Hotkeys

## Cheats disponibles

| Cheat | Effet | Type |
|-------|-------|------|
| God Mode | Active `bIsDebugMuteki` et `IsImmortality`, régénère les PV | Toggle |
| PV infinis | Maintient les PV du joueur au maximum | Toggle |
| PS infinis | Maintient la stamina du joueur au maximum | Toggle |
| Faim infinie | Maintient la faim du joueur au maximum | Toggle |
| Santé mentale infinie | Maintient la SAN du joueur au maximum | Toggle |
| Poids infini | Fixe le poids de l'inventaire à une valeur configurable | Toggle |
| Super vitesse | Multiplie la vitesse de marche/sprint | Toggle |
| Super saut | Multiplie la hauteur de saut | Toggle |
| Mode vol | Grande vitesse de vol, gravité réduite | Toggle |
| No Clip | Traverser les murs | Toggle |
| Téléportation avant | Téléporte selon l'orientation de la caméra | Action |
| Pas de rechargement | Supprime le rechargement des armes | Toggle |
| Durabilité infinie | Empêche la perte de durabilité | Toggle |
| Capture instantanée | Les captures réussissent instantanément | Toggle |
| Capture toujours réussie | 100% de réussite de capture | Toggle |
| Pals rares | Force les spawns rares | Toggle |
| Super dégâts | Augmente les dégâts du joueur et des Pals | Toggle |
| Multiplicateur de dégâts | Multiplie les dégâts (configurable) | Toggle |
| Pas de requis artisanat | Désactive les coûts en matériaux | Toggle |
| Pas de requis construction | Désactive les coûts de construction | Toggle |
| Artisanat instantané | Supprime les timers de fabrication | Toggle |
| Argent illimité | Maintient l'argent au maximum | Toggle |
| Arrêter le temps | Gèle l'heure du monde | Toggle |
| Régler l'heure | Définit l'heure du monde | Action |
| Avancer les heures | Fait avancer le temps | Action |
| Pas de signalement de crimes | Empêche la augmentation du niveau de recherche | Toggle |
| Température normale | Neutralise les effets de température | Toggle |
| Définir niveau | Définit le niveau du joueur | Action |
| Définir XP | Définit l'XP du joueur | Action |
| Définir rang | Définit le rang du joueur | Action |
| Points de stats | Définit les points de stats | Action |
| Points de technologie | Définit les points de technologie | Action |
| Tous les objets | Remplit tous les emplacements d'objets | Action |
| Effigies Lifmunk | Définit le nombre d'effigies | Action |
| Déverrouiller voyages rapides | Déverrouille tous les fast travel | Action |
| Dégager la météo | Nettoie la météo | Action |
| Bouclier infini | Maintient le bouclier au maximum | Toggle |
| Mode furtif | Réduit la détection des ennemis | Toggle |
| Taux de drop 100% | Force le taux de drop au maximum | Toggle |
| Nourriture non périssable | Empêche la décomposition des aliments | Toggle |
| XP infini | Ajoute automatiquement de l'XP | Toggle |
| One Hit Kill | Met les PV des ennemis à 1 | Toggle |
| Cooldown compétences instant | Supprime les cooldowns des Pals | Toggle |
| Stats de base illimitées | Max talents, support, vitesse de craft | Toggle |
| Débloquer World Tree | Débloque le contenu World Tree | Toggle |
| Débloquer Awakening | Débloque le donjon Awakening | Toggle |
| Débloquer tours (boss) | Débloque tous les rematchs de boss de tour | Toggle |

## Offsets

Les offsets statiques sont définis dans `src/trainer/offsets.h` et proviennent de:
- Un scan RVA local de `Palworld-Win64-Shipping.exe` 1.0.1
- `CXXHeaderDump/Pal.hpp` public de `DrRak72/Palworld-Modding-Reference`

Principaux RVAs statiques:
- `GWorld` — `0x965B1E0`
- `GObject` — `0x94EC890`
- `FName` — `0x34C08C4`
- `ProcessEvent` — `0x3645020`

Les offsets dynamiques peuvent être scannés avec `PalOffsetScanner.exe` et sauvegardés dans `runtime_offsets.json`.
