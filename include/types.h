#ifndef TYPES_H
#define TYPES_H


#include <stdint.h>
#include <stddef.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef int64_t i64;
typedef int32_t i32;


#define MAX_PROCS 16 //max processes the system can track simultaneously
#define MAX_SYSCALLS 500 //total unique sys calls defined in OS
#define MAX_NAME 32 //max lenght for names of processes, threads or files
#define MAX_THREADS 8 //max threads for a single process
#define MAX_MUTEXES 16 //max locks(hardware/software) to prevent data races
#define MAX_SEMAPHORES 16 //max on signaling objects used for process synchronization
#define BIG_STRIDE 65536 //val for stride scheduler


/* ANSI color codes for terminal output */
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define GRAY    "\033[90m"

#endif 