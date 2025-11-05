#pragma once

// Definiciones comunes para todo el proyecto
enum class ProcState { NEW, READY, RUNNING, WAITING, TERMINATED };
enum class ProcType { NORMAL, PRODUCER, CONSUMER };
enum class ThreadState { THREAD_NEW, THREAD_READY, THREAD_RUNNING, THREAD_WAITING, THREAD_TERMINATED };
enum class PageAlgo { FIFO, LRU, SECOND_CHANCE, NRU };

const int DEFAULT_QUANTUM = 3;
const int DEFAULT_NUM_FRAMES = 4;
const int DEFAULT_BUFFER_SIZE = 5;
const int MAX_THREADS_PER_PROCESS = 4;