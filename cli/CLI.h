#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include "../kernel-sim/Main.cpp"

class CLI {
private:
    MemoryManager& memManager;
    DiskScheduler& diskScheduler;
    IOManager& ioManager;
    SchedulerRR& scheduler;
    std::map<std::string, std::function<void(std::vector<std::string>&)>> commands;
    bool running;

public:
    CLI(MemoryManager& mm, DiskScheduler& ds, IOManager& io, SchedulerRR& sched)
        : memManager(mm), diskScheduler(ds), ioManager(io), scheduler(sched), running(true) {
        
        initializeCommands();
    }
    
    void run() {
        showWelcome();
        
        while (running) {
            std::cout << "\n> ";
            std::string line;
            std::getline(std::cin, line);
            
            std::vector<std::string> tokens = tokenize(line);
            if (tokens.empty()) continue;
            
            std::string cmd = tokens[0];
            tokens.erase(tokens.begin());
            
            auto it = commands.find(cmd);
            if (it != commands.end()) {
                it->second(tokens);
            } else {
                std::cout << "Comando desconocido. Use 'help' para ver comandos disponibles.\n";
            }
        }
    }
    
private:
    void initializeCommands() {
        commands["help"] = [this](std::vector<std::string>& args) {
            showHelp();
        };
        
        commands["exit"] = [this](std::vector<std::string>& args) {
            running = false;
        };
        
        commands["create"] = [this](std::vector<std::string>& args) {
            if (args.size() < 2) {
                std::cout << "Uso: create <tipo> <burst> [pages]\n";
                std::cout << "Tipos: normal, producer, consumer\n";
                return;
            }
            
            int burst = std::stoi(args[1]);
            int pages = args.size() > 2 ? std::stoi(args[2]) : 4;
            
            ProcType type;
            if (args[0] == "producer") type = ProcType::PRODUCER;
            else if (args[0] == "consumer") type = ProcType::CONSUMER;
            else type = ProcType::NORMAL;
            
            int pid = scheduler.createProcess(burst, pages, type);
            std::cout << "Proceso creado con PID " << pid << "\n";
        };
        
        commands["list"] = [this](std::vector<std::string>& args) {
            scheduler.listProcesses();
        };
        
        commands["kill"] = [this](std::vector<std::string>& args) {
            if (args.empty()) {
                std::cout << "Uso: kill <pid>\n";
                return;
            }
            int pid = std::stoi(args[0]);
            if (scheduler.killProcess(pid)) {
                std::cout << "Proceso " << pid << " terminado.\n";
            } else {
                std::cout << "No se pudo terminar el proceso " << pid << "\n";
            }
        };
        
        commands["step"] = [this](std::vector<std::string>& args) {
            int steps = args.empty() ? 1 : std::stoi(args[0]);
            for (int i = 0; i < steps; i++) {
                scheduler.tick();
                diskScheduler.processNextRequest();
                ioManager.tick();
            }
            std::cout << "Ejecutados " << steps << " ticks.\n";
        };
        
        commands["memory"] = [this](std::vector<std::string>& args) {
            if (args.empty()) {
                memManager.showFrames();
                return;
            }
            
            if (args[0] == "algorithm") {
                if (args.size() < 2) {
                    std::cout << "Uso: memory algorithm <fifo|lru|second|nru>\n";
                    return;
                }
                
                PageAlgo algo;
                if (args[1] == "fifo") algo = PageAlgo::FIFO;
                else if (args[1] == "lru") algo = PageAlgo::LRU;
                else if (args[1] == "second") algo = PageAlgo::SECOND_CHANCE;
                else if (args[1] == "nru") algo = PageAlgo::NRU;
                else {
                    std::cout << "Algoritmo no válido\n";
                    return;
                }
                
                memManager.setAlgorithm(algo);
                std::cout << "Algoritmo de memoria cambiado.\n";
            }
        };
        
        commands["disk"] = [this](std::vector<std::string>& args) {
            if (args.empty()) {
                diskScheduler.showStats();
                return;
            }
            
            if (args[0] == "request") {
                if (args.size() < 2) {
                    std::cout << "Uso: disk request <cilindro>\n";
                    return;
                }
                diskScheduler.addRequest(std::stoi(args[1]));
                std::cout << "Petición de disco añadida.\n";
            } else if (args[0] == "algorithm") {
                if (args.size() < 2) {
                    std::cout << "Uso: disk algorithm <fcfs|sstf|scan>\n";
                    return;
                }
                
                DiskSchedulingAlgo algo;
                if (args[1] == "fcfs") algo = DiskSchedulingAlgo::FCFS;
                else if (args[1] == "sstf") algo = DiskSchedulingAlgo::SSTF;
                else if (args[1] == "scan") algo = DiskSchedulingAlgo::SCAN;
                else {
                    std::cout << "Algoritmo no válido\n";
                    return;
                }
                
                diskScheduler.setAlgorithm(algo);
                std::cout << "Algoritmo de disco cambiado.\n";
            }
        };
        
        commands["io"] = [this](std::vector<std::string>& args) {
            if (args.empty()) {
                ioManager.showAllStats();
                return;
            }
            
            if (args[0] == "request") {
                if (args.size() < 4) {
                    std::cout << "Uso: io request <pid> <printer|disk|network> <high|medium|low> <size>\n";
                    return;
                }
                
                int pid = std::stoi(args[1]);
                
                DeviceType device;
                if (args[2] == "printer") device = DeviceType::PRINTER;
                else if (args[2] == "disk") device = DeviceType::DISK;
                else if (args[2] == "network") device = DeviceType::NETWORK;
                else {
                    std::cout << "Dispositivo no válido\n";
                    return;
                }
                
                IOPriority priority;
                if (args[3] == "high") priority = IOPriority::HIGH;
                else if (args[3] == "medium") priority = IOPriority::MEDIUM;
                else if (args[3] == "low") priority = IOPriority::LOW;
                else {
                    std::cout << "Prioridad no válida\n";
                    return;
                }
                
                int size = std::stoi(args[4]);
                
                ioManager.submitRequest(pid, device, priority, size);
                std::cout << "Petición de E/S añadida.\n";
            }
        };
        
        commands["stats"] = [this](std::vector<std::string>& args) {
            scheduler.showStats();
            memManager.showFrames();
            diskScheduler.showStats();
            ioManager.showAllStats();
        };
    }
    
