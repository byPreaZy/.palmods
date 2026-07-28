# Guide d'installation et de test — Palworld Mods ULTRA MAX

Ce guide explique comment installer automatiquement ou manuellement les mods Palworld ULTRA MAX (`PalCheatMenu`, `PalMiniMapPrototype`, `PalModManager` et les nouveaux mods Lua), comment tester l'installation et comment résoudre les problèmes courants.

---

## Prérequis

- **Palworld** installé via Steam (version 1.0 ou ultérieure).
- Un accès Internet pour télécharger **RE-UE4SS** (fait automatiquement par l'installateur).
- Windows 10/11 ou WSL2/Linux pour les utilisateurs avancés.

---

## Installation automatique (recommandée)

### Windows

1. Téléchargez et décompressez la dernière release (`PalworldMods-v*.zip`) depuis [Releases](../../releases).
2. Ouvrez le dossier décompressé.
3. Double-cliquez sur **`install.bat`**.
   - L'installateur détecte automatiquement Palworld :
     - via les raccourcis du bureau,
     - en cherchant `Palworld.exe` sur tous les disques durs,
     - via les librairies Steam (`libraryfolders.vdf`).
   - Il fonctionne même si le jeu est sur `D:`, `E:` ou un autre disque.
4. L'installateur :
   - Vérifie la présence de `UE4SS.dll` dans `Pal/Binaries/Win64/ue4ss/`.
   - Télécharge et extrait automatiquement la **dernière build RE-UE4SS** depuis GitHub (tag `experimental-latest`).
   - Le bundle local n'est utilisé qu'en `--offline` ou si le téléchargement échoue.
   - Sauvegarde l'ancien `UE4SS-settings.ini` avant de le remplacer.
   - Copie tous les mods `Pal*` détectés dans `Pal/Binaries/Win64/ue4ss/Mods/`.
   - Préserve vos réglages `config.lua` à chaque réinstallation (fusion avec les valeurs par défaut du zip).
5. Une fois terminé, lancez Palworld.

### Si `install.bat` ne fonctionne pas

1. Verifiez que `install-report.txt` a bien ete cree dans le dossier du zip ; il contient le detail exact de l'erreur.
2. Assurez-vous que Python est installe (le .bat essaie `python`, `py` et `python3`).
3. En dernier recours, lancez `install.py` dans un terminal cmd :
   ```cmd
   python install.py --dry-run
   python install.py
   ```

### Note sur la liste des mods du jeu

Ces mods utilisent **UE4SS** (injection Lua). Ils ne s'affichent **pas** dans la liste officielle de gestion des mods de Palworld (Steam Workshop) — ce n'est pas un bug. Ils fonctionnent en jeu via les touches **F1** (menu cheat), **M** (minimap), **L** (déplacer/redimensionner), **N** (filtres minimap) et **F10** (`PalModManager`).

### WSL / Linux

1. Téléchargez et décompressez le zip.
2. Ouvrez un terminal dans le dossier décompressé.
3. Exécutez :
   ```bash
   ./install.sh
   ```
4. Si votre installation Steam n'est pas à l'emplacement par défaut :
   ```bash
   ./install.sh "/mnt/d/Steam/steamapps/common/Palworld"
   # ou
   PALWORLD_DIR="/mnt/d/Steam/steamapps/common/Palworld" ./install.sh
   ```

---

## Utilisation avec Python (alternative)

Si Python est installé, vous pouvez utiliser l'installateur Python pour plus de contrôle :

```bash
# Windows
python install.py

# WSL/Linux
python3 install.py
```

Options utiles :

```bash
python3 install.py --palworld-dir "/mnt/c/.../Palworld"
python3 install.py --skip-ue4ss        # Ne pas télécharger UE4SS
python3 install.py --dry-run             # Simuler sans rien modifier
python3 install.py --source-dir .      # Dossier source des mods
```

### Simulation / dry-run

Avant d'installer pour de vrai, vous pouvez prévisualiser toutes les actions :

- **Windows** : `install.bat --dry-run`
- **Python (Windows/Linux)** : `python3 install.py --dry-run --palworld-dir /chemin/vers/Palworld`
- **Shell (WSL/Linux)** : `./install.sh --dry-run`

Aucun fichier n'est modifié en mode dry-run.

---

## Installation manuelle

Si l'installateur automatique échoue :

1. Téléchargez **RE-UE4SS** : https://github.com/UE4SS-RE/RE-UE4SS/releases/latest
2. Extrayez le contenu de `UE4SS_vX.Y.Z.zip` dans :
   ```
   Palworld/Pal/Binaries/Win64/
   ```
   Vous devez obtenir `Palworld/Pal/Binaries/Win64/ue4ss/UE4SS.dll` et `dwmapi.dll` dans `Pal/Binaries/Win64/`.
3. Copiez les dossiers du mod dans :
   ```
   Palworld/Pal/Binaries/Win64/ue4ss/Mods/
   ```
   - `PalCheatMenu/`
   - `PalMiniMapPrototype/`
   - `PalModManager/`
   - `PalWeight/`
   - `PalAutoLoot/`
   - `PalCaptureCounter/`
   - `PalInspectPal/`
   - `PalQuickDrop/`
   - `PalQuickStack/`
   - `PalInstantFish/`
   - `PalInstantHatch/`
   - `PalInstantBreed/`
4. Lancez Palworld.

---

## Test de l'installation

Après l'installation automatique, exécutez le test :

### Windows

```cmd
test_install.py
```

Ou double-cliquez sur `test_install.py` si Python est associé aux fichiers `.py`.

### WSL / Linux

```bash
python3 test_install.py
```

`test_install.py` vérifie :
- La présence de `UE4SS.dll`.
- La présence de tous les dossiers de mods `Pal*` installés.
- La présence des fichiers clés (`Info.json`, `main.lua`, `config.lua`, `assets_data.lua`).
- L'absence d'erreurs de syntaxe Lua.
- Les erreurs éventuelles dans `UE4SS.log` liées aux mods.

### Test in-game

1. Lancez Palworld.
2. Chargez une partie (solo ou serveur dont vous êtes administrateur).
3. Appuyez sur **F1** : le menu de triches `PalCheatMenu` doit s'afficher.
4. Appuyez sur **M** : la minimap `PalMiniMap` doit s'afficher.
5. Appuyez sur **N** pour ouvrir le menu des filtres de la minimap.
6. Appuyez sur **F10** : le gestionnaire `PalModManager` doit s'ouvrir et permettre de configurer les mods.
7. Si la minimap est trop grande, appuyez sur **L** pour entrer en mode déplacement/redimensionnement.

---

## Raccourcis principaux

| Touche | Effet |
|--------|-------|
| F1 | Ouvrir/fermer le menu de triches |
| M | Afficher/masquer la minimap |
| L | Déplacer/redimensionner la minimap |
| N | Menu filtres de la minimap |
| F10 | Ouvrir/fermer `PalModManager` |
| Numpad + / - | Zoom de la minimap |
| Numpad * | Super zoom |
| Numpad / | Basculer rotation caméra / nord fixe |

Voir `README.md` pour la liste complète des raccourcis.

---

## Dépannage

### L'installateur ne trouve pas Palworld

Définissez manuellement le chemin :

- **Windows** : modifiez `install.bat` ou lancez `cmd` :
  ```cmd
  set PALWORLD_DIR=D:\Steam\steamapps\common\Palworld
  install.bat
  ```
- **WSL/Linux** :
  ```bash
  PALWORLD_DIR="/mnt/d/Steam/steamapps/common/Palworld" ./install.sh
  ```

### UE4SS ne se télécharge pas

Vérifiez votre connexion Internet, puis téléchargez manuellement :
https://github.com/UE4SS-RE/RE-UE4SS/releases/latest

### Le menu F1 ou la minimap M ne s'affichent pas

1. Vérifiez que `UE4SS.dll` est bien dans `Pal/Binaries/Win64/`.
2. Vérifiez que les dossiers de mods (`PalCheatMenu`, `PalMiniMapPrototype`, `PalModManager`, etc.) sont bien dans `Pal/Binaries/Win64/Mods/`.
3. Lancez `test_install.py` pour identifier le problème.
4. Consultez `Pal/Binaries/Win64/UE4SS.log` pour les messages d'erreur Lua.

### Palworld est dans Program Files et l'installateur demande des droits

Lancez `install.bat` ou `install.sh` en tant qu'administrateur, ou copiez manuellement les fichiers avec les droits appropriés.

---

## Désinstallation

### Automatique

Les désinstallateurs détectent automatiquement tous les mods `Pal*` installés et ne suppriment **jamais** vos sauvegardes.

- **Windows** : double-cliquez sur **`uninstall.bat`**. Pour aussi supprimer UE4SS : `uninstall.bat --remove-ue4ss`
- **WSL/Linux** : `./uninstall.sh`. Pour aussi supprimer UE4SS : `./uninstall.sh --remove-ue4ss`
- **PowerShell** : `powershell -ExecutionPolicy Bypass -File uninstall.ps1 -RemoveUE4SS`

Pour **simuler** la désinstallation sans rien effacer, ajoutez `--dry-run` (uninstall.py / uninstall.sh) ou `-DryRun` (uninstall.ps1) :

```bash
python3 uninstall.py --dry-run
./uninstall.sh --dry-run
powershell -ExecutionPolicy Bypass -File uninstall.ps1 -DryRun
```

### Manuelle

Supprimez les dossiers suivants si vous préférez le faire à la main :

```
Palworld/Pal/Binaries/Win64/Mods/PalCheatMenu
Palworld/Pal/Binaries/Win64/Mods/PalMiniMapPrototype
Palworld/Pal/Binaries/Win64/Mods/PalModManager
Palworld/Pal/Binaries/Win64/Mods/PalWeight
Palworld/Pal/Binaries/Win64/Mods/PalAutoLoot
Palworld/Pal/Binaries/Win64/Mods/PalCaptureCounter
Palworld/Pal/Binaries/Win64/Mods/PalInspectPal
Palworld/Pal/Binaries/Win64/Mods/PalQuickDrop
Palworld/Pal/Binaries/Win64/Mods/PalQuickStack
Palworld/Pal/Binaries/Win64/Mods/PalInstantFish
Palworld/Pal/Binaries/Win64/Mods/PalInstantHatch
Palworld/Pal/Binaries/Win64/Mods/PalInstantBreed
```

Et, si vous souhaitez aussi supprimer RE-UE4SS, retirez les fichiers qu'il a ajoutés dans `Pal/Binaries/Win64/`.
