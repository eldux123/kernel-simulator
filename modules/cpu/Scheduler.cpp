#include "Scheduler.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// ========== SCHEDULER ROUND ROBIN - IMPLEMENTACIÓN ==========

SchedulerRR::SchedulerRR(MemoryManager &mm, ProducerConsumer &pc, int q)
    : quantum(q), globalTick(0), nextPid(1), runningPid(-1), quantumUsed(0),
      memManager(mm), prodCons(pc) {}

int SchedulerRR::createProcess(int burst, int pages, ProcType type) {
    int pid = nextPid++;
    PCB pcb(pid, burst, globalTick, pages);
    pcb.state = ProcState::READY;
    pcb.type = type;
    processes[pid] = pcb;
    readyQueue.push(pid);
    return pid;
}

int SchedulerRR::createThreadInProcess(int pid, int burstPerThread) {
    auto it = processes.find(pid);
    if (it == processes.end()) return -1;
    
    PCB &p = it->second;
    if (p.threads.size() >= MAX_THREADS_PER_PROCESS) return -1;
    
    int tid = p.nextThreadId++;
    Thread t(tid, pid, burstPerThread);
    t.state = ThreadState::THREAD_READY;
    p.threads.push_back(t);
    p.hasThreads = true;
    
    return tid;
}

void SchedulerRR::executeThreadTick(PCB &p) {
    // Buscar primer thread READY o RUNNING
    Thread *activeThread = nullptr;
    for (auto &t : p.threads) {
        if (t.state == ThreadState::THREAD_READY || t.state == ThreadState::THREAD_RUNNING) {
            activeThread = &t;
            break;
        }
    }

    if (!activeThread) return;

    activeThread->state = ThreadState::THREAD_RUNNING;
    activeThread->burstRemaining--;

    // Lógica según tipo de proceso
    if (p.type == ProcType::PRODUCER) {
        int item;
        int result = prodCons.tryProduce(p.id, item);
        if (result == 0) {
            activeThread->itemsProduced++;
            p.itemsProduced++;
        } else {
            activeThread->state = ThreadState::THREAD_WAITING;
            activeThread->blockedOnSemaphore = result - 1;
            return;
        }
    } else if (p.type == ProcType::CONSUMER) {
        int item;
        int result = prodCons.tryConsume(p.id, item);
        if (result == 0) {
            activeThread->itemsConsumed++;
            p.itemsConsumed++;
        } else {
            activeThread->state = ThreadState::THREAD_WAITING;
            activeThread->blockedOnSemaphore = result - 1;
            return;
        }
    }

    // Actualizar waiting time de otros threads
    for (auto &t : p.threads) {
        if (t.tid != activeThread->tid && 
            (t.state == ThreadState::THREAD_READY || t.state == ThreadState::THREAD_WAITING)) {
            t.waitingTime++;
        }
    }

    // Check si thread terminó
    if (activeThread->burstRemaining <= 0) {
        activeThread->state = ThreadState::THREAD_TERMINATED;
        
        // Verificar si todos los threads terminaron
        bool allDone = true;
        for (auto &t : p.threads) {
            if (t.state != ThreadState::THREAD_TERMINATED) {
                allDone = false;
                break;
            }
        }
        
        if (allDone) {
            p.burstRemaining = 0;
        }
    } else {
        // Round-robin entre threads
        activeThread->state = ThreadState::THREAD_READY;
    }
}

