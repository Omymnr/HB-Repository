# Helbreath Launcher con Sistema de Actualizaciones via GitHub
# 
# Este launcher:
# 1. Verifica actualizaciones en GitHub
# 2. Descarga los archivos que hayan cambiado
# 3. Ejecuta Game.exe con el parámetro -nolauncher (salta el launcher interno)
#
# CONFIGURACIÓN:
# 1. Cambia GITHUB_REPO por tu repositorio
# 2. Compila: pyinstaller --onefile --windowed --icon=helbreath.ico --name=HelbreathLauncher HelbreathLauncher.py
# 3. Coloca HelbreathLauncher.exe en la misma carpeta que Game.exe

import os
import sys
import json
import hashlib
import requests
import subprocess
import tkinter as tk
from tkinter import ttk, messagebox
from threading import Thread
from pathlib import Path

# ============================================
# CONFIGURACIÓN - CAMBIAR ESTOS VALORES
# ============================================

# Tu repositorio de GitHub (usa raw.githubusercontent.com)
# Formato: https://raw.githubusercontent.com/USUARIO/REPOSITORIO/RAMA
GITHUB_REPO = "https://raw.githubusercontent.com/TU_USUARIO/helbreath-updates/main"

# Nombre del ejecutable del juego
GAME_EXECUTABLE = "Game.exe"

# Parámetro para saltar el launcher interno del Game.exe
SKIP_LAUNCHER_PARAM = "-nolauncher"

# ============================================

