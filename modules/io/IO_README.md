# Módulo IO - Gestión de Dispositivos de E/S

## 📋 Descripción
Implementa gestión de dispositivos de E/S con cola de prioridad y procesamiento concurrente.

## 🔧 Componentes

### **IOManager.h / IOManager.cpp**
Gestor de dispositivos con prioridades.

#### **Configuración**
- **3 Dispositivos**: PRINTER, DISK, NETWORK
- **Prioridades**: 1 (alta) a 5 (baja)
- **Cola**: Min-heap (priority_queue)

## 🖨️ Dispositivos

### 1. **PRINTER**
- Operaciones: Impresión de documentos
- Típicamente lento
- Compartido por múltiples procesos

### 2. **DISK**
- Operaciones: Lectura/escritura
- Rápido, acceso aleatorio
- Alta demanda

### 3. **NETWORK**
- Operaciones: Transmisión de datos
- Variable (depende de red)
- Tiempo real crítico

## 📊 Sistema de Prioridades

| Prioridad | Nivel | Uso Típico |
|-----------|-------|------------|
| 1 | CRÍTICA | Interrupciones, sistema |
| 2 | ALTA | Tiempo real, red |
| 3 | NORMAL | Usuario interactivo |
| 4 | BAJA | Background, batch |
| 5 | MUY BAJA | Idle, mantenimiento |

**Regla**: Menor número = Mayor prioridad

## 🎯 Uso

### Inicialización
```cpp
#include "IOManager.h"

IOManager io;
```

### Agregar Solicitudes
```cpp
// Solicitud crítica de impresión
IORequest printReq = {
    pid: 1,
    priority: 1,         // CRÍTICA
    device: IODevice::PRINTER,
    duration: 5          // 5 ticks
};
io.addIORequest(printReq);

// Solicitud normal de red
IORequest netReq = {
    pid: 2,
    priority: 3,         // NORMAL
    device: IODevice::NETWORK,
    duration: 2
};
io.addIORequest(netReq);
```

### Procesar I/O (cada tick)
```cpp
io.processIOTick();  // Procesa todos los dispositivos
```

### Visualización
```cpp
io.showStatus();
```

**Salida**:
```
=== GESTOR DE I/O ===
Cola de prioridad: 3 solicitudes

Dispositivos:
┌─────────┬────────┬─────────┬─────────────┐
│ Device  │  PID   │ Estado  │ Tiempo Rest │
├─────────┼────────┼─────────┼─────────────┤
│ PRINTER │   1    │  BUSY   │     3       │
│ DISK    │   -    │  IDLE   │     -       │
│ NETWORK │   5    │  BUSY   │     1       │
└─────────┴────────┴─────────┴─────────────┘

Estadísticas:
  Solicitudes completadas: 12
  Tiempo promedio espera: 2.5 ticks
  Throughput: 0.12 req/tick
```

### Obtener Métricas
```cpp
float avgWait = io.getAverageWaitTime();
int completed = io.getCompletedRequests();
```

## 📈 Métricas

| Métrica | Descripción | Fórmula |
|---------|-------------|---------|
| **Throughput** | Solicitudes por tick | completed / globalTick |
| **Avg Wait Time** | Espera promedio | sum(wait) / completed |
| **Utilization** | Uso del dispositivo | busy_time / total_time |
| **Queue Length** | Solicitudes pendientes | queue.size() |

## 🔍 Algoritmo de Planificación

### Priority Queue (Min-Heap)
```cpp
struct IORequest {
    int pid;
    int priority;      // 1 = alta, 5 = baja
    IODevice device;
    int duration;
    int arrivalTime;
};

// Comparador: menor priority = mayor prioridad
bool operator<(const IORequest& other) const {
    return priority > other.priority;  // Invertido para min-heap
}
```

### Procesamiento
```
1. Para cada dispositivo:
   a. Si IDLE y hay solicitudes en cola para ese dispositivo:
      - Extraer solicitud de mayor prioridad
      - Asignar a dispositivo
      - Marcar como BUSY
   
   b. Si BUSY:
      - Decrementar tiempo restante
      - Si termina:
        * Marcar como IDLE
        * Actualizar estadísticas
        * Desbloquear proceso
```

## 🎮 Ejemplo Completo

