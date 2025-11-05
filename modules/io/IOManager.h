#pragma once
#include <queue>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>

enum class IOPriority { HIGH, MEDIUM, LOW };
enum class DeviceType { PRINTER, DISK, NETWORK };

struct IORequest {
    int pid;
    DeviceType device;
    IOPriority priority;
    int size;
    int arrivalTime;
    int completionTime;
    
    IORequest(int p, DeviceType d, IOPriority pr, int s, int t)
        : pid(p), device(d), priority(pr), size(s), arrivalTime(t), completionTime(-1) {}
};

class IODevice {
private:
    std::string name;
    DeviceType type;
    int processingSpeed;  // unidades por tick
    bool busy;
    IORequest* currentRequest;
    std::priority_queue<
        std::pair<IOPriority, IORequest*>,
        std::vector<std::pair<IOPriority, IORequest*>>,
        std::less<std::pair<IOPriority, IORequest*>>
    > waitQueue;
    
    // Estadísticas
    int totalRequests;
    int completedRequests;
    int totalWaitingTime;
    int totalProcessingTime;
    
public:
    IODevice(const std::string& n, DeviceType t, int speed = 1)
        : name(n), type(t), processingSpeed(speed), busy(false), currentRequest(nullptr),
          totalRequests(0), completedRequests(0), totalWaitingTime(0), totalProcessingTime(0) {}
    
    bool isBusy() const { return busy; }
    
    void addRequest(IORequest* request) {
        totalRequests++;
        waitQueue.push({request->priority, request});
    }
    
    void tick(int currentTime) {
        if (busy && currentRequest) {
            currentRequest->size -= processingSpeed;
            if (currentRequest->size <= 0) {
                // Completar request actual
                currentRequest->completionTime = currentTime;
                totalProcessingTime += (currentTime - currentRequest->arrivalTime);
                completedRequests++;
                busy = false;
                currentRequest = nullptr;
            }
        }
        
        if (!busy && !waitQueue.empty()) {
            auto next = waitQueue.top();
            waitQueue.pop();
            currentRequest = next.second;
            busy = true;
            totalWaitingTime += (currentTime - currentRequest->arrivalTime);
        }
    }
    
    void showStats() const {
        std::cout << "\n╔═══════════════════════════════════════════════╗\n";
        std::cout << "║     ESTADÍSTICAS DE DISPOSITIVO DE E/S        ║\n";
        std::cout << "╚═══════════════════════════════════════════════╝\n\n";
        
        std::cout << "Dispositivo: " << name << " (";
        switch (type) {
            case DeviceType::PRINTER: std::cout << "Impresora"; break;
            case DeviceType::DISK: std::cout << "Disco"; break;
            case DeviceType::NETWORK: std::cout << "Red"; break;
        }
        std::cout << ")\n\n";
        
        std::cout << "┌───────────────────────────┬─────────────┐\n";
        std::cout << "│ Peticiones totales        │ " << std::setw(11) << totalRequests << " │\n";
        std::cout << "│ Peticiones completadas    │ " << std::setw(11) << completedRequests << " │\n";
        std::cout << "│ Peticiones pendientes     │ " << std::setw(11) << waitQueue.size() << " │\n";
        
        double avgWait = completedRequests > 0 ? 
            static_cast<double>(totalWaitingTime) / completedRequests : 0;
        double avgProcess = completedRequests > 0 ? 
            static_cast<double>(totalProcessingTime) / completedRequests : 0;
            
        std::cout << "│ Tiempo medio de espera    │ " << std::fixed << std::setprecision(2) 
                  << std::setw(11) << avgWait << " │\n";
        std::cout << "│ Tiempo medio de proceso   │ " << std::setw(11) << avgProcess << " │\n";
        std::cout << "└───────────────────────────┴─────────────┘\n";
        
        if (busy && currentRequest) {
            std::cout << "\nProcesando actualmente:\n";
            std::cout << "PID: " << currentRequest->pid 
                      << " (tamaño restante: " << currentRequest->size << ")\n";
        }
        
        if (!waitQueue.empty()) {
            std::cout << "\nCola de espera:\n";
            auto tempQueue = waitQueue;
            int count = 0;
            while (!tempQueue.empty() && count < 5) {
                auto req = tempQueue.top().second;
                tempQueue.pop();
                std::cout << "PID: " << std::setw(3) << req->pid 
                          << " Pri: " << static_cast<int>(req->priority)
                          << " Tam: " << std::setw(4) << req->size << "\n";
                count++;
            }
            if (!tempQueue.empty()) {
                std::cout << "... y " << tempQueue.size() << " más\n";
            }
        }
    }
    
    double getUtilization() const {
        return totalRequests > 0 ? 
            static_cast<double>(totalProcessingTime) / totalProcessingTime : 0;
    }
};

class IOManager {
private:
    std::map<DeviceType, IODevice*> devices;
    int currentTick;
    
public:
    IOManager() : currentTick(0) {
        // Crear dispositivos por defecto
        devices[DeviceType::PRINTER] = new IODevice("Impresora-1", DeviceType::PRINTER, 10);
        devices[DeviceType::DISK] = new IODevice("Disco-1", DeviceType::DISK, 50);
        devices[DeviceType::NETWORK] = new IODevice("Red-1", DeviceType::NETWORK, 100);
    }
    
    ~IOManager() {
        for (auto& device : devices) {
            delete device.second;
        }
    }
    
    void submitRequest(int pid, DeviceType device, IOPriority priority, int size) {
        if (devices.find(device) != devices.end()) {
            IORequest* request = new IORequest(pid, device, priority, size, currentTick);
            devices[device]->addRequest(request);
        }
    }
    
    void tick() {
        currentTick++;
        for (auto& device : devices) {
            device.second->tick(currentTick);
        }
    }
    
    void showDeviceStats(DeviceType device) const {
        if (devices.find(device) != devices.end()) {
            devices[device]->showStats();
        }
    }
    
    void showAllStats() const {
        std::cout << "\n=== Estado de Dispositivos de E/S ===\n";
        for (const auto& device : devices) {
            device.second->showStats();
            std::cout << "\n";
        }
    }
};