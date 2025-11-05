#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <deque>
#ifdef _WIN32
#include <windows.h>
#endif


const int DEFAULT_QUANTUM = 3;
const int DEFAULT_NUM_FRAMES = 4;
const int DEFAULT_BUFFER_SIZE = 5;
const int MAX_THREADS_PER_PROCESS = 4;

enum class ProcState { NEW, READY, RUNNING, WAITING, TERMINATED };
enum class ProcType { NORMAL, PRODUCER, CONSUMER };
enum class ThreadState { THREAD_NEW, THREAD_READY, THREAD_RUNNING, THREAD_WAITING, THREAD_TERMINATED };

struct Thread {
    int tid;                    // Thread ID (único por proceso)
    int parentPid;              // PID del proceso padre
    ThreadState state;
    int burstRemaining;
    int waitingTime;
    int itemsProduced;
    int itemsConsumed;
    int blockedOnSemaphore;

    Thread(int _tid=0, int _pid=0, int burst=0)
        : tid(_tid), parentPid(_pid), state(ThreadState::THREAD_NEW), 
          burstRemaining(burst), waitingTime(0),
          itemsProduced(0), itemsConsumed(0), blockedOnSemaphore(-1) {}
};

struct PCB {
    int id;
    ProcState state;
    ProcType type;
    int burstRemaining;
    int arrivalTick;
    int finishTick;
    int waitingTime;
    int turnaround;
    int numPages;
    int nextPageToAccess;
    int pageAccesses;
    int pageFaults;
    int itemsProduced;
    int itemsConsumed;
    int blockedOnSemaphore;
    
    // Soporte de hilos
    bool hasThreads;
    std::vector<Thread> threads;
    int nextThreadId;

    PCB(int _id=0, int burst=0, int arrival=0, int pages=4)
        : id(_id), state(ProcState::NEW), type(ProcType::NORMAL), burstRemaining(burst),
          arrivalTick(arrival), finishTick(-1), waitingTime(0), turnaround(0),
          numPages(pages), nextPageToAccess(0), pageAccesses(0), pageFaults(0),
          itemsProduced(0), itemsConsumed(0), blockedOnSemaphore(-1),
          hasThreads(false), nextThreadId(1) {}
};

struct Frame {
    int pid;
    int page;
    Frame(): pid(-1), page(-1) {}
};

enum class PageAlgo { FIFO, LRU };

// modulo de sincronizacion
class Semaphore {
private:
    int value;
    std::queue<int> waitingQueue;
    std::string name;

public:
    Semaphore(int v, std::string n = "") : value(v), name(n) {}

    bool tryWait(int pid) {
        if (value > 0) {
            value--;
            return true;
        }
        waitingQueue.push(pid);
        return false;
    }

    int signal() {
        value++;
        if (!waitingQueue.empty()) {
            int pid = waitingQueue.front();
            waitingQueue.pop();
            value--;
            return pid;
        }
        return -1;
    }

    int getValue() const { return value; }
    int getWaitingCount() const { return waitingQueue.size(); }
    std::string getName() const { return name; }
};

class ProducerConsumer {
private:
    std::deque<int> buffer;
    int bufferSize;
    Semaphore empty;
    Semaphore full;
    Semaphore mutex;
    int itemCounter = 0;

public:
    ProducerConsumer(int size = DEFAULT_BUFFER_SIZE)
        : bufferSize(size), 
          empty(size, "empty"), 
          full(0, "full"), 
          mutex(1, "mutex") {}

    // Retorna: 0=éxito, 1=bloqueado en empty, 2=bloqueado en mutex
    int tryProduce(int pid, int &item) {
        if (!empty.tryWait(pid)) return 1;
        if (!mutex.tryWait(pid)) {
            empty.signal();
            return 2;
        }
        item = ++itemCounter;
        buffer.push_back(item);
        mutex.signal();
        full.signal();
        return 0;
    }

