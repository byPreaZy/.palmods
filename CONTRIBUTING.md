# Contributing / Guide de contribution

---

## 🇫🇷 Français

Merci de votre intérêt pour contribuer à ce projet !

### Signaler un bug

1. Vérifiez que le bug n'est pas déjà signalé dans [Issues](../../issues)
2. Ouvrez une nouvelle issue avec :
   - Un titre clair et descriptif
   - Les étapes pour reproduire le bug
   - Le comportement attendu vs le comportement observé
   - Votre version de Palworld et votre OS

### Proposer une fonctionnalité

1. Ouvrez une issue avec le label `enhancement`
2. Décrivez la fonctionnalité et son utilité
3. Attendez une discussion avant de commencer le développement

### Soumettre une Pull Request

1. Forkez le dépôt
2. Créez une branche : `git checkout -b feature/ma-fonctionnalite`
3. Committez vos changements : `git commit -m "Ajout: ma fonctionnalité"`
4. Pushez : `git push origin feature/ma-fonctionnalite`
5. Ouvrez une Pull Request

### Conventions de code

- **Lua** : indentation 4 espaces, variables en `camelCase`
- **C++** : standard C++17, indentation 4 espaces, `PascalCase` pour les classes
- **Python** : PEP 8
- Commentez en français ou en anglais

### Structure du projet

- `mods/` — Mods Lua UE4SS
- `PalTrainerUltra/` — Trainer C++ (DLL, overlay, minimap, launcher)
- Scripts d'installation à la racine

---

## 🇬🇧 English

Thank you for your interest in contributing to this project!

### Reporting a bug

1. Check if the bug is already reported in [Issues](../../issues)
2. Open a new issue with:
   - A clear and descriptive title
   - Steps to reproduce the bug
   - Expected behavior vs actual behavior
   - Your Palworld version and OS

### Suggesting a feature

1. Open an issue with the `enhancement` label
2. Describe the feature and its usefulness
3. Wait for discussion before starting development

### Submitting a Pull Request

1. Fork the repository
2. Create a branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m "Add: my feature"`
4. Push: `git push origin feature/my-feature`
5. Open a Pull Request

### Code conventions

- **Lua**: 4-space indentation, `camelCase` variables
- **C++**: C++17 standard, 4-space indentation, `PascalCase` for classes
- **Python**: PEP 8
- Comment in French or English

### Project structure

- `mods/` — Lua UE4SS mods
- `PalTrainerUltra/` — C++ trainer (DLL, overlay, minimap, launcher)
- Installation scripts at the root
