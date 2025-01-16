#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "terminal.h"
#include "system_calls.h"
#include "cursor.h"



uint8_t ctrl_pressed = 0;           // ctrl key pressed down
uint8_t shift_pressed = 0;          // shift key pressed down
uint8_t capital = 0;                // capital status on
uint8_t caps_lock_pressed = 0;      // cap lock key pressed down
uint8_t alt_pressed = 0;            // alt key pressed down

// matching scan code for each key, 58 scan codes in total
// using PS/2 scan code set 1 (for a "US QWERTY" keyboard only) from 0x00 to 0x39
// currently only support lower case letters and numbers
char scan_code_match[58] = 
{
	'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	'\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	'[', ']', '\n', '\0',
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l' ,
	';', '\'', '`', '\0', '\\',
	'z', 'x', 'c', 'v', 'b', 'n', 'm',
	',', '.', '/', '\0', '\0', '\0', ' ',
};

// matching scan code for each key, 58 scan codes in total
// currently support both upper and lower case letters, numbers and special characters
// when shift key pressed
char scan_code_shift[58] = 
{
	'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	'\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
	'{', '}', '\n', '\0',
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L' ,
	':', '\"', '`', '\0', '\\',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M',
	'<', '>', '?', '\0', '\0', '\0', ' ',
};

// matching scan code for each key, 58 scan codes in total
// currently support both upper and lower case letters, numbers and special characters
// when caps lock key on
char scan_code_capital[58] = 
{
	'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	'\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
	'{', '}', '\n', '\0',
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L' ,
	':', '\"', '`', '\0', '\\',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M',
	'<', '>', '?', '\0', '\0', '\0', ' ',
};

/*
* keyboard_init
*   DESCRIPTION: initialize keyboard
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: enable keyboard interrupt
*/
void keyboard_init()
{
    // enable keyboard interrupt, keyboard interrupt is IRQ1
    enable_irq(1);
}

