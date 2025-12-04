# GUÍA: CAMBIAR ARMAS INICIALES AL CREAR PERSONAJE
## Helbreath 3.51

---

## ?? RESUMEN

Las armas iniciales que obtiene un personaje al crearse se guardan en los **archivos de datos del cliente** cuando se crea el personaje. Para cambiarlas necesitas:

1. **Modificar el código fuente** donde se asignan los ítems iniciales
2. **Recompilar el servidor**
3. **Asegurarse de que Item.cfg tenga definidos esos ítems**

---

## ?? UBICACIÓN DEL CÓDIGO

El archivo donde se asignan las armas iniciales es probablemente:
- `Game.cpp` - En la función que crea nuevos personajes
- `LoginServer.cpp` - Al procesar la creación de cuenta
- O en archivos de datos del cliente (carpeta `Characters/`)

### Estructura de Datos del Cliente

Los personajes se guardan en:
```
Characters\
??? AscII<Primera letra ASCII>\ 
    ??? <NombreDelPersonaje>.txt
```

Ejemplo: Personaje "Knight" sería:
```
Characters\AscII75\Knight.txt  (ASCII 75 = 'K')
```

---

## ??? CÓMO CAMBIAR LAS ARMAS INICIALES

### Opción 1: Modificar en Item.cfg (RECOMENDADO - SIN RECOMPILACIÓN)

**Paso 1:** Abre `GameConfigs\Item.cfg`

**Paso 2:** Busca los ítems iniciales. Busca líneas como:
```
item-name = Iron Sword
item-effect-value1 = 5
item-effect-value2 = 3
```

**Paso 3:** Identifica los IDs de los ítems iniciales y cámbialos o modifica sus parámetros.

### Opción 2: Modificar el Código Fuente (REQUIERE RECOMPILACIÓN)

**Paso 1:** Busca en el código dónde se asignan los ítems iniciales. Probablemente en una función como:
- `CreateNewCharacter()`
- `InitPlayerData()`
- `_bDecodePlayerDatafileContents()`

**Paso 2:** Encuentra líneas que hacen referencia a ítems de inicio como:
```cpp
SetItemCount(iClientH, "Iron Sword", 1);
SetItemCount(iClientH, "Robe", 1);
```

**Paso 3:** Cámbialos a las armas que desees:
```cpp
SetItemCount(iClientH, "Weapon Name", 1);
SetItemCount(iClientH, "Armor Name", 1);
```

**Paso 4:** Recompila el proyecto.

---

## ?? PROCESO PASO A PASO

### Si quieres hacerlo SIN recompilar:

1. **Abre la carpeta del servidor**
   ```
   D:\HelbreathServer-main\HGServer\GameConfigs\
   ```

2. **Edita Item.cfg** con un editor de texto

3. **Busca los ítems iniciales** (generalmente "Robe", "Tunic", "Iron Sword", "Knife", etc.)

4. **Cambia sus parámetros**:
   - Nombre del ítem
   - Valor de ataque
   - Valor de defensa
   - Efectos especiales

5. **Guarda los cambios**

6. **Reinicia el servidor** (los nuevos personajes tendrán las armas nuevas)

### Si tienes que recompilar:

1. **Abre Visual Studio** con `HGserver.sln`

2. **Busca archivos clave**:
   - `Game.cpp`
   - `LoginServer.cpp`
   - `Client.cpp`

3. **Usa Ctrl+H** para buscar y reemplazar la lógica de ítems iniciales

4. **Modifica las líneas** que asignan ítems iniciales

5. **Compila**: Menú Build > Build Solution

6. **Reinicia el servidor**

---

## ?? ÍTEMS INICIALES TÍPICOS EN HELBREATH

| Tipo de Ítem | Nombre Típico | Para Cambiar |
|--------------|---------------|--------------|
| **Arma pequeña** | Knife, Short Sword | Iron Sword, Long Sword, etc. |
| **Arma grande** | Iron Sword, Long Sword | Battle Axe, Claymore, etc. |
| **Armadura** | Robe, Tunic, Leather Armor | Plate Armor, Chain Mail, etc. |
| **Escudo** | Small Shield | Large Shield, Tower Shield, etc. |
| **Ítems de consumo** | Health Potion | Mana Potion, Stamina Potion, etc. |

---

## ?? NOTAS IMPORTANTES

1. **Los ítems deben existir en Item.cfg**
   - Si cambias el nombre a un ítem que no existe, el servidor puede crashear
   - Verifica que el ítem esté definido en `Item.cfg`, `Item2.cfg` o `Item3.cfg`

2. **Los cambios solo afectan a nuevos personajes**
   - Los personajes existentes no reciben las nuevas armas
   - Solo los personajes creados DESPUÉS del cambio tendrán las armas nuevas

3. **Necesitas recompilar si:**
   - Cambias la lógica del código (qué ítems se dan)
   - Cambias cuántos ítems se dan
   - Cambias en qué orden se dan

4. **NO necesitas recompilar si:**
   - Solo cambias parámetros de ítems en Item.cfg (daño, defensa, etc.)
   - Solo cambias el nombre de ítems existentes
   - Solo cambias efectos especiales en Item.cfg

---

## ?? EJEMPLO DE CAMBIO EN Item.cfg

### ANTES:
```
item-name = Iron Sword
item-effect-type = ATTACK
item-effect-value1 = 2
item-effect-value2 = 5
```

### DESPUÉS:
```
item-name = Battle Axe
item-effect-type = ATTACK
item-effect-value1 = 4
item-effect-value2 = 8
```

---

## ?? TROUBLESHOOTING

| Problema | Solución |
|----------|----------|
| **Nuevo personaje no tiene arma** | Verifica que el ítem existe en Item.cfg |
| **Servidor crashea al crear personaje** | El nombre del ítem no existe o está mal escrito |
| **Armas antiguas siguen apareciendo** | Necesitas recompilar el servidor |
| **Cambios no tienen efecto** | Reinicia el servidor completamente |

---

## ?? ARCHIVOS A MODIFICAR

```
HGServer/
??? GameConfigs/
?   ??? Item.cfg          ? Modificar aquí (SIN recompilación)
?   ??? Item2.cfg         ? O aquí
?   ??? Item3.cfg         ? O aquí
?
??? [Código fuente] (REQUIERE RECOMPILACIÓN)
    ??? Game.cpp
    ??? Game.h
    ??? LoginServer.cpp
    ??? Client.cpp
```

---

**Última actualización**: Basado en análisis del código Helbreath 3.51
**Dificultad**: ?? (Bajo - Solo editar archivos de config)
