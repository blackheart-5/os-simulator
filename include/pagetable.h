#ifndef PAGETABLE_H
#define PAGETABLE_H

#include "types.h"
//simulating the RISC-V SV39 paging scheme.

/*
 * SV39-style 3-level page table simulator
 * (mirrors Chapter 4 / Project 2 of your course)
 *
 * Virtual address layout (39-bit):
 *   [38:30]  VPN[2]  – level-2 index  (9 bits)
 *   [29:21]  VPN[1]  – level-1 index  (9 bits)
 *   [20:12]  VPN[0]  – level-0 index  (9 bits)
 *   [11:0]   offset                   (12 bits)
 *
 * Physical address (56-bit in real SV39; we use 32-bit here for
 * simplicity but keep the same structural logic).
 */



#define PAGE_SIZE 4096u //4kB page table size
#define PHYS_MEM_PAGES 256 // 256 pg = 1MiB
//pte flags or permission bits
#define PTE_V (1u << 0) //valid
#define PTE_R (1u << 1) //readable
#define PTE_W (1u << 2) //writable, use bitwise and to check if writable
#define PTE_X (1u << 3) // executable
#define PTE_U (1u << 4) //user accessible


typedef u32 pte_t; //page table

//frame = 4kb block of physical memory 
//page = 4kb block of virtual memory 
//page is assigned to this frame
typedef struct{
    int used[PHYS_MEM_PAGES]; // 1= allocated, 0 = free
    int total_frames;
    int free_frames;
}PhysMemPool;

/* One process address space */
#define VPN_BITS   9
#define VPN_LEVELS 3
#define PTE_COUNT  (1 << VPN_BITS)   /* 512 entries per table  */

typedef struct PageTable {
    pte_t entries[PTE_COUNT];        /* each entry: PPN | flags */
} PageTable;

//address space of the process
typedef struct{
    PageTable *root;
    PhysMemPool *pool;
    int owner_pid;
}AddrSpace;

///handle the physical hardware
/**
 * set pool when simulator starts
 * total frames set to 256
 * free frames set to 256
 * fill the used[] with zeros --> meaning every frame slot is empty
 */
void pt_pool_init(PhysMemPool *pool);
/**
 * for every zero slot frame, assign 1
 * substruct 1 from free frames and return index(frame number)
 * else return -1
 */
int pt_pool_alloc(PhysMemPool *pool);
void pt_pool_free(PhysMemPool *pool, int frame);
void pt_pool_print(const PhysMemPool *pool);

///handle the virtual page tables/addresses
void pt_init(AddrSpace *as, PhysMemPool *pool, int pid);
void pt_free(AddrSpace *as);

/*
 * Map virtual page vpn -> physical frame ppn with given flags.
 * Allocates intermediate page-table pages as needed (like walk() in vm.c).
 */
int pt_map(AddrSpace *as, u32 vpn, u32 ppn, u32 flags);
/*
 * Translate a virtual address to a physical address.
 * Returns -1 on fault (unmapped / permission violation).
 */
i64  pt_translate(AddrSpace *as, u32 va, u32 access_flags);

/* Pretty-print the page table walk for a given virtual address */
void pt_walk_print(AddrSpace *as, u32 va);

int pt_mmap(AddrSpace *as, u32 va_start, u32 len, u32 flags);
int pt_munmap(AddrSpace *as, u32 len, u32 va_start);



#endif