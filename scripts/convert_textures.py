from PIL import Image
import os

base = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(base, "..")
webp_main = os.path.join(root, "web", "assets", "map", "palworld-map.webp")
webp_tree = os.path.join(root, "web", "assets", "map", "palworld-treemap.webp")

sizes = [
    (2048, "map_2048.rgba", "map_tree_2048.rgba"),
    (4096, "map_4096.rgba", "map_tree_4096.rgba"),
    (8192, "map_8192.rgba", "map_tree_8192.rgba"),
]

img_main = Image.open(webp_main).convert("RGBA")
img_tree = Image.open(webp_tree).convert("RGBA")

for sz, main_name, tree_name in sizes:
    resized_main = img_main.resize((sz, sz), Image.LANCZOS)
    path_main = os.path.join(root, "assets", "maps", main_name)
    with open(path_main, "wb") as f:
        f.write(resized_main.tobytes())
    print(f"Palpagos map: {resized_main.size} -> {path_main} ({os.path.getsize(path_main)} bytes)")

    resized_tree = img_tree.resize((sz, sz), Image.LANCZOS)
    path_tree = os.path.join(root, "assets", "maps", tree_name)
    with open(path_tree, "wb") as f:
        f.write(resized_tree.tobytes())
    print(f"World Tree map: {resized_tree.size} -> {path_tree} ({os.path.getsize(path_tree)} bytes)")
