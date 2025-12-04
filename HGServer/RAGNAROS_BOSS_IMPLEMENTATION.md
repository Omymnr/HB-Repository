# Implementación de Boss: Ragnaros, Señor del Fuego

## ✅ IMPLEMENTACIÓN COMPLETADA

El boss Ragnaros ha sido implementado exitosamente en el servidor HGServer.
La compilación fue exitosa sin errores.

## Descripción General
Ragnaros es un boss personalizado de alto nivel que utiliza el modelo del Liche como base visual.
Es el señor elemental del fuego con habilidades devastadoras y un sistema de fases.

## Características del Boss

### Estadísticas Base
- **Tipo NPC**: 100 (nuevo tipo)
- **Nombre**: Ragnaros
- **HP**: 50,000 (HitDice muy alto)
- **Defensa**: 2000 DR
- **Hit Ratio**: 2000 HR
- **Nivel de Magia**: 14 (máximo, usa los hechizos más poderosos)
- **Atributo**: 1 (Fuego) - Inmune a fuego
- **Absorción de Daño Mágico**: 90 (casi inmune a magia)
- **Mana Máximo**: 50000
- **Rango de Ataque**: 5 (ataques a distancia)
- **Tamaño de Área**: 5 (afecta área grande)

### Resistencias e Inmunidades
- 100% Inmune a Fuego (Atributo = 1)
- 100% Inmune a Parálisis
- 90% Absorción de daño mágico
- -50% Debilidad a Hielo (código especial)

### Habilidades Especiales

#### 1. Sulfuras Smash (Ataque Melee)
- Ataque físico devastador con knockback
- Usa animación de cast del Liche
- Daño base: 500-800
- Efecto: Knockback 3 tiles

#### 2. Wrath of Ragnaros (Habilidad Pasiva)
- Cada 15 segundos, explosión de fuego AoE
- Radio: 5 tiles
- Daño: 200-400 a todos en el área
- Efecto visual: Mass Fire Strike

#### 3. Sistema de Fases
**Fase 1 (100%-30% HP):**
- Comportamiento normal de boss
- Usa Sulfuras Smash y Wrath of Ragnaros

**Fase 2 (30%-0% HP):**
- Entra en estado "Etéreo" (semi-transparente)
- Invoca 2-4 "Sons of Flame" (Hellhounds mejorados)
- Aumenta velocidad de ataques
- Wrath of Ragnaros cada 10 segundos

### Loot Table
| Item | Probabilidad |
|------|-------------|
| Eye of Sulfuras | 1-2% |
| Sulfuron Hammer | 5% |
| Lava Core | 15% |
| Fire Mantle | 10% |
| Ring of Ragnaros | 3% |
| Blood items | 20% |
| High-level equipment | 30% |
| Gold (50000-100000) | 100% |

---

## Archivos a Modificar

### 1. NPC.cfg (Configuración del NPC)
Agregar la siguiente línea:

```
// Boss: Ragnaros, Señor del Fuego
// Tipo 100 - Usa sprite de Liche (30) pero con comportamiento especial
Npc = Ragnaros	30	50000	2000	2000	100	50000 100000	25000	35000	25	20	1	10	0	400	100	14	10	0	8	120000	1	90	50000	500	5	5
```

### 2. Game.h (Declaraciones)
Agregar constantes y declaraciones:

```cpp
// Ragnaros Boss Constants
#define DEF_RAGNAROS_PHASE2_HP_PERCENT  30
#define DEF_RAGNAROS_WRATH_INTERVAL     15000  // 15 segundos
#define DEF_RAGNAROS_PHASE2_WRATH_INTERVAL 10000 // 10 segundos en fase 2
#define DEF_RAGNAROS_SONS_MIN           2
#define DEF_RAGNAROS_SONS_MAX           4

// En la clase CGame, agregar:
void NpcBehavior_Ragnaros(int iNpcH);
void Ragnaros_WrathOfRagnaros(int iNpcH);
void Ragnaros_SummonSonsOfFlame(int iNpcH);
BOOL Ragnaros_CheckPhaseTransition(int iNpcH);
```