    std::vector<std::string> tokenize(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            tokens.push_back(token);
        }
        return tokens;
    }
    
    void showWelcome() {
        std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
        std::cout << "║        SIMULADOR DE KERNEL - INTERFAZ DE LÍNEA        ║\n";
        std::cout << "║                  DE COMANDOS (CLI)                    ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
        std::cout << "Escriba 'help' para ver la lista de comandos disponibles.\n";
    }
    
    void showHelp() {
        std::cout << "\nComandos disponibles:\n\n";
        std::cout << "Gestión de procesos:\n";
        std::cout << "  create <tipo> <burst> [pages] - Crear nuevo proceso (normal|producer|consumer)\n";
        std::cout << "  list                         - Listar procesos activos\n";
        std::cout << "  kill <pid>                   - Terminar proceso\n";
        std::cout << "  step [n]                     - Ejecutar n ticks (1 por defecto)\n\n";
        
        std::cout << "Memoria virtual:\n";
        std::cout << "  memory                       - Mostrar estado de la memoria\n";
        std::cout << "  memory algorithm <algo>      - Cambiar algoritmo (fifo|lru|second|nru)\n\n";
        
        std::cout << "Gestión de disco:\n";
        std::cout << "  disk                         - Mostrar estado del disco\n";
        std::cout << "  disk request <cilindro>      - Añadir petición de disco\n";
        std::cout << "  disk algorithm <algo>        - Cambiar algoritmo (fcfs|sstf|scan)\n\n";
        
        std::cout << "Entrada/Salida:\n";
        std::cout << "  io                           - Mostrar estado de dispositivos E/S\n";
        std::cout << "  io request <args>            - Añadir petición de E/S\n";
        std::cout << "    Uso: io request <pid> <printer|disk|network> <high|medium|low> <size>\n\n";
        
        std::cout << "Sistema:\n";
        std::cout << "  stats                        - Mostrar todas las estadísticas\n";
        std::cout << "  help                         - Mostrar esta ayuda\n";
        std::cout << "  exit                         - Salir del simulador\n";
    }
};