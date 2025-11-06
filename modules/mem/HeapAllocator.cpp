#include "HeapAllocator.h"

// ========== CONSTRUCTOR ==========
HeapAllocator::HeapAllocator(size_t heapSize, size_t minSize) 
    : totalSize(heapSize), minBlockSize(minSize), 
      totalAllocations(0), totalDeallocations(0),
      totalBytesAllocated(0), totalBytesFreed(0),
      internalFragmentation(0), externalFragmentation(0),
      allocTime(0), freeTime(0) {
    
    // Calcular orden máximo
    maxOrder = static_cast<int>(std::log2(totalSize / minBlockSize));
    
    // Inicializar listas libres
    freeLists.resize(maxOrder + 1);
    
    // Crear bloque inicial (todo el heap)
    Block* initialBlock = new Block(totalSize, 0, maxOrder);
    freeLists[maxOrder].push_back(initialBlock);
    
    std::cout << "HeapAllocator inicializado:\n";
    std::cout << "  Tamaño total: " << totalSize << " bytes\n";
    std::cout << "  Bloque mínimo: " << minBlockSize << " bytes\n";
    std::cout << "  Orden máximo: " << maxOrder << "\n";
}

HeapAllocator::~HeapAllocator() {
    // Limpiar bloques
    for (auto& list : freeLists) {
        for (auto block : list) {
            delete block;
        }
    }
    for (auto& pair : allocatedBlocks) {
        delete pair.second;
    }
}

// ========== OPERACIONES PRINCIPALES ==========

void* HeapAllocator::allocate(size_t size) {
    int startTick = 0; // Simulación de tiempo
    
    if (size == 0 || size > totalSize) {
        return nullptr;
    }
    
    // Encontrar el orden necesario
    int order = getOrder(size);
    if (order < 0 || order > maxOrder) {
        return nullptr;
    }
    
    // Buscar bloque libre del orden adecuado
    int currentOrder = order;
    while (currentOrder <= maxOrder && freeLists[currentOrder].empty()) {
        currentOrder++;
    }
    
    if (currentOrder > maxOrder) {
        // No hay memoria disponible
        return nullptr;
    }
    
    // Dividir bloques si es necesario
    while (currentOrder > order) {
        splitBlock(currentOrder);
        currentOrder--;
    }
    
    // Tomar el bloque
    Block* block = freeLists[order].back();
    freeLists[order].pop_back();
    block->isFree = false;
    
    // Registrar asignación
    allocatedBlocks[block->address] = block;
    totalAllocations++;
    totalBytesAllocated += block->size;
    
    // Calcular fragmentación interna
    size_t wastedSpace = block->size - size;
    internalFragmentation += wastedSpace;
    
    // Simular tiempo de asignación (log n)
    allocTime += (currentOrder - order + 1);
    
    return reinterpret_cast<void*>(block->address);
}

bool HeapAllocator::deallocate(void* ptr) {
    if (ptr == nullptr) return false;
    
    size_t address = reinterpret_cast<size_t>(ptr);
    
    auto it = allocatedBlocks.find(address);
    if (it == allocatedBlocks.end()) {
        return false; // Dirección no válida
    }
    
    Block* block = it->second;
    block->isFree = true;
    
    // Actualizar estadísticas
    totalDeallocations++;
    totalBytesFreed += block->size;
    allocatedBlocks.erase(it);
    
    // Intentar fusionar con buddy
    mergeBlock(block);
    
    // Simular tiempo de liberación
    freeTime += 1;
    
    return true;
}

// ========== MÉTODOS PRIVADOS ==========

int HeapAllocator::getOrder(size_t size) const {
    // Encontrar el orden mínimo que puede contener size
    size_t blockSize = minBlockSize;
    int order = 0;
    
    while (blockSize < size && order < maxOrder) {
        blockSize *= 2;
        order++;
    }
    
    return (blockSize >= size) ? order : -1;
}

size_t HeapAllocator::getBlockSize(int order) const {
    return minBlockSize * (1ULL << order);
}