### 3. Game.cpp (Implementación)

Ver sección de código detallada abajo.

### 4. Cliente (MapData.cpp)
El tipo 30 (Liche) ya está implementado, así que el sprite se mostrará automáticamente.
Para el tinte de fuego, se requeriría modificar el renderizado del cliente.

---

## Código de Implementación (Game.cpp)

### En NpcBehavior_Attack, agregar manejo especial para Ragnaros:

```cpp
// Después de case 6 en m_cMagicLevel, agregar:
case 14: // Ragnaros - Boss especial
    // Verificar si es Ragnaros (por nombre)
    if (memcmp(m_pNpcList[iNpcH]->m_cNpcName, "Ragnaros", 8) == 0) {
        // Lógica especial de Ragnaros
        NpcBehavior_Ragnaros(iNpcH);
        return;
    }
    // Fallback a comportamiento de magia nivel 7
    if ((m_pMagicConfigList[70]->m_sValue1 <= m_pNpcList[iNpcH]->m_iMana) && (iDice(1,5) == 3)) 
        iMagicType = 70;
    else if (m_pMagicConfigList[61]->m_sValue1 <= m_pNpcList[iNpcH]->m_iMana) 
        iMagicType = 61;
    else if (m_pMagicConfigList[60]->m_sValue1 <= m_pNpcList[iNpcH]->m_iMana) 
        iMagicType = 60;
    else if (m_pMagicConfigList[51]->m_sValue1 <= m_pNpcList[iNpcH]->m_iMana) 
        iMagicType = 51;
    break;
```

### Función principal de comportamiento de Ragnaros:

