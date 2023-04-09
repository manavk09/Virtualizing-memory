#include "my_vm.h"

unsigned char* physical_mem;
unsigned char* physical_bitmap;
unsigned char* virtual_bitmap;

unsigned int num_physical_pages;
unsigned int num_virtual_pages;

unsigned int num_vpn_bits;
unsigned int num_offset_bits;
unsigned int num_page_directory_bits;

pde_t* page_directory;

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them

    //Calculate number of pages
    num_physical_pages = MEMSIZE / PGSIZE;
    num_virtual_pages = MAX_MEMSIZE / PGSIZE;
    num_vpn_bits = 32 - log2(PGSIZE);
    num_offset_bits = 32 - num_vpn_bits;

    //Set page directory bits
    num_page_directory_bits = 10;

    //Map physical memory
    physical_mem = mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);

    int virtual_bitmap_size = (num_virtual_pages + 7) / 8;
    int physical_bitmap_size = (num_physical_pages + 7) / 8;

    virtual_bitmap = calloc(virtual_bitmap_size, sizeof(unsigned char));
    physical_bitmap = calloc(physical_bitmap_size, sizeof(unsigned char));
    
}


/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */



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

    unsigned int vpn = (unsigned int) va >> num_offset_bits;
    unsigned int offset = (unsigned int) va & (int) pow(2, num_offset_bits);

    //Get page directory and page table indecies
    unsigned int page_directory_index = (vpn >> num_page_directory_bits) & 0x3FF;
    unsigned int page_table_index = vpn & 0x3FF;
    
    //Get page directory entry
    pde_t pde = pgdir[page_directory_index];

    //Check if the page directory entry is present
    if (!(pde & 1))
        return NULL;

    unsigned int pde_page_frame_number = pde & 0xFFFFF000;

    pte_t pte = pde_page_frame_number + (page_table_index * sizeof(pte_t));

    if(!(pte & 1))
        return NULL;

    unsigned int pte_page_frame_number = pte & 0xFFFFF000;

    pte_t* physical_addr = (pte_t*) (pte_page_frame_number | offset);

    return physical_addr;

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

    return -1;
}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
 
    //Use virtual address bitmap to find the next free page
    unsigned int start_page = 0;
    unsigned int num_free_pages = 0;
    for(int i = 0; i < num_virtual_pages; i++) {
        if(!isAllocated(virtual_bitmap, i)) {
            if(num_free_pages == 0) {
                start_page = i;
            }
            num_free_pages++;
            if(num_free_pages == num_pages) {
                return (void*)(start_page * PGSIZE);
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

    if(!physical_mem) {
        set_physical_mem();
    }

    if(!page_directory) {
        page_directory = malloc(PGSIZE);
    }

    unsigned int num_pages = (num_bytes + PGSIZE - 1) / PGSIZE;

    void* va = get_next_avail(num_pages);
    if(!va) {
        return NULL;
    }

    // Update the bitmaps
    for (int i = 0; i < num_pages; i++) {
        unsigned int vpn = ((unsigned int) va + i * PGSIZE) >> num_offset_bits;
        unsigned int physical_page_num = ((unsigned int) physical_mem + i * PGSIZE) >> num_offset_bits;
        set_bit(virtual_bitmap, vpn, 1);
        set_bit(physical_bitmap, physical_page_num, 1);
    }

    // Map the new pages to the given virtual address
    for (int i = 0; i < num_pages; i++) {
        unsigned int vpn = ((unsigned int) va + i * PGSIZE);
        unsigned int physical_page_num = ((unsigned int)physical_mem + i * PGSIZE) >> num_offset_bits;
        page_map(page_directory, (void *) vpn, (void *)physical_page_num);
    }

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
    unsigned int num_pages = (size + PGSIZE - 1) / PGSIZE;
    
    for (int i = 0; i < num_pages; i++) {
        unsigned int vpn = (unsigned int) va >> num_offset_bits;

        //Check if page is allocated
        if(!isAllocated(virtual_bitmap, vpn)) {
            return;
        }

        //Get physical page number
        pte_t *pte = translate(page_directory, va);
        unsigned int ppn = (*pte) >> num_offset_bits;

        //Free physical page
        physical_bitmap[ppn / 8] &= ~(1 << (ppn % 8));
        memset(&physical_mem[ppn * PGSIZE], 0, PGSIZE);
        *pte = 0;

        virtual_bitmap[vpn / 8] &= ~(1 << (vpn % 8));

        va = (void*) ((unsigned int) va + PGSIZE);
    }
    
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


    /*return -1 if put_value failed and 0 if put is successfull*/

}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */


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



void set_bit(char* bitmap, unsigned int index, unsigned int value) {
    unsigned int byte_index = index / 8;
    unsigned int pos = index % 8;

    if (value) {
        bitmap[byte_index] |= (1 << pos);
    } else {
        bitmap[byte_index] &= ~(1 << pos);
    }
}

bool isAllocated(char* bitmap, int vpn) {
    // Calculate the byte index and bit offset within the byte for the given VPN
    int byte_index = vpn / 8;
    int pos = vpn % 8;
    
    // Check if the corresponding bit in the bitmap is set
    return (bitmap[byte_index] & (1 << pos)) != 0;
}