Block* HeapAllocator::findBuddy(Block* block) {
    // El buddy tiene la misma dirección XOR su tamaño
    size_t buddyAddress = block->address ^ block->size;
    
    // Buscar en la lista libre del mismo orden
    for (auto b : freeLists[block->order]) {
        if (b->address == buddyAddress && b->isFree) {
            return b;
        }
    }
    
    return nullptr;
}

void HeapAllocator::splitBlock(int order) {
    if (order <= 0 || freeLists[order].empty()) {
        return;
    }
    
    // Tomar bloque del orden actual
    Block* block = freeLists[order].back();
    freeLists[order].pop_back();
    
    // Crear dos bloques de orden inferior
    size_t newSize = block->size / 2;
    int newOrder = order - 1;
    
    Block* block1 = new Block(newSize, block->address, newOrder);
    Block* block2 = new Block(newSize, block->address + newSize, newOrder);
    
    freeLists[newOrder].push_back(block1);
    freeLists[newOrder].push_back(block2);
    
    delete block;
}

void HeapAllocator::mergeBlock(Block* block) {
    Block* buddy = findBuddy(block);
    
    if (buddy == nullptr || !buddy->isFree) {
        // No se puede fusionar, agregar a lista libre
        freeLists[block->order].push_back(block);
        return;
    }
    
    // Fusionar con buddy
    // Remover buddy de lista libre
    auto& list = freeLists[block->order];
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (*it == buddy) {
            list.erase(it);
            break;
        }
    }
    
    // Crear bloque fusionado
    size_t newAddress = std::min(block->address, buddy->address);
    size_t newSize = block->size * 2;
    int newOrder = block->order + 1;
    
    Block* merged = new Block(newSize, newAddress, newOrder);
    
    delete block;
    delete buddy;
    
    // Intentar fusionar recursivamente
    if (newOrder < maxOrder) {
        mergeBlock(merged);
    } else {
        freeLists[newOrder].push_back(merged);
    }
}

// ========== ESTADÍSTICAS ==========

void HeapAllocator::showStatus() const {
    std::cout << "\n╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║         HEAP ALLOCATOR (BUDDY SYSTEM)             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n\n";
    
    std::cout << "📊 ESTADÍSTICAS GENERALES:\n";
    std::cout << "  Asignaciones totales:    " << totalAllocations << "\n";
    std::cout << "  Liberaciones totales:    " << totalDeallocations << "\n";
    std::cout << "  Bytes asignados:         " << totalBytesAllocated << "\n";
    std::cout << "  Bytes liberados:         " << totalBytesFreed << "\n";
    std::cout << "  Memoria en uso:          " << getTotalAllocated() << " bytes\n";
    std::cout << "  Memoria libre:           " << getTotalFree() << " bytes\n";
    std::cout << "  Utilización:             " << std::fixed << std::setprecision(2) 
              << (getTotalAllocated() * 100.0 / totalSize) << "%\n\n";
    
    std::cout << "📈 FRAGMENTACIÓN:\n";
    std::cout << "  Interna:                 " << std::fixed << std::setprecision(2)
              << getInternalFragmentation() << "% (" << internalFragmentation << " bytes)\n";
    std::cout << "  Externa:                 " << std::fixed << std::setprecision(2)
              << getExternalFragmentation() << "%\n\n";
    
    std::cout << "⏱️  LATENCIA PROMEDIO:\n";
    std::cout << "  Tiempo alloc:            " << getAvgAllocTime() << " ticks\n";
    std::cout << "  Tiempo free:             " << getAvgFreeTime() << " ticks\n\n";
    
    std::cout << "🗂️  LISTAS LIBRES POR ORDEN:\n";
    for (int i = 0; i <= maxOrder; i++) {
        if (!freeLists[i].empty()) {
            std::cout << "  Orden " << i << " (" << getBlockSize(i) << " bytes): " 
                      << freeLists[i].size() << " bloques\n";
        }
    }
}

