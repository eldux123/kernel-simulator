#include "MemoryManager.h"
#include <iostream>
#include <iomanip>
#include <climits>

// ========== FRAME IMPLEMENTATION ==========
Frame::Frame() : pid(-1), page(-1) {}

// ========== MEMORY MANAGER IMPLEMENTATION ==========
MemoryManager::MemoryManager(int nframes, PageAlgo algo)
    : numFrames(nframes), frames(nframes), totalAccesses(0), totalFaults(0),
      algorithm(algo), pffThresholdHigh(3), pffThresholdLow(1), pffWindowSize(10) {}

bool MemoryManager::access(int pid, int page) {
    totalAccesses++;
    auto key = std::make_pair(pid, page);
    auto it = mapping.find(key);

    // HIT: página ya está en memoria
    if (it != mapping.end()) {
        lastUse[key] = totalAccesses;
        return false;
    }

    // MISS: fallo de página
    totalFaults++;
    pidFaultCount[pid]++;
    
    // Buscar marco libre
    int freeIdx = -1;
    for (int i = 0; i < numFrames; i++) {
        if (frames[i].pid == -1) {
            freeIdx = i;
            break;
        }
    }

    if (freeIdx != -1) {
        // Asignar a marco libre
        frames[freeIdx].pid = pid;
        frames[freeIdx].page = page;
        mapping[key] = freeIdx;
        fifoQueue.push(freeIdx);
        lastUse[key] = totalAccesses;
        pidFrameCount[pid]++;
    } else {
        // Seleccionar víctima según algoritmo
        int victim = -1;
        switch (algorithm) {
            case PageAlgo::FIFO:
                victim = selectVictimFIFO();
                break;
            case PageAlgo::LRU:
                victim = selectVictimLRU();
                break;
            case PageAlgo::PFF:
                victim = selectVictimPFF(pid);
                break;
        }

        // Reemplazar víctima
        auto victimKey = std::make_pair(frames[victim].pid, frames[victim].page);
        mapping.erase(victimKey);
        lastUse.erase(victimKey);
        pidFrameCount[frames[victim].pid]--;
        
        frames[victim].pid = pid;
        frames[victim].page = page;
        mapping[key] = victim;
        lastUse[key] = totalAccesses;
        fifoQueue.push(victim);
        pidFrameCount[pid]++;
    }
    return true;
}

int MemoryManager::selectVictimFIFO() {
    int victim = fifoQueue.front();
    fifoQueue.pop();
    return victim;
}

int MemoryManager::selectVictimLRU() {
    int oldest = INT_MAX;
    int victim = -1;
    for (auto &kv : mapping) {
        if (lastUse[kv.first] < oldest) {
            oldest = lastUse[kv.first];
            victim = kv.second;
        }
    }
    return victim;
}

int MemoryManager::selectVictimPFF(int pid) {
    // PFF: Si el proceso tiene alta frecuencia de fallos, le damos más frames
    // Si tiene baja frecuencia, le quitamos frames
    int faultFreq = pidFaultCount[pid];
    
    if (faultFreq > pffThresholdHigh) {
        // Alta frecuencia: buscar víctima de otro proceso
        for (auto &kv : mapping) {
            int victimPid = frames[kv.second].pid;
            if (victimPid != pid && pidFaultCount[victimPid] < pffThresholdLow) {
                return kv.second;
            }
        }
    }
    
    // Fallback a LRU si no se aplica PFF
    return selectVictimLRU();
}

void MemoryManager::freeFramesOfPid(int pid) {
    for (int i = 0; i < numFrames; i++) {
        if (frames[i].pid == pid) {
            auto key = std::make_pair(frames[i].pid, frames[i].page);
            mapping.erase(key);
            lastUse.erase(key);
            frames[i].pid = -1;
            frames[i].page = -1;
        }
    }
    pidFrameCount.erase(pid);
    pidFaultCount.erase(pid);
}

void MemoryManager::setNumFrames(int nframes) {
    numFrames = nframes;
    frames = std::vector<Frame>(nframes);
    mapping.clear();
    fifoQueue = {};
    lastUse.clear();
}

void MemoryManager::setAlgorithm(PageAlgo algo) {
    algorithm = algo;
    mapping.clear();
    fifoQueue = {};
    lastUse.clear();
}

void MemoryManager::showFrames() const {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║         MEMORIA VIRTUAL - " << std::left << std::setw(23) << getAlgorithmName() << "   ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    std::cout << "\n┌────────┬─────────┬─────────┐\n";
    std::cout << "│ Frame  │   PID   │  Page   │\n";
    std::cout << "├────────┼─────────┼─────────┤\n";
    
    for (int i = 0; i < numFrames; i++) {
        if (frames[i].pid == -1) {
            std::cout << "│ " << std::setw(6) << i 
                      << " │ " << std::setw(7) << "FREE"
                      << " │ " << std::setw(7) << "-" << " │\n";
        } else {
            std::cout << "│ " << std::setw(6) << i 
                      << " │ " << std::setw(7) << frames[i].pid
                      << " │ " << std::setw(7) << frames[i].page << " │\n";
        }
    }
    std::cout << "└────────┴─────────┴─────────┘\n\n";
    
    std::cout << "Estadísticas:\n";
    std::cout << "  Accesos totales: " << totalAccesses << "\n";
    std::cout << "  Fallos de página: " << totalFaults << "\n";
    std::cout << "  Hit Rate: " << std::fixed << std::setprecision(2) 
              << getHitRate() << "%\n";
}

double MemoryManager::getHitRate() const {
    return (totalAccesses > 0) 
           ? (1.0 - (double)totalFaults / totalAccesses) * 100 
           : 0.0;
}

int MemoryManager::getTotalFaults() const { return totalFaults; }
int MemoryManager::getTotalAccesses() const { return totalAccesses; }

std::string MemoryManager::getAlgorithmName() const {
    switch (algorithm) {
        case PageAlgo::FIFO: return "FIFO";
        case PageAlgo::LRU: return "LRU";
        case PageAlgo::PFF: return "PFF (Advanced)";
        default: return "UNKNOWN";
    }
}
