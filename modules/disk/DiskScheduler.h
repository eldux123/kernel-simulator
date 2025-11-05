#pragma once
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>

enum class DiskSchedulingAlgo { FCFS, SSTF, SCAN };

class DiskScheduler {
private:
    int currentPosition;
    int maxCylinders;
    bool movingUp;  // Para SCAN
    std::vector<int> requestQueue;
    std::vector<int> completedRequests;
    DiskSchedulingAlgo algorithm;
    int totalMovement;
    int totalRequests;
    
public:
    DiskScheduler(int cylinders = 200) 
        : currentPosition(0), maxCylinders(cylinders), movingUp(true),
          algorithm(DiskSchedulingAlgo::FCFS), totalMovement(0), totalRequests(0) {}
    
    void setAlgorithm(DiskSchedulingAlgo algo) {
        algorithm = algo;
        // Reiniciar estadísticas al cambiar de algoritmo
        totalMovement = 0;
        totalRequests = 0;
        completedRequests.clear();
    }
    
    void addRequest(int cylinder) {
        if (cylinder >= 0 && cylinder < maxCylinders) {
            requestQueue.push_back(cylinder);
        }
    }
    
    bool hasRequests() const {
        return !requestQueue.empty();
    }
    
    void processNextRequest() {
        if (requestQueue.empty()) return;
        
        int nextCylinder;
        
        switch (algorithm) {
            case DiskSchedulingAlgo::FCFS:
                nextCylinder = requestQueue[0];
                requestQueue.erase(requestQueue.begin());
                break;
                
            case DiskSchedulingAlgo::SSTF: {
                auto it = std::min_element(requestQueue.begin(), requestQueue.end(),
                    [this](int a, int b) {
                        return std::abs(a - currentPosition) < std::abs(b - currentPosition);
                    });
                nextCylinder = *it;
                requestQueue.erase(it);
                break;
            }
                
            case DiskSchedulingAlgo::SCAN: {
                auto it = requestQueue.begin();
                if (movingUp) {
                    // Buscar siguiente cilindro en dirección ascendente
                    it = std::find_if(requestQueue.begin(), requestQueue.end(),
                        [this](int c) { return c >= currentPosition; });
                    
                    if (it == requestQueue.end()) {
                        movingUp = false;
                        it = std::max_element(requestQueue.begin(), requestQueue.end());
                    }
                } else {
                    // Buscar siguiente cilindro en dirección descendente
                    auto it_reverse = std::find_if(requestQueue.rbegin(), requestQueue.rend(),
                        [this](int c) { return c <= currentPosition; });
                    
                    if (it_reverse == requestQueue.rend()) {
                        movingUp = true;
                        it = std::min_element(requestQueue.begin(), requestQueue.end());
                    } else {
                        it = std::prev(it_reverse.base());
                    }
                }
                nextCylinder = *it;
                requestQueue.erase(it);
                break;
            }
        }
        
        int movement = std::abs(nextCylinder - currentPosition);
        totalMovement += movement;
        totalRequests++;
        currentPosition = nextCylinder;
        completedRequests.push_back(nextCylinder);
    }
    
    void showStats() const {
        std::cout << "\n╔═══════════════════════════════════════════════╗\n";
        std::cout << "║         ESTADÍSTICAS DE PLANIFICACIÓN         ║\n";
        std::cout << "║               DE DISCO                        ║\n";
        std::cout << "╚═══════════════════════════════════════════════╝\n\n";
        
        std::string algoName;
        switch (algorithm) {
            case DiskSchedulingAlgo::FCFS: algoName = "FCFS"; break;
            case DiskSchedulingAlgo::SSTF: algoName = "SSTF"; break;
            case DiskSchedulingAlgo::SCAN: algoName = "SCAN"; break;
        }
        
        std::cout << "Algoritmo: " << algoName << "\n";
        std::cout << "Posición actual: " << currentPosition << "\n";
        std::cout << "Movimiento total: " << totalMovement << " cilindros\n";
        std::cout << "Promedio por petición: " << 
            (totalRequests > 0 ? static_cast<double>(totalMovement)/totalRequests : 0) 
            << " cilindros\n\n";
        
        // Mostrar gráfico de movimiento del cabezal
        std::cout << "Movimiento del cabezal:\n";
        if (!completedRequests.empty()) {
            int minCyl = *std::min_element(completedRequests.begin(), completedRequests.end());
            int maxCyl = *std::max_element(completedRequests.begin(), completedRequests.end());
            
            const int width = 50;  // Ancho del gráfico
            const int height = 10;  // Alto del gráfico
            
            std::vector<std::vector<char>> graph(height, std::vector<char>(width, ' '));
            
            // Dibujar líneas horizontales
            for (size_t i = 0; i < completedRequests.size() - 1; i++) {
                int x1 = (completedRequests[i] - minCyl) * (width-1) / (maxCyl - minCyl);
                int x2 = (completedRequests[i+1] - minCyl) * (width-1) / (maxCyl - minCyl);
                int y = (i * (height-1)) / (completedRequests.size() - 1);
                
                // Dibujar línea entre puntos
                for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
                    graph[y][x] = '-';
                }
                if (i < completedRequests.size() - 1) {
                    graph[y][x1] = '*';
                    // Línea vertical al siguiente punto
                    int nextY = ((i+1) * (height-1)) / (completedRequests.size() - 1);
                    for (int y2 = std::min(y, nextY); y2 <= std::max(y, nextY); y2++) {
                        graph[y2][x2] = '|';
                    }
                }
            }
            
            // Imprimir el gráfico
            std::cout << " " << std::string(width+2, '_') << "\n";
            for (const auto& row : graph) {
                std::cout << "|";
                for (char c : row) std::cout << c;
                std::cout << "|\n";
            }
            std::cout << " " << std::string(width+2, '‾') << "\n";
            
            // Escala
            std::cout << minCyl;
            std::cout << std::string(width-6, ' ');
            std::cout << maxCyl << " (cilindros)\n";
        }
        
        // Mostrar peticiones pendientes
        std::cout << "\nPeticiones pendientes: ";
        for (int req : requestQueue) {
            std::cout << req << " ";
        }
        std::cout << "\n";
    }
    
    int getCurrentPosition() const { return currentPosition; }
    int getTotalMovement() const { return totalMovement; }
    double getAverageMovement() const {
        return totalRequests > 0 ? static_cast<double>(totalMovement)/totalRequests : 0;
    }
};