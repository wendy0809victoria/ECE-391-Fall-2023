/*
system_calls.h - Defines for various system calls
*/
#ifndef SYSTEM_CALLS_H
#define SYSTEM_CALLS_H
#include "types.h"
#include "pit.h"

#define KERNEL_BOTTOM_ADDR 0x800000 // 8MB in physical memory
#define USER_ADDR 0x8000000 // 128MB in physical memory
#define USER_IMAGE 0x08048000 // The program image itself is linked to execute at virtual address 0x08048000.
#define USER_STACK_ADDR 0x8400000 // 132MB in physical memory
#define USER_VIDEO_ADDR 0x8800000 // 136MB in physical memory
// invalid file operations for stdin and stdout
extern int32_t invalid_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t invalid_write(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t invalid_open(const uint8_t* filename);
extern int32_t invalid_close(int32_t fd);

// the file operations table function pointers
typedef int32_t(*read_ptr)(int32_t fd, void* buf, int32_t nbyte);
typedef int32_t(*write_ptr)(int32_t fd, const void* buf, int32_t nbyte);
typedef int32_t(*open_ptr)(const uint8_t* filename);
typedef int32_t(*close_ptr)(int32_t fd);

// the file operations table
typedef struct file_operations_table
{
    open_ptr open;
    close_ptr close;
    read_ptr read;
    write_ptr write; 
} file_operations_table;

// the file descriptor as an element in a file descriptor array
typedef struct file_descriptor
{
    file_operations_table file_operations_table_ptr;
    uint32_t inode;
    uint32_t file_position;
    uint32_t flags;
} file_descriptor;

// sigaction - information of a signal
typedef struct sigaction
{
    int32_t flag;       // flag - whether this signal is raised
    int32_t mask;       // mask - whether this is permitted to be raised
    void* handler;      // handler - specific task for this signal to do
} sigaction;

// the Process Control Block (pcb) struct
typedef struct pcb
{
    file_descriptor file_descriptor_array[8]; // Up to 8 open files per task
    uint32_t parent_pid; // the parent process id
    uint32_t pid; // the current process id
    uint32_t esp; // the current stack pointer esp
    uint32_t ebp; // the current base pointer ebp
    uint32_t run_ebp; // current running process's base pointer
    uint8_t args[128]; // the command line arguments, 128 is the keyboard buffer size
    // uint8_t terminal_num;
    sigaction signals[5];
} pcb;

// the hardware context structure
typedef struct hw_context
{
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eax;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t irq_num;
    uint32_t dummy;
    uint32_t ret_addr;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} hw_context;

extern int32_t halt(uint8_t status);
extern int32_t execute(const uint8_t* command);
extern int32_t read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t write(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open(const uint8_t* filename);
extern int32_t close(int32_t fd);
extern int32_t getargs(uint8_t* buf, int32_t nbytes);
extern int32_t vidmap(uint8_t** screen_start);
extern int32_t set_handler(int32_t signum, void* handler_address);
extern int32_t sigreturn(void);

extern int32_t KILL();
extern int32_t IGNORE();

extern void flush_tlb();
extern pcb* get_pcb_ptr(int32_t pid);
extern pcb* get_cur_pcb_ptr();
extern int32_t get_pid();

extern int32_t schedule[3];     // 3 - total number of terminal
extern int32_t run_terminal;    // Current running terminal id number

#endif
