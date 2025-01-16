#include "system_calls.h"
#include "page.h"
#include "x86_desc.h"
#include "lib.h"
#include "terminal.h"
#include "rtc.h"
#include "filesystem.h"
// 6 is the maximum number of processes
uint8_t pid_bitmap[6] = {0,0,0,0,0,0};  // the bitmap for process id, 0: available, 1: not available
int32_t schedule[3] = {-1, -1, -1};     // -1 - terminal not running
int32_t run_terminal = 0;               // current running terminal id

/*
* halt
*   DESCRIPTION: halt the current process, returning the specified value to its parent process
*   INPUTS: status -- the status of the process
*   OUTPUTS: none
*   RETURN VALUE: the value to be returned to the parent process
*/
int32_t halt(uint8_t status)
{
    putc('\n', 0);

    int32_t i;
    int32_t ret;
    pcb* pcb_parent;
    pcb* pcb_now;

    // 1 - represents exception
    if (status == 1) {
        // 255 - last valid value representing shell
        ret = 255;
        
        // ++ - exceptions
        ret++;
    } else {
        
        // 0 - ordinary halting
        ret = 0;
    }

    // Get current pcb structure
    pcb_now = get_cur_pcb_ptr();

    // 0 - set the process to be halted as available
    pid_bitmap[pcb_now->pid] = 0;

    // Restart shell by calling execute
    // 0 / 1 / 2 - currently running shell
    if (pcb_now->pid == 0 || pcb_now->pid == 1 || pcb_now->pid == 2) { 
        printf("----------------------------------------------------\n");
        printf("|               Cannot exit base shell             |\n");
        printf("----------------------------------------------------\n");
        execute((uint8_t*)"shell"); 
    }

    // Close all file descriptors
    // 8 - maximum value of open files
    for (i = 0; i < 8; i++) 
    {
        if (pcb_now->file_descriptor_array[i].flags == 1) 
        {
            
            // 0 - set the process to be halted as available
            pcb_now->file_descriptor_array[i].flags = 0;
            pcb_now->file_descriptor_array[i].file_operations_table_ptr.close(i);
        }
    }

    // 3 - total number of terminal
    for (i = 0; i < 3; i++) {

        // Go back to parent process
        if (schedule[i] == pcb_now->pid) {
            schedule[i] = pcb_now->parent_pid;
        }
    }

    // Return to parent task
    pcb_parent = get_pcb_ptr(pcb_now->parent_pid);
    i = pcb_now->parent_pid;

    // set up the 4MB program paging
    set_pde(page_directory, USER_ADDR >> 22, (KERNEL_BOTTOM_ADDR + i * KERNEL_ADDR) >> 12, 1, 0, 1);
    
    // flush the TLB
    flush_tlb(); 

    // set tss 
    // SS0 gets the kernel datasegment descriptor
    // ESP0 gets the value the stack-pointer shall get at a system call
    tss.ss0 = KERNEL_DS;
    // each kernal stack starts at the bottom (larger addr) of an 8KB (0x2000) block inside the kernel
    tss.esp0 = (KERNEL_BOTTOM_ADDR - i * 0x2000) - 4; 

    asm volatile(
        "movl %0, %%eax;"
        "movl %1, %%ebp;"
        "movl %2, %%esp;"
        "leave;"
        "ret;"
        :
        : "r"(ret), "r"(pcb_now->ebp), "r"(pcb_now->esp)
        : "eax", "ebp", "esp"
    );
    
    // halt should never return to the caller, so if it does, return -1 on failure
    return -1;
}