class HelbreathLauncher:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Helbreath Launcher")
        self.root.geometry("450x320")
        self.root.resizable(False, False)
        
        # Colores estilo medieval
        self.bg_color = "#1a1510"
        self.fg_color = "#d4c4a8"
        self.accent_color = "#c9a227"
        self.button_color = "#2d261e"
        self.error_color = "#c0392b"
        self.success_color = "#27ae60"
        
        self.root.configure(bg=self.bg_color)
        
        # Directorio del launcher
        if getattr(sys, 'frozen', False):
            self.base_path = Path(sys.executable).parent
        else:
            self.base_path = Path(os.path.dirname(os.path.abspath(__file__)))
        
        self.game_path = self.base_path
        
        self.setup_ui()
        
        # Verificar actualizaciones automáticamente al iniciar
        self.root.after(500, self.check_updates)
        
    def setup_ui(self):
        # Marco con borde dorado
        main_frame = tk.Frame(self.root, bg=self.bg_color, 
                             highlightbackground=self.accent_color, 
                             highlightthickness=2)
        main_frame.pack(fill="both", expand=True, padx=5, pady=5)
        
        # Título
        title_label = tk.Label(
            main_frame, 
            text="⚔ HELBREATH ⚔", 
            font=("Times New Roman", 26, "bold"),
            bg=self.bg_color, 
            fg=self.accent_color
        )
        title_label.pack(pady=(25, 5))
        
        # Subtítulo
        subtitle = tk.Label(
            main_frame,
            text="~ Launcher ~",
            font=("Times New Roman", 12, "italic"),
            bg=self.bg_color,
            fg=self.fg_color
        )
        subtitle.pack()
        
        # Separador dorado
        sep_frame = tk.Frame(main_frame, bg=self.bg_color)
        sep_frame.pack(fill="x", padx=50, pady=15)
        tk.Frame(sep_frame, height=1, bg=self.accent_color).pack(fill="x")
        
        # Frame de estado
        status_frame = tk.Frame(main_frame, bg=self.bg_color)
        status_frame.pack(pady=5, fill="x", padx=40)
        
        # Label de estado
        self.status_label = tk.Label(
            status_frame,
            text="Iniciando...",
            font=("Arial", 10),
            bg=self.bg_color,
            fg=self.fg_color
        )
        self.status_label.pack()
        
        # Barra de progreso
        style = ttk.Style()
        style.theme_use('clam')
        style.configure(
            "Gold.Horizontal.TProgressbar",
            troughcolor="#2d261e",
            background=self.accent_color,
        )
        
        self.progress = ttk.Progressbar(
            status_frame,
            style="Gold.Horizontal.TProgressbar",
            length=320,
            mode='determinate'
        )
        self.progress.pack(pady=8)
        
        # Label de archivo actual
        self.file_label = tk.Label(
            status_frame,
            text="",
            font=("Arial", 8),
            bg=self.bg_color,
            fg="#807060"
        )
        self.file_label.pack()
        
        # Frame de botones
        button_frame = tk.Frame(main_frame, bg=self.bg_color)
        button_frame.pack(pady=20)
        
        # Botón JUGAR (grande y dorado)
        self.play_button = tk.Button(
            button_frame,
            text="⚔  ENTRAR AL REINO  ⚔",
            font=("Times New Roman", 14, "bold"),
            bg=self.accent_color,
            fg="#1a1510",
            activebackground="#d4b237",
            activeforeground="#1a1510",
            width=22,
            height=2,
            relief="ridge",
            bd=3,
            cursor="hand2",
            command=self.launch_game,
            state="disabled"
        )
        self.play_button.pack(pady=5)
        
        # Botón de reintentar (oculto inicialmente)
        self.retry_button = tk.Button(
            button_frame,
            text="Reintentar",
            font=("Arial", 9),
            bg=self.button_color,
            fg=self.fg_color,
            activebackground=self.accent_color,
            width=12,
            relief="flat",
            cursor="hand2",
            command=self.check_updates
        )
        # No lo mostramos todavía
        
        # Versión en la esquina
        version_text = self.get_local_version()
        self.version_label = tk.Label(
            main_frame,
            text=f"v{version_text}" if version_text != "0.0.0" else "Sin versión",
            font=("Arial", 8),
            bg=self.bg_color,
            fg="#605040"
        )
        self.version_label.pack(side="bottom", pady=5)
        
    def get_local_version(self):
        version_path = self.game_path / "version.txt"
        try:
            if version_path.exists():
                return version_path.read_text().strip()
        except:
            pass
        return "0.0.0"
    
    def save_local_version(self, version):
        version_path = self.game_path / "version.txt"
        version_path.write_text(version)
        self.version_label.config(text=f"v{version}")
        
    def update_status(self, text, color=None):
        self.status_label.config(text=text, fg=color if color else self.fg_color)
        self.root.update_idletasks()
        
    def update_file_label(self, text):
        self.file_label.config(text=text)
        self.root.update_idletasks()
    
    def calculate_md5(self, filepath):
        hash_md5 = hashlib.md5()
        try:
            with open(filepath, "rb") as f:
                for chunk in iter(lambda: f.read(8192), b""):
                    hash_md5.update(chunk)
            return hash_md5.hexdigest()
        except:
            return None
    
    def check_updates(self):
        self.play_button.config(state="disabled")
        self.retry_button.pack_forget()
        Thread(target=self._check_updates_thread, daemon=True).start()
        
    def _check_updates_thread(self):
        try:
            self.update_status("Verificando actualizaciones...")
            self.progress['value'] = 0
            
            # Obtener versión remota
            try:
                response = requests.get(f"{GITHUB_REPO}/version.txt", timeout=15)
                if response.status_code != 200:
                    raise Exception(f"HTTP {response.status_code}")
                remote_version = response.text.strip()
            except requests.exceptions.ConnectionError:
                self.update_status("Sin conexión - Modo offline", "#b0a090")
                self.progress['value'] = 100
                self.play_button.config(state="normal")
                return
            except Exception as e:
                self.update_status("No se pudo conectar al servidor", self.error_color)
                self.show_retry_button()
                self.play_button.config(state="normal")  # Permitir jugar offline
                return
            
            local_version = self.get_local_version()
            
            # Comparar versiones
            if remote_version == local_version:
                self.update_status("✓ Juego actualizado", self.success_color)
                self.progress['value'] = 100
                self.play_button.config(state="normal")
                return
            
            self.update_status(f"Descargando versión {remote_version}...")
            
            # Obtener lista de parches
            try:
                response = requests.get(f"{GITHUB_REPO}/patchlist.json", timeout=15)
                patch_list = response.json()
            except:
                self.update_status("Error al obtener lista de archivos", self.error_color)
                self.show_retry_button()
                self.play_button.config(state="normal")
                return
            
            # Verificar qué archivos necesitan actualización
            files_to_update = []
            all_files = patch_list.get("files", [])
            
            for file_info in all_files:
                local_path = self.game_path / file_info["path"]
                local_hash = self.calculate_md5(local_path)
                
                if local_hash != file_info.get("hash"):
                    files_to_update.append(file_info)
            
            if not files_to_update:
                self.save_local_version(remote_version)
                self.update_status("✓ Juego actualizado", self.success_color)
                self.progress['value'] = 100
                self.play_button.config(state="normal")
                return
            
            # Descargar archivos que cambiaron
            total = len(files_to_update)
            for i, file_info in enumerate(files_to_update):
                filepath = file_info["path"]
                filename = os.path.basename(filepath)
                self.update_status(f"Descargando {i+1}/{total}: {filename}")
                self.update_file_label(filepath)
                
                try:
                    # Intentar descargar
                    url = f"{GITHUB_REPO}/files/{filepath}"
                    response = requests.get(url, stream=True, timeout=120)
                    
                    if response.status_code == 200:
                        local_path = self.game_path / filepath
                        local_path.parent.mkdir(parents=True, exist_ok=True)
                        
                        # Descargar por chunks
                        with open(local_path, 'wb') as f:
                            for chunk in response.iter_content(chunk_size=8192):
                                if chunk:
                                    f.write(chunk)
                    else:
                        raise Exception(f"HTTP {response.status_code}")
                        
                except Exception as e:
                    self.update_status(f"Error descargando {filename}", self.error_color)
                    self.update_file_label(str(e))
                    self.show_retry_button()
                    self.play_button.config(state="normal")
                    return
                
                # Actualizar progreso
                progress = int(((i + 1) / total) * 100)
                self.progress['value'] = progress
            
            # Éxito
            self.save_local_version(remote_version)
            self.update_status("✓ ¡Actualización completada!", self.success_color)
            self.update_file_label("")
            self.progress['value'] = 100
            
        except Exception as e:
            self.update_status(f"Error: {str(e)[:40]}", self.error_color)
            self.show_retry_button()
        
        self.play_button.config(state="normal")
    
    def show_retry_button(self):
        self.retry_button.pack(pady=5)
        
    def launch_game(self):
        game_exe = self.game_path / GAME_EXECUTABLE
        
        if not game_exe.exists():
            messagebox.showerror(
                "Error", 
                f"No se encontró {GAME_EXECUTABLE}\n\n"
                "El launcher debe estar en la misma carpeta que el juego."
            )
            return
        
        try:
            os.chdir(self.game_path)
            # Ejecutar Game.exe con -nolauncher para saltar el launcher interno
            subprocess.Popen([str(game_exe), SKIP_LAUNCHER_PARAM])
            self.root.quit()
        except Exception as e:
            messagebox.showerror("Error", f"No se pudo iniciar el juego:\n{e}")
    
    def run(self):
        # Centrar ventana
        self.root.update_idletasks()
        x = (self.root.winfo_screenwidth() - 450) // 2
        y = (self.root.winfo_screenheight() - 320) // 2
        self.root.geometry(f"450x320+{x}+{y}")
        
        self.root.mainloop()


if __name__ == "__main__":
    launcher = HelbreathLauncher()
    launcher.run()
