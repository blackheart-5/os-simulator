#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "scheduler.h"
#include "pagetable.h"
#include "deadlock.h"
#include "pipe.h"

static void print_banner(void) {
    printf("\n");
    printf("%s%s", CYAN, BOLD);
    printf("  +-------------------------------------------------+\n");
    printf("  |          OS CONCEPTS SIMULATOR  v1.0           |\n");
    printf("  |  Processes | Virtual Memory | IPC | Deadlock   |\n");
    printf("  +-------------------------------------------------+\n");
    printf("%s", RESET);
    printf("%s  Built on concepts from CSE 410 (uCore RISC-V OS)%s\n\n", GRAY, RESET);
}

static void separator(void) {
    printf("%s  ─────────────────────────────────────────────────%s\n", GRAY, RESET);
}

static int read_int(const char *prompt, int min, int max) {
    int v;
    do {
        printf("%s", prompt);
        if (scanf("%d", &v) != 1) { v = min - 1; while (getchar() != '\n'); }
    } while (v < min || v > max);
    return v;
}

static void flush_stdin(void) { int c; while ((c = getchar()) != '\n' && c != EOF); }

/* ---- Module 1: Scheduler ---- */
static void run_scheduler_demo(void) {
    printf("\n%s=== MODULE 1: Process Scheduler ===%s\n", BOLD, RESET);
    printf("  Simulates Round-Robin and Stride (priority) scheduling\n");
    printf("  as implemented in Chapter 3 / Project 3.\n\n");

    printf("  Select algorithm:\n");
    printf("    1) Round-Robin\n");
    printf("    2) Stride (priority-based)\n\n");

    int algo_choice = read_int("  Choice [1-2]: ", 1, 2);
    SchedAlgo algo  = (algo_choice == 2) ? SCHD_STRIDE : SCHED_RR;
    int ts = read_int("  Time slice size (1-10 ticks): ", 1, 10);

    Scheduler s;
    sched_init(&s, algo, ts);

    int np = read_int("  How many processes? (2-6): ", 2, 6);
    flush_stdin();

    for (int i = 0; i < np; i++) {
        char name[MAX_NAME];
        printf("\n  Process %d name: ", i+1);
        if (scanf("%31s", name) != 1) snprintf(name, MAX_NAME, "P%d", i+1);
        flush_stdin();
        int burst   = read_int("  CPU burst (1-20): ", 1, 20);
        int arrival = read_int("  Arrival time  (0-10): ", 0, 10);
        int priority = (algo == SCHD_STRIDE)
                           ? read_int("  Priority (2-32): ", 2, 32) : 16;
        sched_add_process(&s, name, burst, arrival, priority);
    }

    sched_run(&s);
    sched_print_gantt(&s);
    sched_print_stats(&s);
}

/* ---- Module 2: Virtual Memory ---- */
static void run_vm_demo(void) {
    printf("\n%s=== MODULE 2: Virtual Memory & Page Tables ===%s\n", BOLD, RESET);
    printf("  SV39 3-level page table walks, mmap/munmap,\n");
    printf("  physical frame allocation. Mirrors Chapter 4 / Project 2.\n\n");

    static PhysMemPool pool;
    static AddrSpace   as;
    static int initialized = 0;
    if (!initialized) {
        pt_pool_init(&pool);
        pt_init(&as, &pool, 1);
        initialized = 1;
    }

    int running = 1;
    while (running) {
        printf("\n%s  [VM Menu]%s\n", CYAN, RESET);
        printf("    1) mmap  (map virtual pages)\n");
        printf("    2) munmap (unmap virtual pages)\n");
        printf("    3) Translate a virtual address\n");
        printf("    4) Show physical memory map\n");
        printf("    5) Quick demo (code/data/stack)\n");
        printf("    0) Back\n\n");

        int c = read_int("  Choice: ", 0, 5);
        if (c == 0) { running = 0; break; }

        if (c == 1) {
            u32 va;
            printf("  VA start (hex, page-aligned, e.g. 0x1000): ");
            if (scanf("%i", &va) != 1) va = 0x1000;
            flush_stdin();
            int len = read_int("  Length in bytes (1-65536): ", 1, 65536);
            int r = read_int("  Readable?   [1/0]: ", 0, 1);
            int w = read_int("  Writable?   [1/0]: ", 0, 1);
            int x = read_int("  Executable? [1/0]: ", 0, 1);
            u32 flags = (r?PTE_R:0)|(w?PTE_W:0)|(x?PTE_X:0);
            pt_mmap(&as, va, (u32)len, flags);

        } else if (c == 2) {
            u32 va;
            printf("  VA start (hex): ");
            if (scanf("%i", &va) != 1) va = 0;
            flush_stdin();
            int len = read_int("  Length in bytes: ", 1, 65536);
            pt_munmap(&as, va, (u32)len);

        } else if (c == 3) {
            u32 va;
            printf("  Virtual address (hex): ");
            if (scanf("%i", &va) != 1) va = 0;
            flush_stdin();
            pt_walk_print(&as, va);

        } else if (c == 4) {
            pt_pool_print(&pool);

        } else if (c == 5) {
            printf("\n%s  --- Mapping code/data/stack segments ---%s\n\n", BOLD, RESET);
            pt_mmap(&as, 0x1000, PAGE_SIZE,     PTE_R | PTE_X);
            pt_mmap(&as, 0x2000, PAGE_SIZE * 2, PTE_R | PTE_W);
            pt_mmap(&as, 0x5000, PAGE_SIZE,     PTE_R | PTE_W);
            printf("\n  Walk VA 0x1000 (code):\n");
            pt_walk_print(&as, 0x1000);
            printf("  Walk VA 0x2000 (data):\n");
            pt_walk_print(&as, 0x2000);
            printf("  Walk VA 0x9999 (unmapped -> page fault):\n");
            pt_walk_print(&as, 0x9999);
            pt_pool_print(&pool);
        }
    }
}

