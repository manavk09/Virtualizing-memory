#include "my_vm.h"

void* physical_mem;
unsigned char* physical_bitmap;
unsigned char* virtual_bitmap;

pde_t* page_directory;

unsigned int num_physical_pages;
unsigned int num_virtual_pages;

unsigned int num_vpn_bits;
unsigned int num_offset_bits;

unsigned int num_va_space_bits;
unsigned int num_pa_space_bits;
unsigned int max_bits;

unsigned int max_pages_bits;
unsigned int num_page_directory_bits;
unsigned int num_page_table_bits;

tlb tlb_arr[TLB_ENTRIES];
unsigned long tlb_count = 0;
unsigned long tlb_lookups = 0;
unsigned long tlb_misses = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void init_bit_values() {
    num_va_space_bits = 32;
    num_pa_space_bits = num_bits_in_value(MEMSIZE);
    //max_bits = ((num_va_space_bits) < (num_pa_space_bits)) ? (num_va_space_bits) : (num_pa_space_bits); 
    max_bits = 32;
    num_offset_bits = num_bits_in_value(PGSIZE);
    max_pages_bits = max_bits - num_offset_bits;
    num_vpn_bits = max_bits - num_offset_bits;

    num_page_table_bits = num_bits_in_value(PGSIZE / sizeof(pte_t));
    num_page_directory_bits = max_pages_bits - num_page_table_bits;

}

void print_bit_values() {
    printf("Bit values:\n");
    printf("---------------------------\n");
    printf("VA space bits: %d\n", num_va_space_bits);
    printf("PA space bits: %d\n", num_pa_space_bits);
    printf("Max bits: %d\n", max_bits);
    printf("Offset bits: %d\n", num_offset_bits);
    printf("Max pages bits: %d\n", max_pages_bits);
    printf("Page Table Bits: %d\n", num_page_table_bits);
    printf("Page Directory Bits: %d\n", num_page_directory_bits);
    printf("---------------------------\n");
}

unsigned int num_bits_in_value(unsigned int value) {
    int bits = 0;
    while(value >>= 1) {
        bits++;
    }
    return bits;
}

void init_page_tables() {
    //Set page directory base
    unsigned long nextFreePage = next_free_page(physical_bitmap);
    set_bit(physical_bitmap, nextFreePage, 1);
    page_directory = (pde_t*) (get_physical_addr_from_bit(nextFreePage));

    //Allocate page tables and update bitmap
    for(unsigned long i = 0; i < (1 << num_page_directory_bits); i++) {
        nextFreePage = next_free_page(physical_bitmap);
        set_bit(physical_bitmap, nextFreePage, 1);
    }

    //Set 0x0 as used in memory
    set_bit(virtual_bitmap, 0, 1);
}

unsigned long next_free_page(unsigned char* bitmap) {
    if(bitmap == physical_bitmap) {
        for(int i = 0; i < num_physical_pages; i++) {
            if(!get_bit(bitmap, i)) {
                return i;
            }
        }
    }
    else {
        for(int i = 0; i < num_virtual_pages; i++) {
            if(!get_bit(bitmap, i)) {
                return i;
            }
        }
    }
    
}

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

    //If bit values haven't been set, set them
    if(num_va_space_bits == 0) {
        init_bit_values();
    }

    //Create physical memory
    physical_mem = malloc(MEMSIZE);

    //Calculate number of pages
    num_physical_pages = MEMSIZE / PGSIZE;
    num_virtual_pages = MAX_MEMSIZE / PGSIZE;

    int virtual_bitmap_size = (num_virtual_pages + 7) / 8;
    int physical_bitmap_size = (num_physical_pages + 7) / 8;

    virtual_bitmap = calloc(virtual_bitmap_size, sizeof(unsigned char));
    physical_bitmap = calloc(physical_bitmap_size, sizeof(unsigned char));
    //pthread_mutex_init(&lock, NULL);

    //If page directory isn't set, set it
    if(page_directory == NULL) {
        init_page_tables();
    }
    
}


/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */
    unsigned long index = (unsigned long) get_tlb_index(va);
    tlb_arr[index].va = va;
    tlb_arr[index].pa = pa;
    return 1;


    // return -1;
}
unsigned long get_tlb_index(void *va){
    unsigned long vpn =  ((unsigned long) va) >> num_offset_bits;
    unsigned long index = vpn % TLB_ENTRIES;
    return index;
}



/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */
    unsigned long index = get_tlb_index(va);
    //printf("Check Index: %ld, va: %p, tlb_arr_va: %p\n", index, va, tlb_arr[index].va);
    unsigned long va_vpn = ((unsigned long) va) >> num_offset_bits;
    unsigned long tlb_vpn = ((unsigned long) tlb_arr[index].va) >> num_offset_bits;
    if(va_vpn == tlb_vpn) {
        return (pte_t*) tlb_arr[index].pa;
    }
    else {
        return NULL;
    }

   /*This function should return a pte_t pointer*/
}

