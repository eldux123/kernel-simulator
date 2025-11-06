#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>
#include <map>
#include <vector>
#include <iostream>
#include "Process.h"
#include "../mem/MemoryManager.h"
#include "Synchronization.h"

// ========== SCHEDULER ROUND ROBIN ==========
/**
 * Planificador Round-Robin con soporte para:
 * - Hilos (multithreading)
 * - Sincronización con ProductorConsumidor
 * - Gestión de memoria virtual
 */
class SchedulerRR {
private:
    int quantum;
    int globalTick;
    int nextPid;
    std::map<int, PCB> processes;
    std::queue<int> readyQueue;
    int runningPid;
    int quantumUsed;
    MemoryManager &memManager;
    ProducerConsumer &prodCons;
    
    void executeThreadTick(PCB &p);
    void unblockWaitingProcesses();
    void scheduleNext();

public:
    SchedulerRR(MemoryManager &mm, ProducerConsumer &pc, int q=DEFAULT_QUANTUM);
    
    // Gestión de procesos
    int createProcess(int burst, int pages = 4, ProcType type = ProcType::NORMAL);
    int createThreadInProcess(int pid, int burstPerThread);
    bool killProcess(int pid);
    bool suspendProcess(int pid);
    bool resumeProcess(int pid);
    
    // Ejecución
    void tick();
    void runTicks(int n);
    
    // Visualización
    void listProcesses() const;
    void showThreads(int pid) const;
    void showStats() const;
    void showDetailedReport() const;
    
    // Getters
    int getTick() const;
};

// ========== SCHEDULER SJF (Shortest Job First) ==========
/**
 * Planificador SJF (non-preemptive)
 * Selecciona el proceso con menor burst time restante.
 */
class SchedulerSJF {
private:
    int globalTick;
    int nextPid;
    std::map<int, PCB> processes;
    std::vector<int> readyQueue;
    int runningPid;
    MemoryManager &memManager;
    
    void scheduleNext();

public:
    SchedulerSJF(MemoryManager &mm);
    
    // Gestión de procesos
    int createProcess(int burst, int pages = 4);
    
    // Ejecución
    void tick();
    void runTicks(int n);
    
    // Visualización
    void listProcesses() const;
    void showStats() const;
    
    // Getters
    int getTick() const;
};

#endif // SCHEDULER_H
