#include "scheduler.h"
#include <stdio.h>
#include <string.h>

static int next_pid(Scheduler *s) {
    int max = 0;
    for (int i = 0; i < s->proc_count; i++)
        if (s->procs[i].pid > max) max = s->procs[i].pid;
    return max + 1;
}

void sched_init(Scheduler *s, SchedAlgo algo, int time_slice) {
    memset(s, 0, sizeof(*s));
    s->algo = algo;
    s->time_slice = time_slice > 0 ? time_slice : 3;
    s->current_pid = -1;
}

int sched_add_process(Scheduler *s, const char *name, int burst, int arrival, int priority) {
    if (s->proc_count >= MAX_PROCS) return -1;
    PCB *p = &s->procs[s->proc_count++];
    memset(p, 0, sizeof(*p));
    p->pid = next_pid(s);
    strncpy(p->name, name, MAX_NAME - 1);
    p->total_burst = burst;
    p->remaining_burst = burst;
    p->arrival_time = arrival;
    p->state = USED;
    p->priority = (priority >= 2) ? priority : 16;
    p->pass = BIG_STRIDE / p->priority;
    return p->pid;
}

static PCB *rr_pick(Scheduler *s) {
    int start = 0;
    if (s->current_pid >= 0) {
        for (int i = 0; i < s->proc_count; i++) {
            if (s->procs[i].pid == s->current_pid) {
                start = (i + 1) % s->proc_count; break;
            }
        }
    }
    for (int i = 0; i < s->proc_count; i++) {
        PCB *p = &s->procs[(start + i) % s->proc_count];
        if (p->state == RUNNABLE) return p;
    }
    return NULL;
}

static PCB *stride_pick(Scheduler *s) {
    PCB *best = NULL;
    for (int i = 0; i < s->proc_count; i++) {
        PCB *p = &s->procs[i];
        if (p->state != RUNNABLE) continue;
        if (!best || p->stride < best->stride) best = p;
    }
    return best;
}

#define MAX_GANTT 512
typedef struct { int pid; int start; int end; char name[MAX_NAME]; } GanttSlot;
static GanttSlot gantt[MAX_GANTT];
static int gantt_len = 0;

static void gantt_push(int pid, const char *name, int start, int end) {
    if (gantt_len > 0 && gantt[gantt_len-1].pid == pid) {
        gantt[gantt_len-1].end = end; return;
    }
    if (gantt_len >= MAX_GANTT) return;
    gantt[gantt_len].pid = pid;
    gantt[gantt_len].start = start;
    gantt[gantt_len].end = end;
    snprintf(gantt[gantt_len].name, MAX_NAME, "%s", name);
    // gantt[gantt_len].name[MAX_NAME-1] = '\0';
    gantt_len++;
}

void sched_run(Scheduler *s) {
    gantt_len = 0;
    printf("\n%s+-- Scheduler Running --%s\n", CYAN, RESET);
    const char *aname = (s->algo == SCHD_STRIDE) ? "Stride" : "Round-Robin";
    printf("  Algorithm: %s%s%s   Time-slice: %d\n\n", BOLD, aname, RESET, s->time_slice);

    int remaining = s->proc_count;
    while (remaining > 0) {
        for (int i = 0; i < s->proc_count; i++) {
            PCB *p = &s->procs[i];
            if (p->state == USED && p->arrival_time <= s->clock) {
                p->state = RUNNABLE;
                printf("  %s[t=%3d]%s  %-10s arrived\n", GRAY, s->clock, RESET, p->name);
            }
        }
        PCB *cur = (s->algo == SCHD_STRIDE) ? stride_pick(s) : rr_pick(s);
        if (!cur) { s->clock++; continue; }

        cur->state = RUNNABLE;
        s->current_pid = cur->pid;
        int ticks = (cur->remaining_burst < s->time_slice) ? cur->remaining_burst : s->time_slice;
        int t0 = s->clock;
        printf("  %s[t=%3d]%s  %-10s running  prio=%-3d stride=%-6llu\n",
               GREEN, s->clock, RESET, cur->name, cur->priority, (unsigned long long)cur->stride);

        s->clock += ticks;
        cur->time_used += ticks;
        cur->remaining_burst -= ticks;
        cur->stride += cur->pass;
        gantt_push(cur->pid, cur->name, t0, s->clock);

        /* accumulate wait for other runnable procs */
        for (int i = 0; i < s->proc_count; i++) {
            if (s->procs[i].state == RUNNABLE)
                s->procs[i].wait_time += ticks;
        }

        if (cur->remaining_burst <= 0) {
            cur->state = ZOMBIE;
            cur->finish_time = s->clock;
            remaining--;
            printf("  %s[t=%3d]%s  %-10s DONE     turnaround=%d wait=%d\n",
                   YELLOW, s->clock, RESET, cur->name,
                   cur->finish_time - cur->arrival_time, cur->wait_time);
        } else {
            cur->state = RUNNABLE;
        }
    }
    printf("\n  %s[t=%3d]%s  All processes finished.\n\n", CYAN, s->clock, RESET);
}

void sched_print_gantt(Scheduler *s) {
    (void)s;
    printf("%s+-- Gantt Chart ", BLUE);
    for (int i = 0; i < 40; i++) printf("-");
    printf("+%s\n", RESET);
    printf("| ");
    for (int i = 0; i < gantt_len; i++) {
        int w = (gantt[i].end - gantt[i].start) * 2;
        printf("%s%-*s%s|", BOLD, w > 0 ? w : 1, gantt[i].name, RESET);
    }
    printf("\n| 0");
    for (int i = 0; i < gantt_len; i++) {
        int w = (gantt[i].end - gantt[i].start) * 2;
        printf("%*d", w > 0 ? w : 1, gantt[i].end);
    }
    printf("\n%s+", BLUE);
    for (int i = 0; i < 55; i++) printf("-");
    printf("+%s\n\n", RESET);
}

void sched_print_stats(Scheduler *s) {
    printf("%s+-- Process Statistics ", MAGENTA);
    for (int i = 0; i < 36; i++) printf("-");
    printf("+%s\n", RESET);
    printf("| %-10s %7s %8s %9s %8s %7s |\n",
           "Name","Burst","Priority","Arrive","Finish","Wait");
    printf("| %-10s %7s %8s %9s %8s %7s |\n",
           "----------","-------","--------","---------","--------","-------");
    double aw = 0, at = 0; int n = 0;
    for (int i = 0; i < s->proc_count; i++) {
        PCB *p = &s->procs[i];
        if (p->state == ZOMBIE) {
            int turn = p->finish_time - p->arrival_time;
            aw += p->wait_time; at += turn; n++;
            printf("| %-10s %7d %8d %9d %8d %7d |\n",
                   p->name, p->total_burst, p->priority,
                   p->arrival_time, p->finish_time, p->wait_time);
        }
    }
    if (n > 0)
        printf("| %-10s %7s %8s %9s %8.1f %7.1f |\n",
               "AVERAGE","","","", at/n, aw/n);
    printf("%s+", MAGENTA);
    for (int i = 0; i < 58; i++) printf("-");
    printf("+%s\n\n", RESET);
}