void HeapAllocator::showFragmentation() const {
    std::cout << "\n╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║         ANÁLISIS DE FRAGMENTACIÓN                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n\n";
    
    std::cout << "📊 FRAGMENTACIÓN INTERNA:\n";
    std::cout << "  Definición: Espacio desperdiciado dentro de bloques asignados\n";
    std::cout << "  Total: " << internalFragmentation << " bytes\n";
    std::cout << "  Porcentaje: " << std::fixed << std::setprecision(2) 
              << getInternalFragmentation() << "%\n";
    std::cout << "  Causa: Redondeo a potencias de 2 del Buddy System\n\n";
    
    std::cout << "📊 FRAGMENTACIÓN EXTERNA:\n";
    std::cout << "  Definición: Memoria libre pero no contigua\n";
    std::cout << "  Porcentaje: " << std::fixed << std::setprecision(2)
              << getExternalFragmentation() << "%\n";
    std::cout << "  Bloques libres: " << getTotalFree() << " bytes en múltiples bloques\n";
    std::cout << "  Ventaja Buddy: Coalescencia automática reduce fragmentación externa\n";
}

void HeapAllocator::showAllocationMap() const {
    std::cout << "\n╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║         MAPA DE ASIGNACIONES                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Bloques asignados: " << allocatedBlocks.size() << "\n\n";
    
    if (allocatedBlocks.empty()) {
        std::cout << "  (ninguno)\n";
        return;
    }
    
    std::cout << std::setw(12) << "Dirección" << " | " 
              << std::setw(10) << "Tamaño" << " | "
              << std::setw(8) << "Orden" << "\n";
    std::cout << std::string(40, '-') << "\n";
    
    for (const auto& pair : allocatedBlocks) {
        const Block* block = pair.second;
        std::cout << "  0x" << std::hex << std::setw(8) << std::setfill('0') 
                  << block->address << std::dec << std::setfill(' ') << " | "
                  << std::setw(8) << block->size << " B | "
                  << std::setw(8) << block->order << "\n";
    }
}

double HeapAllocator::getInternalFragmentation() const {
    if (totalBytesAllocated == 0) return 0.0;
    return (internalFragmentation * 100.0) / totalBytesAllocated;
}

double HeapAllocator::getExternalFragmentation() const {
    size_t totalFree = getTotalFree();
    if (totalFree == 0) return 0.0;
    
    // Calcular el bloque libre más grande
    size_t largestFree = 0;
    for (int i = maxOrder; i >= 0; i--) {
        if (!freeLists[i].empty()) {
            largestFree = getBlockSize(i);
            break;
        }
    }
    
    // Fragmentación externa = (totalFree - largestFree) / totalFree
    return ((totalFree - largestFree) * 100.0) / totalFree;
}

size_t HeapAllocator::getTotalAllocated() const {
    return totalBytesAllocated - totalBytesFreed;
}

size_t HeapAllocator::getTotalFree() const {
    return totalSize - getTotalAllocated();
}

int HeapAllocator::getAvgAllocTime() const {
    return (totalAllocations > 0) ? (allocTime / totalAllocations) : 0;
}

int HeapAllocator::getAvgFreeTime() const {
    return (totalDeallocations > 0) ? (freeTime / totalDeallocations) : 0;
}

void HeapAllocator::reset() {
    // Limpiar todo
    for (auto& list : freeLists) {
        for (auto block : list) {
            delete block;
        }
        list.clear();
    }
    
    for (auto& pair : allocatedBlocks) {
        delete pair.second;
    }
    allocatedBlocks.clear();
    
    // Reiniciar estadísticas
    totalAllocations = 0;
    totalDeallocations = 0;
    totalBytesAllocated = 0;
    totalBytesFreed = 0;
    internalFragmentation = 0;
    externalFragmentation = 0;
    allocTime = 0;
    freeTime = 0;
    
    // Crear bloque inicial
    Block* initialBlock = new Block(totalSize, 0, maxOrder);
    freeLists[maxOrder].push_back(initialBlock);
}