/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    double miss_rate = 0;	
    miss_rate = ((double) tlb_misses / (double) tlb_lookups) * 100;

    /*Part 2 Code here to calculate and print the TLB miss rate*/


    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va) {
    /* Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.
    *
    * Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return physical address from the TLB.
    */ 
    pte_t *tlb_result = check_TLB(va);
    tlb_lookups++;
    //hit
    if(tlb_result != NULL){
        return tlb_result;
    }

    tlb_misses++;
    unsigned long vpn = ((unsigned long) va) >> num_offset_bits;
    unsigned long offset = ((unsigned long) va) & ((1 << num_offset_bits) - 1);

    //Get page directory and page table indecies
    unsigned long page_table_index = vpn & ((1 << num_page_table_bits) - 1);
    unsigned long page_directory_index = (vpn >> num_page_table_bits) & ((1 << num_page_directory_bits) - 1);
    
    //Get page directory entry
    pde_t* pde = pgdir + page_directory_index;

    //Get page table entry
    pte_t* pte = pde + page_table_index;

    //assuming pte is physical addr **CHECK THIS
    add_TLB(va,pte);


    return pte;

}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
page_map(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */

    unsigned long bit_index = (((unsigned long) va) >> num_offset_bits);
    //Check if address is mapped in bitmap
    if(get_bit(virtual_bitmap, bit_index)) {
        return 0;
    }

    set_bit(virtual_bitmap, bit_index, 1);
    pte_t* pte = translate(page_directory, va);
    *pte = (pte_t) pa;
    return 1;

}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
 
    //Use virtual address bitmap to find the next free page
    unsigned long start_page = 0;
    unsigned int num_free_pages = 0;
    for(int i = 0; i < num_virtual_pages; i++) {
        if(!get_bit(virtual_bitmap, i)) {
            if(num_free_pages == 0) {
                start_page = i;
            }
            num_free_pages++;
            if(num_free_pages == num_pages) {
                return (void*) (start_page << num_offset_bits);
            }
        }
        else {
            num_free_pages = 0;
        }
    }
    return NULL;
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *t_malloc(unsigned int num_bytes) {

    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */

   /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */

    //Initialize physical memory is it hasn't been
    pthread_mutex_lock(&lock);
    if(!physical_mem) {
        set_physical_mem();
        
    }
    

    unsigned int num_pages = (num_bytes + PGSIZE - 1) / PGSIZE;
    
    //Check if there are available pages
    void* va = get_next_avail(num_pages);
    if(!va) {
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    void* pa = get_next_avail_physical(num_pages);
    if(!pa) {
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    for (int i = 0; i < num_pages; i++) {
        unsigned long nextFreePage = next_free_page(physical_bitmap);
        set_bit(physical_bitmap, nextFreePage, 1);
        pa = get_physical_addr_from_bit(nextFreePage);
        page_map(page_directory, va + i * PGSIZE, pa);
    }
    pthread_mutex_unlock(&lock);
    return va;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void t_free(void *va, int size) {

    /* Part 1: Free the page table entries starting from this virtual address
     * (va). Also mark the pages free in the bitmap. Perform free only if the 
     * memory from "va" to va+size is valid.
     *
     * Part 2: Also, remove the translation from the TLB
     */

    //Number of pages to free
    pthread_mutex_lock(&lock);
    unsigned int num_pages = (size + PGSIZE - 1) / PGSIZE;

    //Check if num_pages pages are allocated in the virtual bitmap
    unsigned long vpn = (unsigned long) va >> num_offset_bits;
    for (int i = 0; i < num_pages; i++) {
        if(!get_bit(virtual_bitmap, vpn)) {
            pthread_mutex_unlock(&lock);
            return;
        }
        vpn++;
    }
    
    for (int i = 0; i < num_pages; i++) {
        unsigned long vpn = (unsigned int) va >> num_offset_bits;

        //Get page table entry
        pte_t *pte = translate(page_directory, va);
        unsigned long bitPos = get_bit_position_from_pointer((void*) *pte);

        //Free physical page and update physical bitmap
        set_bit(physical_bitmap, bitPos, 0);
        *pte = 0;

        //Update virtual bitmap
        set_bit(virtual_bitmap, vpn, 0);

        unsigned long index = get_tlb_index(va);
        tlb_arr[index].va = NULL;
        tlb_arr[index].pa = NULL;

        //Set virtual address to the next page
        va = (void*) ((unsigned long) va + PGSIZE);
    }
    pthread_mutex_unlock(&lock);
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
 * The function returns 0 if the put is successfull and -1 otherwise.
*/
int put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
     * the contents of "val" to a physical page. NOTE: The "size" value can be larger 
     * than one page. Therefore, you may have to find multiple pages using translate()
     * function.
     */
    
    //Number of pages needed
    pthread_mutex_lock(&lock);
    unsigned int num_pages = (size + PGSIZE - 1) / PGSIZE;
    unsigned long bytesRemaining = size;
    unsigned long bytesToWrite;
    
    //Check if num_pages pages are allocated in the virtual bitmap
    unsigned long vpn = (unsigned long) va >> num_offset_bits;
    for (int i = 0; i < num_pages; i++) {
        if(!get_bit(virtual_bitmap, vpn)) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
        vpn++;
    }

    for (int i = 0; i < num_pages; i++) {
        unsigned int vpn = (unsigned int) va >> num_offset_bits;

        if(bytesRemaining > PGSIZE) {
            bytesToWrite = PGSIZE;
            bytesRemaining -= PGSIZE;
        }
        else {
            bytesToWrite = bytesRemaining;
            bytesRemaining = 0;
        }

        //Get physical page number and copy bytes of val to it
        pte_t *pte = translate(page_directory, va);
        memcpy((void*) *pte, val + i * PGSIZE, bytesToWrite);

        //Set virtual address to the next page
        va = (void*) ((unsigned long) va + PGSIZE);
    }
    pthread_mutex_unlock(&lock);
    return 0;

    /*return -1 if put_value failed and 0 if put is successfull*/

}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */

    //Number of pages needed
    pthread_mutex_lock(&lock);

    unsigned int num_pages = (size + PGSIZE - 1) / PGSIZE;
    unsigned long bytesRemaining = size;
    unsigned long bytesToGet;

    //Check if num_pages pages are allocated in the virtual bitmap
    unsigned long vpn = (unsigned long) va >> num_offset_bits;
    for (int i = 0; i < num_pages; i++) {
        if(!get_bit(virtual_bitmap, vpn)) {
            pthread_mutex_unlock(&lock);
            return;
        }
        vpn++;
    }

    for (int i = 0; i < num_pages; i++) {
        unsigned int vpn = (unsigned int) va >> num_offset_bits;

        if(bytesRemaining > PGSIZE) {
            bytesToGet = PGSIZE;
            bytesRemaining -= PGSIZE;
        }
        else {
            bytesToGet = bytesRemaining;
            bytesRemaining = 0;
        }

        //Get physical page number and copy bytes of val to it
        pte_t *pte = translate(page_directory, va);
        memcpy(val + i * PGSIZE, (void*) *pte, bytesToGet);

        //Set virtual address to the next page
        va = (void*) ((unsigned long) va + PGSIZE);
    }
    pthread_mutex_unlock(&lock);

}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */
    int x, y, val_size = sizeof(int);
    int i, j, k;
    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            unsigned int a, b, c = 0;
            for (k = 0; k < size; k++) {
                int address_a = (unsigned int)mat1 + ((i * size * sizeof(int))) + (k * sizeof(int));
                int address_b = (unsigned int)mat2 + ((k * size * sizeof(int))) + (j * sizeof(int));
                get_value( (void *)address_a, &a, sizeof(int));
                get_value( (void *)address_b, &b, sizeof(int));
                // printf("Values at the index: %d, %d, %d, %d, %d\n", 
                //     a, b, size, (i * size + k), (k * size + j));
                c += (a * b);
            }
            int address_c = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int));
            // printf("This is the c: %d, address: %x!\n", c, address_c);
            put_value((void *)address_c, (void *)&c, sizeof(int));
        }
    }
}

