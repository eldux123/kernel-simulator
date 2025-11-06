#include "DiskScheduler.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <climits>

DiskScheduler::DiskScheduler(int maxCyl, DiskAlgo algo)
    : headPosition(0), totalMovement(0), algorithm(algo), 
      maxCylinder(maxCyl), direction(1) {}

void DiskScheduler::addRequest(int cylinder) {
    if (cylinder >= 0 && cylinder < maxCylinder) {
        requestQueue.push_back(cylinder);
    }
}

int DiskScheduler::processNext() {
    if (requestQueue.empty()) return -1;

    int target = -1;
    switch (algorithm) {
        case DiskAlgo::FCFS:
            target = processNextFCFS();
            break;
        case DiskAlgo::SSTF:
            target = processNextSSTF();
            break;
        case DiskAlgo::SCAN:
            target = processNextSCAN();
            break;
    }

    if (target != -1) {
        int movement = std::abs(target - headPosition);
        totalMovement += movement;
        headPosition = target;
        accessHistory.push_back(target);
    }

    return target;
}

int DiskScheduler::processNextFCFS() {
    int target = requestQueue.front();
    requestQueue.pop_front();
    return target;
}

int DiskScheduler::processNextSSTF() {
    int minDist = INT_MAX;
    int minIdx = -1;

    for (size_t i = 0; i < requestQueue.size(); i++) {
        int dist = std::abs(requestQueue[i] - headPosition);
        if (dist < minDist) {
            minDist = dist;
            minIdx = i;
        }
    }
    
    if (minIdx != -1) {
        int target = requestQueue[minIdx];
        requestQueue.erase(requestQueue.begin() + minIdx);
        return target;
    }
    return -1;
}

int DiskScheduler::processNextSCAN() {
    std::vector<int> ahead, behind;
    
    for (int req : requestQueue) {
        if (direction == 1 && req >= headPosition) ahead.push_back(req);
        else if (direction == -1 && req <= headPosition) behind.push_back(req);
        else if (direction == 1) behind.push_back(req);
        else ahead.push_back(req);
    }

    int target = -1;
    if (direction == 1 && !ahead.empty()) {
        std::sort(ahead.begin(), ahead.end());
        target = ahead[0];
    } else if (direction == -1 && !behind.empty()) {
        std::sort(behind.begin(), behind.end(), std::greater<int>());
        target = behind[0];
    } else {
        direction *= -1;
        if (!ahead.empty()) {
            std::sort(ahead.begin(), ahead.end());
            target = ahead[0];
        } else if (!behind.empty()) {
            std::sort(behind.begin(), behind.end(), std::greater<int>());
            target = behind[0];
        }
    }

    // Remover target de la cola
    for (size_t i = 0; i < requestQueue.size(); i++) {
        if (requestQueue[i] == target) {
            requestQueue.erase(requestQueue.begin() + i);
            break;
        }
    }

    return target;
}

void DiskScheduler::setAlgorithm(DiskAlgo algo) {
    algorithm = algo;
    direction = 1;
}

void DiskScheduler::showStatus() const {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║         PLANIFICACIÓN DE DISCO                     ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    std::string algoName;
    switch (algorithm) {
        case DiskAlgo::FCFS: algoName = "FCFS (First Come First Served)"; break;
        case DiskAlgo::SSTF: algoName = "SSTF (Shortest Seek Time First)"; break;
        case DiskAlgo::SCAN: algoName = "SCAN (Elevador)"; break;
    }

    std::cout << "Algoritmo: " << algoName << "\n";
    std::cout << "Posición del cabezal: " << headPosition << "\n";
    std::cout << "Movimiento total: " << totalMovement << " cilindros\n";
    std::cout << "Solicitudes pendientes: " << requestQueue.size() << "\n";

    if (!accessHistory.empty()) {
        std::cout << "\nHistorial de accesos (últimos 10):\n";
        int start = std::max(0, (int)accessHistory.size() - 10);
        for (size_t i = start; i < accessHistory.size(); i++) {
            std::cout << accessHistory[i];
            if (i < accessHistory.size() - 1) std::cout << " → ";
        }
        std::cout << "\n";
    }

    // Visualización del disco
    std::cout << "\nRepresentación visual del disco:\n";
    std::cout << "0";
    for (int i = 0; i <= maxCylinder; i += 20) {
        if (i <= headPosition && headPosition < i + 20) {
            std::cout << "───[" << headPosition << "]";
        } else {
            std::cout << "────────";
        }
    }
    std::cout << " " << maxCylinder << "\n";

    if (!requestQueue.empty()) {
        std::cout << "\nCola de solicitudes: ";
        for (size_t i = 0; i < std::min((size_t)10, requestQueue.size()); i++) {
            std::cout << requestQueue[i];
            if (i < std::min((size_t)10, requestQueue.size()) - 1) std::cout << ", ";
        }
        if (requestQueue.size() > 10) std::cout << "...";
        std::cout << "\n";
    }
}

void DiskScheduler::showComparison(const std::vector<int>& requests) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         COMPARATIVA DE ALGORITMOS DE DISCO                     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    // Simular cada algoritmo
    std::vector<DiskAlgo> algorithms = {DiskAlgo::FCFS, DiskAlgo::SSTF, DiskAlgo::SCAN};
    std::vector<std::string> algoNames = {"FCFS", "SSTF", "SCAN"};
    std::vector<int> movements;
    
    for (size_t i = 0; i < algorithms.size(); i++) {
        DiskScheduler tempSched(maxCylinder, algorithms[i]);
        tempSched.headPosition = headPosition;
        
        for (int req : requests) {
            tempSched.addRequest(req);
        }
        
        while (!tempSched.requestQueue.empty()) {
            tempSched.processNext();
        }
        
        movements.push_back(tempSched.getTotalMovement());
    }
    
    std::cout << "\n┌─────────────┬──────────────────┬─────────────┐\n";
    std::cout << "│  Algoritmo  │ Movimiento Total │ Eficiencia  │\n";
    std::cout << "├─────────────┼──────────────────┼─────────────┤\n";
    
    int best = *std::min_element(movements.begin(), movements.end());
    for (size_t i = 0; i < algorithms.size(); i++) {
        double efficiency = (best > 0) ? ((double)best / movements[i]) * 100 : 100;
        std::cout << "│ " << std::setw(11) << std::left << algoNames[i]
                  << " │ " << std::setw(16) << movements[i] << " │ "
                  << std::setw(10) << std::fixed << std::setprecision(1) << efficiency << "% │\n";
    }
    std::cout << "└─────────────┴──────────────────┴─────────────┘\n";
}

int DiskScheduler::getTotalMovement() const { return totalMovement; }
int DiskScheduler::getHeadPosition() const { return headPosition; }

void DiskScheduler::reset() {
    totalMovement = 0;
    headPosition = 0;
    accessHistory.clear();
    direction = 1;
}
