#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include <queue>
#include <vector>
#include <iostream>
#include <iomanip>

// Constantes
const int DEFAULT_BUFFER_SIZE = 5;

// ========== SEMÁFORO ==========
class Semaphore {
private:
    int value;
    std::queue<int> waitingQueue; // cola FIFO de PIDs

public:
    Semaphore(int val = 1) : value(val) {}

    bool tryWait(int pid) {
        if (value > 0) {
            value--;
            return true;
        } else {
            waitingQueue.push(pid);
            return false;
        }
    }

    int signal() {
        if (!waitingQueue.empty()) {
            int pid = waitingQueue.front();
            waitingQueue.pop();
            return pid;
        } else {
            value++;
            return -1;
        }
    }

    int getValue() const { return value; }
    bool hasWaiting() const { return !waitingQueue.empty(); }
};

// ========== PRODUCTOR-CONSUMIDOR ==========
class ProducerConsumer {
private:
    std::vector<int> buffer;
    int maxSize;
    Semaphore empty;
    Semaphore full;
    Semaphore mutex;

public:
    ProducerConsumer(int size = DEFAULT_BUFFER_SIZE)
        : maxSize(size), empty(size), full(0), mutex(1) {}

    bool tryProduce(int pid, int item) {
        if (!empty.tryWait(pid)) return false;
        if (!mutex.tryWait(pid)) {
            empty.signal();
            return false;
        }
        
        buffer.push_back(item);
        mutex.signal();
        full.signal();
        return true;
    }

    bool tryConsume(int pid, int &item) {
        if (!full.tryWait(pid)) return false;
        if (!mutex.tryWait(pid)) {
            full.signal();
            return false;
        }
        
        if (!buffer.empty()) {
            item = buffer.front();
            buffer.erase(buffer.begin());
        }
        mutex.signal();
        empty.signal();
        return true;
    }

    void showBuffer() const {
        std::cout << "Buffer [" << buffer.size() << "/" << maxSize << "]: ";
        for (int item : buffer) std::cout << item << " ";
        std::cout << "\n";
        std::cout << "Semáforos -> empty:" << empty.getValue() 
                  << " full:" << full.getValue() 
                  << " mutex:" << mutex.getValue() << "\n";
    }

    int getSize() const { return buffer.size(); }
    int getMaxSize() const { return maxSize; }
    bool isEmpty() const { return buffer.empty(); }
    bool isFull() const { return buffer.size() >= maxSize; }
};

// ========== FILÓSOFOS CENANDO ==========
class DiningPhilosophers {
private:
    static const int NUM_PHILOSOPHERS = 5;
    std::vector<Semaphore> forks;
    std::vector<int> philosopherState; // 0=thinking, 1=eating
    std::vector<int> eatCount;

public:
    DiningPhilosophers() : forks(NUM_PHILOSOPHERS, Semaphore(1)), 
                          philosopherState(NUM_PHILOSOPHERS, 0),
                          eatCount(NUM_PHILOSOPHERS, 0) {}

    bool tryEat(int philosopherId) {
        if (philosopherId < 0 || philosopherId >= NUM_PHILOSOPHERS) return false;
        
        int leftFork = philosopherId;
        int rightFork = (philosopherId + 1) % NUM_PHILOSOPHERS;

        // Evitar deadlock: filósofo impar toma tenedor derecho primero
        if (philosopherId % 2 == 0) {
            if (!forks[leftFork].tryWait(philosopherId)) return false;
            if (!forks[rightFork].tryWait(philosopherId)) {
                forks[leftFork].signal();
                return false;
            }
        } else {
            if (!forks[rightFork].tryWait(philosopherId)) return false;
            if (!forks[leftFork].tryWait(philosopherId)) {
                forks[rightFork].signal();
                return false;
            }
        }

        philosopherState[philosopherId] = 1;
        eatCount[philosopherId]++;
        return true;
    }

    void finishEating(int philosopherId) {
        if (philosopherId < 0 || philosopherId >= NUM_PHILOSOPHERS) return;
        
        int leftFork = philosopherId;
        int rightFork = (philosopherId + 1) % NUM_PHILOSOPHERS;

        forks[leftFork].signal();
        forks[rightFork].signal();
        philosopherState[philosopherId] = 0;
    }

    void showStatus() const {
        std::cout << "\n╔═══════════════════════════════════════════════════╗\n";
        std::cout << "║         FILÓSOFOS CENANDO                         ║\n";
        std::cout << "╚═══════════════════════════════════════════════════╝\n";
        
        for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
            std::cout << "Filósofo " << i << ": " 
                      << (philosopherState[i] == 1 ? "COMIENDO 🍝" : "PENSANDO 💭")
                      << " | Comidas: " << eatCount[i] << "\n";
        }
    }

    int getEatCount(int philosopherId) const {
        return (philosopherId >= 0 && philosopherId < NUM_PHILOSOPHERS) 
               ? eatCount[philosopherId] : 0;
    }
};

// ========== LECTORES-ESCRITORES ==========
class ReadersWriters {
private:
    Semaphore mutex;
    Semaphore wrt;
    int readCount;
    int totalReads;
    int totalWrites;

public:
    ReadersWriters() : mutex(1), wrt(1), readCount(0), totalReads(0), totalWrites(0) {}

    bool tryRead(int pid) {
        if (!mutex.tryWait(pid)) return false;
        
        readCount++;
        if (readCount == 1) {
            if (!wrt.tryWait(pid)) {
                readCount--;
                mutex.signal();
                return false;
            }
        }
        mutex.signal();
        totalReads++;
        return true;
    }

    void finishRead() {
        mutex.tryWait(-1); // forzar
        readCount--;
        if (readCount == 0) {
            wrt.signal();
        }
        mutex.signal();
    }

    bool tryWrite(int pid) {
        if (!wrt.tryWait(pid)) return false;
        totalWrites++;
        return true;
    }

    void finishWrite() {
        wrt.signal();
    }

    void showStatus() const {
        std::cout << "\n╔═══════════════════════════════════════════════════╗\n";
        std::cout << "║         LECTORES-ESCRITORES                       ║\n";
        std::cout << "╚═══════════════════════════════════════════════════╝\n";
        
        std::cout << "Lectores activos: " << readCount << "\n";
        std::cout << "Total lecturas: " << totalReads << "\n";
        std::cout << "Total escrituras: " << totalWrites << "\n";
    }

    int getTotalReads() const { return totalReads; }
    int getTotalWrites() const { return totalWrites; }
};

#endif // SYNCHRONIZATION_H