/*
* execute
*   DESCRIPTION: execute a new program, handing off the processor to the new program until it terminates
*   INPUTS: command -- the command to be executed
*   OUTPUTS: none
*   RETURN VALUE: -1 on failure, for example, the command cannot be executed,the program does not exist or the filename specified is not an executable
*                 256 if the program dies by an exception
*                 a value in the range 0 to 255 if the program executes a halt system call, in which case the value returned is that given by the program’s call to halt
*/
int32_t execute(const uint8_t* command)
{
    int i;
    int j;
    int32_t signal;
    int new_pid;
    uint8_t args[128]; // the command line arguments, 128 is the maximum length
    uint8_t filename[32]; // the maximum length of a filename is 32
    uint8_t filename_length = 0; // the length of the filename
    dentry_t dentry;
    pcb* pcb_ptr;
    uint32_t entry_point;
    // in executable file, a header that occupies the first 40 bytes gives information for loading and starting the program
    uint8_t buf_header[40];
    if (command == NULL)
    {
        return -1;
    }
    for (i = 0; (i < strlen((int8_t*)command)) && (command[i] == ' '); i++)
    {
        // skip the leading spaces
    }
    if (i == strlen((int8_t*)command))
    {
        // the command is empty, return -1
        return -1;
    }
    // parse the file name of the program to be executed
    // if there is a space or a null-terminated character, then the filename has been parsed
    for (; (i < strlen((int8_t*)command)) && (command[i] != ' ') && (command[i] != '\0'); i++)
    {
        // if the filename exceed the max length 32, return -1
        if (filename_length >= 32)
        {
            return -1;
        }
        // copy the filename
        filename[filename_length] = command[i];
        filename_length++;
    }
    // null-terminate the filename
    filename[filename_length] = '\0';
    // parse the arguments
    for (; (i < strlen((int8_t*)command)) && (command[i] == ' '); i++)
    {
        // skip the spaces between the filename and the arguments
    }
    // if there is no argument, set the argument to be null-terminated
    if (i == strlen((int8_t*)command))
    {
        args[0] = '\0';
    }
    // copy the arguments
    j = 0;
    while (i < strlen((int8_t*)command) && command[i] != '\0' && command[i] != ' ')
    {
        args[j] = command[i];
        i++;
        j++; 
    }
    // null-terminate the argument
    args[j] = '\0';    
    // check if the file exists
    if (read_dentry_by_name(filename, &dentry) == -1)
    {
        return -1;
    }
    // read the header of the file (the first 40 bytes), return -1 if the read fails
    if (read_data(dentry.inode_num, 0, buf_header, 40) != 40)
    {
        return -1;
    }
    // check if the file is an executable
    // The first 4 bytes (0: 0x7f; 1: 0x45; 2: 0x4c; 3: 0x46) of the file represent a “magic number” that identifies the file as an executable.
    if ((buf_header[0] != 0x7f) || (buf_header[1] != 0x45) || (buf_header[2] != 0x4c) || (buf_header[3] != 0x46))
    {
        return -1;
    }
    // find an available process id
    // 6 is the maximum number of processes
    for (i = 0; i < 6; i++)
    {
        if (pid_bitmap[i] == 0)
        {
            pid_bitmap[i] = 1;
            new_pid = i;
            break;
        }
    }
    // if no available process id, return -1
    if (i == 6)
    {
        printf("----------------------------------------------------\n");
        printf("|            Maximum process number reached        |\n");
        printf("----------------------------------------------------\n");
        return -1;
    }
    
    pid_bitmap[i] = 1;           
    new_pid = i;
    pcb_ptr = get_pcb_ptr(new_pid);
    pcb_ptr->pid = new_pid;

    // set the parent pid of the new process if there is a parent process
    if (new_pid == 0 || new_pid == 1 || new_pid == 2) {
        pcb_ptr->parent_pid = 255;
    } else {
        pcb_ptr->parent_pid = get_pid();
    }

    // 3 - total number of terminal
    for (i = 0; i < 3; i++) {
        if (schedule[i] == -1 || schedule[i] == pcb_ptr->parent_pid){
            schedule[i] = pcb_ptr->pid;
            break;
        }
    }

    // copy the arguments to the pcb
    memcpy(pcb_ptr->args, args, 128);
    // When a process is started, automatically open stdin and stdout, which correspond to file descriptors 0 and 1 respectively.
    pcb_ptr->file_descriptor_array[0].file_operations_table_ptr.read = terminal_read;
    pcb_ptr->file_descriptor_array[0].file_operations_table_ptr.open = invalid_open;
    pcb_ptr->file_descriptor_array[0].file_operations_table_ptr.close = invalid_close;
    pcb_ptr->file_descriptor_array[0].file_operations_table_ptr.write = invalid_write;

    pcb_ptr->file_descriptor_array[0].inode = 0;
    pcb_ptr->file_descriptor_array[0].file_position = 0;
    pcb_ptr->file_descriptor_array[0].flags = 1;

    pcb_ptr->file_descriptor_array[1].file_operations_table_ptr.read = invalid_read;
    pcb_ptr->file_descriptor_array[1].file_operations_table_ptr.open = invalid_open;
    pcb_ptr->file_descriptor_array[1].file_operations_table_ptr.close = invalid_close;
    pcb_ptr->file_descriptor_array[1].file_operations_table_ptr.write = terminal_write;

    pcb_ptr->file_descriptor_array[1].inode = 0;
    pcb_ptr->file_descriptor_array[1].file_position = 0;
    pcb_ptr->file_descriptor_array[1].flags = 1;

    // 5 - total signal number
    for (signal = 0; signal < 5; signal++) {
        
        // for signal 0, 1 and 2, default action is "kill the task"
        if (signal < 3) {
            pcb_ptr->signals[signal].handler = KILL;
        } else {
            
            // for signal 3 and 4, default action is "ignore"
            pcb_ptr->signals[signal].handler = IGNORE;
        }
        
        // 0 for mask - unmask all signals
        // 0 for flage - signal not raised currently
        pcb_ptr->signals[signal].mask = 0;
        pcb_ptr->signals[signal].flag = 0;
    }

	// set up the 4MB program paging
    set_pde(page_directory,USER_ADDR >> 22,(KERNEL_BOTTOM_ADDR+new_pid*KERNEL_ADDR) >> 12,1,0,1);
    // flush the TLB
    flush_tlb();
    // load the program into memory
    read_data(dentry.inode_num, 0, (uint8_t*)USER_IMAGE, USER_STACK_ADDR - USER_IMAGE);

    // context switch
    // For each CPU which executes processes possibly wanting to do system calls via interrupts, one TSS is required.
    // The only interesting fields are SS0 and ESP0. Whenever a system call occurs, the CPU gets the SS0 and ESP0-value in its TSS and assigns the stack-pointer to it.
    // SS0 gets the kernel datasegment descriptor
    // ESP0 gets the value the stack-pointer shall get at a system call
    tss.ss0 = KERNEL_DS;
    // each kernal stack starts at the bottom (larger addr) of an 8KB (0x2000) block inside the kernel
    tss.esp0 = KERNEL_BOTTOM_ADDR - (new_pid) * 0x2000 - 4;
    // the entry point of the program (the virtual address of the first instruction that should be executed) is the 24th to 27th bytes of the header
    entry_point = buf_header[27] << 24 | buf_header[26] << 16 | buf_header[25] << 8 | buf_header[24];
    // store the esp and ebp
    asm volatile ("movl %%ebp, %0" : "=r"(pcb_ptr->ebp));
    asm volatile ("movl %%esp, %0" : "=r"(pcb_ptr->esp));

    sti();
    // push the iret context onto the stack
    // IRET needs 5 elements on stack: User DS,ESP,EFLAG,CS,EIP
    // The ESP is the user stack pointer to the bottom of the 4 MB page already holding the executable image - 4 
    // The DS is the user data segment, which is 0x2B
    // The CS is the user code segment, which is 0x23
    // The EIP need to jump to is the entry point from bytes 24-27 of the executable that you have just loaded
    asm volatile(
        "pushl %0;"
        "pushl %1;"
        "pushfl;"
        "pushl %2;"
        "pushl %3;"
        "iret;"
        :
        : "r"(USER_DS), "r"(USER_STACK_ADDR - 4), "r"(USER_CS),"r"(entry_point)
        : "memory", "cc"
    );
    return 0;
}

