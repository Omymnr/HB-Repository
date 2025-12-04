# LISTA DE ARMAS Y ARMADURAS INICIALES DISPONIBLES
## Helbreath 3.51 - Item.cfg

---

## ??? ARMAS PEQUEÑAS (ONE-HAND)

| Nombre | Daño | Defensa | Nivel Recomendado | Notas |
|--------|------|---------|-------------------|-------|
| Knife | 1-3 | - | 1 | Arma muy débil, inicio básico |
| Short Sword | 2-4 | - | 2 | Mejora del knife |
| Iron Sword | 3-5 | - | 3 | Espada estándar de inicio |
| Copper Dagger | 2-3 | - | 1 | Alternativa al knife |
| Wooden Sword | 1-2 | - | 1 | Práctica/Entrenamiento |
| Steel Dagger | 3-4 | - | 2 | Mejor que dagger de cobre |

---

## ?? ARMAS GRANDES (TWO-HAND)

| Nombre | Daño | Defensa | Nivel Recomendado | Notas |
|--------|------|---------|-------------------|-------|
| Long Sword | 4-6 | - | 4 | Arma dos manos básica |
| Battle Axe | 5-7 | - | 5 | Más daño que Long Sword |
| Great Sword | 6-8 | - | 6 | Espada muy poderosa |
| War Hammer | 5-7 | - | 5 | Alternativa a Battle Axe |
| Halberd | 5-8 | - | 6 | Lanza combinada |
| Claymore | 7-9 | - | 8 | Espada enorme |

---

## ??? ARMADURAS/ROBES (CUERPO)

| Nombre | Defensa | Salud | Nivel Recomendado | Notas |
|--------|---------|-------|-------------------|-------|
| Robe | 0 | - | 1 | Túnica básica |
| Tunic | 1 | - | 1 | Levadura estándar |
| Leather Armor | 2 | - | 2 | Cuero básico |
| Iron Armor | 3 | - | 3 | Armadura de hierro |
| Chain Mail | 3 | - | 3 | Malla de cadenas |
| Plate Armor | 4 | - | 4 | Armadura de placas |
| Steel Armor | 5 | - | 5 | Acero completo |
| Dragon Scale Armor | 6 | - | 8 | Armadura mítica |

---

## ??? ESCUDOS

| Nombre | Defensa | Bloqueo | Nivel Recomendado | Notas |
|--------|---------|---------|-------------------|-------|
| Small Shield | 1 | 5% | 1 | Escudo pequeño básico |
| Shield | 2 | 10% | 2 | Escudo mediano |
| Iron Shield | 2 | 12% | 3 | Escudo de hierro |
| Large Shield | 3 | 15% | 3 | Escudo grande |
| Tower Shield | 4 | 20% | 5 | Escudo enorme |
| Diamond Shield | 5 | 25% | 6 | Escudo reforzado |

---

## ?? ACCESORIOS

| Nombre | Efecto | Bonificación | Nivel Recomendado |
|--------|--------|--------------|-------------------|
| Leather Glove | Ataque | +1 | 1 |
| Leather Boot | Velocidad | +5% | 1 |
| Copper Ring | HP | +10 | 1 |
| Iron Ring | Defensa | +2 | 2 |
| Necklace | Magia | +5% | 2 |

---

## ?? ÍTEMS DE CONSUMO (INICIO)

| Nombre | Efecto | Cantidad | Notas |
|--------|--------|----------|-------|
| Health Potion | +50 HP | 2-5 | Pociones de salud básicas |
| Mana Potion | +30 MP | 1-2 | Pociones de maná |
| Stamina Potion | +20 SP | 1 | Pociones de resistencia |
| Cure Potion | Cura veneno | 1 | Para emergencias |

---

## ? COMBINACIONES RECOMENDADAS DE INICIO

### Opción 1: PRINCIPIANTE EQUILIBRADO (Predeterminado)
```
Robe + Small Shield + Iron Sword + Health Potion (x3)
```

### Opción 2: GUERRERO AGRESIVO
```
Leather Armor + Iron Sword (2H) + Health Potion (x5)
```

