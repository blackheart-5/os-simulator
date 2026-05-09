#ifndef DEADLOCK_H
#define DEADLOCK_H

#define DL_MAX_THREADS MAX_THREADS
#define DL_MAX_RESOURCES MAX_MUTEXES

#include "types.h"


typedef struct{
    int n_threads;
    int n_resources;

    int available[DL_MAX_RESOURCES];
    int allocation[DL_MAX_THREADS][DL_MAX_RESOURCES];
    int requests[DL_MAX_THREADS][DL_MAX_RESOURCES];

    char thread_name[DL_MAX_THREADS][MAX_NAME];
    char res_name[DL_MAX_RESOURCES][MAX_NAME];
    int enables;  //detection on/off 
}DeadlockState;

void dl_init(DeadlockState *dl, int n_threads, int n_resources);
void dl_set_available(DeadlockState *dl, int res, int count);
void dl_set_allocation(DeadlockState *dl, int thread, int res, int count);
void dl_set_request(DeadlockState *dl, int thread, int res, int count);


/*
 * Run the safety algorithm.
 * Returns 0 = safe, 1 = deadlock detected.
 * Fills safe_seq[] with the safe execution order (if safe).
 */
int  dl_detect(DeadlockState *dl, int safe_seq[DL_MAX_THREADS]);
 
void dl_print_state(const DeadlockState *dl);
void dl_print_result(int deadlocked, int safe_seq[DL_MAX_THREADS], int n);
 
/* Interactive scenario builder */
void dl_demo_no_deadlock(DeadlockState *dl);
void dl_demo_deadlock(DeadlockState *dl);


#endif