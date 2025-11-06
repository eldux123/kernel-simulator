# Módulo CPU - Gestión de Procesos y Sincronización

## 📋 Descripción
Este módulo implementa la gestión de procesos, planificación y sincronización del kernel simulator.

## 🔧 Componentes

### 1. **Process.h / Process.cpp**
Estructuras y tipos fundamentales:
- **PCB (Process Control Block)**: Control completo del proceso
  - 6 tipos: NORMAL, PRODUCER, CONSUMER, PHILOSOPHER, READER, WRITER
  - 6 estados: NEW, READY, RUNNING, WAITING, SUSPENDED, TERMINATED
  - Soporte para hasta 4 hilos por proceso
- **Thread**: Estructura de hilo con estado independiente
- **Operaciones**: suspend, resume, kill

### 2. **Synchronization.h / Synchronization.cpp**
Problemas clásicos de sincronización:

#### **Semaphore**
- Implementación de semáforos de Dijkstra
- Cola FIFO de espera
- Operaciones: wait(), signal()

#### **ProducerConsumer**
- Buffer limitado (tamaño configurable)
- 3 semáforos: empty, full, mutex
- Protección contra condiciones de carrera

#### **DiningPhilosophers**
- 5 filósofos con estrategia anti-deadlock
- Filósofos pares toman tenedor izquierdo primero
- Filósofos impares toman tenedor derecho primero
- Tracking de comidas por filósofo

#### **ReadersWriters**
- Prioridad a lectores (múltiples lectores simultáneos)
- Exclusión mutua para escritores
- Contadores de lecturas/escrituras

## 📊 Métricas
- Tiempo de espera por proceso
- Tiempo de turnaround
- Utilización de CPU
- Items producidos/consumidos
- Comidas por filósofo
- Lecturas y escrituras totales

## 🎯 Uso
```cpp
#include "Process.h"
#include "Synchronization.h"

// Crear proceso
PCB process(1, 10, 0, 4); // PID=1, burst=10, arrival=0, pages=4

// Productor-Consumidor
ProducerConsumer pc(5); // buffer de 5
int item;
int result = pc.tryProduce(pid, item);

// Filósofos
DiningPhilosophers philosophers;
philosophers.assignPhilosopher(0, pid);
if (philosophers.tryEat(0)) {
    // filósofo comiendo...
    philosophers.finishEating(0);
}
```

## 📈 Algoritmos Implementados
1. **Round Robin (RR)**: Planificación con quantum
2. **Shortest Job First (SJF)**: No preemptivo
3. **Semáforos**: Sincronización con cola FIFO
4. **Anti-deadlock**: Estrategia asimétrica en filósofos
