#include "CLI.h"
#include <iostream>

CLI::CLI() {
    mem = new MemoryManager(DEFAULT_NUM_FRAMES);
    prodCons = new ProducerConsumer(DEFAULT_BUFFER_SIZE);
    sched = new SchedulerRR(*mem, *prodCons, DEFAULT_QUANTUM);
    heap = new HeapAllocator(1024 * 64, 64); // 64KB heap, bloques mínimos de 64B
}

CLI::~CLI() {
    delete heap;
    delete sched;
    delete prodCons;
    delete mem;
}

void CLI::showMenu() {
    std::cout << "\n╔═══════════════════════════════════════╗\n";
    std::cout << "║        MENÚ PRINCIPAL                 ║\n";
    std::cout << "╚═══════════════════════════════════════╝\n";
    std::cout << "┌─────────────────────────────────────────┐\n";
    std::cout << "│  GESTIÓN DE PROCESOS                    │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│ 1.  Crear proceso normal                │\n";
    std::cout << "│ 2.  Crear proceso productor             │\n";
    std::cout << "│ 3.  Crear proceso consumidor            │\n";
    std::cout << "│ 4.  Mostrar procesos (tabla simple)     │\n";
    std::cout << "│ 5.  Terminar proceso                    │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│  GESTIÓN DE HILOS                       │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│ 14. Crear hilos en proceso              │\n";
    std::cout << "│ 15. Mostrar hilos de un proceso         │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│  EJECUCIÓN                              │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│ 6.  Avanzar 1 tick                      │\n";
    std::cout << "│ 7.  Ejecutar varios ticks               │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│  REPORTES Y ESTADÍSTICAS                │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│ 8.  Estadísticas resumidas              │\n";
    std::cout << "│ 9.  Reporte completo detallado          │\n";
    std::cout << "│ 10. Mostrar marcos de memoria           │\n";
    std::cout << "│ 11. Mostrar buffer (prod-cons)          │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│  HEAP ALLOCATOR (BUDDY SYSTEM)          │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│ 16. Asignar memoria del heap            │\n";
    std::cout << "│ 17. Liberar memoria del heap            │\n";
    std::cout << "│ 18. Estado del heap                     │\n";
    std::cout << "│ 19. Análisis de fragmentación           │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│  CONFIGURACIÓN                          │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│ 12. Cambiar tamaño de memoria           │\n";
    std::cout << "│ 13. Cambiar algoritmo de paginación     │\n";
    std::cout << "├─────────────────────────────────────────┤\n";
    std::cout << "│ 0.  Salir                               │\n";
    std::cout << "└─────────────────────────────────────────┘\n";
}