/*
* keyboard_handler
*   DESCRIPTION: handler for keyboard interrupt
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: print the corresponding character for the pressed key
*/
void keyboard_handler()
{
    // cli();
    send_eoi(1);        // keyboard interrupt is IRQ1
    uint8_t scan_code;
    scan_code = inb(KEYBOARD_DATA_PORT);

    if (scan_code == L_SHIFT_PRESS || scan_code == R_SHIFT_PRESS) {

        // 1 - shift key is pressed down
        shift_pressed = 1;
        return;
    }

    if (scan_code == L_SHIFT_RELEASE || scan_code == R_SHIFT_RELEASE) {

        // 0 - shift key is released
        shift_pressed = 0;
        return;
    }

    if (scan_code == CTRL_PRESS) {

        // 1 - ctrl key is pressed down
        ctrl_pressed = 1;
        return;
    }

    if (scan_code == CTRL_RELEASE) {

        // 0 - ctrl key is released
        ctrl_pressed = 0;
        return;
    }

    if (scan_code == CAPS_LOCK_PRESS) {
        if (caps_lock_pressed == 0) {

            // 1 - caps lock key is pressed down
            // switch the status of caps lock by 1 - capital
            capital = 1 - capital;
            caps_lock_pressed = 1;
        }
        return;
    }

    if (scan_code == CAPS_LOCK_RELEASE) {

        // 0 - caps lock key is released
        caps_lock_pressed = 0;
        return;
    }
    
    if (scan_code == ALT_PRESS) {
        alt_pressed = 1;
        return;
    }
    
    if (scan_code == ALT_RELEASE) {
        alt_pressed = 0;
        return;
    }
    
    if (scan_code == F1 && alt_pressed == 1) {
        if (cur_terminal == 0) {
            return;
        } else {

            // 0 - press F1 then switch to terminal 0
            memory_switch(cur_terminal);

            // 2 - offset from VIDEO to VIDEO backup buffer
            // 4096 - size of VIDEO as well as its buffers
            memcpy((void*)(VIDEO + (cur_terminal + 2) * 4096), (void*)VIDEO, 4096);
            memcpy((void*)VIDEO, (void*)(VIDEO + 2 * 4096), 4096);

            // 0 - press F1 then switch to terminal 0
            update_cursor(terminal[0].cursor_x, terminal[0].cursor_y); 
            cur_terminal = 0; 
            memory_switch(run_terminal);

            return;
        }
    }
    
    if (scan_code == F2 && alt_pressed == 1) {
        if (cur_terminal == 1) {
            return;
        } else {

            // 1 - press F2 then switch to terminal 1
            memory_switch(cur_terminal);

            // 2 - offset from VIDEO to VIDEO backup buffer
            // 4096 - size of VIDEO as well as its buffers
            memcpy((void*)(VIDEO + (cur_terminal + 2) * 4096), (void*)VIDEO, 4096);
            memcpy((void*)VIDEO, (void*)(VIDEO + 3 * 4096), 4096);

            // 1 - press F2 then switch to terminal 1
            update_cursor(terminal[1].cursor_x, terminal[1].cursor_y);
            cur_terminal = 1;  
            memory_switch(run_terminal);

            return;
        }
    }
    
    if (scan_code == F3 && alt_pressed == 1) {
        if (cur_terminal == 2) {
            return;
        } else {

            // 2 - press F3 then switch to terminal 2
            memory_switch(cur_terminal);

            // 2 - offset from VIDEO to VIDEO backup buffer
            // 4096 - size of VIDEO as well as its buffers
            memcpy((void*)(VIDEO + (cur_terminal + 2) * 4096), (void*)VIDEO, 4096);
            memcpy((void*)VIDEO, (void*)(VIDEO + 4 * 4096), 4096);

            // 2 - press F3 then switch to terminal 2
            update_cursor(terminal[2].cursor_x, terminal[2].cursor_y);  
            cur_terminal = 2;
            memory_switch(run_terminal);

            return;
        }
    }

    if (scan_code == BACKSPACE) {

        // 0 - the start of keyboard buffer
        if (terminal[cur_terminal].index != 0) {
            putc(8, 1);                                // 8 - ASCII code for backspace
            // terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index - 1] = '\0';
            terminal[cur_terminal].index--;            // Delete one location
        }
        return;
    }

    // Not used function keys, return without printing on the screen
    if (scan_code == TAB_PRESS || scan_code == ESC) {
        return;
    }

    // 58 - number of keys supported
    if (scan_code < 58) {

        // If ctrl key pressed
        if (ctrl_pressed == 1) {
            
            // ctrl + l / ctrl + L
            if (scan_code_match[scan_code] == 'l'){
                
                // Clearing the screen without resetting the buffer
                clear();
                
                // Return without printing on the screen
                return;
            }
        }

        if (shift_pressed == 1 && shift_pressed == capital) {

            // Handling buffer overflow
            // 127 - maximum char number excluding new line
            if ((terminal[cur_terminal].index >= 127) && scan_code_match[scan_code] != '\n') {
                return;
            }

            if (scan_code_match[scan_code] == '\n') {

                // 1 - display the character on current terminal instead of running terminal
                putc(scan_code_match[scan_code], 1);
                terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index++] = '\n';
                
                // 1 - enter key is pressed 
                terminal[cur_terminal].enter_pressed = 1;	
                return;						
            } 

            if (scan_code_match[scan_code] >= 'a' && scan_code_match[scan_code] <= 'z') {

                // 1 - display the character on current terminal instead of running terminal
                putc(scan_code_match[scan_code], 1);
				terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index] = scan_code_match[scan_code];
                terminal[cur_terminal].index++;
                return;
            } else {

                // 1 - display the character on current terminal instead of running terminal
                putc(scan_code_shift[scan_code], 1);
				terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index] = scan_code_match[scan_code];
                terminal[cur_terminal].index++;
                return;
            }
        }

        if (shift_pressed == 1 && shift_pressed != capital) {

            // Handling buffer overflow
            // 127 - maximum char number excluding new line
            if ((terminal[cur_terminal].index >= 127) && scan_code_match[scan_code] != '\n') {
                return;
            }

            if (scan_code_match[scan_code] == '\n') {

                // 1 - display the character on current terminal instead of running terminal
                putc(scan_code_match[scan_code], 1);
                terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index++] = '\n';
                
                // 1 - enter key is pressed 
                terminal[cur_terminal].enter_pressed = 1;	
                return;						
            } 
            
            // 1 - display the character on current terminal instead of running terminal
            putc(scan_code_shift[scan_code], 1);
			terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index] = scan_code_match[scan_code];
            terminal[cur_terminal].index++;
            return;
        }

        if (shift_pressed != 1 && shift_pressed != capital) {

            // Handling buffer overflow
            // 127 - maximum char number excluding new line
            if ((terminal[cur_terminal].index >= 127) && scan_code_match[scan_code] != '\n') {
                return;
            }

            if (scan_code_match[scan_code] == '\n') {

                // 1 - display the character on current terminal instead of running terminal
                putc(scan_code_match[scan_code], 1);
                terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index++] = '\n';
                
                // 1 - enter key is pressed 
                terminal[cur_terminal].enter_pressed = 1;	
                return;						
            } 
            
            // 1 - display the character on current terminal instead of running terminal
            putc(scan_code_capital[scan_code], 1);
			terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index] = scan_code_match[scan_code];
            terminal[cur_terminal].index++;

            return;
        }

		if (scan_code_match[scan_code] == '\n') {

            // 1 - display the character on current terminal instead of running terminal
			putc(scan_code_match[scan_code], 1);
			terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index++] = '\n';

            // 1 - enter key is pressed 
			terminal[cur_terminal].enter_pressed = 1;		

            return;					
		} 
        
        if (scan_code_match[scan_code] != '\0') {

            // 127 - maximum char number excluding new line
			if (terminal[cur_terminal].index < 127) {

                // 1 - display the character on current terminal instead of running terminal
				putc(scan_code_match[scan_code], 1);
				terminal[cur_terminal].keyboard_buffer[terminal[cur_terminal].index++] = scan_code_match[scan_code];
			}
		}
    }
    // sti();
}
