#ifndef SCHEDULER_H

#include "types.h"

// process states
typedef enum {
    UNUSED = 0,
    USED,
    RUNNABLE,
    SLEEPING,
    ZOMBIE
} ProcState;


//schedulling algorithm
typedef enum{
    SCHED_RR = 0, //round robin algo
    SCHD_STRIDE = 1 //stride with priority
} SchedAlgo;


//PCB 
typedef struct{
    int pid;
    char name[MAX_NAME];
    ProcState state;

    //track process life
    int total_burst; //total time process needs
    int remaining_burst;//time still left for process to complete run
    int time_used;//time so far process has used while running
    int arrival_time; //time proc gets into ready queue
    int finish_time; //time process to finish running
    int wait_time; // time to wait until its time for process to use cpu


    //syscalls
    u32 syscall_times[MAX_SYSCALLS];

    //stride fields
    int priority;
    u64 stride; 
    u64 pass;
} PCB;


//scheduler state
typedef struct{
    PCB procs[MAX_PROCS];
    int proc_count;
    int current_pid; //curr pid
    int clock;
    int time_slice;
    SchedAlgo algo; //algo for stride
} Scheduler;


void sched_init(Scheduler *s, SchedAlgo algo, int time_slice);
int sched_add_process(Scheduler *s, const char *name, int burst, int arrival, int prority);

void sched_run(Scheduler *s); //run full simulation
void sched_print_gantt(Scheduler *s); // draw the chart of gantt
void sched_print_stats(Scheduler *s);// wait times stats

#endif