void CLI::handleOption(int opcion) {
    if (opcion == 1) {
        int burst, pages;
        std::cout << "Ingrese ráfagas (ticks): "; std::cin >> burst;
        std::cout << "Ingrese número de páginas: "; std::cin >> pages;
        int pid = sched->createProcess(burst, pages, ProcType::NORMAL);
        std::cout << "Proceso NORMAL creado con PID=" << pid << "\n";
    }
    else if (opcion == 2) {
        int burst, pages;
        std::cout << "Ingrese ráfagas (ticks): "; std::cin >> burst;
        std::cout << "Ingrese número de páginas: "; std::cin >> pages;
        int pid = sched->createProcess(burst, pages, ProcType::PRODUCER);
        std::cout << "Proceso PRODUCTOR creado con PID=" << pid << "\n";
    }
    else if (opcion == 3) {
        int burst, pages;
        std::cout << "Ingrese ráfagas (ticks): "; std::cin >> burst;
        std::cout << "Ingrese número de páginas: "; std::cin >> pages;
        int pid = sched->createProcess(burst, pages, ProcType::CONSUMER);
        std::cout << "Proceso CONSUMIDOR creado con PID=" << pid << "\n";
    }
    else if (opcion == 4) sched->listProcesses();
    else if (opcion == 5) {
        int pid; std::cout << "PID a terminar: "; std::cin >> pid;
        if (sched->killProcess(pid)) std::cout << "Proceso " << pid << " terminado.\n";
        else std::cout << "PID no encontrado.\n";
    }
    else if (opcion == 6) { 
        sched->tick(); 
        std::cout << "Avanzado 1 tick. Tick actual: " << sched->getTick() << "\n"; 
    }
    else if (opcion == 7) {
        int n; std::cout << "Cuántos ticks desea ejecutar: "; std::cin >> n;
        sched->runTicks(n);
        std::cout << "Ejecutados " << n << " ticks. Tick actual: " << sched->getTick() << "\n";
    }
    else if (opcion == 8) sched->showStats();
    else if (opcion == 9) sched->showDetailedReport();
    else if (opcion == 10) mem->showFrames();
    else if (opcion == 11) prodCons->showBuffer();
    else if (opcion == 12) {
        int n; std::cout << "Nuevo número de marcos: "; std::cin >> n;
        mem->setNumFrames(n);
        std::cout << "Tamaño de memoria actualizado.\n";
    }
    else if (opcion == 13) {
        int m;
        std::cout << "Seleccione algoritmo de paginación (1=FIFO, 2=LRU): ";
        std::cin >> m;
        mem->setAlgorithm(m == 2 ? PageAlgo::LRU : PageAlgo::FIFO);
        std::cout << "Algoritmo actualizado.\n";
    }
    else if (opcion == 14) {
        int pid, numThreads, burstPerThread;
        std::cout << "PID del proceso: "; std::cin >> pid;
        std::cout << "Número de hilos a crear (máx " << MAX_THREADS_PER_PROCESS << "): "; std::cin >> numThreads;
        std::cout << "Burst por hilo: "; std::cin >> burstPerThread;
        
        if (numThreads > MAX_THREADS_PER_PROCESS) {
            std::cout << "Error: Máximo " << MAX_THREADS_PER_PROCESS << " hilos por proceso.\n";
        } else {
            int created = 0;
            for (int i = 0; i < numThreads; i++) {
                int tid = sched->createThreadInProcess(pid, burstPerThread);
                if (tid != -1) created++;
            }
            if (created > 0) {
                std::cout << "✓ Creados " << created << " hilos en proceso PID=" << pid << "\n";
            } else {
                std::cout << "✗ Error: No se pudieron crear hilos. Verifique el PID.\n";
            }
        }
    }
    else if (opcion == 15) {
        int pid;
        std::cout << "PID del proceso: "; std::cin >> pid;
        sched->showThreads(pid);
    }
    else if (opcion == 16) {
        size_t size;
        std::cout << "Tamaño a asignar (bytes): "; std::cin >> size;
        void* ptr = heap->allocate(size);
        if (ptr) {
            std::cout << "✓ Memoria asignada en dirección: 0x" << std::hex << reinterpret_cast<size_t>(ptr) << std::dec << "\n";
        } else {
            std::cout << "✗ Error: No se pudo asignar memoria\n";
        }
    }
    else if (opcion == 17) {
        size_t addr;
        std::cout << "Dirección a liberar (hex, sin 0x): "; std::cin >> std::hex >> addr >> std::dec;
        void* ptr = reinterpret_cast<void*>(addr);
        if (heap->deallocate(ptr)) {
            std::cout << "✓ Memoria liberada correctamente\n";
        } else {
            std::cout << "✗ Error: Dirección inválida\n";
        }
    }
    else if (opcion == 18) {
        heap->showStatus();
    }
    else if (opcion == 19) {
        heap->showFragmentation();
        heap->showAllocationMap();
    }
    else if (opcion != 0) {
        std::cout << "Opción inválida.\n";
    }
}

void CLI::run() {
    int opcion = 0;
    while (true) {
        showMenu();
        std::cout << "Seleccione una opción: ";
        std::cin >> opcion;
        
        if (opcion == 0) {
            std::cout << "Saliendo...\n";
            break;
        }
        
        handleOption(opcion);
    }
}
