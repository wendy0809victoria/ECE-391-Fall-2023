//
//  pit.c
//  
//
//  Created by Wendi Wang on 2023/11/24.
//

#include "pit.h"
#include "i8259.h"
#include "system_calls.h"
#include "page.h"
#include "x86_desc.h"
#include "lib.h"

/*
* pit_init
*   DESCRIPTION: initialize pit driver
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*/
void pit_init()
{
    outb(0x37, PIT_CMD);                // 0x37 - initial pit mode
    outb(0x9C, PIT_DATA);               // 0x9C - high byte of frequency (100 Hz)
    outb(0x2E, PIT_DATA);               // 0x2E - low byte of frequency (100 Hz)
    enable_irq(0);                      // 0 - irq number of pit
}

/*
* pit_handler
*   DESCRIPTION: handle pit interrupts
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*/
void pit_handler(void)
{
    // 0 - irq number of pit
    send_eoi(0);

    // Next running process id number
    int32_t new_pid;
    pcb* cur_pcb;
    
    // Get current process structure
    cur_pcb = get_cur_pcb_ptr();

    // Get current running process's base pointer 
    asm volatile ("movl %%ebp, %0" : "=r"(cur_pcb->run_ebp));

    // Update running terminal id and process id number to next one
    // 1 - move on to next terminal
    // 3 - total number of terminal
    run_terminal =(run_terminal + 1) % 3;
    new_pid = schedule[(int32_t)run_terminal];
    
    // If next process id number is invalid
    // -1 - next process id number is invalid
    if (new_pid == -1) {
        
        // Switch back to current running terminal
        memory_switch(run_terminal);

        // Restart shell
        execute((uint8_t*)"shell");
    }

    // set up the 4MB program paging
    set_pde(page_directory, USER_ADDR >> 22, (KERNEL_BOTTOM_ADDR + new_pid * KERNEL_ADDR) >> 12, 1, 0, 1);
    
    // flush the TLB
    flush_tlb(); 
    
    // switch back to running terminal
    memory_switch(run_terminal);

    // context switch
    // For each CPU which executes processes possibly wanting to do system calls via interrupts, one TSS is required.
    // The only interesting fields are SS0 and ESP0. Whenever a system call occurs, the CPU gets the SS0 and ESP0-value in its TSS and assigns the stack-pointer to it.
    // SS0 gets the kernel datasegment descriptor
    // ESP0 gets the value the stack-pointer shall get at a system call
    tss.ss0 = KERNEL_DS;
    // each kernal stack starts at the bottom (larger addr) of an 8KB (0x2000) block inside the kernel
    tss.esp0 = (uint32_t)get_pcb_ptr(new_pid - 1) - 4; 

    // Update running process's base pointer for next running terminal
    asm volatile(
        "movl %0, %%ebp \n\
        leave          \n\
        ret            \n"
        :  
        :  "r" ((get_pcb_ptr(new_pid))->run_ebp) \
        :  "ebp");
}
