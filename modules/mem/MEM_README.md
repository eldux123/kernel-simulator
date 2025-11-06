# Módulo MEM - Gestión de Memoria Virtual

## 📋 Descripción
Implementa memoria virtual con paginación bajo demanda y 3 algoritmos de reemplazo de páginas.

## 🔧 Componentes

### **MemoryManager.h / MemoryManager.cpp**
Gestión de memoria con tabla de marcos (frames).

#### **Configuración**
- Marcos de memoria: Configurable (defecto: 10)
- Tamaño de página: Fijo (4 KB conceptual)
- Algoritmos disponibles: FIFO, LRU, PFF

## 📊 Algoritmos de Reemplazo

### 1. **FIFO (First In First Out)**
- **Complejidad**: O(1)
- **Estrategia**: Reemplaza la página más antigua
- **Implementación**: Cola circular con índice
- **Ventajas**: Simple, predecible
- **Desventajas**: Anomalía de Belady

```cpp
MemoryManager mem(10, PageAlgo::FIFO);
mem.access(1, 5);  // Proceso 1, página 5
```

### 2. **LRU (Least Recently Used)**
- **Complejidad**: O(n)
- **Estrategia**: Reemplaza la página menos usada recientemente
- **Implementación**: Contador lastUsed por frame
- **Ventajas**: Buen rendimiento, evita Belady
- **Desventajas**: Overhead de actualización

```cpp
MemoryManager mem(10, PageAlgo::LRU);
mem.access(1, 5);  // Actualiza tiempo de uso
```

### 3. **PFF (Page Fault Frequency)** ⭐
- **Complejidad**: O(n)
- **Estrategia**: Ajusta marcos según frecuencia de fallos
- **Parámetros**:
  - `pffThreshold`: 3 fallos
  - `pffWindowSize`: 10 ticks
  - Ajuste dinámico de marcos por proceso
- **Ventajas**: Adapta a comportamiento
- **Desventajas**: Más complejo

```cpp
MemoryManager mem(10, PageAlgo::PFF);
mem.setPFFParams(3, 10);  // threshold=3, window=10
```

## 📈 Métricas

| Métrica | Descripción | Fórmula |
|---------|-------------|---------|
| **Hit Rate** | % de accesos sin fallo | hits / (hits + misses) × 100 |
| **Miss Rate** | % de fallos de página | misses / (hits + misses) × 100 |
| **Page Faults** | Total de fallos | count |

## 🎯 Uso

### Inicialización
```cpp
#include "MemoryManager.h"

// Crear con 10 marcos, algoritmo LRU
MemoryManager memory(10, PageAlgo::LRU);
```

### Acceso a Página
```cpp
int pid = 1;
int pageNumber = 5;

bool hit = memory.access(pid, pageNumber);
if (hit) {
    cout << "HIT: Página en memoria\n";
} else {
    cout << "MISS: Page fault\n";
}
```

### Visualización
```cpp
memory.showFrames();
```

**Salida**:
```
=== MARCOS DE MEMORIA (LRU) ===
Frame | PID  | Página
------|------|-------
  0   |  1   |   3
  1   |  1   |   5
  2   |  2   |   1
  3   |  -   |  FREE
...

Estadísticas:
  Hits: 45
  Misses: 12
  Hit Rate: 78.95%
```

### Cambiar Algoritmo
```cpp
memory.changeAlgorithm(PageAlgo::PFF);
memory.setPFFParams(4, 15);  // Ajustar parámetros
```

### Liberar Proceso
```cpp
memory.removeProcess(pid);  // Libera todos los marcos del proceso
```

## 🔍 Comparación de Algoritmos

| Característica | FIFO | LRU | PFF |
|----------------|------|-----|-----|
| Simplicidad | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| Rendimiento | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| Overhead | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| Adaptabilidad | ⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| Anomalía Belady | ❌ | ✅ | ✅ |

## 📊 Casos de Uso Óptimos

### FIFO
- Sistemas simples con poca variación
- Recursos limitados (memoria muy pequeña)
- Acceso secuencial predecible

### LRU
- Aplicaciones con localidad temporal
- Balance entre rendimiento y complejidad
- Carga de trabajo moderada

### PFF
- Multiprogramación dinámica
- Carga variable con picos
- Sistemas que requieren QoS adaptativo

## 🧪 Experimentos Sugeridos

### Experimento 1: Comparar FIFO vs LRU
```
Script: scripts/mem_comparison.txt
Accesos: 1-2-3-4-1-2-5-1-2-3-4-5
Marcos: 3
Esperado: LRU < FIFO fallos
```

### Experimento 2: Anomalía de Belady (FIFO)
```
Script: scripts/mem_belady.txt
Accesos: 1-2-3-4-1-2-5-1-2-3-4-5
Probar: 3 vs 4 marcos
Resultado: Más marcos = más fallos!
```

### Experimento 3: Adaptación PFF
```
Script: scripts/mem_pff.txt
Fase 1: Alta carga (muchos accesos)
Fase 2: Baja carga (pocos accesos)
Observar: Ajuste dinámico de marcos
```

## 🐛 Debugging

### Verificar Estado
```cpp
cout << "Marcos libres: " << memory.getFreeFrames() << endl;
cout << "Hit rate: " << memory.getHitRate() << "%" << endl;
```

### Resetear Estadísticas
```cpp
memory.reset();  // Limpia marcos y estadísticas
```

## 📚 Referencias
- Tanenbaum, "Modern Operating Systems", Cap. 3
- Silberschatz, "Operating System Concepts", Cap. 9
- PFF: Working Set Model, Denning (1968)
