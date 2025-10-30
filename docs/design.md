# Diseño Técnico del Kernel Simulator

## 📐 Arquitectura del Sistema

### Visión General

El Kernel Simulator es un sistema modular que simula las funcionalidades principales de un núcleo de sistema operativo, incluyendo planificación de procesos, gestión de memoria y sincronización.

```
┌────────────────────────────────────────────────────────────┐
│                    KERNEL SIMULATOR                         │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │   CPU        │  │   Memory     │  │ Synchronization│   │
│  │   Scheduler  │  │   Manager    │  │    Module     │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
│         │                 │                  │             │
│         └─────────────────┴──────────────────┘             │
│                           │                                │
│                    ┌──────┴──────┐                        │
│                    │     PCB     │                        │
│                    │  (Process)  │                        │
│                    └─────────────┘                        │
└────────────────────────────────────────────────────────────┘
```

## 🏗️ Componentes Principales

### 1. Process Control Block (PCB)

**Propósito**: Estructura de datos que contiene toda la información de un proceso.

**Atributos**:
```cpp
struct PCB {
    // Identificación
    int id;                      // Process ID único
    
    // Estado y tipo
    ProcState state;             // Estado actual del proceso
    ProcType type;               // Tipo: NORMAL, PRODUCER, CONSUMER
    
    // Información de CPU
    int burstRemaining;          // Tiempo de CPU restante
    int arrivalTick;             // Tick de llegada al sistema
    int finishTick;              // Tick de finalización
    int waitingTime;             // Tiempo total esperando
    int turnaround;              // Tiempo de retorno (finish - arrival)
    
    // Información de memoria
    int numPages;                // Número de páginas del proceso
    int nextPageToAccess;        // Siguiente página a acceder
    int pageAccesses;            // Total de accesos a memoria
    int pageFaults;              // Fallos de página
    
    // Sincronización
    int itemsProduced;           // Items producidos (si es productor)
    int itemsConsumed;           // Items consumidos (si es consumidor)
    int blockedOnSemaphore;      // ID del semáforo que lo bloquea (-1 si no está bloqueado)
};
```

**Estados Posibles**:
```cpp
enum class ProcState {
    NEW,         // Proceso recién creado
    READY,       // Listo para ejecutar
    RUNNING,     // En ejecución
    WAITING,     // Bloqueado (esperando recurso)
    TERMINATED   // Finalizado
};
```

**Tipos de Proceso**:
```cpp
enum class ProcType {
    NORMAL,      // Proceso estándar (solo usa CPU)
    PRODUCER,    // Productor (genera items)
    CONSUMER     // Consumidor (consume items)
};
```

### 2. Memory Manager

**Propósito**: Gestiona la memoria paginada y los algoritmos de reemplazo de páginas.

**Componentes Internos**:

```cpp
class MemoryManager {
private:
    int numFrames;                              // Número de marcos físicos
    std::vector<Frame> frames;                  // Tabla de marcos
    std::queue<int> fifoQueue;                  // Cola FIFO para algoritmo FIFO
    std::map<pair<pid,page>, frameIdx> mapping; // Tabla de páginas
    std::map<pair<pid,page>, lastUse> lastUse;  // Timestamps para LRU
    PageAlgo algorithm;                         // FIFO o LRU
    int totalAccesses;                          // Contador de accesos
    int totalFaults;                            // Contador de fallos
};
```

**Estructura Frame**:
```cpp
struct Frame {
    int pid;     // ID del proceso dueño (-1 si libre)
    int page;    // Número de página almacenada (-1 si libre)
};
```

**Algoritmos de Reemplazo**:

1. **FIFO (First In First Out)**:
   - Reemplaza la página que ha estado más tiempo en memoria
   - Implementación: Cola de índices de frames
   - Complejidad: O(1) para buscar víctima

2. **LRU (Least Recently Used)**:
   - Reemplaza la página menos recientemente usada
   - Implementación: Map con timestamps
   - Complejidad: O(n) para buscar víctima

**Flujo de Acceso a Memoria**:
```
1. Proceso solicita acceso a página P
2. ¿Está P en memoria?
   ├─ SÍ: Hit → Actualizar timestamp (si LRU) → Continuar
   └─ NO: Page Fault
       ├─ ¿Hay frame libre?
       │  ├─ SÍ: Cargar página en frame libre
       │  └─ NO: Ejecutar algoritmo de reemplazo
       │      ├─ FIFO: Reemplazar página del frente de la cola
       │      └─ LRU: Reemplazar página con menor timestamp
       └─ Actualizar estructuras de datos
```

### 3. Scheduler (Round Robin)