/*
* read
*   DESCRIPTION: read data from the keyboard, a file, device (RTC), or directory
*   INPUTS: fd -- file descriptor
*           buf -- the buffer to be read
*           nbytes -- the number of bytes to be read
*   OUTPUTS: none
*   RETURN VALUE: -1 on failure, the number of bytes read on success
*/
int32_t read(int32_t fd, void* buf, int32_t nbytes)
{
    // Get pcb structure of current pid_bitmap (pid)
    pcb* cur_pcb = get_cur_pcb_ptr();
    uint32_t byte_read = 0;
    int32_t ret = 0;
    
    // Validate arguments
    // fd - index to file_descriptor_array of size 8
    // nbytes - byte to be read, invalid if less than / equals 0
    if (fd < 0 || fd >= 8 || buf == NULL || nbytes <= 0) {
        return -1;          // return -1 for failure
    }

    // If the file hasn't been open
    if (cur_pcb->file_descriptor_array[fd].flags == 0) {
        return -1;
    }

    // 0 - the first operation in file_operations_table_ptrs_table_pointer->read is "read"
    byte_read = ((cur_pcb->file_descriptor_array)[fd]).file_operations_table_ptr.read(fd, buf, nbytes);

    // Update file position after reading
    ((cur_pcb->file_descriptor_array)[fd]).file_position = ((cur_pcb->file_descriptor_array)[fd]).file_position + byte_read;

    // Cast byte_read from unsigned int to int for return
    ret = (int32_t)byte_read;
        
    // Return byte number reads
    return ret;
}
/*
* write
*   DESCRIPTION: write data to the terminal or to a device (RTC)
*   INPUTS: fd -- file descriptor
*           buf -- the buffer to be written
*           nbytes -- the number of bytes to be written
*   OUTPUTS: none
*   RETURN VALUE: -1 on failure, the number of bytes written on success
*/
int32_t write(int32_t fd, const void* buf, int32_t nbytes)
{
    // Get pcb structure of current pid_array (pid)
    pcb* cur_pcb = get_cur_pcb_ptr();
    uint32_t byte_write = 0;
    int32_t ret = 0;
    
    // Validate arguments
    // fd - index to file_descriptor_array of size 8
    // nbytes - byte to be read, invalid if less than / equals 0
    if (fd < 0 || fd >= 8 || buf == NULL || nbytes <= 0) {
        return -1;          // return -1 for failure
    }

    // If the file hasn't been open
    if (cur_pcb->file_descriptor_array[fd].flags == 0) {
        return -1;
    }

    // 1 - the second operation in file_operations_table_ptr.read is "write"
    byte_write = ((cur_pcb->file_descriptor_array)[fd]).file_operations_table_ptr.write(fd, buf, nbytes);

    // Cast byte_write from unsigned int to int for return
    ret = (int32_t)byte_write;

    // Return byte number reads
    return ret;
}