void SchedulerRR::unblockWaitingProcesses() {
    // Desbloquear procesos esperando en semáforos
    for (auto &kv : processes) {
        PCB &p = kv.second;
        
        // Desbloquear procesos sin hilos
        if (!p.hasThreads && p.state == ProcState::WAITING) {
            // Reintentar la operación que falló
            bool unblocked = false;
            
            if (p.type == ProcType::PRODUCER) {
                int item;
                int result = prodCons.tryProduce(p.id, item);
                if (result == 0) {
                    p.itemsProduced++;
                    p.state = ProcState::READY;
                    p.blockedOnSemaphore = -1;
                    readyQueue.push(p.id);
                    unblocked = true;
                }
            } else if (p.type == ProcType::CONSUMER) {
                int item;
                int result = prodCons.tryConsume(p.id, item);
                if (result == 0) {
                    p.itemsConsumed++;
                    p.state = ProcState::READY;
                    p.blockedOnSemaphore = -1;
                    readyQueue.push(p.id);
                    unblocked = true;
                }
            }
        }
        
        // Desbloquear threads en procesos con hilos
        if (p.hasThreads) {
            for (auto &t : p.threads) {
                if (t.state == ThreadState::THREAD_WAITING) {
                    bool unblocked = false;
                    
                    if (p.type == ProcType::PRODUCER) {
                        int item;
                        int result = prodCons.tryProduce(p.id, item);
                        if (result == 0) {
                            t.itemsProduced++;
                            p.itemsProduced++;
                            t.state = ThreadState::THREAD_READY;
                            t.blockedOnSemaphore = -1;
                            unblocked = true;
                        }
                    } else if (p.type == ProcType::CONSUMER) {
                        int item;
                        int result = prodCons.tryConsume(p.id, item);
                        if (result == 0) {
                            t.itemsConsumed++;
                            p.itemsConsumed++;
                            t.state = ThreadState::THREAD_READY;
                            t.blockedOnSemaphore = -1;
                            unblocked = true;
                        }
                    }
                    
                    // Si desbloqueamos un thread y el proceso estaba WAITING, ponerlo READY
                    if (unblocked && p.state == ProcState::WAITING) {
                        p.state = ProcState::READY;
                        readyQueue.push(p.id);
                    }
                }
            }
        }
    }
}

void SchedulerRR::tick() {
    globalTick++;
    if (runningPid == -1) scheduleNext();
    for (auto &kv : processes) {
        if (kv.second.state == ProcState::READY)
            kv.second.waitingTime++;
        else if (kv.second.state == ProcState::WAITING)
            kv.second.waitingTime++;
    }

    if (runningPid != -1) {
        PCB &p = processes[runningPid];
        p.state = ProcState::RUNNING;
        quantumUsed++;
        
        // Acceso a memoria
        bool pf = memManager.access(p.id, p.nextPageToAccess);
        if (pf) p.pageFaults++;
        p.pageAccesses++;
        p.nextPageToAccess = (p.nextPageToAccess + 1) % p.numPages;

        // Si el proceso tiene hilos, ejecutar lógica de threads
        if (p.hasThreads) {
            executeThreadTick(p);
            
            // Verificar si todos los threads terminaron
            if (p.burstRemaining <= 0) {
                p.state = ProcState::TERMINATED;
                p.finishTick = globalTick;
                p.turnaround = p.finishTick - p.arrivalTick;
                memManager.freeFramesOfPid(p.id);
                runningPid = -1;
                quantumUsed = 0;
            } else if (quantumUsed >= quantum) {
                p.state = ProcState::READY;
                readyQueue.push(p.id);
                runningPid = -1;
                quantumUsed = 0;
            }
        } else {
            // Lógica sin threads (proceso normal)
            p.burstRemaining--;
            
            // Lógica de productor-consumidor
            if (p.type == ProcType::PRODUCER) {
                int item;
                int result = prodCons.tryProduce(p.id, item);
                if (result == 0) {
                    p.itemsProduced++;
                } else {
                    p.state = ProcState::WAITING;
                    p.blockedOnSemaphore = result - 1;
                    runningPid = -1;
                    quantumUsed = 0;
                    return;
                }
            } else if (p.type == ProcType::CONSUMER) {
                int item;
                int result = prodCons.tryConsume(p.id, item);
                if (result == 0) {
                    p.itemsConsumed++;
                } else {
                    p.state = ProcState::WAITING;
                    p.blockedOnSemaphore = result - 1;
                    runningPid = -1;
                    quantumUsed = 0;
                    return;
                }
            }

            if (p.burstRemaining <= 0) {
                p.state = ProcState::TERMINATED;
                p.finishTick = globalTick;
                p.turnaround = p.finishTick - p.arrivalTick;
                memManager.freeFramesOfPid(p.id);
                runningPid = -1;
                quantumUsed = 0;
            } else if (quantumUsed >= quantum) {
                p.state = ProcState::READY;
                readyQueue.push(p.id);
                runningPid = -1;
                quantumUsed = 0;
            }
        }
    }
    
    // DESBLOQUEO AUTOMÁTICO: intentar despertar procesos/threads bloqueados
    unblockWaitingProcesses();
}

