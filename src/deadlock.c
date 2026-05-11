#include "deadlock.h"
#include <stdio.h>
#include <string.h>

void dl_init(DeadlockState *dl, int n_threads, int n_resources){
    memset(dl, 0, sizeof(*dl)); //clear the entire deadlock structures to zero
    dl->n_threads = n_threads;
    dl->n_resources = n_resources;
    dl->enabled = 1;

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

void dl_set_allocation(Deadlock *dl, int thread, int res, int count){
    //check thread is valid --> row
    if (t >= 0 && t < dl->n_threads){
        //check resource is valid --> column
        if (res >= 0 && res < dl->n_resources){
            dl->allocation[thread][res] = count; //allocate units of this resource this threads needs
        }
    }
}

void dl_set_request(DeadlockState *dl, int thread, int res, int count){
    //check thread is valid --> row
    if (t >= 0 && t < dl->n_threads){
        //check resource is valid --> column
        if (res >= 0 && res < dl->n_resources){
            dl->request[thread][res] = count; //set units of this resource this threads needs
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

        for (int i = 0; i < n_threads; i++){
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
}
