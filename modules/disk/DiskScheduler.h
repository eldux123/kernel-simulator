#ifndef DISK_SCHEDULER_H
#define DISK_SCHEDULER_H

#include <deque>
#include <vector>
#include <string>

// ========== ALGORITMOS DE DISCO ==========
enum class DiskAlgo { 
    FCFS,   // First Come First Served
    SSTF,   // Shortest Seek Time First
    SCAN    // Elevator Algorithm
};

// ========== DISK SCHEDULER ==========
/**
 * Planificador de disco con 3 algoritmos:
 * - FCFS: Orden de llegada
 * - SSTF: Busca el cilindro más cercano
 * - SCAN: Barrido tipo elevador
 */
class DiskScheduler {
private:
    std::deque<int> requestQueue;
    int headPosition;
    int totalMovement;
    DiskAlgo algorithm;
    int maxCylinder;
    std::vector<int> accessHistory;
    int direction; // 1 = hacia arriba, -1 = hacia abajo (para SCAN)

public:
    DiskScheduler(int maxCyl = 200, DiskAlgo algo = DiskAlgo::FCFS);
    
    // Operaciones principales
    void addRequest(int cylinder);
    int processNext();
    void setAlgorithm(DiskAlgo algo);
    
    // Estadísticas y visualización
    void showStatus() const;
    void showComparison(const std::vector<int>& requests);
    int getTotalMovement() const;
    int getHeadPosition() const;
    void reset();
    
private:
    int processNextFCFS();
    int processNextSSTF();
    int processNextSCAN();
};

#endif // DISK_SCHEDULER_H
