//
//  cursor.h
//  
//
//  Created by Wendi Wang on 2023/10/21.
//

#ifndef cursor_h
#define cursor_h

#include "types.h"
#include "lib.h"

/* Enabling the Cursor */
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);

/* Disabling the Cursor */
void disable_cursor();

/* Moving the Cursor */
void update_cursor(int x, int y);

/* Get Cursor Position */
uint16_t get_cursor_position(void);

#endif /* cursor_10210850_h */