/*
* open
*   DESCRIPTION: open the file corresponding to the given filename
*   INPUTS: filename -- the name of the file to be opened
*   OUTPUTS: none
*   RETURN VALUE: -1 on failure, the index of the file descriptor in the file descriptor array on success
*/
int32_t open(const uint8_t* filename)
{
    pcb* cur_pcb; 
    uint32_t filetype;
    dentry_t dentry;
    int32_t i;
    int32_t full = 0;

    // Validate arguments
    if (filename == NULL) {
        return -1;          // return -1 for failure
    } else {

        // If filename doesn't exist
        if (read_dentry_by_name(filename, &dentry) == -1) {
            return -1;      // return -1 for failure
        }
    }

    // Get pcb structure of current pid_array (pid)
    cur_pcb = get_cur_pcb_ptr();

    // 8 - maximum number of files open in one pid_array
    for (i = 0; i < 8; i++) {

        // 0 - file descriptor is available
        if (cur_pcb->file_descriptor_array[i].flags == 0) {
            break;
        } else {

            // 7 - reaches the last file descriptor, but it is still occupied
            if (i == 7) {

                // 1 - no available file descriptors
                full = 1;
            }
        }
    }

    // If no available file descriptors
    if (full == 1) {
        return -1;
    }

    filetype = dentry.file_type;

    // 0 - rtc
    if (filetype == 0) {
        cur_pcb->file_descriptor_array[i].flags = 1;
        cur_pcb->file_descriptor_array[i].file_position = 0;
        cur_pcb->file_descriptor_array[i].inode = dentry.inode_num;
        cur_pcb->file_descriptor_array[i].file_operations_table_ptr.read = RTC_read;
        cur_pcb->file_descriptor_array[i].file_operations_table_ptr.write = RTC_write;
        cur_pcb->file_descriptor_array[i].file_operations_table_ptr.open = RTC_open;
        cur_pcb->file_descriptor_array[i].file_operations_table_ptr.close = RTC_close;
    } else {
        
        // 1 - directory
        if (filetype == 1) {
            cur_pcb->file_descriptor_array[i].flags = 1;
            cur_pcb->file_descriptor_array[i].file_position = 0;
            cur_pcb->file_descriptor_array[i].inode = dentry.inode_num;
            cur_pcb->file_descriptor_array[i].file_operations_table_ptr.read = dir_read;
            cur_pcb->file_descriptor_array[i].file_operations_table_ptr.write = dir_write;
            cur_pcb->file_descriptor_array[i].file_operations_table_ptr.open = dir_open;
            cur_pcb->file_descriptor_array[i].file_operations_table_ptr.close = dir_close;
        } else {
            
            // 2 - ordinary file
            if (filetype == 2) {
                cur_pcb->file_descriptor_array[i].flags = 1;
                cur_pcb->file_descriptor_array[i].file_position = 0;
                cur_pcb->file_descriptor_array[i].inode = dentry.inode_num;
                cur_pcb->file_descriptor_array[i].file_operations_table_ptr.read = file_read;
                cur_pcb->file_descriptor_array[i].file_operations_table_ptr.write = file_write;
                cur_pcb->file_descriptor_array[i].file_operations_table_ptr.open = file_open;
                cur_pcb->file_descriptor_array[i].file_operations_table_ptr.close = file_close;
            } else {

                // Other filetype values are invalid
                cur_pcb->file_descriptor_array[i].flags = 0;
                return -1;
            }
        }
    }

    cur_pcb->file_descriptor_array[i].file_operations_table_ptr.open(filename);
    
    return i;
}
/*
* close
*   DESCRIPTION: close the file corresponding to the given file descriptor
*   INPUTS: fd -- file descriptor
*   OUTPUTS: none
*   RETURN VALUE: -1 on failure, 0 on success
*/
int32_t close(int32_t fd)
{
    // Get pcb structure of current process (pid)
    pcb* cur_pcb = get_cur_pcb_ptr();
    
    // Validate arguments
    // fd - index to file_descriptor_array of size 8
    // not allow the user to close the default descriptors (0 for input and 1 for output)
    if (fd <= 1 || fd >= 8) {
        return -1;          // return -1 for failure
    }

    // If the file hasn't been open
    if (cur_pcb->file_descriptor_array[fd].flags == 0) {
        return -1;
    }

    // 3 - the fourth operation in file_operations_table_ptr is "read"
    ((cur_pcb->file_descriptor_array)[fd]).file_operations_table_ptr.close(fd);
    ((cur_pcb->file_descriptor_array)[fd]).flags = 0;

    // Return 0 for success
    return 0;
}