void SchedulerRR::runTicks(int n) { 
    for (int i=0;i<n;i++) tick(); 
}

void SchedulerRR::scheduleNext() {
    while(!readyQueue.empty() && processes[readyQueue.front()].state == ProcState::TERMINATED)
        readyQueue.pop();
    if (!readyQueue.empty()) {
        int pid = readyQueue.front(); 
        readyQueue.pop();
        if (processes[pid].burstRemaining > 0) {
            runningPid = pid;
            quantumUsed = 0;
        } else processes[pid].state = ProcState::TERMINATED;
    }
}

bool SchedulerRR::killProcess(int pid) {
    auto it = processes.find(pid);
    if (it == processes.end()) return false;
    it->second.state = ProcState::TERMINATED;
    it->second.finishTick = globalTick;
    it->second.turnaround = it->second.finishTick - it->second.arrivalTick;
    memManager.freeFramesOfPid(pid);
    return true;
}

bool SchedulerRR::suspendProcess(int pid) {
    auto it = processes.find(pid);
    if (it == processes.end()) return false;
    if (it->second.state == ProcState::TERMINATED || it->second.state == ProcState::SUSPENDED) 
        return false;
    
    // Si está corriendo, remover de ejecución
    if (runningPid == pid) {
        runningPid = -1;
        quantumUsed = 0;
    }
    
    it->second.state = ProcState::SUSPENDED;
    return true;
}

bool SchedulerRR::resumeProcess(int pid) {
    auto it = processes.find(pid);
    if (it == processes.end()) return false;
    if (it->second.state != ProcState::SUSPENDED) return false;
    
    // Pasar a READY y agregar a la cola
    it->second.state = ProcState::READY;
    readyQueue.push(pid);
    return true;
}

void SchedulerRR::listProcesses() const {
    std::cout << "\n+-----+----------+----------+-------+---------+---------+----------+----------+---------+\n";
    std::cout << "| pid | tipo     | estado   | burst | waiting | pages   | prod/cons | blocked  | threads |\n";
    std::cout << "+-----+----------+----------+-------+---------+---------+----------+----------+---------+\n";
    for (auto &kv : processes) {
        const PCB &p = kv.second;
        std::string st, tp;
        switch (p.state) {
            case ProcState::NEW: st="NEW"; break;
            case ProcState::READY: st="READY"; break;
            case ProcState::RUNNING: st="RUN"; break;
            case ProcState::WAITING: st="WAIT"; break;
            case ProcState::SUSPENDED: st="SUSP"; break;
            case ProcState::TERMINATED: st="TERM"; break;
        }
        switch (p.type) {
            case ProcType::NORMAL: tp="NORMAL"; break;
            case ProcType::PRODUCER: tp="PRODUCER"; break;
            case ProcType::CONSUMER: tp="CONSUMER"; break;
            case ProcType::PHILOSOPHER: tp="PHILOSOPH"; break;
            case ProcType::READER: tp="READER"; break;
            case ProcType::WRITER: tp="WRITER"; break;
        }
        std::string items = (p.type == ProcType::PRODUCER) ? std::to_string(p.itemsProduced) :
                           (p.type == ProcType::CONSUMER) ? std::to_string(p.itemsConsumed) : "-";
        std::string blocked = (p.state == ProcState::WAITING) ? "Sem" + std::to_string(p.blockedOnSemaphore) : "-";
        std::string threads = p.hasThreads ? std::to_string(p.threads.size()) : "-";
        
        std::cout << "| " << std::setw(3) << p.id << " | " << std::setw(8) << tp
                  << " | " << std::setw(8) << st
                  << " | " << std::setw(5) << p.burstRemaining
                  << " | " << std::setw(7) << p.waitingTime
                  << " | " << std::setw(7) << p.numPages
                  << " | " << std::setw(8) << items
                  << " | " << std::setw(8) << blocked
                  << " | " << std::setw(7) << threads << " |\n";
    }
    std::cout << "+-----+----------+----------+-------+---------+---------+----------+----------+---------+\n";
}

