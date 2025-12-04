# Sistema de Zonas de Fuego Telegrafiadas - Ragnaros

## Resumen
Sistema avanzado de mecánica de boss que implementa zonas de fuego "telegraphed" (anunciadas con antelación) basadas en umbrales de HP.

## Arquitectura del Sistema

### Estructuras de Datos (Game.h)

```cpp
// Zona de fuego pendiente de ejecución
struct stPendingFireZone {
    BOOL    bActive;            // Está activa esta entrada
    short   sOwnerNpcH;         // Handle del NPC dueño (Ragnaros)
    char    cMapIndex;          // Índice del mapa
    short   sX, sY;             // Coordenadas
    DWORD   dwWarningTime;      // Timestamp cuando se creó la advertencia
    int     iWarningObjectID;   // ID del dynamic object de advertencia
};

// Flags de umbrales HP (uno por cada 10%)
struct stRagnarosHPThresholds {
    BOOL bTriggered90, bTriggered80, bTriggered70;
    BOOL bTriggered60, bTriggered50, bTriggered40;
    BOOL bTriggered30, bTriggered20, bTriggered10;
};
```

### Constantes de Configuración

| Constante | Valor | Descripción |
|-----------|-------|-------------|
| `DEF_RAGNAROS_FIREZONE_WARNING_TIME` | 3000ms | Tiempo de advertencia antes de la explosión |
| `DEF_RAGNAROS_FIREZONE_DURATION` | 10000ms | Duración del fuego activo |
| `DEF_RAGNAROS_FIREZONE_MIN_COUNT` | 3 | Mínimo zonas por umbral |
| `DEF_RAGNAROS_FIREZONE_MAX_COUNT` | 5 | Máximo zonas por umbral |
| `DEF_RAGNAROS_FIREZONE_RADIUS` | 6 | Radio en tiles alrededor del boss |
| `DEF_RAGNAROS_FIREZONE_DAMAGE` | 25 | Daño por tick |
| `DEF_RAGNAROS_MAX_PENDING_ZONES` | 50 | Máximo zonas pendientes simultáneas |

### Nuevos DynamicObject Types

```cpp
#define DEF_DYNAMICOBJECT_FIRE_WARNING  15  // Marker visual de advertencia (sin daño)
#define DEF_DYNAMICOBJECT_FIRE_INTENSE  16  // Zona de fuego intenso (alto daño)
```

## Flujo de Ejecución

### 1. Detección de Umbral (Ragnaros_CheckHPThresholds)
```
┌─────────────────────────────────────────────────────────┐
│ NpcBehavior_Ragnaros() se ejecuta cada tick             │
│                    ↓                                     │
│ Ragnaros_CheckHPThresholds(iNpcH)                       │
│                    ↓                                     │
│ ¿HP cruzó algún umbral (90%, 80%, 70%...10%)?           │
│         ↓ NO                    ↓ SÍ                     │
│     [return]          ¿Flag ya activado para este       │
│                        umbral?                           │
│                        ↓ SÍ         ↓ NO                 │
│                    [return]    Activar flag             │
│                                Llamar SpawnFireZoneWarnings()
└─────────────────────────────────────────────────────────┘
```

### 2. Fase 1 - Advertencia (Ragnaros_SpawnFireZoneWarnings)
```
┌─────────────────────────────────────────────────────────┐
│ Calcular 3-5 coordenadas aleatorias en radio de 6 tiles │
│                    ↓                                     │
│ Para cada coordenada válida:                            │
│   - Crear DEF_DYNAMICOBJECT_FIRE_WARNING                │
│   - Registrar en m_stPendingFireZones[]                 │
│   - Guardar timestamp actual                            │
│                    ↓                                     │
│ Enviar mensaje de advertencia a jugadores cercanos      │
└─────────────────────────────────────────────────────────┘
```

### 3. Fase 2 - Ejecución (Ragnaros_ProcessPendingFireZones)
```
┌─────────────────────────────────────────────────────────┐
│ Se ejecuta cada segundo desde GameProcess()             │
│                    ↓                                     │
│ Para cada zona pendiente activa:                        │
│   ¿Han pasado 3 segundos desde dwWarningTime?           │
│         ↓ NO                    ↓ SÍ                     │
│     [continue]         1. Eliminar objeto de advertencia│
│                        2. Crear DEF_DYNAMICOBJECT_FIRE3 │
│                           (fuego real con daño)         │
│                        3. Marcar zona como inactiva     │
│                        4. Notificar jugadores afectados │
└─────────────────────────────────────────────────────────┘
```