    // Retorna: 0=éxito, 1=bloqueado en full, 2=bloqueado en mutex
    int tryConsume(int pid, int &item) {
        if (!full.tryWait(pid)) return 1;
        if (!mutex.tryWait(pid)) {
            full.signal();
            return 2;
        }
        item = buffer.front();
        buffer.pop_front();
        mutex.signal();
        empty.signal();
        return 0;
    }

    void unblockProcess(int semId) {
        int pid = -1;
        if (semId == 0) pid = empty.signal();
        else if (semId == 1) pid = full.signal();
        else if (semId == 2) pid = mutex.signal();
    }

    void showBuffer() const {
        std::cout << "\n--- Buffer Productor-Consumidor ---\n";
        std::cout << "Tamaño: " << buffer.size() << "/" << bufferSize << "\n";
        std::cout << "Contenido: [";
        for (size_t i = 0; i < buffer.size(); i++) {
            std::cout << buffer[i];
            if (i < buffer.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
        std::cout << "Semáforo empty: " << empty.getValue() 
                  << " (esperando: " << empty.getWaitingCount() << ")\n";
        std::cout << "Semáforo full: " << full.getValue() 
                  << " (esperando: " << full.getWaitingCount() << ")\n";
        std::cout << "Semáforo mutex: " << mutex.getValue() 
                  << " (esperando: " << mutex.getWaitingCount() << ")\n";
    }

    int getBufferSize() const { return buffer.size(); }
    int getBufferCapacity() const { return bufferSize; }
};

class MemoryManager {
private:
    int numFrames;
    std::vector<Frame> frames;
    std::queue<int> fifoQueue;
    std::map<std::pair<int, int>, int> mapping;
    std::map<std::pair<int, int>, int> lastUse; // for LRU
    int totalAccesses = 0;
    int totalFaults = 0;
    PageAlgo algorithm;

public:
    MemoryManager(int nframes = DEFAULT_NUM_FRAMES, PageAlgo algo = PageAlgo::FIFO)
        : numFrames(nframes), frames(nframes), algorithm(algo) {}

    void setAlgorithm(PageAlgo algo) {
        algorithm = algo;
        mapping.clear();
        fifoQueue = {};
        lastUse.clear();
    }

    bool access(int pid, int page) {
        totalAccesses++;
        auto key = std::make_pair(pid, page);
        auto it = mapping.find(key);

        if (it != mapping.end()) {
            lastUse[key] = totalAccesses; // update LRU
            return false;
        }

        totalFaults++;
        int freeIdx = -1;
        for (int i = 0; i < numFrames; i++) {
            if (frames[i].pid == -1) {
                freeIdx = i;
                break;
            }
        }

        if (freeIdx != -1) {
            frames[freeIdx].pid = pid;
            frames[freeIdx].page = page;
            mapping[key] = freeIdx;
            fifoQueue.push(freeIdx);
            lastUse[key] = totalAccesses;
        } else {
            int victim = -1;
            if (algorithm == PageAlgo::FIFO) {
                victim = fifoQueue.front();
                fifoQueue.pop();
            } else { // LRU
                int oldest = INT_MAX;
                for (auto &kv : mapping) {
                    if (lastUse[kv.first] < oldest) {
                        oldest = lastUse[kv.first];
                        victim = kv.second;
                    }
                }
            }
            auto victimKey = std::make_pair(frames[victim].pid, frames[victim].page);
            mapping.erase(victimKey);
            lastUse.erase(victimKey);
            frames[victim].pid = pid;
            frames[victim].page = page;
            mapping[key] = victim;
            lastUse[key] = totalAccesses;
            fifoQueue.push(victim);
        }
        return true;
    }

    void freeFramesOfPid(int pid) {
        for (int i = 0; i < numFrames; i++) {
            if (frames[i].pid == pid) {
                auto key = std::make_pair(frames[i].pid, frames[i].page);
                mapping.erase(key);
                lastUse.erase(key);
                frames[i].pid = -1;
                frames[i].page = -1;
            }
        }
    }

    void setNumFrames(int nframes) {
        numFrames = nframes;
        frames = std::vector<Frame>(nframes);
        mapping.clear();
        fifoQueue = {};
        lastUse.clear();
    }

    void showFrames() const {
        std::cout << "\n--- Memory Frames (" << (algorithm == PageAlgo::FIFO ? "FIFO" : "LRU") << ") ---\n";
        for (int i = 0; i < numFrames; i++) {
            if (frames[i].pid == -1)
                std::cout << "[" << i << "]: free\n";
            else
                std::cout << "[" << i << "]: PID=" << frames[i].pid << " Page=" << frames[i].page << "\n";
        }
        std::cout << "Faults: " << totalFaults << " / Accesses: " << totalAccesses
                  << " | Hit Rate: " << std::fixed << std::setprecision(2)
                  << (totalAccesses ? (1.0 - (double)totalFaults / totalAccesses) * 100 : 0.0) << "%\n";
    }
};


class SchedulerRR {
private:
    int quantum;
    int globalTick = 0;
    int nextPid = 1;
    std::map<int, PCB> processes;
    std::queue<int> readyQueue;
    int runningPid = -1;
    int quantumUsed = 0;
    MemoryManager &memManager;
    ProducerConsumer &prodCons;

public:
    SchedulerRR(MemoryManager &mm, ProducerConsumer &pc, int q=DEFAULT_QUANTUM) 
        : quantum(q), memManager(mm), prodCons(pc) {}

    int createProcess(int burst, int pages = 4, ProcType type = ProcType::NORMAL) {
        int pid = nextPid++;
        PCB pcb(pid, burst, globalTick, pages);
        pcb.state = ProcState::READY;
        pcb.type = type;
        processes[pid] = pcb;
        readyQueue.push(pid);
        return pid;
    }

    int createThreadInProcess(int pid, int burstPerThread) {
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

    void executeThreadTick(PCB &p) {
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

    void unblockWaitingProcesses() {
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

    void tick() {
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

    void runTicks(int n) { for (int i=0;i<n;i++) tick(); }

    void scheduleNext() {
        while(!readyQueue.empty() && processes[readyQueue.front()].state == ProcState::TERMINATED)
            readyQueue.pop();
        if (!readyQueue.empty()) {
            int pid = readyQueue.front(); readyQueue.pop();
            if (processes[pid].burstRemaining > 0) {
                runningPid = pid;
                quantumUsed = 0;
            } else processes[pid].state = ProcState::TERMINATED;
        }
    }

    bool killProcess(int pid) {
        auto it = processes.find(pid);
        if (it == processes.end()) return false;
        it->second.state = ProcState::TERMINATED;
        it->second.finishTick = globalTick;
        it->second.turnaround = it->second.finishTick - it->second.arrivalTick;
        memManager.freeFramesOfPid(pid);
        return true;
    }

    void listProcesses() const {
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
                case ProcState::TERMINATED: st="TERM"; break;
            }
            switch (p.type) {
                case ProcType::NORMAL: tp="NORMAL"; break;
                case ProcType::PRODUCER: tp="PRODUCER"; break;
                case ProcType::CONSUMER: tp="CONSUMER"; break;
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

    void showThreads(int pid) const {
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

    void showStats() const {
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
    
    void showDetailedReport() const {
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
            }
            
            switch (p.state) {
                case ProcState::NEW: estado = "NEW"; break;
                case ProcState::READY: estado = "READY"; break;
                case ProcState::RUNNING: estado = "RUNNING"; break;
                case ProcState::WAITING: estado = "WAITING"; break;
                case ProcState::TERMINATED: estado = "TERM"; break;
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

    int getTick() const { return globalTick; }
};
class SchedulerSJF {
private:
    int globalTick = 0;
    int nextPid = 1;
    std::map<int, PCB> processes;
    std::vector<int> readyQueue;
    int runningPid = -1;
    MemoryManager &memManager;

public:
    SchedulerSJF(MemoryManager &mm) : memManager(mm) {}

    int createProcess(int burst, int pages = 4) {
        int pid = nextPid++;
        PCB pcb(pid, burst, globalTick, pages);
        pcb.state = ProcState::READY;
        processes[pid] = pcb;
        readyQueue.push_back(pid);
        return pid;
    }

    void tick() {
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

    void runTicks(int n) { for (int i=0;i<n;i++) tick(); }

    void scheduleNext() {
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

    void listProcesses() const {
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
            }
            std::cout << "PID=" << p.id << " Estado=" << st
                      << " Burst=" << p.burstRemaining
                      << " Espera=" << p.waitingTime << "\n";
        }
    }

    void showStats() const {
        double avgWait = 0, avgTurn = 0; int finished = 0;
        for (auto &kv : processes) {
            const PCB &p = kv.second;
            if (p.state == ProcState::TERMINATED) {
                finished++;
                avgWait += p.waitingTime;
                avgTurn += p.turnaround;
            }
        }
        if (finished > 0) { avgWait/=finished; avgTurn/=finished; }
        std::cout << "\n--- Estadísticas SJF ---\n";
        std::cout << "Tick global: " << globalTick << "\n";
        std::cout << "Procesos terminados: " << finished << "/" << processes.size() << "\n";
        std::cout << "Promedio de espera: " << std::fixed << std::setprecision(2) << avgWait << "\n";
        std::cout << "Promedio de retorno: " << avgTurn << "\n";
        memManager.showFrames();
    }
};

int main() {
#ifdef _WIN32
    // Switch Windows console to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    MemoryManager mem(DEFAULT_NUM_FRAMES);
    ProducerConsumer prodCons(DEFAULT_BUFFER_SIZE);
    SchedulerRR sched(mem, prodCons, DEFAULT_QUANTUM);

    int opcion = 0;
    while (true) {
        std::cout << "\n╔═══════════════════════════════════════╗\n";
        std::cout << "║        MENÚ PRINCIPAL                 ║\n";
        std::cout << "╚═══════════════════════════════════════╝\n";
        std::cout << "┌─────────────────────────────────────────┐\n";
        std::cout << "│  GESTIÓN DE PROCESOS                    │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│ 1.  Crear proceso normal                │\n";
        std::cout << "│ 2.  Crear proceso productor             │\n";
        std::cout << "│ 3.  Crear proceso consumidor            │\n";
        std::cout << "│ 4.  Mostrar procesos (tabla simple)     │\n";
        std::cout << "│ 5.  Terminar proceso                    │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│  GESTIÓN DE HILOS                       │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│ 14. Crear hilos en proceso              │\n";
        std::cout << "│ 15. Mostrar hilos de un proceso         │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│  EJECUCIÓN                              │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│ 6.  Avanzar 1 tick                      │\n";
        std::cout << "│ 7.  Ejecutar varios ticks               │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│  REPORTES Y ESTADÍSTICAS                │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│ 8.  Estadísticas resumidas              │\n";
        std::cout << "│ 9.  Reporte completo detallado          │\n";
        std::cout << "│ 10. Mostrar marcos de memoria           │\n";
        std::cout << "│ 11. Mostrar buffer (prod-cons)          │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│  CONFIGURACIÓN                          │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│ 12. Cambiar tamaño de memoria           │\n";
        std::cout << "│ 13. Cambiar algoritmo de paginación     │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│ 0.  Salir                               │\n";
        std::cout << "└─────────────────────────────────────────┘\n";


        std::cout << "Seleccione una opción: ";
        std::cin >> opcion;

        if (opcion == 1) {
            int burst, pages;
            std::cout << "Ingrese ráfagas (ticks): "; std::cin >> burst;
            std::cout << "Ingrese número de páginas: "; std::cin >> pages;
            int pid = sched.createProcess(burst, pages, ProcType::NORMAL);
            std::cout << "Proceso NORMAL creado con PID=" << pid << "\n";
        }
        else if (opcion == 2) {
            int burst, pages;
            std::cout << "Ingrese ráfagas (ticks): "; std::cin >> burst;
            std::cout << "Ingrese número de páginas: "; std::cin >> pages;
            int pid = sched.createProcess(burst, pages, ProcType::PRODUCER);
            std::cout << "Proceso PRODUCTOR creado con PID=" << pid << "\n";
        }
        else if (opcion == 3) {
            int burst, pages;
            std::cout << "Ingrese ráfagas (ticks): "; std::cin >> burst;
            std::cout << "Ingrese número de páginas: "; std::cin >> pages;
            int pid = sched.createProcess(burst, pages, ProcType::CONSUMER);
            std::cout << "Proceso CONSUMIDOR creado con PID=" << pid << "\n";
        }
        else if (opcion == 4) sched.listProcesses();
        else if (opcion == 5) {
            int pid; std::cout << "PID a terminar: "; std::cin >> pid;
            if (sched.killProcess(pid)) std::cout << "Proceso " << pid << " terminado.\n";
            else std::cout << "PID no encontrado.\n";
        }
        else if (opcion == 6) { 
            sched.tick(); 
            std::cout << "Avanzado 1 tick. Tick actual: " << sched.getTick() << "\n"; 
        }
        else if (opcion == 7) {
            int n; std::cout << "Cuántos ticks desea ejecutar: "; std::cin >> n;
            sched.runTicks(n);
            std::cout << "Ejecutados " << n << " ticks. Tick actual: " << sched.getTick() << "\n";
        }
        else if (opcion == 8) sched.showStats();
        else if (opcion == 9) sched.showDetailedReport();
        else if (opcion == 10) mem.showFrames();
        else if (opcion == 11) prodCons.showBuffer();
        else if (opcion == 12) {
            int n; std::cout << "Nuevo número de marcos: "; std::cin >> n;
            mem.setNumFrames(n);
            std::cout << "Tamaño de memoria actualizado.\n";
        }
        else if (opcion == 13) {
            int m;
            std::cout << "Seleccione algoritmo de paginación (1=FIFO, 2=LRU): ";
            std::cin >> m;
            mem.setAlgorithm(m == 2 ? PageAlgo::LRU : PageAlgo::FIFO);
            std::cout << "Algoritmo actualizado.\n";
        }
        else if (opcion == 14) {
            int pid, numThreads, burstPerThread;
            std::cout << "PID del proceso: "; std::cin >> pid;
            std::cout << "Número de hilos a crear (máx " << MAX_THREADS_PER_PROCESS << "): "; std::cin >> numThreads;
            std::cout << "Burst por hilo: "; std::cin >> burstPerThread;
            
            if (numThreads > MAX_THREADS_PER_PROCESS) {
                std::cout << "Error: Máximo " << MAX_THREADS_PER_PROCESS << " hilos por proceso.\n";
            } else {
                int created = 0;
                for (int i = 0; i < numThreads; i++) {
                    int tid = sched.createThreadInProcess(pid, burstPerThread);
                    if (tid != -1) created++;
                }
                if (created > 0) {
                    std::cout << "✓ Creados " << created << " hilos en proceso PID=" << pid << "\n";
                } else {
                    std::cout << "✗ Error: No se pudieron crear hilos. Verifique el PID.\n";
                }
            }
        }
        else if (opcion == 15) {
            int pid;
            std::cout << "PID del proceso: "; std::cin >> pid;
            sched.showThreads(pid);
        }
        else if (opcion == 0) { std::cout << "Saliendo...\n"; break; }
        else std::cout << "Opción inválida.\n";
    }
    return 0;
}