void SchedulerRR::showThreads(int pid) const {
    auto it = processes.find(pid);
    if (it == processes.end()) {
        std::cout << "Proceso no encontrado.\n";
        return;
    }

    const PCB &p = it->second;
    if (!p.hasThreads || p.threads.empty()) {
        std::cout << "El proceso " << pid << " no tiene hilos.\n";
        return;
    }

    std::cout << "\n╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║       HILOS DEL PROCESO PID=" << std::setw(3) << pid << "                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    std::cout << "\n┌─────┬──────────────┬───────┬─────────┬──────────┬──────────┐\n";
    std::cout << "│ TID │    Estado    │ Burst │ Waiting │ Prod/Cons│ Blocked  │\n";
    std::cout << "├─────┼──────────────┼───────┼─────────┼──────────┼──────────┤\n";

    for (const auto &t : p.threads) {
        std::string st;
        switch (t.state) {
            case ThreadState::THREAD_NEW: st = "NEW"; break;
            case ThreadState::THREAD_READY: st = "READY"; break;
            case ThreadState::THREAD_RUNNING: st = "RUNNING"; break;
            case ThreadState::THREAD_WAITING: st = "WAITING"; break;
            case ThreadState::THREAD_TERMINATED: st = "TERMINATED"; break;
        }

        std::string items = "-";
        if (p.type == ProcType::PRODUCER && t.itemsProduced > 0) 
            items = std::to_string(t.itemsProduced) + "p";
        else if (p.type == ProcType::CONSUMER && t.itemsConsumed > 0)
            items = std::to_string(t.itemsConsumed) + "c";

        std::string blocked = (t.state == ThreadState::THREAD_WAITING) ? 
                              "Sem" + std::to_string(t.blockedOnSemaphore) : "-";

        std::cout << "│ " << std::setw(3) << t.tid
                  << " │ " << std::setw(12) << st
                  << " │ " << std::setw(5) << t.burstRemaining
                  << " │ " << std::setw(7) << t.waitingTime
                  << " │ " << std::setw(8) << items
                  << " │ " << std::setw(8) << blocked << " │\n";
    }

    std::cout << "└─────┴──────────────┴───────┴─────────┴──────────┴──────────┘\n";
}

