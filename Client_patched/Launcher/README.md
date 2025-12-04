# Helbreath Apocalypse - Launcher

## Descripción

Este es el launcher separado para Helbreath Apocalypse. Permite configurar opciones del juego antes de iniciar y luego ejecuta `Game.exe`.

## Estructura de Archivos

```
Launcher/
├── Launcher.cpp          # Código fuente principal
├── Launcher.rc           # Archivo de recursos
├── resource.h            # Definiciones de recursos
├── Launcher.vcxproj      # Proyecto Visual Studio 2022
├── Launcher.vcxproj.filters
├── Launcher.sln          # Solución Visual Studio
├── launcher_image.png    # Imagen del launcher (490x180 recomendado)
└── launcher_icon.ico     # Icono del launcher
```

## Compilación

### Requisitos
- Visual Studio 2022 (o posterior)
- Windows SDK 10.0
- Platform Toolset v143

### Pasos
1. Abrir `Launcher.sln` en Visual Studio
2. Seleccionar configuración `Release | Win32`
3. Compilar (F7 o Build > Build Solution)
4. El ejecutable se genera en `Release\Launcher.exe`

## Uso

### Archivos necesarios en la carpeta del juego:
```
CarpetaDelJuego/
├── Launcher.exe          # El launcher compilado
├── Game.exe              # El cliente del juego
├── CONTENTS/
│   └── LOGIN.CFG         # Se genera automáticamente
└── ... otros archivos del juego
```

### Funcionamiento:
1. El usuario ejecuta `Launcher.exe`
2. Selecciona servidor (Principal o Pruebas)
3. Configura modo de pantalla (Ventana o Pantalla completa)
4. Pulsa "ENTRAR AL REINO"
5. El launcher:
   - Guarda la configuración en el Registro de Windows
   - Genera/actualiza `CONTENTS\LOGIN.CFG` con la IP del servidor
   - Ejecuta `Game.exe -nolauncher -borderless` (o `-fullscreen`)
   - Se cierra

### Parámetros de línea de comandos de Game.exe:
- `-nolauncher`: Salta el launcher integrado del juego
- `-borderless`: Inicia en modo ventana sin bordes
- `-fullscreen`: Inicia en pantalla completa

## Configuración de Servidores

Editar en `Launcher.cpp` las constantes:

```cpp
// Server Online (para amigos conectando remotamente)
#define ONLINE_SERVER_IP "89.7.69.125"
#define ONLINE_SERVER_PORT 2500

// Test Server (para pruebas en red local)
#define TEST_SERVER_IP "192.168.0.15"
#define TEST_SERVER_PORT 2500
```

## Personalización

### Cambiar imagen del launcher:
1. Reemplazar `launcher_image.png` (recomendado: 490x180 píxeles)
2. Recompilar

### Cambiar icono:
1. Reemplazar `launcher_icon.ico`
2. Recompilar

### Cambiar colores del tema:
Editar las constantes de color en `Launcher.cpp`:
```cpp
#define COLOR_BG RGB(25, 20, 15)              // Fondo
#define COLOR_TEXT RGB(210, 190, 150)         // Texto
#define COLOR_ACCENT RGB(180, 140, 60)        // Dorado/Acento
#define COLOR_BUTTON RGB(60, 45, 30)          // Botones
```

## Almacenamiento de Configuración

La configuración se guarda en el Registro de Windows:
```
HKEY_CURRENT_USER\SOFTWARE\HelbreathApocalypse\Launcher
```

Valores guardados:
- `Borderless` (DWORD): 1 = modo ventana, 0 = pantalla completa
- `TestServer` (DWORD): 1 = servidor de pruebas, 0 = servidor principal
- `LauncherWidth` (DWORD): Ancho de la ventana del launcher
- `LauncherHeight` (DWORD): Alto de la ventana del launcher
