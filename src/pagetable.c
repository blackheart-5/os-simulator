#include "pagetable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//initialize pool of physical ram -- frames
void pt_pool_init(PhysMemPool *pool){
    memset(pool, 0, sizeof(*pool)); //wipe e verything before starting
    pool->total_frames = PHYS_MEM_PAGES;
    pool->free_frames = PHYS_MEM_PAGES;
}


void pt_pool_alloc(PhysMemPool *pool){
    for (int i=0; i<pool->total_frames; i++){
        if (pool->used[i] == 0){
            pool->used[i] = 1;
            pool->free_frames--;
            return i;
        }
    }
    return -1;
}

void pt_pool_free(PhysMemPool *pool, int thread, int frame){
    if(frame>=0 && frame < pool->total_frames && pool->used[frame]){
        pool->used[frame] = 0;
        pool->free_frames++;
    }
}

void pt_pool_print(const PhysMemPool *pool) {
    printf("\n%s+-- Physical Memory (%d/%d frames free) -+%s\n",
           CYAN, pool->free_frames, pool->total_frames, RESET);
    printf("| ");
    for (int i = 0; i < pool->total_frames; i++) {
        if (i > 0 && i % 32 == 0) printf("\n| ");
        printf("%s%s%s", pool->used[i]?RED:GREEN, pool->used[i]?"#":".", RESET);
    }
    printf("\n| # = allocated   . = free\n");
    printf("%s+----------------------------------------+%s\n\n", CYAN, RESET);
}

void pt_init(AddrSpace *as,PhysMemPool *pool, int pid){
    as->owner_pid = pid;
    as->pool = pool;
    as->root = calloc(1,sizeof(PageTable));
}

static pte_t(AddrSpace *as, int alloc, u32 vpn){
    PageTable *table = as->root;
    for(int level=2;level>0;level--){
        int idx = ((vpn >> (level*VPN_BITS))) & (PTE_COUNT-1)
        pte_t *pte = &table->entries[idx];//grab a pointer to that specific entry.
        if(*pte && PTE_V){
            //shift bits to get the physical page number and <<12 to covert that number to full address
            //2^12 =4096bytes
            table =  (PageTable *)(uintptr_t)((*pte >> 10) << 12); //table value now points to next-level
        }
        else{
            if(!alloc){ //allocation not allowed 
                return NULL;
            }
            PageTable *nt = calloc(1, sizeof(PageTable));//pointer to newly create page table
            if (!nt) return NULL;
            *pte = (pte_t)(((uintptr_t)nt >> 12) >> 10) | PTE_V;
            table = nt;
        }
    }
    return &table->entries[vpn & (PTE_COUNT-1)];//return pointer to final pte for the vpn
}

int pt_map(AddrSpace *as, u32 vpn, u32 ppn, u32 flags){
    pte_t *pte = walk(as, vpn, 1); // val here is gotten from static pte_t
    if (!pte) return -1;
    *pte = (pte_t)((ppn << 10) | flags | PTE_V);
    return 0;
}






