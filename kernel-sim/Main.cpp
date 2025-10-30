#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

const int DEFAULT_QUANTUM = 3;
const int DEFAULT_NUM_FRAMES = 4;

enum class ProcState { NEW, READY, RUNNING, WAITING, TERMINATED };

struct PCB {
    int id;
    ProcState state;
    int burstRemaining;
    int arrivalTick;
    int finishTick;
    int waitingTime;
    int turnaround;
    int numPages;
    int nextPageToAccess;
    int pageAccesses;
    int pageFaults;

    PCB(int _id=0, int burst=0, int arrival=0, int pages=4)
        : id(_id), state(ProcState::NEW), burstRemaining(burst),
          arrivalTick(arrival), finishTick(-1), waitingTime(0), turnaround(0),
          numPages(pages), nextPageToAccess(0), pageAccesses(0), pageFaults(0) {}
};

struct Frame {
    int pid;
    int page;
    Frame(): pid(-1), page(-1) {}
};

enum class PageAlgo { FIFO, LRU };

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

public:
    SchedulerRR(MemoryManager &mm, int q=DEFAULT_QUANTUM) : quantum(q), memManager(mm) {}

    int createProcess(int burst, int pages = 4) {
        int pid = nextPid++;
        PCB pcb(pid, burst, globalTick, pages);
        pcb.state = ProcState::READY;
        processes[pid] = pcb;
        readyQueue.push(pid);
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
            quantumUsed++;
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
                quantumUsed = 0;
            } else if (quantumUsed >= quantum) {
                p.state = ProcState::READY;
                readyQueue.push(p.id);
                runningPid = -1;
                quantumUsed = 0;
            }
        }
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
        std::cout << "\n+-----+----------+-------+---------+---------+\n";
        std::cout << "| pid | estado   | burst | waiting | pages   |\n";
        std::cout << "+-----+----------+-------+---------+---------+\n";
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
            std::cout << "| " << std::setw(3) << p.id << " | " << std::setw(8) << st
                      << " | " << std::setw(5) << p.burstRemaining
                      << " | " << std::setw(7) << p.waitingTime
                      << " | " << std::setw(7) << p.numPages << " |\n";
        }
        std::cout << "+-----+----------+-------+---------+---------+\n";
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
        std::cout << "\n--- Estadísticas ---\n";
        std::cout << "Tick global: " << globalTick << "\n";
        std::cout << "Procesos terminados: " << finished << "/" << processes.size() << "\n";
        std::cout << "Promedio de espera: " << std::fixed << std::setprecision(2) << avgWait << "\n";
        std::cout << "Promedio de retorno: " << avgTurn << "\n";
        memManager.showFrames();
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
    MemoryManager mem(DEFAULT_NUM_FRAMES);
    SchedulerRR sched(mem, DEFAULT_QUANTUM);

    int opcion = 0;
    while (true) {
        std::cout << "\n===== MENÚ PRINCIPAL =====\n";
        std::cout << "1. Crear proceso\n";
        std::cout << "2. Mostrar procesos\n";
        std::cout << "3. Avanzar 1 tick\n";
        std::cout << "4. Ejecutar varios ticks\n";
        std::cout << "5. Terminar proceso\n";
        std::cout << "6. Mostrar estadísticas\n";
        std::cout << "7. Mostrar marcos de memoria\n";
        std::cout << "8. Cambiar tamaño de memoria\n";
        std::cout << "9. Cambiar algoritmo de paginación\n";
        std::cout << "10. Salir\n";


        std::cout << "Seleccione una opción: ";
        std::cin >> opcion;

        if (opcion == 1) {
            int burst, pages;
            std::cout << "Ingrese ráfagas (ticks): "; std::cin >> burst;
            std::cout << "Ingrese número de páginas: "; std::cin >> pages;
            int pid = sched.createProcess(burst, pages);
            std::cout << "Proceso creado con PID=" << pid << "\n";
        }
        else if (opcion == 2) sched.listProcesses();
        else if (opcion == 3) { sched.tick(); std::cout << "Avanzado 1 tick. Tick actual: " << sched.getTick() << "\n"; }
        else if (opcion == 4) {
            int n; std::cout << "Cuántos ticks desea ejecutar: "; std::cin >> n;
            sched.runTicks(n);
            std::cout << "Ejecutados " << n << " ticks. Tick actual: " << sched.getTick() << "\n";
        }
        else if (opcion == 5) {
            int pid; std::cout << "PID a terminar: "; std::cin >> pid;
            if (sched.killProcess(pid)) std::cout << "Proceso " << pid << " terminado.\n";
            else std::cout << "PID no encontrado.\n";
        }
        else if (opcion == 6) sched.showStats();
        else if (opcion == 7) mem.showFrames();
        else if (opcion == 8) {
            int n; std::cout << "Nuevo número de marcos: "; std::cin >> n;
            mem.setNumFrames(n);
            std::cout << "Tamaño de memoria actualizado.\n";
        }
        else if (opcion == 9) {
            int m;
            std::cout << "Seleccione algoritmo de paginación (1=FIFO, 2=LRU): ";
            std::cin >> m;
            mem.setAlgorithm(m == 2 ? PageAlgo::LRU : PageAlgo::FIFO);
            std::cout << "Algoritmo actualizado.\n";
        }

        else if (opcion == 10) { std::cout << "Saliendo...\n"; break; }
        else std::cout << "Opción inválida.\n";
    }
    return 0;
}
