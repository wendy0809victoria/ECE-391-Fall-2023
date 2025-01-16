//
//  terminal.h
//  
//
//  Created by Wendi Wang on 2023/10/21.
//

#ifndef terminal_h
#define terminal_h

#include "types.h"
#include "lib.h"
#include "keyboard.h"

typedef struct terminal_t {    
    char keyboard_buffer[128];      // 128 - maximum charater number from keyboard
    volatile int enter_pressed;     // If enter_pressed, read from terminal
    int cursor_x;                   // x location of the cursor
    int cursor_y;                   // y location of the cursor
    int index;                      // Next buffer location to put the new character
} terminal_t;

// Open terminal driver
int32_t terminal_open(const uint8_t* filename);

// Close terminal driver
int32_t terminal_close(int32_t fd);

// Read from keyboard
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

// Write to terminal
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

// Initialize terminal driver
int32_t terminal_init();

// Switch to terminal[switch_id] from terminal[cur_terminal]
int32_t memory_switch(int32_t switch_id);

extern terminal_t terminal[3];      // Terminal window
extern int32_t cur_terminal;        // Current terminal id

#endif /* terminal_h */