/*
* getargs
*   DESCRIPTION: get the arguments of the command
*   INPUTS: buf -- the buffer to which the arguments are copied
*           nbytes -- the number of bytes to be read
*   OUTPUTS: none
*   RETURN VALUE: -1 on failure, 0 on success
*/
int32_t getargs(uint8_t* buf, int32_t nbytes)
{
    // Get current pcb structure
    pcb* cur_pcb = get_cur_pcb_ptr();
    // check if the buffer is valid
    if (buf == NULL || nbytes <= 0 || cur_pcb->args[0] == '\0')
    {
        return -1;
    }
    // check if args are merely copied into the user space, return -1 if not
    if ((int32_t)buf < USER_ADDR || (int32_t)buf > USER_STACK_ADDR)
    {
        return -1;
    }
    // check if the length of args is greater than nbytes, return -1 if so
    if (strlen((int8_t*)cur_pcb->args) > nbytes)
    {
        return -1;
    }
    // copy args to the buffer
    memcpy(buf, cur_pcb->args, nbytes);
    // Return 0 for success
    return 0;
}

/*
* vidmap
*   DESCRIPTION: maps the text-mode video memory into user space at a pre-set virtual address.
*   INPUTS: screen_start -- the pointer to the pre-set virtual address
*   OUTPUTS: none
*   RETURN VALUE: -1 on failure, 0 on success
*/
int32_t vidmap(uint8_t** screen_start)
{
    // check if the screen_start is valid
    if (screen_start == NULL || (uint32_t)screen_start < USER_ADDR || (uint32_t)screen_start > USER_STACK_ADDR)
    {
        return -1;
    }
    // map the video memory in the user space to the physical video memory
    // the virtual address of video memory in the user space starts at 136MB (0x8800000)
    set_pde(page_directory,(USER_VIDEO_ADDR) >> 22,((uint32_t) &user_video_page_table) >> 12,0,0,1);
    // flush the TLB
    flush_tlb();
    // set the screen_start to the video memory
    *screen_start = (uint8_t*)(USER_VIDEO_ADDR);
    // Return 0 for success
    return 0;
}