```cpp
#include "IOManager.h"
#include <iostream>

int main() {
    IOManager io;
    
    // Simular múltiples solicitudes
    io.addIORequest({1, 1, IODevice::PRINTER, 5, 0});  // Alta prioridad
    io.addIORequest({2, 3, IODevice::DISK, 2, 0});     // Normal
    io.addIORequest({3, 5, IODevice::NETWORK, 3, 0});  // Baja
    io.addIORequest({4, 1, IODevice::PRINTER, 4, 0});  // Alta
    
    // Simular 20 ticks
    for (int tick = 0; tick < 20; tick++) {
        std::cout << "\n=== Tick " << tick << " ===\n";
        io.processIOTick();
        io.showStatus();
    }
    
    // Mostrar estadísticas finales
    std::cout << "\n=== RESUMEN FINAL ===\n";
    std::cout << "Completadas: " << io.getCompletedRequests() << "\n";
    std::cout << "Espera promedio: " << io.getAverageWaitTime() << " ticks\n";
    
    return 0;
}
```

## 🧪 Experimentos Sugeridos

### Experimento 1: Inversión de Prioridad
```
Script: scripts/io_priority.txt
Agregar: P1 (pri=5), P2 (pri=1)
Observar: P2 se ejecuta primero aunque llegó después
```

### Experimento 2: Starvation Prevention
```
Script: scripts/io_starvation.txt
Agregar: 10 solicitudes de pri=1
Luego: 1 solicitud de pri=5
Observar: ¿Cuánto espera la pri=5?
```

### Experimento 3: Carga por Dispositivo
```
Script: scripts/io_load.txt
Agregar: 5 PRINTER, 10 DISK, 3 NETWORK
Medir: Utilización de cada dispositivo
```

### Experimento 4: Throughput vs Carga
```
Script: scripts/io_throughput.txt
Variar: Número de solicitudes (10, 50, 100)
Graficar: Throughput vs Carga
```

## 🔄 Integración con CPU

### Bloqueo de Procesos
Cuando un proceso solicita I/O:
```cpp
// En el planificador de CPU
if (proceso necesita I/O) {
    proceso.state = ProcState::WAITING;
    io.addIORequest({proceso.pid, priority, device, duration, tick});
}
```

### Desbloqueo de Procesos
Cuando se completa I/O:
```cpp
// En IOManager::processIOTick()
if (solicitud completada) {
    // Notificar al planificador
    scheduler.unblock(pid);  // Cambiar a READY
}
```

## 📊 Casos de Uso Óptimos

### Priority Queue
- ✅ Sistemas de tiempo real (prioridades estrictas)
- ✅ Multiprogramación con QoS
- ✅ Diferentes clases de servicio

### Round Robin (alternativa)
- ✅ Justicia estricta
- ❌ No respeta urgencia

### FCFS (alternativa)
- ✅ Simple, predecible
- ❌ No maneja prioridades

## 🐛 Debugging

### Verificar Estado de Cola
```cpp
int pending = io.getPendingRequests();
cout << "Solicitudes pendientes: " << pending << endl;
```

### Verificar Dispositivo
```cpp
bool busy = io.isDeviceBusy(IODevice::PRINTER);
if (busy) {
    int pid = io.getCurrentPID(IODevice::PRINTER);
    cout << "PRINTER ocupado por PID " << pid << endl;
}
```

### Resetear Estadísticas
```cpp
io.reset();  // Limpia cola y estadísticas
```

## 🎨 Visualización Avanzada

### Diagrama de Gantt para I/O
```
PRINTER: |--P1--|  idle  |--P4--|
DISK:    |--P2--||--P5--|  idle
NETWORK: |-----P3-----|  idle   
         0    5    10   15   20
```

### Cola de Prioridad
```
Cola: [P4(1)] → [P1(1)] → [P2(3)] → [P3(5)]
       ↑ TOP (siguiente a procesar)
```

## 📚 Referencias
- Tanenbaum, "Modern Operating Systems", Cap. 5
- Silberschatz, "Operating System Concepts", Cap. 13
- Priority Queue: Min-heap, O(log n) insert/extract
- DMA: Direct Memory Access (no simulado aquí)
- Interrupts: Manejo de interrupciones (no simulado)

## 🚀 Extensiones Posibles
- [ ] DMA para transferencias grandes
- [ ] Buffering y caching
- [ ] Interrupciones asíncronas
- [ ] Spooling para impresoras
- [ ] Controladores específicos por dispositivo
- [ ] Detección de deadlock en I/O
