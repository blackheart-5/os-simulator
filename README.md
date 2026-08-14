# OS Concepts Simulator

<img width="1905" height="1097" alt="image" src="https://github.com/user-attachments/assets/8bc588e5-e59e-4c71-bf80-08ead8e4c328" />

<img width="1401" height="1048" alt="image" src="https://github.com/user-attachments/assets/7cf8dd90-b363-4b6c-b447-372ae84bd8d4" />

A terminal-based operating system concepts simulator written in C, demonstrating core OS mechanisms from process scheduling to virtual memory management. Built as a portfolio project to showcase systems programming knowledge.

## Table of Contents

- [What It Demonstrates](#what-it-demonstrates)
- [Quick Start](#quick-start)
- [Module Details](#module-details)
  - [1. Process Scheduler](#1-process-scheduler)
  - [2. Virtual Memory & Page Tables](#2-virtual-memory--page-tables)
  - [3. Deadlock Detection](#3-deadlock-detection)
  - [4. Pipes & IPC](#4-pipes--ipc)
- [Project Structure](#project-structure)
- [Build Options](#build-options)
- [Key Design Decisions](#key-design-decisions)
- [Skills Demonstrated](#skills-demonstrated)
- [Background](#background)

---

## What It Demonstrates

| Module | Concept | Course Equivalent |
|---|---|---|
| Process Scheduler | Round-Robin & Stride scheduling, PCB, Gantt chart | Ch. 3 — Multiprogramming |
| Virtual Memory | SV39 3-level page table, mmap/munmap, frame pool | Ch. 4 — Address Spaces |
| Deadlock Detection | Banker's safety algorithm, resource allocation | Ch. 8 — Concurrency |
| Pipes & IPC | Ring-buffer pipes, file descriptors, producer/consumer | Ch. 7 — IPC |

---

## Quick Start

> Requires: `gcc`, `make` (Linux or macOS)

```bash
git clone <your-repo-url>
cd os-simulator
make
./os-sim
```

No external dependencies. Pure C11, ~900 lines across 5 source files.

---

## Module Details

### 1. Process Scheduler

Simulates two scheduling algorithms:

**Round-Robin** — each process gets a fixed time slice before being preempted. Implements the exact mechanism from RISC-V uCore: a process pool, PCB state machine (`USED → RUNNABLE → RUNNING → ZOMBIE`), and a scheduler loop.

**Stride Scheduling** — priority-based algorithm where each process accumulates a stride value. The scheduler always picks the process with the smallest stride, guaranteeing CPU time proportional to priority.

```
pass = BIG_STRIDE / priority
```

Output: tick-by-tick log, Gantt chart, turnaround/wait time statistics.

```
[t=  0]  P1   running  prio=16  stride=0
[t=  3]  P2   running  prio=16  stride=0
[t=  6]  P1   running  prio=16  stride=4096
[t=  8]  P1   DONE     turnaround=8  wait=3
```

---

### 2. Virtual Memory & Page Tables

Implements a 3-level page table modeled on RISC-V SV39:

```
Virtual address (39-bit):
  [38:30]  VPN[2]  level-2 index  (9 bits)
  [29:21]  VPN[1]  level-1 index  (9 bits)
  [20:12]  VPN[0]  level-0 index  (9 bits)
  [11:0]   offset                 (12 bits)
```

The `walk()` function mirrors `walk()` in the course's `vm.c`: it traverses three levels, allocating intermediate nodes on demand (like `kalloc()`).

**Features:**
- `mmap` — allocate physical frames and map them with R/W/X permissions
- `munmap` — unmap pages and return frames to the pool
- Full address translation trace showing each page-table level
- Page fault detection at any level

```
+-- VA Walk: 0x00001ABC --
| VPN[2]=0  VPN[1]=0  VPN[0]=1  offset=0xABC
| L2[0] = 0x...  V=1 R=0 W=0 X=0
| L1[0] = 0x...  V=1 R=0 W=0 X=0
| L0[1] = 0x...  V=1 R=1 W=1 X=0
| Translation OK  PPN=5  PA=0x000005ABC
```

---

### 3. Deadlock Detection

Implements the safety algorithm (Banker's algorithm) from the course:

```
Work = Available
Finish[0..n] = false

Find i where Finish[i]==false AND Request[i] <= Work
  -> Work += Allocation[i]; Finish[i] = true; repeat

If all Finish[i]==true: SAFE STATE, else: DEADLOCK
```

**Two built-in scenarios:**
- **Safe state** — 3 threads, 2 resources → safe sequence `T0 → T1 → T2`
- **Deadlock** — circular wait: T0 holds Mutex0 needs Mutex1, T1 holds Mutex1 needs Mutex0

Custom input mode lets you define your own thread/resource matrix.

---

### 4. Pipes & IPC

Ring-buffer pipe matching the course's `pipe.c` design:
- 512-byte circular buffer with `nread`/`nwrite` absolute counters
- Two file descriptors per pipe (read end / write end)
- Handles partial writes, closed-end detection, empty-pipe blocking semantics

```
[pipe]  Created pipe 0  (read_fd=0  write_fd=1)
[write] pipe0  20 bytes: "Hello from producer!"
[write] pipe0  26 bytes: "OS pipes use ring buffers."
[close] write end pipe0
[read]  pipe0  46 bytes: "Hello from producer!OS pipes..."
```

---

## Project Structure

```
os-simulator/
├── Makefile
├── README.md
├── include/
│   ├── types.h        # Shared typedefs, constants, ANSI color codes
│   ├── scheduler.h    # PCB struct, SchedAlgo enum, scheduler API
│   ├── pagetable.h    # AddrSpace, PageTable, PhysMemPool, PTE flags
│   ├── deadlock.h     # DeadlockState, safety algorithm API
│   └── pipe.h         # Pipe, PipePool, ring-buffer API
└── src/
    ├── main.c         # Interactive CLI menu
    ├── scheduler.c    # RR + Stride scheduling simulation
    ├── pagetable.c    # 3-level page table walk, mmap/munmap
    ├── deadlock.c     # Banker's algorithm
    └── pipe.c         # Ring-buffer pipe IPC
```

---

## Build Options

```bash
make          # build release binary
make clean    # remove binary
make run      # build and run immediately
```

Compiler flags: `gcc -Wall -Wextra -std=c11 -O2`. Zero warnings on clean build.

---

## Key Design Decisions

**Why C?**
The course OS (uCore) is written in C targeting RISC-V. Using C here means the data structures map directly to real kernel code — `PCB`, `walk()`, `pte_t`, and `PhysMemPool` all mirror actual kernel structures from the labs.

**Why simulate rather than run a real kernel?**
A real kernel requires hardware, bootloaders, and privilege-level transitions that cannot run in userspace. This simulator makes the logic fully observable: every scheduling decision, every page-table walk, every deadlock check is printed step by step.

**Why keep everything in one binary?**
Real OS components are tightly coupled. Running all modules from a single process makes the interactions clear and keeps the project self-contained — no Docker, no QEMU, no special toolchain required.

---

## Skills Demonstrated

- **C systems programming** — manual memory management, pointer arithmetic, bitfield manipulation for page table entries
- **Process scheduling** — PCB state machines, FIFO queues, stride/priority algorithms with provable fairness
- **Virtual memory** — multi-level page table construction and traversal, physical frame allocation pools
- **Concurrency & deadlock** — safety-state detection via Banker's algorithm, resource allocation graph reasoning
- **IPC** — ring-buffer design, producer/consumer semantics, file descriptor model
- **Software design** — clean separation of headers and implementations, modular C API design

---

## Background

Built after completing CSE 410 (Operating Systems) at Michigan State University, which used the [uCore-Tutorial](https://github.com/rcore-os/rCore-Tutorial-v3) RISC-V OS as a teaching platform. Each module in this simulator corresponds directly to a lab from that course.

The goal was to rebuild the core algorithms from scratch — without copying course code — to demonstrate genuine understanding of the underlying mechanisms.