## Integración en el Game Loop

### En GameProcess() (cada 1 segundo)
```cpp
if ((dwTime - m_dwGameTime3) > 1000) {
    // ... otras llamadas ...
    
    // RAGNAROS: Procesar zonas de fuego pendientes
    Ragnaros_ProcessPendingFireZones();
    
    m_dwGameTime3 = dwTime;
}
```

### En NpcBehavior_Ragnaros()
```cpp
void CGame::NpcBehavior_Ragnaros(int iNpcH)
{
    // ... validaciones ...
    
    // Sistema de zonas de fuego - chequear umbrales
    Ragnaros_CheckHPThresholds(iNpcH);
    
    // ... resto del comportamiento ...
}
```

## Manejo de Reset

### Al morir Ragnaros (Ragnaros_OnDeath)
- Se llama a `Ragnaros_ResetThresholds(iNpcH)`
- Se resetean todos los flags de umbral a FALSE
- Se eliminan todas las zonas pendientes de ese NPC
- Se destruyen los objetos de advertencia activos

### Al respawn
- Los arrays ya están inicializados a FALSE por el constructor

## Funciones Implementadas

| Función | Descripción |
|---------|-------------|
| `Ragnaros_GetHPPercentage(int iNpcH)` | Retorna el % de HP actual |
| `Ragnaros_CheckHPThresholds(int iNpcH)` | Detecta cruce de umbrales y dispara Fase 1 |
| `Ragnaros_SpawnFireZoneWarnings(int iNpcH)` | Crea markers visuales de advertencia |
| `Ragnaros_ProcessPendingFireZones()` | Procesa zonas pendientes y ejecuta Fase 2 |
| `Ragnaros_ResetThresholds(int iNpcH)` | Resetea flags al morir/respawn |

## Archivos Modificados

### Servidor (HGServer)
- `Game.h` - Estructuras, constantes y declaraciones
- `Game.cpp` - Implementación de funciones + integración en game loop
- `DynamicObjectID.h` - Nuevos tipos de dynamic object

### Cliente (Client_patched)
- `DynamicObjectID.h` - Definiciones coincidentes con servidor
- `MapData.cpp` - Renderizado de nuevos tipos de dynamic object

## Ejemplo de Log del Servidor

```
(!) RAGNAROS: HP crossed 90% threshold - spawning fire zones!
(!) RAGNAROS: Fire warning 1 placed at (145, 203)
(!) RAGNAROS: Fire warning 2 placed at (142, 198)
(!) RAGNAROS: Fire warning 3 placed at (148, 201)
(!) RAGNAROS: Spawned 3 fire zone warnings (Fase 1)
... [3 segundos después] ...
(!) RAGNAROS: Fire zone ACTIVATED at (145, 203) - Fase 2!
(!) RAGNAROS: Fire zone ACTIVATED at (142, 198) - Fase 2!
(!) RAGNAROS: Fire zone ACTIVATED at (148, 201) - Fase 2!
```

## Ajustes de Balance

Para modificar el comportamiento, edita las constantes en `Game.h`:

```cpp
// Tiempo de reacción para jugadores (más alto = más fácil)
#define DEF_RAGNAROS_FIREZONE_WARNING_TIME  3000  // 3 segundos

// Cantidad de zonas (más bajo = más fácil)
#define DEF_RAGNAROS_FIREZONE_MIN_COUNT     3
#define DEF_RAGNAROS_FIREZONE_MAX_COUNT     5

// Área de efecto (más bajo = más fácil)
#define DEF_RAGNAROS_FIREZONE_RADIUS        6

// Duración del fuego (más bajo = más fácil)
#define DEF_RAGNAROS_FIREZONE_DURATION      10000  // 10 segundos
```

## Notas Técnicas

1. **Sin sleep() ni funciones bloqueantes**: Todo usa comparación de timestamps
2. **Thread-safe**: Los arrays son de tamaño fijo y se procesan secuencialmente
3. **Soporte multi-boss**: `m_stRagnarosThresholds[DEF_MAXNPCS]` permite múltiples Ragnaros
4. **Limpieza automática**: Las zonas pendientes se limpian al morir el boss
5. **Fallback visual**: Si el tipo WARNING falla, usa PCLOUD_BEGIN como alternativa
