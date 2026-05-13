#include "deadlock.h"
#include <stdio.h>
#include <string.h>

void dl_init(DeadlockState *dl, int n_threads, int n_resources){
    memset(dl, 0, sizeof(*dl)); //clear the entire deadlock structures to zero
    dl->n_threads = n_threads;
    dl->n_resources = n_resources;
    dl->enables = 1;

    //print thread name
    for (int i = 0; i<n_threads; i++){
        snprintf(dl->thread_name[i], MAX_NAME, "T%d", i);
    }
    //print resources
    for (int j = 0; j < n_resources; j++){
        snprintf(dl->res_name[j], MAX_NAME, "R%d", j);
    }
}

void dl_set_available(DeadlockState *dl, int res, int count){
    if (res >= 0 && res < dl->n_resources){
        dl->available[res] = count; //how many unit available for this resource
    }
}

void dl_set_allocation(DeadlockState *dl, int thread, int res, int count){
    //check thread is valid --> row
    if (thread >= 0 && thread < dl->n_threads){
        //check resource is valid --> column
        if (res >= 0 && res < dl->n_resources){
            dl->allocation[thread][res] = count; //allocate units of this resource this threads needs
        }
    }
}

void dl_set_request(DeadlockState *dl, int thread, int res, int count){
    //check thread is valid --> row
    if (thread >= 0 && thread < dl->n_threads){
        //check resource is valid --> column
        if (res >= 0 && res < dl->n_resources){
            dl->requests[thread][res] = count; //set units of this resource this threads needs
        }
    }
}

//bankers algo
//is there any possible order 
//these threads can finish in without getting stuck
int dl_detect(DeadlockState *dl, int safe_seq[DL_MAX_THREADS]){
    int work[DL_MAX_RESOURCES];
    int finish[DL_MAX_THREADS];
    int finished_thread = 0;
    int n = dl->n_threads;
    int m = dl->n_resources;

    //copy available into work u can use this----> memcpy(work, dl->available, sizeof(int)*m);
    for (int i = 0; i<dl->n_resources; i++){
        work[i] = dl->available[i];
    }

    //set finish thread to 0 for all slots (no one finshed) ---> memset(finish, 0, sizeof(int)*n);
    for (int i = 0; i < n; i++){
        finish[i] = 0;
    }

    int seq_len = 0, progress = 1;
    while (progress){
        progress = 0;

        for (int i = 0; i < dl->n_threads; i++){
            if (finish[i]) continue; //thread is finished so skip

            int ok = 1;
            //can threads request be satisfied with the resource j
            for (int j = 0; j < m; j++){
                if (dl->requests[i][j] > work[j]){
                    ok = 0;
                    break;
                }
            }
            //if all requests can be satisfied 
            //now release the resorces
            if (ok){
                for (int j = 0; j < m; j++){
                    work[j] += dl->allocation[i][j];
                }
                finish[i] = 1; //thread finished
                safe_seq[seq_len++] = 1; //add threads id to safe seq
                progress = 1; // control loop for checking other can run
            }

        }
    }
    //check that all threads finished
    for (int i = 0; i<n;i++){
        if (!finish[i]) return 1;
    }
    return 0;
}

void dl_print_state(const DeadlockState *dl) {
    int n=dl->n_threads, m=dl->n_resources;
    printf("\n%s+-- Resource Allocation State --%s\n",MAGENTA,RESET);
    printf("| Available: ");
    for (int j=0;j<m;j++) printf("%s=%d  ",dl->res_name[j],dl->available[j]);
    printf("\n+--\n");
    printf("| %-8s","Thread");
    for (int j=0;j<m;j++) printf("  Alloc[%s]",dl->res_name[j]);
    printf("  |");
    for (int j=0;j<m;j++) printf("  Need[%s]",dl->res_name[j]);
    printf("\n");
    for (int i=0;i<n;i++) {
        printf("| %-8s",dl->thread_name[i]);
        for (int j=0;j<m;j++) printf("  %9d",dl->allocation[i][j]);
        printf("  |");
        for (int j=0;j<m;j++) printf("  %8d",dl->requests[i][j]);
        printf("\n");
    }
    printf("%s+----------------------------------------------+%s\n\n",MAGENTA,RESET);
}



void dl_print_result(int deadlocked, int safe_seq[DL_MAX_THREADS], int n) {
    if (deadlocked) {
        printf("%s+-- DEADLOCK DETECTED ---------%s\n",RED,RESET);
        printf("| System is in an UNSAFE state.\n");
        printf("| One or more threads are permanently blocked.\n");
        printf("%s+------------------------------%s\n\n",RED,RESET);
    } else {
        printf("%s+-- SAFE STATE ---------------%s\n",GREEN,RESET);
        printf("| Safe sequence: ");
        for (int i=0;i<n;i++) { printf("T%d",safe_seq[i]); if(i<n-1) printf(" -> "); }
        printf("\n%s+-----------------------------%s\n\n",GREEN,RESET);
    }
}


/*
test cases  to run
  */
void dl_demo_no_deadlock(DeadlockState *dl) {
    /*
     * 3 threads, 2 resource types.
     * Available: R0=2  R1=1
     *
     *       Allocation    Need/Request
     * T0:   R0=1  R1=0   R0=0  R1=1     <- T0 can run (0<=2, 1<=1) -> Work=3,1
     * T1:   R0=0  R1=1   R0=1  R1=0     <- T1 can run (1<=3, 0<=1) -> Work=3,2
     * T2:   R0=1  R1=0   R0=0  R1=2     <- T2 can run (0<=3, 2<=2) -> done
     * Safe sequence: T0 -> T1 -> T2
     */
    dl_init(dl,3,2);
    strncpy(dl->res_name[0],"R0",MAX_NAME);
    strncpy(dl->res_name[1],"R1",MAX_NAME);
    dl_set_available(dl,0,2); dl_set_available(dl,1,1);
    dl_set_allocation(dl,0,0,1); dl_set_allocation(dl,0,1,0);
    dl_set_allocation(dl,1,0,0); dl_set_allocation(dl,1,1,1);
    dl_set_allocation(dl,2,0,1); dl_set_allocation(dl,2,1,0);
    dl_set_request(dl,0,0,0); dl_set_request(dl,0,1,1);
    dl_set_request(dl,1,0,1); dl_set_request(dl,1,1,0);
    dl_set_request(dl,2,0,0); dl_set_request(dl,2,1,2);
}
 
void dl_demo_deadlock(DeadlockState *dl) {
    dl_init(dl,2,2);
    strncpy(dl->res_name[0],"Mutex0",MAX_NAME);
    strncpy(dl->res_name[1],"Mutex1",MAX_NAME);
    dl_set_available(dl,0,0); dl_set_available(dl,1,0); //set resource units r0=0,r1=0
    dl_set_allocation(dl,0,0,1); dl_set_allocation(dl,0,1,0); //t0 resource unit allocations
    dl_set_request(dl,0,0,0);   dl_set_request(dl,0,1,1);//t0 unit requested unit 1 from r1
    dl_set_allocation(dl,1,0,0); dl_set_allocation(dl,1,1,1);
    dl_set_request(dl,1,0,1);   dl_set_request(dl,1,1,0);
}