/*
* set_handler
*   DESCRIPTION: sets signal signum's handler to handler_address
*   INPUTS: signum - the signal number whose handler to be set
*           handler_address - handler's address to set
*   OUTPUTS: set current process's signal signum's handler
*   RETURN VALUE: -1 on failure, 0 on success
*/
int32_t set_handler(int32_t signum, void* handler_address)
{
    // Get current pcb structure of current process
    pcb* cur_pcb = get_cur_pcb_ptr();
    
    // If handler_address is NULL, set to default action
    if (handler_address == NULL) {
        
        // for signal 0, 1 and 2, default action is "kill the task"
        if (signum == 0 || signum == 1 || signum == 2) {
            (cur_pcb->signals[signum]).handler = KILL;
        } else {
            
            // for signal 3 and 4, default action is "ignore"
            if (signum == 3 || signum == 4) {
                (cur_pcb->signals[signum]).handler = IGNORE;
            }
        }
    }

    // signum only valid for 0, 1, 2, 3 and 4
    if (signum < 0 || signum > 4) {
        return -1;
    }
    
    // Set handler
    (cur_pcb->signals[signum]).handler = handler_address;
    
    // 0 - success
    return 0;
}

/*
* sigreturn
*   DESCRIPTION: copies the hardware context that was on the user-level stack back onto the
*                processor
*   INPUTS: none
*   OUTPUTS: copies current hardware context onto the processor
*   RETURN VALUE: -1 on failure, 0 on success
*/
int32_t sigreturn(void)
{
    int32_t i;                                          // Looping index
    uint32_t ebp_k;                                     // Kernel ebp value
    pcb* cur_pcb = get_cur_pcb_ptr();                   // Get current pcb of current process 
    asm volatile ("movl %%ebp, %0" : "=r"(ebp_k));      // Get ebp of hardware context structure
    
    // 20 - offset to get return address on the hardware context structure
    // 4 - offset to get previous hardware context structure on user-level stack frame
    memcpy((hw_context*)(ebp_k + 20), (hw_context*)(((hw_context*)(ebp_k + 20))->esp + 4), sizeof(*((hw_context*)(ebp_k + 20))));
    
    // 5 - total number of signals
    for (i = 0; i < 5; i++) {
        
        // 0 - unmask every signal
        (cur_pcb->signals[i]).mask = 0;
    }
    
    // 0 - success
    return 0;
}

