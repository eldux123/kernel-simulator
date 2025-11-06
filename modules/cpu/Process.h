#ifndef PROCESS_H
#define PROCESS_H

#include <vector>
#include <string>

// ========== ESTADOS Y TIPOS ==========
enum class ProcState { NEW, READY, RUNNING, WAITING, SUSPENDED, TERMINATED };
enum class ProcType { NORMAL, PRODUCER, CONSUMER, PHILOSOPHER, READER, WRITER };
enum class ThreadState { THREAD_NEW, THREAD_READY, THREAD_RUNNING, THREAD_WAITING, THREAD_TERMINATED };

// ========== CONSTANTES ==========
const int DEFAULT_QUANTUM = 3;
const int MAX_THREADS_PER_PROCESS = 4;

// ========== ESTRUCTURA DE HILO ==========
struct Thread {
    int tid;                    // Thread ID (único por proceso)
    int parentPid;              // PID del proceso padre
    ThreadState state;
    int burstRemaining;
    int waitingTime;
    int itemsProduced;
    int itemsConsumed;
    int blockedOnSemaphore;

    Thread(int _tid=0, int _pid=0, int burst=0);
    std::string getStateString() const;
};

// ========== PCB (Process Control Block) ==========
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

    PCB(int _id=0, int burst=0, int arrival=0, int pages=4);
    
    std::string getStateString() const;
    std::string getTypeString() const;
    bool isTerminated() const;
    bool isReady() const;
    bool isRunning() const;
    bool isWaiting() const;
    bool isSuspended() const;
};

#endif // PROCESS_H