void SchedulerRR::showStats() const {
    double avgWait = 0, avgTurn = 0, totalCpuTime = 0;
    int finished = 0, totalProduced = 0, totalConsumed = 0;
    int normalProcs = 0, producers = 0, consumers = 0;
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           ESTADÍSTICAS DETALLADAS DEL SIMULADOR                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    // Tabla de procesos terminados
    std::cout << "\n┌─────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│                    PROCESOS TERMINADOS - DETALLE                            │\n";
    std::cout << "├─────┬──────────┬─────────┬──────────┬───────────┬──────────┬──────────────┤\n";
    std::cout << "│ PID │   Tipo   │ Arrival │  Finish  │ Turnaround│  Waiting │   Prod/Cons  │\n";
    std::cout << "├─────┼──────────┼─────────┼──────────┼───────────┼──────────┼──────────────┤\n";
    
    for (auto &kv : processes) {
        const PCB &p = kv.second;
        if (p.state == ProcState::TERMINATED) {
            finished++;
            avgWait += p.waitingTime;
            avgTurn += p.turnaround;
            totalCpuTime += (p.turnaround - p.waitingTime);
            
            std::string tipo;
            std::string items = "-";
            
            if (p.type == ProcType::NORMAL) {
                tipo = "NORMAL";
                normalProcs++;
            } else if (p.type == ProcType::PRODUCER) {
                tipo = "PRODUCER";
                producers++;
                totalProduced += p.itemsProduced;
                items = std::to_string(p.itemsProduced) + " prod";
            } else {
                tipo = "CONSUMER";
                consumers++;
                totalConsumed += p.itemsConsumed;
                items = std::to_string(p.itemsConsumed) + " cons";
            }
            
            std::cout << "│ " << std::setw(3) << p.id 
                      << " │ " << std::setw(8) << tipo
                      << " │ " << std::setw(7) << p.arrivalTick
                      << " │ " << std::setw(8) << p.finishTick
                      << " │ " << std::setw(9) << p.turnaround
                      << " │ " << std::setw(8) << p.waitingTime
                      << " │ " << std::setw(12) << items << " │\n";
        }
    }
    
    std::cout << "└─────┴──────────┴─────────┴──────────┴───────────┴──────────┴──────────────┘\n";
    
    // Promedios
    if (finished > 0) {
        avgWait /= finished;
        avgTurn /= finished;
    }
    
    // Tabla de resumen general
    std::cout << "\n┌──────────────────────────────────────────────────────────────┐\n";
    std::cout << "│                    RESUMEN GENERAL                           │\n";
    std::cout << "├──────────────────────────────────────┬───────────────────────┤\n";
    std::cout << "│ Tick Global del Sistema              │ " << std::setw(21) << globalTick << " │\n";
    std::cout << "│ Total de Procesos Creados            │ " << std::setw(21) << processes.size() << " │\n";
    std::cout << "│ Procesos Terminados                  │ " << std::setw(21) << finished << " │\n";
    std::cout << "│ Procesos en Ejecución                │ " << std::setw(21) << (processes.size() - finished) << " │\n";
    std::cout << "├──────────────────────────────────────┼───────────────────────┤\n";
    std::cout << "│ Promedio Tiempo de Espera            │ " << std::setw(18) << std::fixed << std::setprecision(2) << avgWait << " ticks │\n";
    std::cout << "│ Promedio Tiempo de Retorno           │ " << std::setw(18) << avgTurn << " ticks │\n";
    std::cout << "│ Utilización de CPU                   │ " << std::setw(17) << (globalTick > 0 ? (totalCpuTime/globalTick)*100 : 0) << " % │\n";
    std::cout << "└──────────────────────────────────────┴───────────────────────┘\n";
    
    // Tabla por tipo de proceso
    std::cout << "\n┌──────────────────────────────────────────────────────────────┐\n";
    std::cout << "│              DISTRIBUCIÓN POR TIPO DE PROCESO                │\n";
    std::cout << "├──────────────────────────────────────┬───────────────────────┤\n";
    std::cout << "│ Procesos Normales                    │ " << std::setw(21) << normalProcs << " │\n";
    std::cout << "│ Procesos Productores                 │ " << std::setw(21) << producers << " │\n";
    std::cout << "│ Procesos Consumidores                │ " << std::setw(21) << consumers << " │\n";
    std::cout << "└──────────────────────────────────────┴───────────────────────┘\n";
    
    // Estadísticas de sincronización
    if (producers > 0 || consumers > 0) {
        std::cout << "\n┌──────────────────────────────────────────────────────────────┐\n";
        std::cout << "│           ESTADÍSTICAS DE SINCRONIZACIÓN                     │\n";
        std::cout << "├──────────────────────────────────────┬───────────────────────┤\n";
        std::cout << "│ Total Items Producidos               │ " << std::setw(21) << totalProduced << " │\n";
        std::cout << "│ Total Items Consumidos               │ " << std::setw(21) << totalConsumed << " │\n";
        std::cout << "│ Items en Buffer                      │ " << std::setw(21) << (totalProduced - totalConsumed) << " │\n";
        std::cout << "│ Throughput (items/tick)              │ " << std::setw(18) << std::fixed << std::setprecision(3) << (globalTick > 0 ? (double)totalProduced/globalTick : 0) << " │\n";
        std::cout << "└──────────────────────────────────────┴───────────────────────┘\n";
    }
    
    // Mostrar estadísticas de memoria
    memManager.showFrames();
}

