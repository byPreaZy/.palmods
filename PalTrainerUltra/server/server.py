#!/usr/bin/env python3
"""Serveur HTTP local de PalTrainerUltra.

Sert l'application web de la carte et fait le pont avec la DLL injectée (qui écrit paltrainer.json
et lit commands.json dans un répertoire de données).
"""
import json
import os
import sys
from pathlib import Path
from http.server import HTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse

# Répertoire de données par défaut : le dossier 'dist' voisin ; remplaçable avec la variable d'environnement PALTRAINER_DATA_DIR
SCRIPT_DIR = Path(__file__).resolve().parent
DATA_DIR = Path(os.environ.get("PALTRAINER_DATA_DIR", SCRIPT_DIR.parent)).resolve()
WEB_DIR = (SCRIPT_DIR.parent / "web").resolve()


def cors_headers(handler):
    handler.send_header("Access-Control-Allow-Origin", "*")
    handler.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
    handler.send_header("Access-Control-Allow-Headers", "Content-Type")


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        # Serve static files from WEB_DIR
        super().__init__(*args, directory=str(WEB_DIR), **kwargs)

    def log_message(self, fmt, *args):
        # Supprimer les logs trop verbeux
        pass

    def do_OPTIONS(self):
        self.send_response(204)
        cors_headers(self)
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == "/paltrainer.json":
            self.serve_json(DATA_DIR / "paltrainer.json")
            return
        if parsed.path == "/commands.json":
            self.serve_json(DATA_DIR / "commands.json")
            return
        super().do_GET()

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path == "/commands":
            self.handle_commands()
            return
        self.send_error(404)

    def serve_json(self, path: Path):
        try:
            if not path.exists():
                self.send_response(200)
                cors_headers(self)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(b'{}')
                return
            data = path.read_bytes()
            self.send_response(200)
            cors_headers(self)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        except Exception as e:
            self.send_error(500, str(e))

    def handle_commands(self):
        try:
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length)
            # Valider le JSON
            payload = json.loads(body)
            DATA_DIR.mkdir(parents=True, exist_ok=True)
            (DATA_DIR / "commands.json").write_text(json.dumps(payload, indent=2))
            self.send_response(200)
            cors_headers(self)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"ok":true}')
        except Exception as e:
            self.send_error(500, str(e))


def main():
    global DATA_DIR
    if len(sys.argv) > 1:
        DATA_DIR = Path(sys.argv[1]).resolve()
    host = os.environ.get("PALTRAINER_HOST", "127.0.0.1")
    port = int(os.environ.get("PALTRAINER_PORT", "8765"))

    DATA_DIR.mkdir(parents=True, exist_ok=True)
    # Créer des fichiers JSON vides pour que l'interface ait quelque chose à lire avant l'injection.
    if not (DATA_DIR / "paltrainer.json").exists():
        (DATA_DIR / "paltrainer.json").write_text('{"ready":false}')
    if not (DATA_DIR / "commands.json").exists():
        (DATA_DIR / "commands.json").write_text('{}')

    server = HTTPServer((host, port), Handler)
    print(f"Serveur PalTrainerUltra en cours d'exécution sur http://{host}:{port}")
    print(f"Répertoire de données : {DATA_DIR}")
    print(f"Répertoire web : {WEB_DIR}")
    print("Ouvre l'URL dans ton navigateur. La minimap écrit les données du joueur automatiquement.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nArrêt du serveur.")
        server.shutdown()


if __name__ == "__main__":
    main()
