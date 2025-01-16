/* page.h - Defines used to work with paging
*/
#ifndef PAGE_H
#define PAGE_H
#include "types.h"
#define KERNEL_ADDR 4194304 // 4MB in physical memory
// #define KERNEL_BOTTOM_ADDR 0x800000 // 8MB in physical memory
// Page Directory Entry
typedef union page_directory_entry_t{
    uint32_t val;
    struct{
        uint32_t present:1;             // 1 if page is present in physical memory
        uint32_t read_write:1;
        uint32_t user_supervisor:1;
        uint32_t write_through:1;
        uint32_t cache_disabled:1;
        uint32_t accessed:1;
        uint32_t dirty:1;
        uint32_t page_size:1;
        uint32_t global_page:1;
        uint32_t available:3;
        uint32_t page_table_base_address:20;
    }__attribute__((packed));
} page_directory_entry_t;

// Page Table Entry
typedef union page_table_entry_t{
    uint32_t val;
    struct{
        uint32_t present:1;
        uint32_t read_write:1;
        uint32_t user_supervisor:1;
        uint32_t write_through:1;
        uint32_t cache_disabled:1;
        uint32_t accessed:1;
        uint32_t dirty:1;
        uint32_t page_table_attribute_table:1;
        uint32_t global_page:1;
        uint32_t available:3;
        uint32_t page_base_address:20;
    }__attribute__((packed));
} page_table_entry_t;

// the page directory with 1024 entries, page-aligned addresses being a multiple of 4096 (4KB)
extern page_directory_entry_t page_directory[1024]__attribute__((aligned(4096)));
// the page table with 1024 entries, page-aligned addresses being a multiple of 4096 (4KB)
extern page_table_entry_t page_table[1024]__attribute__((aligned(4096)));
// the page table with 1024 entries, page-aligned addresses being a multiple of 4096 (4KB) for the video memory in the user space
extern page_table_entry_t user_video_page_table[1024]__attribute__((aligned(4096))); 

// init the page directory and page table
extern void page_init();
// set the page directory entry
extern void set_pde(page_directory_entry_t* directory, uint32_t index,uint32_t address,uint8_t ps,uint8_t g,uint8_t u_s);
// set the page table entry
extern void set_pte(page_table_entry_t* table, uint32_t index, uint32_t address,uint8_t u_s);
// set the cr0, cr3, cr4 to enable paging and load Page Directory
extern void set_crs();
#endif