void SchedulerRR::showDetailedReport() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              REPORTE COMPLETO DE TODOS LOS PROCESOS               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
    
    std::cout << "\n┌─────┬──────────┬──────────┬──────┬─────────┬───────┬──────┬──────────┬────────┐\n";
    std::cout << "│ PID │   Tipo   │  Estado  │Burst │ Waiting │ Pages │Faults│  Accesos │Prod/Con│\n";
    std::cout << "├─────┼──────────┼──────────┼──────┼─────────┼───────┼──────┼──────────┼────────┤\n";
    
    for (auto &kv : processes) {
        const PCB &p = kv.second;
        
        std::string tipo, estado;
        switch (p.type) {
            case ProcType::NORMAL: tipo = "NORMAL"; break;
            case ProcType::PRODUCER: tipo = "PRODUCER"; break;
            case ProcType::CONSUMER: tipo = "CONSUMER"; break;
            default: tipo = "OTHER"; break;
        }
        
        switch (p.state) {
            case ProcState::NEW: estado = "NEW"; break;
            case ProcState::READY: estado = "READY"; break;
            case ProcState::RUNNING: estado = "RUNNING"; break;
            case ProcState::WAITING: estado = "WAITING"; break;
            case ProcState::TERMINATED: estado = "TERM"; break;
            default: estado = "UNKNOWN"; break;
        }
        
        std::string items = "-";
        if (p.type == ProcType::PRODUCER) items = std::to_string(p.itemsProduced);
        else if (p.type == ProcType::CONSUMER) items = std::to_string(p.itemsConsumed);
        
        std::cout << "│ " << std::setw(3) << p.id
                  << " │ " << std::setw(8) << tipo
                  << " │ " << std::setw(8) << estado
                  << " │ " << std::setw(4) << p.burstRemaining
                  << " │ " << std::setw(7) << p.waitingTime
                  << " │ " << std::setw(5) << p.numPages
                  << " │ " << std::setw(4) << p.pageFaults
                  << " │ " << std::setw(8) << p.pageAccesses
                  << " │ " << std::setw(6) << items << " │\n";
    }
    
    std::cout << "└─────┴──────────┴──────────┴──────┴─────────┴───────┴──────┴──────────┴────────┘\n";
    
    // Gráfico de barras ASCII para tiempos de espera
    std::cout << "\n┌────────────────────────────────────────────────────────┐\n";
    std::cout << "│        GRÁFICO DE TIEMPOS DE ESPERA (TERMINADOS)       │\n";
    std::cout << "└────────────────────────────────────────────────────────┘\n";
    
    int maxWait = 0;
    for (auto &kv : processes) {
        if (kv.second.state == ProcState::TERMINATED && kv.second.waitingTime > maxWait)
            maxWait = kv.second.waitingTime;
    }
    
    if (maxWait > 0) {
        for (auto &kv : processes) {
            const PCB &p = kv.second;
            if (p.state == ProcState::TERMINATED) {
                std::cout << "PID " << std::setw(3) << p.id << " │";
                int bars = (p.waitingTime * 40) / maxWait;
                for (int i = 0; i < bars; i++) std::cout << "█";
                std::cout << " " << p.waitingTime << " ticks\n";
            }
        }
    }
}

