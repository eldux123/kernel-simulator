#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <vector>
#include <map>
#include <queue>
#include <utility>
#include <string>

// ========== ALGORITMOS DE REEMPLAZO ==========
enum class PageAlgo { 
    FIFO,   // First In First Out
    LRU,    // Least Recently Used
    PFF     // Page Fault Frequency (avanzado)
};

const int DEFAULT_NUM_FRAMES = 4;

// ========== FRAME (MARCO) ==========
struct Frame {
    int pid;
    int page;
    Frame();
};

// ========== MEMORY MANAGER ==========
/**
 * Gestor de memoria virtual con paginación.
 * Soporta algoritmos FIFO, LRU y PFF (Page Fault Frequency).
 */
class MemoryManager {
private:
    int numFrames;
    std::vector<Frame> frames;
    std::queue<int> fifoQueue;
    std::map<std::pair<int, int>, int> mapping;      // (pid, page) -> frameIndex
    std::map<std::pair<int, int>, int> lastUse;      // for LRU
    int totalAccesses;
    int totalFaults;
    PageAlgo algorithm;

    // PFF (Page Fault Frequency) - parámetros avanzados
    int pffThresholdHigh;
    int pffThresholdLow;
    int pffWindowSize;
    std::map<int, int> pidFrameCount;
    std::map<int, int> pidFaultCount;

public:
    MemoryManager(int nframes = DEFAULT_NUM_FRAMES, PageAlgo algo = PageAlgo::FIFO);
    
    // Operaciones principales
    bool access(int pid, int page);
    void freeFramesOfPid(int pid);
    void setNumFrames(int nframes);
    void setAlgorithm(PageAlgo algo);
    
    // Estadísticas y visualización
    void showFrames() const;
    double getHitRate() const;
    int getTotalFaults() const;
    int getTotalAccesses() const;
    std::string getAlgorithmName() const;
    
private:
    int selectVictimFIFO();
    int selectVictimLRU();
    int selectVictimPFF(int pid);
};

#endif // MEMORY_MANAGER_H