**Propósito**: Implementa el algoritmo de planificación Round Robin con soporte para sincronización.

**Componentes**:
```cpp
class SchedulerRR {
private:
    int quantum;                        // Quantum de tiempo
    int globalTick;                     // Reloj del sistema
    int nextPid;                        // Próximo PID a asignar
    map<int, PCB> processes;            // Tabla de procesos
    queue<int> readyQueue;              // Cola de procesos listos
    int runningPid;                     // PID del proceso en ejecución
    int quantumUsed;                    // Quantum usado por proceso actual
    MemoryManager& memManager;          // Referencia a gestor de memoria
    ProducerConsumer& prodCons;         // Referencia a módulo de sincronización
};
```

**Algoritmo Round Robin**:
```
Por cada tick:
1. Si no hay proceso ejecutando, seleccionar siguiente de readyQueue
2. Incrementar waiting time de procesos en READY y WAITING
3. Si hay proceso ejecutando:
   a. Marcar como RUNNING
   b. Decrementar burstRemaining
   c. Incrementar quantumUsed
   d. Simular acceso a memoria (puede generar page fault)
   e. Si es PRODUCER o CONSUMER, intentar operación
      - Si falla (bloqueado), cambiar a WAITING
      - Si éxito, continuar
   f. Si burstRemaining == 0:
      - Marcar como TERMINATED
      - Liberar frames de memoria
   g. Si quantumUsed >= quantum:
      - Mover a readyQueue
      - Cambiar a READY
      - Resetear quantumUsed
```

**Decisión de Planificación**:
```
scheduleNext():
1. Limpiar procesos TERMINATED de readyQueue
2. Si readyQueue no está vacía:
   a. Tomar primer proceso (FIFO)
   b. Verificar que tenga burst > 0
   c. Marcarlo como proceso en ejecución
   d. Resetear quantumUsed
```

### 4. Módulo de Sincronización

**Propósito**: Implementa el problema productor-consumidor usando semáforos.

#### 4.1 Clase Semaphore

```cpp
class Semaphore {
private:
    int value;                  // Valor del semáforo
    queue<int> waitingQueue;    // Procesos esperando
    string name;                // Nombre descriptivo
    
public:
    bool tryWait(int pid);      // Intenta decrementar (puede bloquear)
    int signal();               // Incrementa y despierta proceso
};
```

**Operaciones**:

```cpp
tryWait(pid):
    if (value > 0):
        value--
        return true (éxito)
    else:
        waitingQueue.push(pid)
        return false (proceso debe bloquearse)

signal():
    value++
    if (waitingQueue no vacía):
        pid = waitingQueue.front()
        waitingQueue.pop()
        value--
        return pid (proceso a despertar)
    return -1 (nadie esperando)
```

#### 4.2 Clase ProducerConsumer

```cpp
class ProducerConsumer {
private:
    deque<int> buffer;          // Buffer circular
    int bufferSize;             // Capacidad máxima
    Semaphore empty;            // Cuenta espacios vacíos
    Semaphore full;             // Cuenta items disponibles
    Semaphore mutex;            // Exclusión mutua
    int itemCounter;            // Contador global de items
};
```

**Invariantes**:
```
empty + full = bufferSize
buffer.size() = full.value (cuando no hay procesos bloqueados)
mutex.value ∈ {0, 1}
```

**Algoritmo del Productor**:
```cpp
tryProduce(pid, &item):
    // Verificar espacio disponible
    if (!empty.tryWait(pid)):
        return BLOCKED_ON_EMPTY
    
    // Adquirir exclusión mutua
    if (!mutex.tryWait(pid)):
        empty.signal()  // Liberar empty
        return BLOCKED_ON_MUTEX
    
    // SECCIÓN CRÍTICA
    item = ++itemCounter
    buffer.push_back(item)
    
    // Liberar recursos
    mutex.signal()
    full.signal()
    
    return SUCCESS
```

**Algoritmo del Consumidor**:
```cpp
tryConsume(pid, &item):
    // Verificar item disponible
    if (!full.tryWait(pid)):
        return BLOCKED_ON_FULL
    
    // Adquirir exclusión mutua
    if (!mutex.tryWait(pid)):
        full.signal()  // Liberar full
        return BLOCKED_ON_MUTEX
    
    // SECCIÓN CRÍTICA
    item = buffer.front()
    buffer.pop_front()
    
    // Liberar recursos
    mutex.signal()
    empty.signal()
    
    return SUCCESS
```

## 🔄 Flujos de Ejecución

### Ciclo de Vida de un Proceso Normal