int SchedulerRR::getTick() const { 
    return globalTick; 
}


// ========== SCHEDULER SJF - IMPLEMENTACIÓN ==========

SchedulerSJF::SchedulerSJF(MemoryManager &mm) 
    : globalTick(0), nextPid(1), runningPid(-1), memManager(mm) {}

int SchedulerSJF::createProcess(int burst, int pages) {
    int pid = nextPid++;
    PCB pcb(pid, burst, globalTick, pages);
    pcb.state = ProcState::READY;
    processes[pid] = pcb;
    readyQueue.push_back(pid);
    return pid;
}

void SchedulerSJF::tick() {
    globalTick++;
    if (runningPid == -1) scheduleNext();
    for (auto &kv : processes)
        if (kv.second.state == ProcState::READY)
            kv.second.waitingTime++;

    if (runningPid != -1) {
        PCB &p = processes[runningPid];
        p.state = ProcState::RUNNING;
        p.burstRemaining--;
        bool pf = memManager.access(p.id, p.nextPageToAccess);
        if (pf) p.pageFaults++;
        p.pageAccesses++;
        p.nextPageToAccess = (p.nextPageToAccess + 1) % p.numPages;

        if (p.burstRemaining <= 0) {
            p.state = ProcState::TERMINATED;
            p.finishTick = globalTick;
            p.turnaround = p.finishTick - p.arrivalTick;
            memManager.freeFramesOfPid(p.id);
            runningPid = -1;
        }
    }
}

void SchedulerSJF::runTicks(int n) { 
    for (int i=0;i<n;i++) tick(); 
}

void SchedulerSJF::scheduleNext() {
    // eliminar terminados
    readyQueue.erase(
        std::remove_if(readyQueue.begin(), readyQueue.end(), [&](int pid){
            return processes[pid].state == ProcState::TERMINATED;
        }),
        readyQueue.end()
    );
    if (!readyQueue.empty()) {
        int best = readyQueue.front();
        for (int pid : readyQueue)
            if (processes[pid].burstRemaining < processes[best].burstRemaining)
                best = pid;
        readyQueue.erase(std::remove(readyQueue.begin(), readyQueue.end(), best), readyQueue.end());
        runningPid = best;
    }
}

void SchedulerSJF::listProcesses() const {
    std::cout << "\n--- Procesos (SJF) ---\n";
    for (auto &kv : processes) {
        const PCB &p = kv.second;
        std::string st;
        switch (p.state) {
            case ProcState::NEW: st="NEW"; break;
            case ProcState::READY: st="READY"; break;
            case ProcState::RUNNING: st="RUN"; break;
            case ProcState::WAITING: st="WAIT"; break;
            case ProcState::TERMINATED: st="TERM"; break;
            default: st="UNKNOWN"; break;
        }
        std::cout << "PID=" << p.id << " Estado=" << st
                  << " Burst=" << p.burstRemaining
                  << " Espera=" << p.waitingTime << "\n";
    }
}

void SchedulerSJF::showStats() const {
    double avgWait = 0, avgTurn = 0; 
    int finished = 0;
    for (auto &kv : processes) {
        const PCB &p = kv.second;
        if (p.state == ProcState::TERMINATED) {
            finished++;
            avgWait += p.waitingTime;
            avgTurn += p.turnaround;
        }
    }
    if (finished > 0) { 
        avgWait/=finished; 
        avgTurn/=finished; 
    }
    std::cout << "\n--- Estadísticas SJF ---\n";
    std::cout << "Tick global: " << globalTick << "\n";
    std::cout << "Procesos terminados: " << finished << "/" << processes.size() << "\n";
    std::cout << "Promedio de espera: " << std::fixed << std::setprecision(2) << avgWait << "\n";
    std::cout << "Promedio de retorno: " << avgTurn << "\n";
    memManager.showFrames();
}

int SchedulerSJF::getTick() const {
    return globalTick;
}
