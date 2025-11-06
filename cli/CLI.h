#ifndef CLI_H
#define CLI_H

#include "../modules/cpu/Scheduler.h"
#include "../modules/mem/MemoryManager.h"
#include "../modules/mem/HeapAllocator.h"
#include "../modules/cpu/Synchronization.h"

class CLI {
private:
    MemoryManager* mem;
    ProducerConsumer* prodCons;
    SchedulerRR* sched;
    HeapAllocator* heap;
    
    void showMenu();
    void handleOption(int opcion);
    
public:
    CLI();
    ~CLI();
    void run();
};

#endif