```
1. Creación (createProcess)
   ├─ Asignar PID único
   ├─ Inicializar PCB
   ├─ Estado = READY
   └─ Agregar a readyQueue

2. Ejecución (tick)
   ├─ Scheduler selecciona proceso
   ├─ Estado = RUNNING
   ├─ Ejecutar por 1 tick
   │  ├─ Acceder a memoria (simular)
   │  └─ Decrementar burst
   └─ Decisión:
      ├─ burst == 0 → TERMINATED
      ├─ quantum agotado → READY (volver a cola)
      └─ continuar ejecutando

3. Finalización
   ├─ Estado = TERMINATED
   ├─ Registrar finishTick
   ├─ Calcular turnaround
   └─ Liberar frames de memoria
```

### Ciclo de Vida de un Productor

```
1. Creación
   ├─ Igual que proceso normal
   └─ type = PRODUCER

2. Ejecución (cada tick)
   ├─ Acceder a memoria
   └─ Intentar producir:
      ├─ Éxito:
      │  ├─ itemsProduced++
      │  └─ Continuar
      └─ Bloqueado:
         ├─ Estado = WAITING
         ├─ blockedOnSemaphore = {empty|mutex}
         └─ Sacar de ejecución

3. Desbloqueo
   ├─ Otro proceso hace signal()
   ├─ Estado = READY
   ├─ blockedOnSemaphore = -1
   └─ Agregar a readyQueue

4. Finalización
   └─ Igual que proceso normal
```

### Ciclo de Vida de un Consumidor

```
(Similar al productor, pero con tryConsume)
```

## 📊 Estructuras de Datos

### Tabla de Procesos
```cpp
map<int, PCB> processes
```
- **Clave**: Process ID
- **Valor**: Process Control Block
- **Complejidad**: O(log n) búsqueda, inserción, eliminación

### Cola de Listos
```cpp
queue<int> readyQueue
```
- **Contenido**: PIDs de procesos en estado READY
- **Orden**: FIFO
- **Complejidad**: O(1) enqueue, dequeue

### Tabla de Páginas
```cpp
map<pair<int,int>, int> mapping
```
- **Clave**: (PID, Page Number)
- **Valor**: Frame Index
- **Complejidad**: O(log n) búsqueda

### Buffer Compartido
```cpp
deque<int> buffer
```
- **Contenido**: Items producidos
- **Operaciones**: push_back (productor), pop_front (consumidor)
- **Complejidad**: O(1) para ambas operaciones

## 🎯 Decisiones de Diseño

### 1. ¿Por qué usar map en lugar de array para procesos?

**Ventajas**:
- PIDs no necesitan ser consecutivos
- Fácil eliminar procesos (O(log n) vs O(n) con desplazamiento)
- Búsqueda eficiente por PID

**Desventajas**:
- Más memoria (overhead de árbol rojo-negro)
- Acceso más lento que array (O(log n) vs O(1))

**Decisión**: Map es mejor porque la flexibilidad supera el costo.

### 2. ¿Por qué separar SchedulerRR y SchedulerSJF?

**Alternativa considerada**: Una sola clase con parámetro de algoritmo

**Decisión**: Clases separadas porque:
- SJF usa vector en lugar de queue
- Lógica de selección completamente diferente
- Más fácil de extender (agregar nuevos algoritmos)
- Código más limpio y mantenible

### 3. ¿Por qué usar deque para el buffer?

**Alternativas**:
- `queue`: No permite acceso aleatorio para visualización
- `vector`: pop_front es O(n)
- `list`: Más overhead de memoria

**Decisión**: deque es óptimo porque:
- push_back y pop_front son O(1)
- Permite iteración para mostrar contenido
- Menor overhead que list

### 4. ¿Por qué no implementar desbloqueo automático?

**Diseño actual**: Procesos bloqueados permanecen en WAITING hasta que otro proceso hace signal()

**Alternativa considerada**: Verificar periódicamente si el recurso está disponible

**Decisión**: Diseño actual es más realista:
- Simula cómo funcionan los semáforos reales
- Más eficiente (no requiere polling)
- Educativamente correcto

### 5. ¿Por qué la memoria accede a cada tick?

**Diseño actual**: Cada proceso accede a una página por tick

**Alternativa**: Acceso solo cuando es necesario

**Decisión**: Diseño actual porque:
- Simula comportamiento real (cada instrucción accede memoria)
- Genera suficientes page faults para análisis
- Más simple de implementar

## 🔐 Propiedades de Corrección

### Exclusión Mutua
```
∀t: |{p : p.state == RUNNING && p está en sección crítica}| ≤ 1
```
Garantizado por: Semáforo mutex (binario)