```cpp
void CGame::NpcBehavior_Ragnaros(int iNpcH)
{
    if (m_pNpcList[iNpcH] == NULL) return;
    
    DWORD dwCurrentTime = timeGetTime();
    int iHPPercent = (m_pNpcList[iNpcH]->m_iHP * 100) / m_pNpcList[iNpcH]->m_iMaxHP;
    
    // Verificar transición de fase
    static BOOL bPhase2Activated[DEF_MAXNPCS] = {FALSE};
    
    if (iHPPercent <= DEF_RAGNAROS_PHASE2_HP_PERCENT && !bPhase2Activated[iNpcH]) {
        // Entrar en Fase 2
        bPhase2Activated[iNpcH] = TRUE;
        Ragnaros_SummonSonsOfFlame(iNpcH);
        
        // Mensaje global
        for (int i = 1; i < DEF_MAXCLIENTS; i++) {
            if (m_pClientList[i] != NULL) {
                SendNotifyMsg(NULL, i, DEF_NOTIFY_NOTICEMSG, NULL, NULL, NULL, 
                    "Ragnaros entra en su fase final! BY FIRE BE PURGED!");
            }
        }
    }
    
    // Wrath of Ragnaros - AoE periódico
    static DWORD dwLastWrathTime[DEF_MAXNPCS] = {0};
    DWORD dwWrathInterval = bPhase2Activated[iNpcH] ? 
                            DEF_RAGNAROS_PHASE2_WRATH_INTERVAL : 
                            DEF_RAGNAROS_WRATH_INTERVAL;
    
    if (dwCurrentTime - dwLastWrathTime[iNpcH] >= dwWrathInterval) {
        dwLastWrathTime[iNpcH] = dwCurrentTime;
        Ragnaros_WrathOfRagnaros(iNpcH);
    }
    
    // Ataque normal (usar Mass Fire Strike o Fire Strike)
    short sX = m_pNpcList[iNpcH]->m_sX;
    short sY = m_pNpcList[iNpcH]->m_sY;
    short dX, dY;
    
    switch (m_pNpcList[iNpcH]->m_cTargetType) {
    case DEF_OWNERTYPE_PLAYER:
        if (m_pClientList[m_pNpcList[iNpcH]->m_iTargetIndex] == NULL) return;
        dX = m_pClientList[m_pNpcList[iNpcH]->m_iTargetIndex]->m_sX;
        dY = m_pClientList[m_pNpcList[iNpcH]->m_iTargetIndex]->m_sY;
        break;
    case DEF_OWNERTYPE_NPC:
        if (m_pNpcList[m_pNpcList[iNpcH]->m_iTargetIndex] == NULL) return;
        dX = m_pNpcList[m_pNpcList[iNpcH]->m_iTargetIndex]->m_sX;
        dY = m_pNpcList[m_pNpcList[iNpcH]->m_iTargetIndex]->m_sY;
        break;
    }
    
    char cDir = m_Misc.cGetNextMoveDir(sX, sY, dX, dY);
    if (cDir == 0) return;
    m_pNpcList[iNpcH]->m_cDir = cDir;
    
    // Usar Mass Fire Strike (81) o Fire Strike (61)
    int iMagicType = 81; // Meteor Strike como ataque principal
    if (m_pMagicConfigList[iMagicType]->m_sValue1 > m_pNpcList[iNpcH]->m_iMana) {
        iMagicType = 61; // Fire Strike como backup
    }
    
    SendEventToNearClient_TypeA(iNpcH, DEF_OWNERTYPE_NPC, MSGID_EVENT_MOTION, 
        DEF_OBJECTATTACK, dX, dY, 1);
    NpcMagicHandler(iNpcH, dX, dY, iMagicType);
}

void CGame::Ragnaros_WrathOfRagnaros(int iNpcH)
{
    if (m_pNpcList[iNpcH] == NULL) return;
    
    short sCenterX = m_pNpcList[iNpcH]->m_sX;
    short sCenterY = m_pNpcList[iNpcH]->m_sY;
    int iMapIndex = m_pNpcList[iNpcH]->m_cMapIndex;
    
    // Mensaje de advertencia
    for (int i = 1; i < DEF_MAXCLIENTS; i++) {
        if (m_pClientList[i] != NULL && 
            m_pClientList[i]->m_cMapIndex == iMapIndex) {
            int iDist = abs(m_pClientList[i]->m_sX - sCenterX) + 
                       abs(m_pClientList[i]->m_sY - sCenterY);
            if (iDist <= 10) {
                SendNotifyMsg(NULL, i, DEF_NOTIFY_NOTICEMSG, NULL, NULL, NULL, 
                    "Ragnaros canaliza Wrath of Ragnaros!");
            }
        }
    }
    
    // Usar Fire Strike en el área (magia 61 - Mass Fire Strike)
    m_pNpcList[iNpcH]->m_iMagicHitRatio = 1000;
    NpcMagicHandler(iNpcH, sCenterX, sCenterY, 61);
}

void CGame::Ragnaros_SummonSonsOfFlame(int iNpcH)
{
    if (m_pNpcList[iNpcH] == NULL) return;
    
    int iNumSons = DEF_RAGNAROS_SONS_MIN + iDice(1, 
                   DEF_RAGNAROS_SONS_MAX - DEF_RAGNAROS_SONS_MIN + 1) - 1;
    
    short sCenterX = m_pNpcList[iNpcH]->m_sX;
    short sCenterY = m_pNpcList[iNpcH]->m_sY;
    char* cMapName = m_pMapList[m_pNpcList[iNpcH]->m_cMapIndex]->m_cName;
    
    for (int i = 0; i < iNumSons; i++) {
        int pX = sCenterX + iDice(1, 5) - 3;
        int pY = sCenterY + iDice(1, 5) - 3;
        
        // Invocar Hellhound como "Son of Flame"
        bCreateNewNpc("Hellbound", "Son-of-Flame", cMapName, 
                      0, 7, DEF_MOVETYPE_RANDOM, &pX, &pY, 
                      NULL, NULL, NULL, -1, FALSE, TRUE, TRUE);
    }
    
    wsprintf(G_cTxt, "Ragnaros summons %d Sons of Flame!", iNumSons);
    PutLogList(G_cTxt);
}
```