/*
* KILL
*   DESCRIPTION: stop current process
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: -1 on failure, 0 on success
*/
int32_t KILL()
{
    int32_t i;                              // Looping index
    pcb* cur_pcb = get_cur_pcb_ptr();       // Get current pcb of current process
    
    // 5 - total number of signals
    for (i = 0; i < 5; i++) {
        
        // 0 - unmask every signal
        cur_pcb->signals[i].mask = 0;
    }
    
    // Call halt to stop current process
    halt(0);
    
    // 0 - success
    return 0;
}

/*
* IGNORE
*   DESCRIPTION: ignore raised signal
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: -1 on failure, 0 on success
*/
int32_t IGNORE()
{
    int32_t i;                              // Looping index
    pcb* cur_pcb = get_cur_pcb_ptr();       // Get current pcb of current process
    
    // 5 - total number of signals
    for (i = 0; i < 5; i++) {
        
        // 0 - unmask every signal
        cur_pcb->signals[i].mask = 0;
    }
    
    // 0 - success
    return 0;
}

/*
* get_pcb_ptr
*   DESCRIPTION: get the pointer to the process's PCB
*   INPUTS: pid -- the process id
*   OUTPUTS: none
*   RETURN VALUE: the pointer to the process's PCB
*/
pcb* get_pcb_ptr(int32_t pid)
{
    //  the bottom of the 4 MB kernel page is 0x800000
    //  each PCB starts at the top (smaller addr) of an 8KB (0x2000) block inside the kernel, starting at the bottom of kernel page
    return (pcb*)(KERNEL_BOTTOM_ADDR - (pid + 1) * 0x2000);
}

/*
* get_cur_pcb_ptr
*   DESCRIPTION: get the pointer to the current process's PCB
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: the pointer to the current process's PCB
*/
pcb* get_cur_pcb_ptr()
{
    int32_t esp;
    asm volatile ("movl %%esp, %0" : "=r"(esp));
    return get_pcb_ptr((KERNEL_BOTTOM_ADDR - esp) / 0x2000);
}

/*
* get_pid
*   DESCRIPTION: get current process's id number
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: current process's id number
*/
int32_t get_pid() 
{
    int32_t esp;
    asm volatile ("movl %%esp, %0" : "=r"(esp));
    return ((KERNEL_BOTTOM_ADDR - esp) / 0x2000); 
}

/*
* invalid_read
*   DESCRIPTION: invalid read
*   INPUTS: fd -- file descriptor
*           buf -- the buffer to be read
*           nbytes -- the number of bytes to be read
*   OUTPUTS: none
*   RETURN VALUE: -1
*/
int32_t invalid_read(int32_t fd, void* buf, int32_t nbytes)
{
    return -1;
}

/*
* invalid_write
*   DESCRIPTION: invalid write
*   INPUTS: fd -- file descriptor
*           buf -- the buffer to be written
*           nbytes -- the number of bytes to be written
*   OUTPUTS: none
*   RETURN VALUE: -1
*/
int32_t invalid_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return -1;
}

/*
* invalid_open
*   DESCRIPTION: invalid open
*   INPUTS: filename -- the name of the file to be opened
*   OUTPUTS: none
*   RETURN VALUE: -1
*/
int32_t invalid_open(const uint8_t* filename)
{
    return -1;
}

/*
* invalid_close
*   DESCRIPTION: invalid close
*   INPUTS: fd -- file descriptor
*   OUTPUTS: none
*   RETURN VALUE: -1
*/
int32_t invalid_close(int32_t fd)
{
    return -1;
}

/*
* flush_tlb
*   DESCRIPTION: flush the TLB
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*/
void flush_tlb()
{
    asm volatile(
        "movl %%cr3, %%eax;"
        "movl %%eax, %%cr3;"
        :
        :
        : "eax"
    );
}
