#!/usr/bin/env python3
"""config_manager.py — Fenetre externe de configuration des mods Palworld ULTRA MAX.

Permet d'editer les fichiers `mods/<mod>/Scripts/config.lua` hors du jeu.
Lancez ce script depuis la racine du zip release ou du projet.
"""

from __future__ import annotations

import re
import shutil
import sys
from pathlib import Path
from typing import Any

try:
    import tkinter as tk
    from tkinter import messagebox, ttk
except Exception as exc:  # pragma: no cover
    print(f"[config_manager] Tkinter non disponible : {exc}")
    sys.exit(1)


BOOL_RE = re.compile(r"^(true|false)$", re.IGNORECASE)
NIL_RE = re.compile(r"^nil$", re.IGNORECASE)
NUM_RE = re.compile(r"^-?\d+(?:\.\d+)?$")
STRING_RE = re.compile(r'^("((?:[^"\\]|\\.)*)"|\'((?:[^\'\\]|\\.)*)\')$')
COMMENT_BLOCK_RE = re.compile(r"--\[\[.*?\]\]", re.DOTALL)
COMMENT_LINE_RE = re.compile(r"--[^\n]*")
ASSIGN_RE = re.compile(
    r"""
    (\w+)\s*=\s*(
        true|false|nil
        |-?\d+(?:\.\d+)?
        |"(?:[^"\\]|\\.)*"
        |'(?:[^'\\]|\\.)*'
    )
    """,
    re.IGNORECASE | re.VERBOSE,
)


def _unescape(s: str) -> str:
    """Retourne une chaine sans les echappements Python/Lua basiques."""
    return s.encode("utf-8").decode("unicode_escape")


def parse_lua_config(text: str) -> dict[str, Any]:
    """Parse un `config.lua` simplifie (return { cle = valeur, ... })."""
    text = COMMENT_BLOCK_RE.sub("", text)
    text = COMMENT_LINE_RE.sub("", text)
    cfg: dict[str, Any] = {}
    for m in ASSIGN_RE.finditer(text):
        key = m.group(1)
        raw = m.group(2).strip()
        low = raw.lower()
        if low == "true":
            cfg[key] = True
        elif low == "false":
            cfg[key] = False
        elif low == "nil":
            cfg[key] = None
        elif raw.startswith('"') and raw.endswith('"'):
            cfg[key] = _unescape(raw[1:-1])
        elif raw.startswith("'") and raw.endswith("'"):
            cfg[key] = _unescape(raw[1:-1])
        elif NUM_RE.match(raw):
            cfg[key] = int(raw) if "." not in raw else float(raw)
    return cfg


