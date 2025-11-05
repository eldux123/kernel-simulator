#pragma once
#include <vector>
#include <map>
#include <cmath>

class BuddyAllocator {
private:
    static const int MIN_BLOCK_SIZE = 4096;  // 4KB
    static const int MAX_ORDER = 10;         // Hasta 4MB (4096 * 2^10)
    
    struct Block {
        size_t size;
        bool used;
        Block(size_t s) : size(s), used(false) {}
    };
    
    std::vector<std::vector<Block>> freeLists;
    std::map<void*, size_t> allocations;
    size_t totalMemory;
    size_t usedMemory;
    int fragmentationCount;

public:
    BuddyAllocator(size_t totalSize = 4 * 1024 * 1024) {  // 4MB por defecto
        freeLists.resize(MAX_ORDER + 1);
        totalMemory = totalSize;
        usedMemory = 0;
        fragmentationCount = 0;
        
        // Inicializar con un bloque del tamaño máximo
        size_t initialOrder = static_cast<size_t>(std::log2(totalSize / MIN_BLOCK_SIZE));
        freeLists[initialOrder].push_back(Block(totalSize));
    }
    
    void* allocate(size_t size) {
        // Calcular el orden necesario
        size_t requiredSize = std::max(size, static_cast<size_t>(MIN_BLOCK_SIZE));
        size_t order = static_cast<size_t>(std::ceil(std::log2(requiredSize / MIN_BLOCK_SIZE)));
        
        // Buscar el primer bloque disponible de tamaño suficiente
        for (size_t i = order; i <= MAX_ORDER; i++) {
            if (!freeLists[i].empty()) {
                Block block = freeLists[i].back();
                freeLists[i].pop_back();
                
                // Dividir el bloque si es necesario
                while (i > order) {
                    i--;
                    size_t newSize = block.size / 2;
                    freeLists[i].push_back(Block(newSize));
                    block.size = newSize;
                }
                
                block.used = true;
                usedMemory += block.size;
                void* ptr = reinterpret_cast<void*>(freeLists[i].size());
                allocations[ptr] = block.size;
                return ptr;
            }
        }
        
        fragmentationCount++;
        return nullptr;  // No hay memoria disponible
    }
    
    void free(void* ptr) {
        auto it = allocations.find(ptr);
        if (it != allocations.end()) {
            size_t size = it->second;
            usedMemory -= size;
            
            // Calcular el orden del bloque
            size_t order = static_cast<size_t>(std::log2(size / MIN_BLOCK_SIZE));
            
            // Añadir el bloque a la lista libre correspondiente
            freeLists[order].push_back(Block(size));
            
            // Intentar fusionar bloques buddy
            mergeBuddies(order);
            
            allocations.erase(it);
        }
    }
    
    double getFragmentationRatio() const {
        return static_cast<double>(fragmentationCount) / allocations.size();
    }
    
    size_t getUsedMemory() const { return usedMemory; }
    size_t getTotalMemory() const { return totalMemory; }
    
private:
    void mergeBuddies(size_t order) {
        if (order >= MAX_ORDER) return;
        
        auto& list = freeLists[order];
        for (size_t i = 0; i < list.size(); i += 2) {
            if (i + 1 < list.size()) {
                // Fusionar bloques buddy adyacentes
                Block merged(list[i].size * 2);
                list.erase(list.begin() + i, list.begin() + i + 2);
                freeLists[order + 1].push_back(merged);
                mergeBuddies(order + 1);
            }
        }
    }
};