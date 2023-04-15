#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE 4096

// Maximum size of virtual memory
#define MAX_MEMSIZE 4ULL*1024*1024*1024

// Size of "physcial memory"
#define MEMSIZE 1024*1024*1024

// Represents a page table entry
typedef unsigned long pte_t;

// Represents a page directory entry
typedef unsigned long pde_t;

#define TLB_ENTRIES 512

//Structure to represents TLB
typedef struct tlb {
    /*Assume your TLB is a direct mapped TLB with number of entries as TLB_ENTRIES
    * Think about the size of each TLB entry that performs virtual to physical
    * address translation.
    */
   void *va;
   void *pa;
   struct tlb *next;

}tlb;
struct tlb tlb_store;


void set_physical_mem();
pte_t* translate(pde_t *pgdir, void *va);
int page_map(pde_t *pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *t_malloc(unsigned int num_bytes);
void t_free(void *va, int size);
int put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);
void print_TLB_missrate();

void set_bit(unsigned char* bitmap, unsigned long index, unsigned int value);
int get_bit(unsigned char* bitmap, unsigned long index);
void init_bit_values();
void print_bit_values();
unsigned int num_bits_in_value(unsigned int value);
void init_page_tables();
unsigned long next_free_page(unsigned char* bitmap);
void* get_physical_addr_from_bit(unsigned long pageNumInBitmap);
void *get_next_avail_physical(int num_pages);
tlb* search_tlb_list(void *va);
void remove_and_add(tlb* node);
void remove_end();


#endif
