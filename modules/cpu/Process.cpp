#include "Process.h"

// ========== IMPLEMENTACIÓN DE THREAD ==========
Thread::Thread(int _tid, int _pid, int burst)
    : tid(_tid), parentPid(_pid), state(ThreadState::THREAD_NEW), 
      burstRemaining(burst), waitingTime(0),
      itemsProduced(0), itemsConsumed(0), blockedOnSemaphore(-1) {}

std::string Thread::getStateString() const {
    switch (state) {
        case ThreadState::THREAD_NEW: return "NEW";
        case ThreadState::THREAD_READY: return "READY";
        case ThreadState::THREAD_RUNNING: return "RUNNING";
        case ThreadState::THREAD_WAITING: return "WAITING";
        case ThreadState::THREAD_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

// ========== IMPLEMENTACIÓN DE PCB ==========
PCB::PCB(int _id, int burst, int arrival, int pages)
    : id(_id), state(ProcState::NEW), type(ProcType::NORMAL), burstRemaining(burst),
      arrivalTick(arrival), finishTick(-1), waitingTime(0), turnaround(0),
      numPages(pages), nextPageToAccess(0), pageAccesses(0), pageFaults(0),
      itemsProduced(0), itemsConsumed(0), blockedOnSemaphore(-1),
      hasThreads(false), nextThreadId(1) {}

std::string PCB::getStateString() const {
    switch (state) {
        case ProcState::NEW: return "NEW";
        case ProcState::READY: return "READY";
        case ProcState::RUNNING: return "RUNNING";
        case ProcState::WAITING: return "WAITING";
        case ProcState::SUSPENDED: return "SUSPENDED";
        case ProcState::TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

std::string PCB::getTypeString() const {
    switch (type) {
        case ProcType::NORMAL: return "NORMAL";
        case ProcType::PRODUCER: return "PRODUCER";
        case ProcType::CONSUMER: return "CONSUMER";
        case ProcType::PHILOSOPHER: return "PHILOSOPHER";
        case ProcType::READER: return "READER";
        case ProcType::WRITER: return "WRITER";
        default: return "UNKNOWN";
    }
}

bool PCB::isTerminated() const { return state == ProcState::TERMINATED; }
bool PCB::isReady() const { return state == ProcState::READY; }
bool PCB::isRunning() const { return state == ProcState::RUNNING; }
bool PCB::isWaiting() const { return state == ProcState::WAITING; }
bool PCB::isSuspended() const { return state == ProcState::SUSPENDED; }
