#include "page.h"
#include "lib.h"


// the page directory with 1024 entries, page-aligned addresses being a multiple of 4096 (4KB)
page_directory_entry_t page_directory[1024]__attribute__((aligned(4096)));
// the page table with 1024 entries, page-aligned addresses being a multiple of 4096 (4KB)
// for the video memory in the kernel space
page_table_entry_t page_table[1024]__attribute__((aligned(4096)));
// the page table with 1024 entries, page-aligned addresses being a multiple of 4096 (4KB) 
// for the video memory in the user space
page_table_entry_t user_video_page_table[1024]__attribute__((aligned(4096)));
/* page_init
 *   DESCRIPTION: initialize the page directory table and page table
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: initialize the page directory table and page table
*/
void page_init()
{
    int i;
    // initialize all the entries in page directory table and page table, 1024 entries
    for (i = 0; i < 1024; i++)
    {
        page_directory[i].val = 0;
        page_table[i].val = 0;
    }
    // for page directory, set the first entry to be 4kB video memory mapping
    set_pde(page_directory, 0, ((uint32_t) &page_table) >> 12, 0,0,0);
    // for the first page table, set the first entry to be 4kB video memory mapping
    set_pte(page_table, VIDEO >> 12, VIDEO >> 12,0);
    set_pte(page_table, (VIDEO >> 12) + 2, (VIDEO >> 12) + 2, 0);                   // 2 - offset of 1st terminal video backup buffer
    set_pte(page_table, (VIDEO >> 12) + 3, (VIDEO >> 12) + 3, 0);                   // 3 - offset of 2nd terminal video backup buffer
    set_pte(page_table, (VIDEO >> 12) + 4, (VIDEO >> 12) + 4, 0);                   // 4 - offset of 3rd terminal video backup buffer
    // for pdt, set the second entry to be the 4MB kernel mapping, enable global page and page size
    set_pde(page_directory, 1, KERNEL_ADDR >> 12, 1,1,0);
    // set the cr0, cr3, cr4 to enable paging and load Page Directory
    set_crs();
}

/*
* set_pde
*   DESCRIPTION: helper function to set the page directory entry
*   INPUTS: directory -- the page directory table
*           index -- the index of the entry
*           address -- the address of the page table
*           ps -- the page size bit
*           g -- the global page bit
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: set the page directory entry
*/
void set_pde(page_directory_entry_t* directory, uint32_t index, uint32_t address,uint8_t ps,uint8_t g,uint8_t u_s)
{
    directory[index].present = 1;
    directory[index].read_write = 1;
    directory[index].user_supervisor = u_s;
    directory[index].write_through = 0;
    directory[index].cache_disabled = 0;
    directory[index].accessed = 0;
    directory[index].dirty = 0;
    directory[index].page_size = ps;
    directory[index].global_page = g;
    directory[index].available = 0;
    directory[index].page_table_base_address = address;
}

/*
* set_pte
*   DESCRIPTION: helper function to set the page table entry
*   INPUTS: table -- the page table
*           index -- the index of the entry
*           address -- the address of the page
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: set the page table entry
*/
void set_pte(page_table_entry_t* table,uint32_t index, uint32_t address,uint8_t u_s)
{
    table[index].present = 1;
    table[index].read_write = 1;
    table[index].user_supervisor = u_s;
    table[index].write_through = 0;
    table[index].cache_disabled = 0;
    table[index].accessed = 0;
    table[index].dirty = 0;
    table[index].page_table_attribute_table = 0;
    table[index].global_page = 0;
    table[index].available = 0;
    table[index].page_base_address = address;
}

// put the address of page directory table into cr3
// set the highest bit of cr0 to be 1, the paging bit.
// set the bit 4 and bit 7 of cr4 to be 1, the page size bit and the enable global page bit.
/*
* set_crs
*   DESCRIPTION: helper function to set the cr0, cr3, cr4
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: set the cr0, cr3, cr4
*/
void set_crs()
{
    asm volatile(
       "movl  %0, %%eax;           \
        movl  %%eax, %%cr3;        \
        movl  %%cr4, %%eax;        \
        orl   $0x00000090, %%eax;  \
        movl  %%eax, %%cr4;        \
        movl  %%cr0, %%eax;        \
        orl   $0x80000000, %%eax;  \
        movl  %%eax, %%cr0;"
        : /*no output*/
        : "r" (page_directory)
        : "%eax"
    );
}

