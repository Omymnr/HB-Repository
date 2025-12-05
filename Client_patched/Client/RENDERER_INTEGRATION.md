# 🔧 Guía de Integración del Nuevo Renderer

## Resumen

Este documento explica cómo integrar el nuevo sistema de renderizado Direct3D 11 
con el cliente existente de Helbreath.

## Estado Actual (Actualizado)

✅ **MODO HÍBRIDO IMPLEMENTADO**

El cliente ahora soporta renderizado híbrido DirectDraw + D3D11:
- Los sprites se dibujan en el backbuffer de DirectDraw (compatibilidad total)
- D3D11 copia el backbuffer y lo presenta en pantalla
- Permite usar características modernas de D3D11 (VSync, escalado, etc.)

## Archivos Creados

```
IRenderer.h              - Interfaz abstracta
DirectDrawRenderer.h/cpp - Wrapper del DXC_ddraw
Direct3D11Renderer.h/cpp - Nuevo renderer D3D11 (1500+ líneas)
RendererConfig.h/cpp     - Configuración de video
RendererBridge.h/cpp     - Puente de integración
TextureManager.h/cpp     - Conversión de sprites DDraw a texturas D3D11
SpriteRenderer.h/cpp     - Sistema de renderizado de sprites unificado
Shaders/SpriteShaders.hlsl - Shaders HLSL
video.cfg                - Archivo de configuración
```

## Cómo Usar

### Configuración (video.cfg)

```ini
# Renderer: 0=Auto, 1=DirectDraw, 2=Direct3D11
Renderer=2

# Resolución (solo D3D11)
ScreenWidth=1920
ScreenHeight=1080

# Pantalla completa
Fullscreen=0

# VSync (solo D3D11)
VSync=1
```

### Activar D3D11

1. Editar `video.cfg` y poner `Renderer=2`
2. Ejecutar el cliente
3. El cliente usará D3D11 para presentar (modo híbrido)

## Arquitectura

```
┌─────────────────────────────────────────────────────────────┐
│                      CGame                                  │
│                         │                                   │
│         ┌───────────────┴───────────────┐                  │
│         │                               │                  │
│         ▼                               ▼                  │
│   ┌─────────────┐                ┌──────────────┐         │
│   │ m_DDraw     │                │ RendererBridge│         │
│   │ (DXC_ddraw) │◄───────────────│              │         │
│   └─────────────┘                └──────┬───────┘         │
│                                         │                  │
│                                         │                  │
│              ┌──────────────────────────┼──────────────┐  │
│              │                          │              │  │
│              ▼                          ▼              │  │
│     ┌────────────────┐          ┌────────────────┐    │  │
│     │DirectDrawRenderer│        │Direct3D11Renderer│   │  │
│     │  (usa m_DDraw)  │         │  (D3D11 nativo) │    │  │
│     └────────────────┘          └────────────────┘    │  │
└─────────────────────────────────────────────────────────────┘
```

## Modo Híbrido (Actual)

```
┌────────────────────────────────────────────────────────────┐
│                    MODO HÍBRIDO                            │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  1. Sprites se dibujan → Backbuffer DirectDraw (16-bit)   │
│                              │                             │
│  2. FlipScreen() → Lock backbuffer                        │
│                              │                             │
│  3. Copiar píxeles → Textura D3D11 (32-bit BGRA)         │
│                              │                             │
│  4. DrawTexturedQuad → Render fullscreen quad             │
│                              │                             │
│  5. Present() → Mostrar en pantalla                       │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### Fases de Integración

#### Fase 1: Sin Cambios ✅
El cliente funciona exactamente igual que antes. Los nuevos archivos compilan
pero no se usan activamente.

#### Fase 2: Integración Opcional
Modificar `Game.cpp` para usar RendererBridge después de inicializar m_DDraw:

```cpp
// En CGame::bInit(), después de m_DDraw.bInit(m_hWnd):

#include "RendererBridge.h"

// Inicializar el bridge con el DDraw existente
CRendererBridge::GetInstance().Initialize(m_hWnd, &m_DDraw);
```

#### Fase 3: Selector de Renderer
Crear un archivo de configuración `video.cfg`:

```ini
# video.cfg
# Renderer: 0=Auto, 1=DirectDraw, 2=Direct3D11
Renderer=0
Width=800
Height=600
Fullscreen=1
VSync=1
```

#### Fase 4: Migración de Sprites
Gradualmente, reemplazar llamadas directas a CSprite por llamadas al renderer:

```cpp
// Antes:
m_pSprite[id]->PutSpriteFast(x, y, frame, dwTime);

// Después:
g_Renderer->DrawSprite(x, y, m_pSprite[id], frame, FALSE);
```

## Cómo Probar

### 1. Probar que el cliente sigue funcionando
El cliente ya compila. Simplemente ejecutar `Game.exe` para verificar
que todo funciona igual que antes.

### 2. Activar D3D11 (experimental)
Modificar `Game.cpp` y agregar después de `m_DDraw.bInit()`:

```cpp
// TEST: Inicializar RendererBridge
if (CRendererConfig::IsD3D11Available()) {
    OutputDebugStringA("D3D11 disponible en este sistema\n");
}
```

### 3. Verificar D3D11
El renderer D3D11 se puede probar de forma aislada creando una ventana
de prueba y verificando que crea el device correctamente.

## Compatibilidad

| Sistema | DirectDraw | Direct3D11 |
|---------|-----------|------------|
| Windows XP | ✅ | ❌ |
| Windows Vista | ✅ | ⚠️ (SP2) |
| Windows 7 | ✅ | ✅ |
| Windows 8/8.1 | ✅ | ✅ |
| Windows 10 | ⚠️ (emulado) | ✅ |
| Windows 11 | ⚠️ (emulado) | ✅ |

## Ventajas del Nuevo Sistema

1. **Mejor Rendimiento** - D3D11 usa GPU nativa en lugar de emulación
2. **Cualquier Resolución** - No limitado a 800x600
3. **Alpha Blending Real** - 32-bit color con transparencia
4. **VSync Nativo** - Sin tearing
5. **Compatibilidad Futura** - Windows 10/11 y más allá

## Próximos Pasos

1. ✅ Crear abstracción de renderizado
2. ✅ Implementar DirectDrawRenderer (wrapper)
3. ✅ Implementar Direct3D11Renderer
4. 🔄 Integrar con CGame
5. ⏳ Migrar sprites a texturas D3D11
6. ⏳ Agregar soporte de resolución dinámica
7. ⏳ Modernizar input (Raw Input)
8. ⏳ Modernizar audio (XAudio2)
