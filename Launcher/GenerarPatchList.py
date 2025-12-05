# Generador de PatchList para el servidor de updates
# Ejecutar este script cada vez que actualices archivos del cliente

import os
import json
import hashlib
from pathlib import Path

# Carpetas y archivos a incluir en el patchlist
INCLUDE_FOLDERS = ["CONTENTS", "FONTS", "MAPDATA", "SPRITES", "SOUNDS", "MUSIC"]
INCLUDE_FILES = ["Game.exe", "search.dll"]

def calculate_md5(filepath):
    """Calcula el hash MD5 de un archivo"""
    hash_md5 = hashlib.md5()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def generate_patchlist(game_folder, version):
    """Genera el patchlist.json"""
    game_path = Path(game_folder)
    files_list = []
    
    print(f"Generando patchlist para versión {version}...")
    print(f"Carpeta del juego: {game_path}")
    print("-" * 50)
    
    # Procesar archivos individuales
    for filename in INCLUDE_FILES:
        filepath = game_path / filename
        if filepath.exists():
            file_hash = calculate_md5(filepath)
            file_size = filepath.stat().st_size
            files_list.append({
                "path": filename,
                "hash": file_hash,
                "size": file_size
            })
            print(f"[+] {filename} ({file_size} bytes)")
    
    # Procesar carpetas
    for folder in INCLUDE_FOLDERS:
        folder_path = game_path / folder
        if folder_path.exists():
            for filepath in folder_path.rglob("*"):
                if filepath.is_file():
                    relative_path = filepath.relative_to(game_path)
                    file_hash = calculate_md5(filepath)
                    file_size = filepath.stat().st_size
                    files_list.append({
                        "path": str(relative_path).replace("\\", "/"),
                        "hash": file_hash,
                        "size": file_size
                    })
                    print(f"[+] {relative_path} ({file_size} bytes)")
    
    # Crear el JSON
    patchlist = {
        "version": version,
        "files": files_list
    }
    
    # Guardar patchlist.json
    output_path = Path("patchlist.json")
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(patchlist, f, indent=2)
    
    print("-" * 50)
    print(f"Total de archivos: {len(files_list)}")
    print(f"Patchlist guardado en: {output_path.absolute()}")
    
    # También guardar version.txt
    version_path = Path("version.txt")
    version_path.write_text(version)
    print(f"Version guardada en: {version_path.absolute()}")
    
    return patchlist

if __name__ == "__main__":
    import sys
    
    # Valores por defecto
    game_folder = "../"  # Carpeta Helbreath (un nivel arriba de Launcher)
    version = "1.0.0"
    
    # Permitir argumentos
    if len(sys.argv) >= 2:
        version = sys.argv[1]
    if len(sys.argv) >= 3:
        game_folder = sys.argv[2]
    
    print("=" * 50)
    print("  GENERADOR DE PATCHLIST - HELBREATH")
    print("=" * 50)
    print()
    
    generate_patchlist(game_folder, version)
    
    print()
    print("¡Listo! Ahora sube estos archivos a tu servidor:")
    print("  - patchlist.json")
    print("  - version.txt")
    print("  - Carpeta 'files/' con todos los archivos del cliente")
    print()
    input("Presiona Enter para continuar...")