### Opción 3: MAGO DEFENSIVO
```
Tunic + Small Shield + Short Sword + Mana Potion (x3) + Health Potion (x2)
```

### Opción 4: CABALLERO EQUILIBRADO
```
Chain Mail + Large Shield + Long Sword + Health Potion (x4)
```

### Opción 5: ASESINO RÁPIDO
```
Leather Armor + Knife + Copper Dagger + Health Potion (x2)
```

---

## ?? CÓMO ENCONTRAR ÍTEMS EN Item.cfg

**Formato típico:**

```
item-name = Iron Sword
item-effect-type = ATTACK
item-effect-value1 = 3
item-effect-value2 = 5
item-level-limit = 1
item-weight = 15
item-price = 500
```

**Para buscar:**
1. Abre `GameConfigs\Item.cfg`
2. Presiona **Ctrl+F** 
3. Busca por nombre (ej: "Iron Sword")
4. Anota los parámetros
5. Edita según sea necesario

---

## ?? EFECTOS DE ÍTEMS DISPONIBLES

| Código | Efecto | Ejemplo |
|--------|--------|---------|
| 0 | Ninguno | - |
| 1 | Ataque | Armas |
| 2 | Defensa | Armaduras |
| 4 | HP + | Pociones |
| 5 | MP + | Pociones mágicas |
| 6 | SP + | Pociones de resistencia |

---

## ?? CÓMO HACER UN SETUP PERSONALIZADO

### Paso 1: Decide qué quieres
Ejemplo: Quiero que los nuevos personajes empiecen con una espada grande y armadura de cadena

### Paso 2: Busca los ítems en Item.cfg
- Busca "Battle Axe"
- Busca "Chain Mail"

### Paso 3: Verifica que existan
Si no existen, crea nuevas definiciones en Item.cfg

### Paso 4: Modifica el código (si es necesario)
Si necesitas más de dos ítems iniciales, modifica el código fuente

### Paso 5: Recompila
Ejecuta Build > Build Solution en Visual Studio

### Paso 6: Prueba
Crea un nuevo personaje y verifica que tenga los ítems correctos

---

## ?? PARÁMETROS EDITABLES POR ÍTEM

### Nombre del Ítem
```
item-name = NOMBRE_AQUI
```
**Rango**: Máx 20 caracteres

### Tipo de Efecto
```
item-effect-type = 0-25 (ver tabla arriba)
```

### Valor de Daño/Defensa
```
item-effect-value1 = 0-100 (daño mínimo o defensa)
item-effect-value2 = 0-100 (daño máximo)
```

### Límite de Nivel para Usar
```
item-level-limit = 1-180 (nivel mínimo requerido)
```

### Peso del Ítem
```
item-weight = 1-1000 (peso en inventario)
```

### Precio en Tienda
```
item-price = 0-9999999 (monedas de oro)
```

---

## ?? EJEMPLO COMPLETO DE CAMBIO

### ANTES (Setup predeterminado):
```
Weapon: Iron Sword (daño 3-5)
Armor: Robe (defensa 0)
Shield: Small Shield (defensa 1)
Consumables: 3x Health Potion
```

### DESPUÉS (Setup mejorado):
```
Weapon: Battle Axe (daño 5-7)
Armor: Chain Mail (defensa 3)
Shield: Iron Shield (defensa 2)
Consumables: 5x Health Potion + 2x Mana Potion
```

**Cambios en Item.cfg:**
1. Busca las definiciones de cada ítem
2. Modifica los valores
3. Guarda
4. Reinicia servidor

---

## ? CHECKLIST ANTES DE IMPLEMENTAR

- [ ] Todos los ítems existen en Item.cfg
- [ ] Los nombres están escritos exactamente igual (mayúsculas/minúsculas)
- [ ] Los niveles de los ítems son apropiados para inicio
- [ ] Hay suficientes pociones de salud
- [ ] Los pesos totales no exceden capacidad de inicio
- [ ] Probaste con un nuevo personaje
- [ ] Confirmaste que los ítems aparecen correctamente

---

**Última actualización**: Helbreath 3.51
**Nota**: Los valores exactos pueden variar según tu configuración de servidor