/* ---- Module 3: Deadlock ---- */
static void run_deadlock_demo(void) {
    printf("\n%s=== MODULE 3: Deadlock Detection ===%s\n", BOLD, RESET);
    printf("  Implements the safety/banker's algorithm from\n");
    printf("  Chapter 8 / Project 5.\n\n");

    printf("  Choose scenario:\n");
    printf("    1) Safe state (no deadlock — classic example)\n");
    printf("    2) Deadlock   (circular wait T0 <-> T1)\n");
    printf("    3) Custom input\n\n");

    int choice = read_int("  Choice [1-3]: ", 1, 3);
    DeadlockState dl;
    int safe_seq[DL_MAX_THREADS] = {0};

    if (choice == 1) {
        dl_demo_no_deadlock(&dl);
    } else if (choice == 2) {
        dl_demo_deadlock(&dl);
    } else {
        int nt = read_int("  Number of threads   (1-8): ", 1, 8);
        int nr = read_int("  Number of resources (1-8): ", 1, 8);
        dl_init(&dl, nt, nr);
        printf("\n  Available resources:\n");
        for (int j = 0; j < nr; j++) {
            printf("  R%d available: ", j);
            int v = read_int("", 0, 100);
            dl_set_available(&dl, j, v);
        }
        printf("\n  Allocation (what each thread currently holds):\n");
        for (int i = 0; i < nt; i++)
            for (int j = 0; j < nr; j++) {
                printf("  T%d holds R%d: ", i, j);
                dl_set_allocation(&dl, i, j, read_int("", 0, 100));
            }
        printf("\n  Request (what each thread still needs):\n");
        for (int i = 0; i < nt; i++)
            for (int j = 0; j < nr; j++) {
                printf("  T%d needs R%d: ", i, j);
                dl_set_request(&dl, i, j, read_int("", 0, 100));
            }
    }

    dl_print_state(&dl);
    int deadlocked = dl_detect(&dl, safe_seq);
    dl_print_result(deadlocked, safe_seq, dl.n_threads);
}

/* ---- Module 4: Pipes ---- */
static void run_pipe_demo(void) {
    printf("\n%s=== MODULE 4: Pipes & Inter-Process Communication ===%s\n", BOLD, RESET);
    printf("  Simulates the ring-buffer pipe from Chapter 7.\n\n");

    printf("  Choose:\n");
    printf("    1) Automatic demo (producer writes, consumer reads)\n");
    printf("    2) Interactive\n\n");

    static PipePool pool;
    static int pool_init = 0;
    if (!pool_init) { pipe_pool_init(&pool); pool_init = 1; }

    int c = read_int("  Choice [1-2]: ", 1, 2);
    if (c == 1) {
        pipe_demo(&pool);
    } else {
        int fds[2];
        pipe_open(&pool, fds);
        flush_stdin();
        int go = 1;
        while (go) {
            printf("\n  1) Write  2) Read  3) Close write  0) Done\n");
            int opt = read_int("  ", 0, 3);
            if (opt == 0) { go = 0; }
            else if (opt == 1) {
                char msg[128];
                printf("  Message: ");
                if (fgets(msg, sizeof(msg), stdin)) {
                    msg[strcspn(msg, "\n")] = 0;
                    pipe_write(&pool, fds[1], msg, (int)strlen(msg));
                }
            } else if (opt == 2) {
                char buf[128];
                pipe_read(&pool, fds[0], buf, 127);
            } else {
                pipe_close(&pool, fds[1]);
            }
        }
        pipe_print_status(&pool);
    }
}

/* ---- Main menu ---- */
int main(void) {
    print_banner();
    int running = 1;
    while (running) {
        separator();
        printf("%s  Main Menu%s\n", BOLD, RESET);
        separator();
        printf("    1)  Process Scheduler    (Round-Robin & Stride)\n");
        printf("    2)  Virtual Memory       (SV39 page tables, mmap/munmap)\n");
        printf("    3)  Deadlock Detection   (Banker's algorithm)\n");
        printf("    4)  Pipes & IPC          (Ring-buffer pipes)\n");
        printf("    0)  Exit\n\n");
        int choice = read_int("  Select [0-4]: ", 0, 4);
        switch (choice) {
            case 1: run_scheduler_demo(); break;
            case 2: run_vm_demo();        break;
            case 3: run_deadlock_demo();  break;
            case 4: run_pipe_demo();      break;
            case 0: running = 0;          break;
        }
    }
    printf("\n%s  Goodbye!%s\n\n", CYAN, RESET);
    return 0;
}