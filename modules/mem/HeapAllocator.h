#ifndef HEAP_ALLOCATOR_H
#define HEAP_ALLOCATOR_H

#include <vector>
#include <map>
#include <cmath>
#include <iostream>
#include <iomanip>

// ========== BUDDY SYSTEM ALLOCATOR ==========

/**
 * Bloque de memoria en el sistema Buddy
 */
struct Block {
    size_t size;           // Tamaño del bloque (potencia de 2)
    bool isFree;           // ¿Está libre?
    size_t address;        // Dirección de inicio
    int order;             // Orden del bloque (log2(size))
    
    Block(size_t sz, size_t addr, int ord) 
        : size(sz), isFree(true), address(addr), order(ord) {}
};

/**
 * Asignador de Heap con Buddy System
 * 
 * Características:
 * - División recursiva en potencias de 2
 * - Coalescencia de bloques adyacentes
 * - Fragmentación interna controlada
 * - O(log n) para alloc y free
 */
class HeapAllocator {
private:
    size_t totalSize;                          // Tamaño total del heap
    size_t minBlockSize;                       // Bloque mínimo (ej: 64 bytes)
    int maxOrder;                              // Orden máximo (log2(totalSize))
    
    std::vector<std::vector<Block*>> freeLists; // Listas libres por orden
    std::map<size_t, Block*> allocatedBlocks;   // Bloques asignados (addr -> Block)
    
    // Estadísticas
    size_t totalAllocations;
    size_t totalDeallocations;
    size_t totalBytesAllocated;
    size_t totalBytesFreed;
    size_t internalFragmentation;
    size_t externalFragmentation;
    
    // Métricas de tiempo (simuladas en ticks)
    int allocTime;
    int freeTime;
    
public:
    HeapAllocator(size_t heapSize = 1024 * 1024, size_t minSize = 64);
    ~HeapAllocator();
    
    // Operaciones principales
    void* allocate(size_t size);
    bool deallocate(void* ptr);
    
    // Estadísticas
    void showStatus() const;
    void showFragmentation() const;
    void showAllocationMap() const;
    double getInternalFragmentation() const;
    double getExternalFragmentation() const;
    size_t getTotalAllocated() const;
    size_t getTotalFree() const;
    int getAvgAllocTime() const;
    int getAvgFreeTime() const;
    
    // Utilidades
    void reset();
    
private:
    int getOrder(size_t size) const;
    size_t getBlockSize(int order) const;
    Block* findBuddy(Block* block);
    void splitBlock(int order);
    void mergeBlock(Block* block);
};

#endif // HEAP_ALLOCATOR_H
