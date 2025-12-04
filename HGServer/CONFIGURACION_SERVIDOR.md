# DOCUMENTACIÓN COMPLETA DE CONFIGURACIÓN DEL SERVIDOR HELBREATH 3.51

## ?? ÍNDICE
1. [Archivos de Configuración](#archivos-de-configuración)
2. [Configuración Principal (GServer.cfg)](#configuración-principal)
3. [Configuración de Mapas (GMaps.cfg)](#configuración-de-mapas)
4. [Configuración de Ajustes (Settings.cfg)](#configuración-de-ajustes)
5. [Configuración Administrativa (AdminSettings.cfg)](#configuración-administrativa)
6. [Configuración de Listas (AdminList.cfg, BannedList.cfg)](#configuración-de-listas)
7. [Configuración Especial](#configuración-especial)
8. [Parámetros Edibles por Archivo](#parámetros-edibles)

---

## ARCHIVOS DE CONFIGURACIÓN

| Archivo | Ubicación | Descripción |
|---------|-----------|-------------|
| `GServer.cfg` | Raíz del servidor | Configuración base del servidor |
| `GMaps.cfg` | Raíz del servidor | Definición de mapas disponibles |
| `Settings.cfg` | `GameConfigs\` | Parámetros de juego y balance |
| `AdminSettings.cfg` | `GameConfigs\` | Niveles de permisos administrativos |
| `AdminList.cfg` | `GameConfigs\` | Lista de administradores |
| `BannedList.cfg` | `GameConfigs\` | Lista de IPs banneadas |
| `Schedule.cfg` | `GameConfigs\` | Horarios de eventos (Crusade, Heldenian, Apocalypse) |
| `CrusadeStructure.cfg` | `GameConfigs\` | Estructura de edificios de la Crusade |
| `Item.cfg`, `Item2.cfg`, `Item3.cfg` | `GameConfigs\` | Definición de ítems |
| `NPC.cfg` | `GameConfigs\` | Definición de NPCs |
| `Magic.cfg` | `GameConfigs\` | Definición de magias |
| `Skill.cfg` | `GameConfigs\` | Definición de habilidades |
| `Quest.cfg` | `GameConfigs\` | Definición de quests |
| `Potion.cfg` | `GameConfigs\` | Definición de pociones y crafting |
| `notice.txt` | `GameConfigs\` | Mensajes de noticia |

---

## CONFIGURACIÓN PRINCIPAL (GServer.cfg)

### Parámetros Básicos del Servidor

```
game-server-name = SERVERNAME          // Nombre del servidor (máx 10 caracteres)
game-server-port = 8888                // Puerto del servidor de juego
log-server-address = 127.0.0.1         // Dirección del servidor de logs
internal-log-server-port = 9999        // Puerto del servidor de logs
gate-server-address = 127.0.0.1        // Dirección del Gate Server
gate-server-port = 7777                // Puerto del Gate Server
game-server-internal-address = 127.0.0.1   // Dirección interna (LAN)
game-server-external-address = 0.0.0.0    // Dirección externa (Internet)
game-server-address = 127.0.0.1            // Dirección principal del servidor
game-server-mode = LAN                     // Modo: LAN o INTERNET
gate-server-dns = false                    // Usar DNS para Gate Server
log-server-dns = false                     // Usar DNS para Log Server
```

### Mapas Disponibles

```
game-server-map = aresden
game-server-map = elvine
game-server-map = middleland
game-server-map = 2ndmiddle
game-server-map = bisle
game-server-map = arefarm
game-server-map = elvfarm
game-server-map = [otros mapas...]
```

---

## CONFIGURACIÓN DE MAPAS (GMaps.cfg)

Simplemente lista los mapas del servidor (ver arriba en GServer.cfg).

---

## CONFIGURACIÓN DE AJUSTES (Settings.cfg)

### Tasas de Drop

```
primary-drop-rate = 100                // Tasa de drop principal (1-10000)
secondary-drop-rate = 100              // Tasa de drop secundaria (1-10000)
```

### Modo de Eliminación de Enemigos

```
enemy-kill-mode = CLASSIC              // CLASSIC o DEATHMATCH
    // CLASSIC: Sistema tradicional
    // DEATHMATCH: Kill count basado en enemigos
enemy-kill-adjust = 1                  // Puntos EK por kill de enemigo (1-100)
```

### Seguridad Administrativa

```
admin-security = ON                    // Activar/desactivar restricciones de GM (ON/OFF)
admin-security-code = SECURITYCODE     // Código de seguridad admin (máx 10 caracteres)
```

### Horarios de Asalto (Raid Times)

```
monday-raid-time = 3                   // Lunes (minutos)
tuesday-raid-time = 3                  // Martes
wednesday-raid-time = 3                // Miércoles
thursday-raid-time = 3                 // Jueves
friday-raid-time = 10                  // Viernes
saturday-raid-time = 45                // Sábado
sunday-raid-time = 60                  // Domingo
```

### Configuración de Log de Chat

```
log-chat-settings = NONE               // NONE, PLAYER, GM, ALL
    // NONE: No registrar chats
    // PLAYER: Solo chats de jugadores
    // GM: Solo chats de GMs
    // ALL: Todos los chats
```

### Configuración de Gremios

```
summonguild-cost = 100000              // Costo en oro para invocar gremio
```

### Objetos Mágicos (Slate)

```
slate-success-rate = 14                // Porcentaje de éxito para upgrade (0-100)
```

### Límites de Personaje

```
character-stat-limit = 200             // Límite máximo por stat (STR, INT, etc.)
character-skill-limit = 700            // Límite total de puntos de habilidad
max-player-level = 180                 // Nivel máximo de jugador
```

### Modificador de Reputación/Drop

```
rep-drop-modifier = 0                  // Modificador de reputación vs drop
```

---

## CONFIGURACIÓN ADMINISTRATIVA (AdminSettings.cfg)

### Niveles de Permiso de Administradores

```
admin-level-who = 1                    // Comando /who
admin-level-gm-kill = 10               // Matar jugadores
admin-level-gm-revive = 10             // Revivir jugadores
admin-level-gm-closeconn = 10          // Cerrar conexión
admin-level-gm-check-rep = 5           // Ver reputación
admin-level-energy-sphere = 8          // Crear esferas de energía
admin-level-shutdown = 20              // Apagar servidor
admin-level-observer = 10              // Modo observador
admin-level-shutup = 5                 // Silenciar jugadores
admin-level-call-guard = 5             // Invocar guardias
admin-level-summon-demon = 15          // Invocar demonio
admin-level-summon-death = 15          // Invocar muerte
admin-level-reserve-fightzone = 10     // Reservar zona de combate
admin-level-create-fish = 5            // Crear peces
admin-level-teleport = 5               // Teletransporte
admin-level-check-ip = 5               // Ver IPs
admin-level-polymorph = 10             // Polimorfismo
admin-level-set-invis = 10             // Invisibilidad
admin-level-set-zerk = 10              // Modo berserker
admin-level-set-ice = 10               // Hielo
admin-level-get-npc-status = 5         // Ver estado NPC
admin-level-set-attack-mode = 10       // Cambiar modo ataque
admin-level-unsummon-all = 15          // Desconvocar todos
admin-level-unsummon-demon = 15        // Desconvocar demonio
admin-level-summon = 10                // Convocar
admin-level-summon-all = 10            // Convocar todos
admin-level-summon-player = 10         // Convocar jugador
admin-level-disconnect-all = 20        // Desconectar todos
admin-level-enable-create-item = 15    // Crear ítems
admin-level-create-item = 15           // Crear ítems específicos
admin-level-storm = 10                 // Tormenta
admin-level-weather = 5                // Clima
admin-level-set-status = 10            // Cambiar stats
admin-level-goto = 5                   // Ir a ubicación
admin-level-monster-count = 1          // Ver cantidad de monstruos
admin-level-set-recall-time = 10       // Cambiar tiempo de recall
admin-level-unsummon-boss = 15         // Desconvocar jefe
admin-level-clear-npc = 10             // Limpiar NPCs
admin-level-time = 5                   // Cambiar hora
admin-level-push-player = 5            // Empujar jugador
admin-level-summon-guild = 10          // Invocar gremio
admin-level-check-status = 5           // Ver estado
admin-level-clean-map = 10             // Limpiar mapa
```

---

## CONFIGURACIÓN DE LISTAS

### AdminList.cfg

```
admin-name = NOMBREDEGM                // Nombre exacto del GM (máx 10 caracteres)
```

### BannedList.cfg

```
banned-ip = 192.168.1.100              // IPs bloqueadas
```

---

## CONFIGURACIÓN ESPECIAL

### Límites Predefinidos (en Game.h)

```cpp
#define DEF_MAXCLIENTS              2000   // Clientes máximos
#define DEF_MAXNPCS                 5000   // NPCs máximos
#define DEF_MAXITEMTYPES            5000   // Tipos de ítems
#define DEF_MAXGUILDS               1000   // Gremios máximos
#define DEF_MAXONESERVERUSERS       800    // Jugadores por servidor
#define DEF_MAXFISHS                200    // Peces máximos
#define DEF_MAXMINERALS             200    // Minerales máximos
#define DEF_MAXMAPS                 100    // Mapas máximos
#define DEF_MAXDYNAMICOBJECTS       60000  // Objetos dinámicos
```

### Tiempos (en ms)

```cpp
#define DEF_CLIENTTIMEOUT           10000  // Timeout cliente (10 seg)
#define DEF_HPUPTIME                15000  // Regeneración HP (15 seg)
#define DEF_MPUPTIME                20000  // Regeneración MP (20 seg)
#define DEF_SPUPTIME                10000  // Regeneración SP (10 seg)
#define DEF_HUNGERTIME              60000  // Tiempo hambre (60 seg)
#define DEF_AUTOSAVETIME            600000 // Auto-guardado (10 min)
#define DEF_NOTICETIME              80000  // Noticia (80 seg)
```

---

## PARÁMETROS EDIBLES POR ARCHIVO

### ?? EDITABLE - Fácil de cambiar sin recompilar:

? **GServer.cfg**
- Nombre del servidor
- Puertos (juego, logs, gate)
- Direcciones IP (interna, externa)
- Modo (LAN/INTERNET)
- Mapas disponibles

? **Settings.cfg**
- Tasas de drop
- Modo de kill (CLASSIC/DEATHMATCH)
- Horarios de asalto
- Límites de stats/skills
- Nivel máximo de jugador
- Chat logging

? **AdminSettings.cfg**
- Niveles de permisos de GMs
- Control granular por comando

? **AdminList.cfg**
- Agregar/remover GMs

? **BannedList.cfg**
- Agregar/remover IPs banneadas

? **Schedule.cfg** (no mostrado aquí)
- Horarios de eventos globales

? **Item.cfg, NPC.cfg, Magic.cfg, Skill.cfg, Quest.cfg**
- Parámetros de ítems, NPCs, magias, habilidades, quests

---

### ?? REQUIERE RECOMPILACIÓN - Difícil de cambiar:

? **Game.h**
- Límites máximos (clientes, NPCs, ítems, etc.)
- Tiempos en milisegundos
- Valores predefinidos

? **Código fuente**
- Lógica de combate
- Fórmulas de daño
- Mecánicas de juego

---

## EJEMPLO DE ARCHIVO DE CONFIGURACIÓN

### GServer.cfg

```
game-server-name = MyServer
game-server-port = 8888
log-server-address = 127.0.0.1
internal-log-server-port = 9999
gate-server-address = 127.0.0.1
gate-server-port = 7777
game-server-internal-address = 192.168.1.1
game-server-external-address = 203.0.113.0
game-server-address = 127.0.0.1
game-server-mode = INTERNET
gate-server-dns = false
log-server-dns = false

game-server-map = aresden
game-server-map = elvine
game-server-map = middleland
game-server-map = 2ndmiddle
game-server-map = bisle
game-server-map = arefarm
game-server-map = elvfarm
```

### Settings.cfg

```
primary-drop-rate = 100
secondary-drop-rate = 50
enemy-kill-mode = CLASSIC
enemy-kill-adjust = 1
admin-security = ON
admin-security-code = SERVERPASS
monday-raid-time = 3
tuesday-raid-time = 3
wednesday-raid-time = 3
thursday-raid-time = 3
friday-raid-time = 10
saturday-raid-time = 45
sunday-raid-time = 60
log-chat-settings = ALL
summonguild-cost = 100000
slate-success-rate = 14
character-stat-limit = 200
character-skill-limit = 700
max-player-level = 180
rep-drop-modifier = 0
```

### AdminList.cfg

```
admin-name = Administrator
admin-name = Moderator1
admin-name = Moderator2
```

### BannedList.cfg

```
banned-ip = 192.168.1.100
banned-ip = 10.0.0.50
```

---

## NOTAS IMPORTANTES

1. **Nivel de Admin**: Cuanto más bajo el número, más permisos tiene el GM
2. **Drop Rates**: 100 = 100%, 50 = 50%, 200 = 200% (dropeados más items)
3. **Raid Times**: Tiempo en minutos que dura el asalto por día
4. **Stat Limit**: El máximo que puede tener un jugador en cualquier stat
5. **Skill Limit**: El máximo total de puntos de habilidad
6. **Recompilación**: Cambios en Game.h requieren compilación del servidor
7. **Rutas**: Las rutas son relativas al directorio raíz del servidor

---

## ESTRUCTURA DE DIRECTORIOS

```
HGServer/
??? GServer.cfg
??? GMaps.cfg
??? Game.exe (compilado)
??? GameConfigs/
    ??? Settings.cfg
    ??? AdminSettings.cfg
    ??? AdminList.cfg
    ??? BannedList.cfg
    ??? Schedule.cfg
    ??? CrusadeStructure.cfg
    ??? Item.cfg
    ??? Item2.cfg
    ??? Item3.cfg
    ??? NPC.cfg
    ??? Magic.cfg
    ??? Skill.cfg
    ??? Quest.cfg
    ??? Potion.cfg
    ??? notice.txt
    ??? DupItemID.cfg
    ??? builditem.cfg
```

---

**Última actualización**: Basado en análisis del código fuente Helbreath 3.51
**Estado**: Documentación Completa ?