void pt_walk_print(AddrSpace *as, u32 va) {
    u32 vpn = va>>12, offset = va&0xFFF;
    u32 vpn2=(vpn>>18)&0x1FF, vpn1=(vpn>>9)&0x1FF, vpn0=vpn&0x1FF;
    printf("\n%s+-- VA Walk: 0x%08X --%s\n", BLUE, va, RESET);
    printf("| VPN[2]=%u  VPN[1]=%u  VPN[0]=%u  offset=0x%03X\n",vpn2,vpn1,vpn0,offset);
    printf("+-----------------------------------------------------\n");
 
    PageTable *t = as->root;
    pte_t e2 = t->entries[vpn2];
    printf("| L2[%3u] = 0x%08X  V=%d R=%d W=%d X=%d\n",
           vpn2,e2,(e2>>0)&1,(e2>>1)&1,(e2>>2)&1,(e2>>3)&1);
    if (!(e2&PTE_V)) { printf("| %sPAGE FAULT at L2%s\n+---\n\n",RED,RESET); return; }
 
    PageTable *l1=(PageTable*)(uintptr_t)((e2>>10)<<12);
    pte_t e1=l1->entries[vpn1];
    printf("| L1[%3u] = 0x%08X  V=%d R=%d W=%d X=%d\n",
           vpn1,e1,(e1>>0)&1,(e1>>1)&1,(e1>>2)&1,(e1>>3)&1);
    if (!(e1&PTE_V)) { printf("| %sPAGE FAULT at L1%s\n+---\n\n",RED,RESET); return; }
 
    PageTable *l0=(PageTable*)(uintptr_t)((e1>>10)<<12);
    pte_t e0=l0->entries[vpn0];
    printf("| L0[%3u] = 0x%08X  V=%d R=%d W=%d X=%d\n",
           vpn0,e0,(e0>>0)&1,(e0>>1)&1,(e0>>2)&1,(e0>>3)&1);
    if (!(e0&PTE_V)) { printf("| %sPAGE FAULT at L0%s\n+---\n\n",RED,RESET); return; }
 
    u32 ppn=(e0>>10)&0xFFFFF;
    printf("| %sTranslation OK%s  PPN=%u  PA=0x%08X\n",
           GREEN,RESET,ppn,(ppn<<12)|offset);
    printf("%s+-----------------------------------------------------+%s\n\n",BLUE,RESET);
}

i64 pt_translate(AddrSpace *as, u32 va, u32 access_flags) {
    u32 vpn = va >> 12, offset = va & 0xFFF;
    pte_t *pte = walk(as, vpn, 0);
    if (!pte || !(*pte & PTE_V)) return -1;
    if (access_flags & ~(*pte & 0xF)) return -2;
    u32 ppn = (*pte >> 10) & 0xFFFFF;
    return (i64)((ppn << 12) | offset);
}


void pt_free(AddrSpace *as){
    free(as->root);
    as->root = NUL;
}

int pt_mmap(AddrSpace *as, u32 va_start, u32 len, u32 flags) {
    if (va_start % PAGE_SIZE != 0) {
        printf("%s  [mmap] Error: not page-aligned%s\n",RED,RESET); return -1;
    }
    if (!(flags & (PTE_R|PTE_W|PTE_X))) {
        printf("%s  [mmap] Error: no R/W/X bits%s\n",RED,RESET); return -1;
    }
    u32 pages=(len+PAGE_SIZE-1)/PAGE_SIZE, vpn=va_start>>12;
    for (u32 i=0; i<pages; i++) {
        pte_t *ex=walk(as,vpn+i,0);
        if (ex && (*ex&PTE_V)) {
            printf("%s  [mmap] Error: VA 0x%08X already mapped%s\n",RED,(vpn+i)<<12,RESET);
            return -1;
        }
        int frame=pt_pool_alloc(as->pool);
        if (frame<0) { printf("%s  [mmap] Out of physical memory%s\n",RED,RESET); return -1; }
        pt_map(as,vpn+i,(u32)frame,flags|PTE_U);
        printf("%s  [mmap]%s VA=0x%08X -> PPN=%d  [%s%s%s]\n",
               GREEN,RESET,(vpn+i)<<12,frame,
               (flags&PTE_R)?"R":"-",(flags&PTE_W)?"W":"-",(flags&PTE_X)?"X":"-");
    }
    return 0;
}
 
int pt_munmap(AddrSpace *as, u32 va_start, u32 len) {
    u32 pages=(len+PAGE_SIZE-1)/PAGE_SIZE, vpn=va_start>>12;
    for (u32 i=0; i<pages; i++) {
        pte_t *pte=walk(as,vpn+i,0);
        if (!pte || !(*pte&PTE_V)) {
            printf("%s  [munmap] VA 0x%08X not mapped%s\n",RED,(vpn+i)<<12,RESET);
            return -1;
        }
        u32 ppn=(*pte>>10)&0xFFFFF;
        pt_pool_free(as->pool,ppn); *pte=0;
        printf("%s  [munmap]%s VA=0x%08X (PPN %d freed)\n",YELLOW,RESET,(vpn+i)<<12,ppn);
    }
    return 0;
}