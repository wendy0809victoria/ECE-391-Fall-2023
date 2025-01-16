#ifndef KEYBOARD_H
#define KEYBOARD_H
#include "types.h"
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

#define L_SHIFT_PRESS       0x2A
#define L_SHIFT_RELEASE     0xAA
#define R_SHIFT_PRESS       0x36
#define R_SHIFT_RELEASE     0xB6
#define CTRL_PRESS          0x1D
#define CTRL_RELEASE        0x9D
#define CAPS_LOCK_PRESS     0x3A
#define CAPS_LOCK_RELEASE   0xBA
#define ALT_PRESS           0x38
#define ALT_RELEASE         0xB8
#define BACKSPACE           0x0E
#define TAB_PRESS           0x0F 
#define ESC                 0x01
#define F1                  0x3B
#define F2                  0x3C
#define F3                  0x3D

extern void keyboard_init();
extern void keyboard_handler();

extern char scan_code_match[58];
extern char scan_code_shift[58];
extern char scan_code_capital[58];
#endif

