# Módulo DISK - Planificación de Disco

## 📋 Descripción
Implementa algoritmos de planificación de acceso a disco para optimizar el movimiento del cabezal.

## 🔧 Componentes

### **DiskScheduler.h / DiskScheduler.cpp**
Planificador de disco con 3 algoritmos clásicos.

#### **Configuración**
- Cilindros: 200 (0-199)
- Posición inicial: 50
- Algoritmos: FCFS, SSTF, SCAN

## 📊 Algoritmos de Planificación

### 1. **FCFS (First Come First Served)**
- **Complejidad**: O(1) por solicitud
- **Estrategia**: Orden de llegada
- **Ventajas**: Justo, simple, sin inanición
- **Desventajas**: Movimiento no optimizado

```cpp
DiskScheduler disk(200, 50, DiskAlgo::FCFS);
disk.addRequest(90);
disk.addRequest(10);
disk.processNext();  // Va a 90 primero
```

**Movimiento**: 50→90→10 = 40+80 = **120 cilindros**

### 2. **SSTF (Shortest Seek Time First)**
- **Complejidad**: O(n) por solicitud
- **Estrategia**: Solicitud más cercana
- **Ventajas**: Mínimo movimiento local
- **Desventajas**: Posible inanición

```cpp
DiskScheduler disk(200, 50, DiskAlgo::SSTF);
disk.addRequest(90);
disk.addRequest(55);
disk.addRequest(10);
disk.processNext();  // Va a 55 (más cercano)
```

**Movimiento**: 50→55→90→10 = 5+35+80 = **120 cilindros**
*Pero con más solicitudes puede ser mucho mejor*

### 3. **SCAN (Elevator Algorithm)** ⭐
- **Complejidad**: O(n log n) para ordenar
- **Estrategia**: Barrido en una dirección
- **Ventajas**: No hay inanición, predecible
- **Desventajas**: Espera promedio mayor

```cpp
DiskScheduler disk(200, 50, DiskAlgo::SCAN);
disk.addRequest(90);
disk.addRequest(10);
disk.addRequest(150);
// Dirección inicial: derecha
```

**Movimiento**: 50→90→150→199→10 = 40+60+49+189 = **338 cilindros**
*Barrer hasta el final*

**Optimización C-SCAN**: Volver al inicio sin servir

## 📈 Métricas

| Métrica | Descripción | Unidad |
|---------|-------------|---------|
| **Movimiento Total** | Cilindros recorridos | cilindros |
| **Solicitudes Servidas** | Total procesadas | count |
| **Throughput** | Solicitudes/movimiento | req/cyl |
| **Avg Seek Time** | Promedio por solicitud | cilindros |

## 🎯 Uso

### Inicialización
```cpp
#include "DiskScheduler.h"

// 200 cilindros, inicio en 50, algoritmo SCAN
DiskScheduler disk(200, 50, DiskAlgo::SCAN);
```

### Agregar Solicitudes
```cpp
disk.addRequest(90);
disk.addRequest(150);
disk.addRequest(10);
disk.addRequest(120);
```

### Procesar Siguiente
```cpp
if (disk.processNext()) {
    cout << "Solicitud procesada\n";
    cout << "Posición actual: " << disk.getCurrentPosition() << endl;
} else {
    cout << "No hay solicitudes pendientes\n";
}
```

### Visualización
```cpp
disk.showStatus();
```

**Salida**:
```
=== PLANIFICADOR DE DISCO (SCAN) ===
Posición actual: 90
Dirección: RIGHT
Cola: [150, 120, 10]

Disco: [--HEAD--  .  .  .  *  .  *  .  *  ]
       0     50     90    120   150   199

Estadísticas:
  Movimiento total: 40 cilindros
  Solicitudes servidas: 1
  Throughput: 0.025 req/cyl
```

### Comparación de Algoritmos
```cpp
disk.showComparison();
```

**Salida**:
```
=== COMPARACIÓN DE ALGORITMOS ===
Solicitudes: [90, 150, 10, 120]
Posición inicial: 50

┌──────────┬────────────────┬───────────┬────────────┐
│ Algoritmo│  Movimiento    │ Eficiencia│ Secuencia  │
├──────────┼────────────────┼───────────┼────────────┤
│ FCFS     │  330 cilindros │  0.012    │ 90→150→10→ │
│ SSTF     │  180 cilindros │  0.022    │ 90→120→150 │
│ SCAN     │  259 cilindros │  0.015    │ 90→120→150 │
└──────────┴────────────────┴───────────┴────────────┘
```

### Cambiar Algoritmo
```cpp
disk.changeAlgorithm(DiskAlgo::SSTF);
```

### Limpiar Cola
```cpp
disk.clearQueue();  // Elimina todas las solicitudes pendientes
```

## 🔍 Comparación de Algoritmos

| Característica | FCFS | SSTF | SCAN |
|----------------|------|------|------|
| Simplicidad | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| Eficiencia | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| Justicia | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| Inanición | ✅ Nunca | ❌ Posible | ✅ Nunca |
| Predecibilidad | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ |

## 📊 Casos de Uso Óptimos

### FCFS
- Carga ligera con pocas solicitudes
- Sistemas donde la justicia es crítica
- Debugging y testing

### SSTF
- Alta carga con localidad espacial
- Maximizar throughput a corto plazo
- Sistemas donde la inanición es manejada por capas superiores

### SCAN
- Carga variable con requisitos de QoS
- Sistemas de tiempo real (predecible)
- Balance entre eficiencia y justicia

## 🧪 Experimentos Sugeridos

### Experimento 1: Peor caso FCFS
```
Script: scripts/disk_fcfs_worst.txt
Solicitudes: 0, 199, 0, 199, 0, 199
Inicio: 100
Movimiento: ~600 cilindros
```

### Experimento 2: Inanición en SSTF
```
Script: scripts/disk_sstf_starvation.txt
Inicio: 100
Añadir: 90, 110, 95, 105, 93, 107...
Observar: Solicitudes lejanas esperan mucho
```

### Experimento 3: Eficiencia de SCAN
```
Script: scripts/disk_scan_efficiency.txt
Solicitudes: Distribuidas uniformemente
Comparar: Total movement SCAN vs FCFS
Resultado: SCAN gana en carga alta
```

### Experimento 4: Comparación completa
```
Script: scripts/disk_comparison.txt
Solicitudes: 95, 180, 34, 119, 11, 124, 65, 67
Inicio: 50
Ejecutar: showComparison()
```

## 🐛 Debugging

### Verificar Estado
```cpp
cout << "Posición: " << disk.getCurrentPosition() << endl;
cout << "Pendientes: " << disk.getPendingRequests() << endl;
cout << "Movimiento: " << disk.getTotalMovement() << endl;
```

### Resetear
```cpp
disk.reset(50);  // Vuelve a posición inicial, limpia estadísticas
```

## 🎮 Visualización ASCII

```
Cilindro:  0    25   50   75   100  125  150  175  199
           |     |    |    |     |    |    |    |    |
Requests:  *          *         *         *
                      ^HEAD
```

## 📚 Referencias
- Tanenbaum, "Modern Operating Systems", Cap. 5
- Silberschatz, "Operating System Concepts", Cap. 10
- C-SCAN y LOOK: Variantes optimizadas
- N-step SCAN: Prevención de inanición
