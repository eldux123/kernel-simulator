#include "IOManager.h"
#include <iostream>
#include <iomanip>

// ========== IOREQUEST IMPLEMENTATION ==========
bool IORequest::operator<(const IORequest& other) const {
    // Min-heap: prioridad más baja = mayor urgencia
    return priority > other.priority;
}

// ========== IO MANAGER IMPLEMENTATION ==========
IOManager::IOManager() 
    : totalRequests(0), completedRequests(0), globalTime(0) {
    initializeDevices();
}

void IOManager::initializeDevices() {
    deviceBusyTime["PRINTER"] = 0;
    deviceBusyTime["DISK"] = 0;
    deviceBusyTime["NETWORK"] = 0;
    deviceUsedBy["PRINTER"] = -1;
    deviceUsedBy["DISK"] = -1;
    deviceUsedBy["NETWORK"] = -1;
}

void IOManager::addIORequest(int pid, int priority, std::string device, int duration) {
    IORequest req = {pid, priority, device, duration, globalTime};
    requestQueue.push(req);
    totalRequests++;
}

bool IOManager::processIOTick() {
    globalTime++;
    bool assigned = false;

    // Procesar dispositivos ocupados
    for (auto& pair : deviceBusyTime) {
        if (pair.second > 0) {
            pair.second--;
            if (pair.second == 0) {
                completedRequests++;
                deviceUsedBy[pair.first] = -1;
            }
        }
    }

    // Asignar nueva solicitud si hay dispositivo libre
    if (!requestQueue.empty()) {
        IORequest req = requestQueue.top();
        if (deviceBusyTime[req.deviceType] == 0) {
            requestQueue.pop();
            deviceBusyTime[req.deviceType] = req.duration;
            deviceUsedBy[req.deviceType] = req.pid;
            assigned = true;
        }
    }
    return assigned;
}

void IOManager::showStatus() const {
    std::cout << "\n╔════════════════════════════════════════════════════╗\n";
    std::cout << "║         GESTIÓN DE DISPOSITIVOS E/S                ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    std::cout << "\nEstadísticas Globales:\n";
    std::cout << "  Solicitudes totales: " << totalRequests << "\n";
    std::cout << "  Completadas: " << completedRequests << "\n";
    std::cout << "  Pendientes en cola: " << requestQueue.size() << "\n";
    std::cout << "  Tiempo global: " << globalTime << " ticks\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(2) 
              << getThroughput() << " req/tick\n\n";

    std::cout << "┌──────────────┬─────────────────┬──────────────┐\n";
    std::cout << "│ Dispositivo  │ Tiempo restante │ Usado por    │\n";
    std::cout << "├──────────────┼─────────────────┼──────────────┤\n";
    
    for (const auto& pair : deviceBusyTime) {
        std::string status = (pair.second > 0) 
                           ? std::to_string(pair.second) + " ticks" 
                           : "Libre";
        std::string user = (deviceUsedBy.at(pair.first) == -1) 
                         ? "-" 
                         : "PID " + std::to_string(deviceUsedBy.at(pair.first));
        
        std::cout << "│ " << std::left << std::setw(12) << pair.first 
                  << " │ " << std::setw(15) << status
                  << " │ " << std::setw(12) << user << " │\n";
    }
    std::cout << "└──────────────┴─────────────────┴──────────────┘\n";

    if (!requestQueue.empty()) {
        std::cout << "\nPróxima solicitud en cola (prioridad más alta):\n";
        auto temp = requestQueue;
        IORequest next = temp.top();
        std::cout << "  PID: " << next.pid 
                  << " | Prioridad: " << next.priority 
                  << " | Dispositivo: " << next.deviceType 
                  << " | Duración: " << next.duration << " ticks\n";
    }
}

int IOManager::getPendingRequests() const { return requestQueue.size(); }
int IOManager::getCompletedRequests() const { return completedRequests; }

double IOManager::getAverageWaitTime() const {
    return (completedRequests > 0) 
           ? (double)globalTime / completedRequests 
           : 0.0;
}

double IOManager::getThroughput() const {
    return (globalTime > 0) 
           ? (double)completedRequests / globalTime 
           : 0.0;
}
