//
//  cursor_10210850.c
//  
//
//  Created by Wendi Wang on 2023/10/21.
//

#include "cursor.h"

/*
* enable_cursor
*   DESCRIPTION: Enabling the cursor
*   INPUTS: cursor_start - start position of the cursor
*           cursor_end - end position of the cursor
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: none
*/
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    
    // 0x3D4 - VGA control register (cursor start register)
    // 0x0A - Select the index of the cursor start register
    // 0x3D5 - VGA control register (cursor end register)
    // 0x0B - Select the index of the cursor end register
    // 0xC0 - Clear Low
    // 0xE0 - Clear Low
    
    outb(0x0A, 0x3D4);
    outb((inb(0x3D5) & 0xC0) | cursor_start, 0x3D5);
 
    outb(0x0B, 0x3D4);
    outb((inb(0x3D5) & 0xE0) | cursor_end, 0x3D5);
}


/*
* disable_cursor
*   DESCRIPTION: Disabling the cursor
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: none
*/
void disable_cursor() {
    
    // 0x3D4 - VGA control register (cursor start register)
    // 0x3D5 - VGA control register (cursor end register)
    // 0x0A - Select the index of the cursor start register
    // 0x20 - Exceed the screen range, thereby hiding the cursor
    
    outb(0x0A, 0x3D4);
    outb(0x20, 0x3D5);
}


/*
* update_cursor
*   DESCRIPTION: Updating cursor position
*   INPUTS: x - x position of the cursor
*           y - y position of the cursor
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: none
*/
void update_cursor(int x, int y) {
    
    // 0x3D4 - VGA control register (cursor start register)
    // 0x0F - Select the index of the cursor start register
    // 0x3D5 - VGA control register (cursor end register)
    // 0x0E - Select the index of the cursor end register
    // 0xFF - Choose low 8 bits

    uint16_t pos = y * 80 + x;                    // 80 - screen width
 
    outb(0x0F, 0x3D4);
    outb((uint8_t) (pos & 0xFF), 0x3D5);
    outb(0x0E, 0x3D4);
    outb((uint8_t) ((pos >> 8) & 0xFF), 0x3D5);   // 8 - offset
}


/*
* get_cursor_position
*   DESCRIPTION: Get cursor position
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: position offset of the cursor
*   SIDE EFFECTS: none
*/
uint16_t get_cursor_position(void) {
    
    // 0x3D4 - VGA control register (cursor start register)
    // 0x0F - Select the index of the cursor start register
    // 0x3D5 - VGA control register (cursor end register)
    // 0x0E - Select the index of the cursor end register
    
    uint16_t pos = 0;
    outb(0x0F, 0x3D4);
    pos |= inb(0x3D5);
    outb(0x0E, 0x3D4);
    pos |= ((uint16_t)inb(0x3D5)) << 8;           // 8 - offset
    return pos;
}