def _escape_string(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def serialize_lua_config(cfg: dict[str, Any]) -> str:
    """Genere un `config.lua` simplifie."""
    lines = ["return {"]
    # On garde un ordre stable pour la lisibilite
    for key in sorted(cfg.keys()):
        value = cfg[key]
        if value is None:
            lines.append(f"    {key} = nil,")
        elif isinstance(value, bool):
            lines.append(f"    {key} = {str(value).lower()},")
        elif isinstance(value, int):
            lines.append(f"    {key} = {value},")
        elif isinstance(value, float):
            lines.append(f"    {key} = {value},")
        elif isinstance(value, str):
            lines.append(f'    {key} = "{_escape_string(value)}",')
        else:
            # Types complexes ignores (nested tables)
            lines.append(f"    {key} = nil,")
    lines.append("}")
    return "\n".join(lines) + "\n"


def find_mods(root: Path) -> dict[str, Path]:
    """Trouve tous les mods de la forme `mods/Pal*/Scripts/config.lua`."""
    mods: dict[str, Path] = {}
    mods_dir = root / "mods"
    if not mods_dir.exists():
        return mods
    for subdir in sorted(mods_dir.iterdir()):
        if not subdir.is_dir():
            continue
        name = subdir.name
        config_path = subdir / "Scripts" / "config.lua"
        if config_path.exists():
            mods[name] = config_path
            continue
        # Support du layout dev `PalMiniMap/Prototype/Scripts/config.lua`
        if name == "PalMiniMap":
            prototype = subdir / "Prototype" / "Scripts" / "config.lua"
            if prototype.exists():
                mods["PalMiniMapPrototype"] = prototype
    return mods


class PalModConfigGUI:
    def __init__(self, root_dir: Path) -> None:
        self.root_dir = root_dir
        self.mods: dict[str, Path] = find_mods(root_dir)
        self.current_mod: str | None = None
        self.current_cfg: dict[str, Any] = {}
        self.variables: dict[str, tk.Variable] = {}

        self.window = tk.Tk()
        self.window.title("Palworld Mod Config Manager")
        self.window.geometry("650x500")
        self.window.minsize(500, 350)

        self._build_ui()

        if self.mods:
            # Selectionner le premier mod
            first = list(self.mods.keys())[0]
            self.mod_listbox.selection_set(0)
            self._on_mod_select()
        else:
            messagebox.showwarning("Aucun mod", f"Aucun config.lua trouve dans {root_dir}")

    def _build_ui(self) -> None:
        # Panneau gauche : liste des mods
        left = tk.Frame(self.window, width=200)
        left.pack(side=tk.LEFT, fill=tk.Y, padx=5, pady=5)

        tk.Label(left, text="Mods", font=("Arial", 10, "bold")).pack(anchor=tk.W)
        self.mod_listbox = tk.Listbox(left, exportselection=0)
        self.mod_listbox.pack(fill=tk.BOTH, expand=True)
        for name in sorted(self.mods.keys()):
            self.mod_listbox.insert(tk.END, name)
        self.mod_listbox.bind("<<ListboxSelect>>", lambda _: self._on_mod_select())

        reload_btn = tk.Button(left, text="Recharger la liste", command=self._reload_mods)
        reload_btn.pack(fill=tk.X, pady=(5, 0))

        # Panneau droit : editeur
        right = tk.Frame(self.window)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.mod_title = tk.Label(right, text="", font=("Arial", 12, "bold"))
        self.mod_title.pack(anchor=tk.W, pady=(0, 5))

        canvas = tk.Canvas(right, highlightthickness=0)
        scrollbar = ttk.Scrollbar(right, orient=tk.VERTICAL, command=canvas.yview)
        self.fields_frame = tk.Frame(canvas)

        self.fields_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all")),
        )
        canvas.create_window((0, 0), window=self.fields_frame, anchor=tk.NW)
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Boutons bas
        btn_frame = tk.Frame(self.window)
        btn_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=5)
        tk.Button(btn_frame, text="Sauvegarder", command=self._save).pack(side=tk.RIGHT, padx=5)
        tk.Button(btn_frame, text="Recharger", command=self._on_mod_select).pack(side=tk.RIGHT, padx=5)

    def _reload_mods(self) -> None:
        self.mods = find_mods(self.root_dir)
        self.mod_listbox.delete(0, tk.END)
        for name in sorted(self.mods.keys()):
            self.mod_listbox.insert(tk.END, name)
        if self.mods:
            self.mod_listbox.selection_set(0)
            self._on_mod_select()

    def _on_mod_select(self) -> None:
        selection = self.mod_listbox.curselection()
        if not selection:
            return
        index = selection[0]
        name = self.mod_listbox.get(index)
        if name not in self.mods:
            return

        self.current_mod = name
        path = self.mods[name]
        try:
            text = path.read_text(encoding="utf-8")
            self.current_cfg = parse_lua_config(text)
        except Exception as exc:
            messagebox.showerror("Erreur de lecture", f"Impossible de lire {path}:\n{exc}")
            return

        self.mod_title.config(text=name)
        self._build_fields()

    def _build_fields(self) -> None:
        # Nettoie l'ancien contenu
        for widget in self.fields_frame.winfo_children():
            widget.destroy()
        self.variables = {}

        if not self.current_cfg:
            tk.Label(self.fields_frame, text="Aucune option lisible.").pack(anchor=tk.W)
            return

        # Trier : enabled, keybind, puis le reste
        ordered_keys = sorted(self.current_cfg.keys(), key=lambda k: (k != "enabled", k != "keybind", k.lower() != "modname", k))
        for key in ordered_keys:
            value = self.current_cfg[key]
            frame = tk.Frame(self.fields_frame)
            frame.pack(fill=tk.X, pady=2)

            tk.Label(frame, text=key, width=18, anchor=tk.W).pack(side=tk.LEFT)

            if isinstance(value, bool):
                var = tk.BooleanVar(value=value)
                tk.Checkbutton(frame, variable=var).pack(side=tk.LEFT)
            elif isinstance(value, (int, float)):
                var = tk.StringVar(value=str(value))
                tk.Entry(frame, textvariable=var).pack(side=tk.LEFT, fill=tk.X, expand=True)
            elif value is None:
                var = tk.StringVar(value="nil")
                tk.Entry(frame, textvariable=var, state="readonly").pack(side=tk.LEFT, fill=tk.X, expand=True)
            else:
                var = tk.StringVar(value=str(value))
                tk.Entry(frame, textvariable=var).pack(side=tk.LEFT, fill=tk.X, expand=True)

            self.variables[key] = var

    def _save(self) -> None:
        if not self.current_mod or not self.current_cfg:
            return
        path = self.mods[self.current_mod]

        for key, var in self.variables.items():
            raw = var.get()
            original = self.current_cfg.get(key)
            if isinstance(original, bool):
                self.current_cfg[key] = str(raw).lower() in ("1", "true", "yes", "on")
            elif isinstance(original, int):
                try:
                    self.current_cfg[key] = int(raw)
                except ValueError:
                    messagebox.showerror("Valeur invalide", f"{key} doit etre un entier.")
                    return
            elif isinstance(original, float):
                try:
                    self.current_cfg[key] = float(raw)
                except ValueError:
                    messagebox.showerror("Valeur invalide", f"{key} doit etre un nombre.")
                    return
            else:
                self.current_cfg[key] = raw

        # Sauvegarde de l'ancien fichier
        backup = path.with_suffix(path.suffix + ".bak")
        try:
            shutil.copy2(path, backup)
        except Exception:
            pass

        try:
            path.write_text(serialize_lua_config(self.current_cfg), encoding="utf-8")
            messagebox.showinfo("Sauvegarde", f"{self.current_mod}/Scripts/config.lua mis a jour.")
        except Exception as exc:
            messagebox.showerror("Erreur d'ecriture", f"Impossible d'ecrire {path}:\n{exc}")

    def run(self) -> None:
        self.window.mainloop()


def main() -> None:
    root_dir = Path.cwd()
    if len(sys.argv) > 1:
        root_dir = Path(sys.argv[1]).resolve()
    if not root_dir.exists():
        print(f"[config_manager] Dossier introuvable : {root_dir}")
        sys.exit(1)
    app = PalModConfigGUI(root_dir)
    app.run()


if __name__ == "__main__":
    main()