void set_bit(unsigned char* bitmap, unsigned long index, unsigned int value) {
    unsigned int byte_index = index / 8;
    unsigned int pos = index % 8;

    if (value) {
        bitmap[byte_index] |= (1 << pos);
    } else {
        bitmap[byte_index] &= ~(1 << pos);
    }
}

int get_bit(unsigned char* bitmap, unsigned long index) {
    unsigned int byte_index = index / 8;
    unsigned int pos = index % 8;
    return (bitmap[byte_index] & (1 << pos)) != 0;
}

void* get_physical_addr_from_bit(unsigned long pageNumInBitmap) {
    return (void*) ((pageNumInBitmap * PGSIZE) + physical_mem);
}

unsigned long get_bit_position_from_pointer(void* pa) {
    return (unsigned long) (pa - physical_mem) / PGSIZE;
}

void *get_next_avail_physical(int num_pages) {
 
    //Use physical address bitmap to find the next free page
    unsigned long start_page = 0;
    unsigned int num_free_pages = 0;
    for(int i = 0; i < num_physical_pages; i++) {
        if(!get_bit(physical_bitmap, i)) {
            if(num_free_pages == 0) {
                start_page = i;
            }
            num_free_pages++;
            if(num_free_pages == num_pages) {
                return (void*) (get_physical_addr_from_bit(start_page));
            }
        }
        else {
            num_free_pages = 0;
        }
    }
    return NULL;
}