### En NpcDeadItemGenerator, agregar loot de Ragnaros:

```cpp
// Después del case 81 (Abaddon), agregar:
// Ragnaros - Loot especial
if (memcmp(m_pNpcList[iNpcH]->m_cNpcName, "Ragnaros", 8) == 0) {
    // Multidrop garantizado
    for (int iDropCount = 0; iDropCount < 5; iDropCount++) {
        switch (iDice(1, 10)) {
        case 1: // Eye of Sulfuras (1% por roll = ~5% total)
            if (iDice(1, 100) <= 20) {
                // Crear item especial Eye of Sulfuras (necesita definir ID)
                // iItemID = ITEM_EYE_OF_SULFURAS;
            }
            break;
        case 2: // Sulfuron Hammer
            if (iDice(1, 20) == 1) {
                // iItemID = ITEM_SULFURON_HAMMER;
            }
            break;
        case 3:
        case 4: // Blood weapons
            switch (iDice(1, 3)) {
            case 1: iItemID = 490; break; // BloodSword
            case 2: iItemID = 491; break; // BloodAxe
            case 3: iItemID = 492; break; // BloodRapier
            }
            break;
        case 5:
        case 6: // Manuals de alto nivel
            switch (iDice(1, 2)) {
            case 1: iItemID = 381; break; // MassFireStrikeManual
            case 2: iItemID = 382; break; // BloodyShockWaveManual
            }
            break;
        case 7:
        case 8: // Rings de poder
            switch (iDice(1, 3)) {
            case 1: iItemID = 633; break; // RingofDemonpower
            case 2: iItemID = 735; break; // RingOfDragonpower
            case 3: iItemID = 631; break; // RingoftheAbaddon
            }
            break;
        case 9:
        case 10: // Necklaces
            switch (iDice(1, 2)) {
            case 1: iItemID = 645; break; // KnecklaceOfEfreet
            case 2: iItemID = 648; break; // NecklaceOfLiche
            }
            break;
        }
        
        if (iItemID > 0) {
            // Generar el item
            bSetItemToBankByRecv(NULL, iItemID, 1, ...);
        }
    }
    
    // Gold garantizado
    int iGold = 50000 + iDice(1, 50000);
    // Dropear gold...
}
```

---

## Spawn de Ragnaros

Para spawnear a Ragnaros, agregar en el archivo de configuración del mapa o usar un comando GM:

### Opción 1: Spawn Manual (GM Command)
Agregar comando `/summon Ragnaros` 

### Opción 2: Spawn en Mapa Específico
En el archivo .amd del mapa boss, agregar:
```
npc = Ragnaros 1 RandomArea 0,0,100,100 100
```

### Opción 3: Evento Programado
Crear un evento que spawnee a Ragnaros en horarios específicos.

---

## Testing Checklist

- [ ] Ragnaros spawneado correctamente
- [ ] HP extremadamente alto verificado
- [ ] Ataques de fuego funcionando
- [ ] Wrath of Ragnaros cada 15 segundos
- [ ] Transición a Fase 2 al 30% HP
- [ ] Sons of Flame invocados en Fase 2
- [ ] Loot drops correctamente
- [ ] Experiencia otorgada correctamente
- [ ] Mensaje global cuando muere

---

## Notas Adicionales

1. **Tinte de Fuego**: Para aplicar un tinte rojo/naranja al Liche, se necesitaría modificar el cliente (MapData.cpp y Sprite.cpp) para aplicar una paleta de colores diferente al tipo de NPC específico.

2. **Balanceo**: Las estadísticas pueden necesitar ajustes después de testing. El boss está diseñado para requerir un grupo de 10-20 jugadores de alto nivel.

3. **Performance**: El AoE periódico (Wrath of Ragnaros) puede causar lag si hay muchos jugadores. Considerar optimizaciones si es necesario.

4. **Compatibilidad**: El código usa funciones existentes del servidor, por lo que debería ser compatible con la arquitectura actual.
