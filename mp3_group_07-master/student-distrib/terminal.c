//
//  terminal.c
//  
//
//  Created by Wendi Wang on 2023/10/21.
//

#include "terminal.h"
#include "system_calls.h"
#include "page.h"
#include "cursor.h"

struct terminal_t terminal[3];              // Terminal window
int32_t cur_terminal = 0;                   // Current terminal id

/*
* terminal_open
*   DESCRIPTION: Open terminal driver
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: 0 for success
*   SIDE EFFECTS: none
*/
int32_t terminal_open(const uint8_t* filename)
{
    return 0;       // 0 - represents success
}

/*
* terminal_close
*   DESCRIPTION: Close terminal driver
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: 0 for success
*   SIDE EFFECTS: Clear keyboard buffer and terminal window buffer
*/
int32_t terminal_close(int32_t fd)
{
    // 0 - Start from the first location in the buffer
    terminal[run_terminal].index = 0;
    
    // 128 - maximum charater number from keyboard
    // Clear terminal window
    memset(terminal[run_terminal].keyboard_buffer, 0, 128);
    
    return 0;       // 0 - return value for success
}

/*
* terminal_read
*   DESCRIPTION: Read from keyboard buffer
*   INPUTS: arg - argument from file system
*           buffer - pointer to the buffer read
*           number - byte number to be read
*   OUTPUTS: none
*   RETURN VALUE: byte number read for success, -1 for invalid arguments
*   SIDE EFFECTS: none
*/
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes)
{
    // If buffer pointer is invalid or byte number to read is invalid, return -1 for failure
    if (buf == NULL || nbytes < 0) {
        return -1;      // -1 - return value for failure
    }
    
    char* buffer = (char*)buf;      // Convert buffer from void* to char*
    int ct;                         // ct - loop count while reading
    int ret = 0;                    // ret - record number of bytes read
    int i;                          // i - loop count while adding 0s to the end of buffer
    int last = 0;                   // last - "binary" variable to indicate '\n' appearance
    
    memset(terminal[run_terminal].keyboard_buffer, 0, 128);	
	terminal[run_terminal].index = 0;	

    // Initialize enter_pressed to be 0
    terminal[run_terminal].enter_pressed = 0;                // 0 - enter key not pressed down
    while (!(terminal[run_terminal].enter_pressed)) {}       // Wait until enter key pressed down
    
    // Loop to read
    for (ct = 0; ct < nbytes; ct++) {
        
        // Buffer overflow, 128 - maximum charater number from keyboard
        if (ct >= 128) {
            return ret;
        } else {
            
            // If new line reached
            if (terminal[run_terminal].keyboard_buffer[ct] == '\n') {
                last = 1;
                buffer[ct] = terminal[run_terminal].keyboard_buffer[ct];
                ret++;
                break;
            } else {
                
                // Read from keyboard buffer
                buffer[ct] = terminal[run_terminal].keyboard_buffer[ct];
                ret++;
            }
        }
    }
    
    // New line reached before reading 128 characters
    // 1 - New line reached
    if (last == 1) {
        for (i = ct + 1; i < nbytes; i++) {
            
            // Padding 0 to obtain 128 characters
            buffer[ct] = '\0';
        }
    }
    
    // Return the number of bytes read
    return ret;
}


/*
* terminal_write
*   DESCRIPTION: Write to the terminal window
*   INPUTS: arg - argument from file system
*           buffer - pointer to the buffer for writing
*           number - byte number written
*   OUTPUTS: none
*   RETURN VALUE: byte number written for success, -1 for invalid arguments
*   SIDE EFFECTS: none
*/
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes)
{
    // If buffer pointer is invalid or byte number to read is invalid, return -1 for failure
    if (buf == NULL || nbytes < 0) {
        return -1;      // -1 - return value for failure
    }
    
    char* buffer = (char*)buf;      // Convert buffer from void* to char*
    int ct;                         // ct - loop count while reading
    
    // Loop to write
    for (ct = 0; ct < nbytes; ct++) {
        
        // Write byte to the terminal window
        putc(buffer[ct], 0);
    }
    
    // Return the number of bytes written
    return ct;
}


/*
* terminal_init
*   DESCRIPTION: Initialize terminal driver
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: 0 for success
*   SIDE EFFECTS: none
*/
int terminal_init() 
{
    int i;
    
    // Loop through each terminal (three terminals in total)
    for (i = 0; i < 3; i++) {
        
        // Initialize various attributes of the terminal structure
        terminal[i].index = 0;              // 0 - starting position of keyboard buffer
        terminal[i].enter_pressed = 0;      // 0 - enter key is not pressed
        terminal[i].cursor_x = 0;           // 0 - starting position of cursor
        terminal[i].cursor_y = 0;           // 0 - starting position of cursor
        enable_cursor(0, 14);
        update_cursor(terminal[i].cursor_x, terminal[i].cursor_y);
    }
    
    return 0;
}


/*
* memory_switch
*   DESCRIPTION: Set user video page when switching terminals
*   INPUTS: switch_id - terminal id to switch to
*   OUTPUTS: none
*   RETURN VALUE: 0 for success
*   SIDE EFFECTS: none
*/
int32_t memory_switch(int32_t switch_id) 
{
    // If terminal id to be switch to is current terminal id
    if (switch_id == cur_terminal) {

        // 12 - 4kB size
        set_pte(page_table, VIDEO >> 12, VIDEO >> 12, 0);

        // 0x003FF000 - take middle 10 bits
		set_pte(user_video_page_table, (USER_VIDEO_ADDR & 0x003FF000) >> 12, VIDEO >> 12, 1);
    } else {

        // 12 - 4kB size
        // 2 - offset of terminal video backup buffers
        set_pte(page_table, VIDEO >> 12, (VIDEO >> 12) + 2 + switch_id, 0);

        // 0x003FF000 - take middle 10 bits
		set_pte(user_video_page_table, (USER_VIDEO_ADDR & 0x003FF000) >> 12, (VIDEO >> 12) + 2 + switch_id, 1);
    }

    // Update cr3 after setting page table and user video page table
    flush_tlb();
    
    return 0;
}