### Ausencia de Deadlock
```
No existe ciclo de espera en el grafo de recursos
```
Garantizado por: Orden consistente de adquisición de semáforos

### Ausencia de Starvation
```
∀p: p.state == WAITING → ∃t': t' > t ∧ p.state == READY
```
Garantizado por: Colas FIFO en semáforos

### Progreso
```
Si hay procesos en READY, el sistema eventualmente ejecutará alguno
```
Garantizado por: Scheduler siempre selecciona si readyQueue no vacía

## 📈 Complejidad Temporal

### Operaciones del Scheduler

| Operación | Complejidad | Justificación |
|-----------|-------------|---------------|
| createProcess | O(log n) | Inserción en map |
| tick | O(n + log m) | Iterar procesos + acceso memoria |
| scheduleNext | O(k) | k = procesos en readyQueue |
| killProcess | O(log n + f) | Búsqueda en map + liberar frames |
| listProcesses | O(n) | Iterar todos los procesos |

donde:
- n = número total de procesos
- m = número de páginas mapeadas
- f = número de frames
- k = procesos en cola (k ≤ n)

### Operaciones de Memoria

| Operación | Complejidad | Justificación |
|-----------|-------------|---------------|
| access (hit) | O(log m) | Búsqueda en map |
| access (miss, free) | O(log m) | Inserción en map |
| access (miss, FIFO) | O(log m) | Eliminar + insertar en map |
| access (miss, LRU) | O(m) | Buscar mínimo en map |
| freeFramesOfPid | O(f * log m) | Iterar frames, eliminar de map |

### Operaciones de Sincronización

| Operación | Complejidad | Justificación |
|-----------|-------------|---------------|
| tryWait | O(1) | Operaciones en valor y queue |
| signal | O(1) | Operaciones en valor y queue |
| tryProduce/Consume | O(1) | Operaciones de deque |

## 🧪 Casos de Prueba Críticos

### Test 1: Race Condition en Buffer
```
Escenario: 2 productores, 1 consumidor, buffer=1
Esperado: Nunca más de 1 item en buffer
Verificar: mutex funciona correctamente
```

### Test 2: Page Replacement
```
Escenario: 1 proceso, 5 páginas, 3 frames
Esperado: Page faults según algoritmo (FIFO vs LRU)
Verificar: Contadores y hit rate correctos
```

### Test 3: Quantum Expiration
```
Escenario: 2 procesos, burst=10, quantum=3
Esperado: Alternancia cada 3 ticks
Verificar: Fairness en CPU time
```

### Test 4: Deadlock Free
```
Escenario: 5 productores, 5 consumidores, burst alto
Esperado: Sistema siempre progresa
Verificar: Ningún proceso queda permanentemente bloqueado
```

## 🚀 Posibles Extensiones

### Corto Plazo
1. Agregar planificador por prioridades
2. Implementar algoritmo SCAN para disco
3. Agregar múltiples buffers (varios recursos)
4. Implementar Reader-Writer problem

### Mediano Plazo
1. Separar código en módulos (.h y .cpp)
2. Agregar tests unitarios
3. Implementar detección de deadlocks
4. Agregar visualización gráfica (curses/Qt)

### Largo Plazo
1. Simular múltiples CPUs (SMP)
2. Implementar memoria virtual con swap
3. Agregar sistema de archivos simulado
4. Implementar scheduling de disco

## 📚 Referencias Teóricas

### Algoritmos Implementados
- **Round Robin**: Silberschatz 6.3.3
- **FIFO Page Replacement**: Silberschatz 10.4.1
- **LRU Page Replacement**: Silberschatz 10.4.4
- **Semáforos**: Dijkstra (1965), THE system
- **Productor-Consumidor**: Clásico de sincronización

### Conceptos Aplicados
- Process Control Block (PCB)
- Context Switching
- Page Table
- Translation Lookaside Buffer (simplificado)
- Critical Section
- Mutual Exclusion
- Bounded Buffer Problem

## 📝 Notas de Implementación

### Limitaciones Conocidas
1. No hay cambio de contexto real (no se guarda/restaura estado)
2. Memoria simulada (no hay latencia real)
3. Disco no implementado (páginas aparecen instantáneamente)
4. Un solo CPU (no concurrencia real)

### Simplificaciones Educativas
1. Un tick = una operación completa
2. Acceso a memoria por tick (no por instrucción)
3. Páginas secuenciales (patrón predecible)
4. Semáforos sin prioridades

### Trade-offs de Diseño
- **Simplicidad vs Realismo**: Prioriza claridad educativa
- **Eficiencia vs Flexibilidad**: Prioriza código mantenible
- **Completitud vs Complejidad**: Implementa lo esencial bien
