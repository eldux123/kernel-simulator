#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include <queue>
#include <map>
#include <string>

// ========== SOLICITUD DE E/S ==========
struct IORequest {
    int pid;
    int priority;           // 1 (alta) a 5 (baja)
    std::string deviceType; // "PRINTER", "DISK", "NETWORK"
    int duration;
    int arrivalTime;

    bool operator<(const IORequest& other) const;
};

// ========== IO MANAGER ==========
/**
 * Gestor de dispositivos de E/S con cola de prioridad.
 * Simula 3 dispositivos: PRINTER, DISK, NETWORK
 */
class IOManager {
private:
    std::priority_queue<IORequest> requestQueue;
    std::map<std::string, int> deviceBusyTime;
    std::map<std::string, int> deviceUsedBy;
    int totalRequests;
    int completedRequests;
    int globalTime;

public:
    IOManager();
    
    // Operaciones principales
    void addIORequest(int pid, int priority, std::string device, int duration);
    bool processIOTick();
    
    // Estadísticas y visualización
    void showStatus() const;
    int getPendingRequests() const;
    int getCompletedRequests() const;
    double getAverageWaitTime() const;
    double getThroughput() const;
    
private:
    void initializeDevices();
};

#endif // IO_MANAGER_H
