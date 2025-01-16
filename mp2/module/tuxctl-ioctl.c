/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

int ack;		// Check if MTCP_ACK returned
unsigned char button;		// Store the value of pressed button
int led;			// Store the value of led when resetting

/* int tux_init(struct tty_struct* tty, unsigned long arg)
 * Input - Tux controller tty, a 32-bit argument (ignored)
 * Output - None
 * Return value - return 0 for success
 */
int tux_init (struct tty_struct* tty, unsigned long arg);

/* int tux_buttons(struct tty_struct* tty, unsigned long arg)
 * Input - Tux controller tty, a 32-bit argument storing the information of button pressed
 * Output - None
 * Return value - return 0 for success
 */
int tux_buttons (struct tty_struct* tty, unsigned long arg);

/* int tux_set_led (struct tty_struct* tty, unsigned long arg)
 * Input - Tux controller tty, a 32-bit argument storing the information of led value
 * Output - None
 * Return value - return 0 for success
 */
int tux_set_led (struct tty_struct* tty, unsigned long arg);


/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;
	unsigned char direction;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

	if (a == MTCP_ACK) {
		ack = 1;		// Permitting commands
	}

	if (a == MTCP_BIOC_EVENT) {
		direction = (~c) & 0x0F;		// Active low response, 0x0F - acquire low 4 bits of the first byte
		// 0x02 - if left button pressed
		// 0x04 - if down button pressed
		// 0x09 - if right/up button pressed
		button = ((((direction & 0x02) << 1) | ((direction & 0x04) >> 1) | (direction & 0x09)) << 4);
		button = button | ((~b) & 0x0F);
	}

	// Reinitialize and store led when resetting
	if (a == MTCP_RESET) {
		// 0 - to be ignored when calling tux_init
		tux_init (tty, 0);
		tux_set_led (tty, led);
	}

    /*printk("packet : %x %x %x\n", a, b, c); */
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:
		tux_init (tty, arg);
		return 0;
	case TUX_BUTTONS:
		tux_buttons (tty, arg);
		return 0;
	case TUX_SET_LED:
		tux_set_led (tty, arg);
		return 0;
	case TUX_LED_ACK:
	case TUX_LED_REQUEST:
	case TUX_READ_LED:
	default:
	    return -EINVAL;
    }
}


/* int tux_init()
 * Input - Tux controller tty, a 32-bit argument (ignored)
 * Output - None
 * Return value - return 0 for success
 */
int
tux_init (struct tty_struct* tty, unsigned long arg) 
{
	unsigned char buf_button[1];		// Send button command 
	unsigned char buf_led[1];			// Send led command
	buf_button[0] = MTCP_BIOC_ON;
	buf_led[0] = MTCP_LED_USR;			// Set user mode to change led values
	tuxctl_ldisc_put (tty, buf_button, 1);
	tuxctl_ldisc_put (tty, buf_led, 1);

	// Initialize all variables defined
	led = 0;
	ack = 0;
	button = 0;

	// Success
	return 0;
}


/* int tux_buttons(struct tty_struct* tty, unsigned long arg)
 * Input - Tux controller tty, a 32-bit argument storing the information of button pressed
 * Output - None
 * Return value - return 0 for success
 */
int
tux_buttons (struct tty_struct* tty, unsigned long arg) 
{
	// Change unsigned long arg for a pointer
	if ((uint32_t*)arg == NULL) {
		// Invalid argument
		return -EINVAL;
	}
	copy_to_user ((uint32_t*)arg, (uint32_t*)(&button), sizeof(uint32_t));
	return 0;
}


/* int tux_set_led (struct tty_struct* tty, unsigned long arg)
 * Input - Tux controller tty, a 32-bit argument storing the information of led value
 * Output - None
 * Return value - return 0 for success
 */
int
tux_set_led (struct tty_struct* tty, unsigned long arg) 
{
	unsigned long ct = 2;		// Initial value of buf number of layers
    unsigned long i;
	// 0x000F0000 - low 4 bits of the third byte stores which led is set
    unsigned long which = ((arg & 0x000F0000) >> 16);
	// 0x000F0000 - low 4 bits of the last byte stores which led's decimal point is set
    unsigned long decimal = ((arg & 0x0F000000) >> 24);

	unsigned char buf_usr[1];			// Set user mode to change led values
    unsigned char buf[6];				// Send MTCP_LED_SET command and bit mask of the 4 leds
	unsigned char number[16];
    buf_usr[0] = MTCP_LED_USR;
    tuxctl_ldisc_put (tty, buf_usr, 1);
    buf[0] = MTCP_LED_SET;
    buf[1] = which;

	// Number 0 to F (16 numbers) appearing on the leds
    number[0] = 0xE7;
    number[1] = 0x06;
    number[2] = 0xCB;
    number[3] = 0x8F;
    number[4] = 0x2E;
    number[5] = 0xAD;
    number[6] = 0xED;
    number[7] = 0x86;
    number[8] = 0xEF;
    number[9] = 0xAF;
    number[10] = 0xEE;
    number[11] = 0x6D;
    number[12] = 0xE1;
    number[13] = 0x4F;
    number[14] = 0xE9;
    number[15] = 0xE8;
	
	// If command is not permitted / finishes, return
	if (ack == 0){
	 	return 0;
	}
	ack = 0;

	// 4 - 4 leds in total
	for (i = 0; i < 4; i++){
		// 0x01 - Determine whether the led is on after shifting corresponding bit
		if (((buf[1] >> i) & 0x01) != 0){
			buf[ct] = (number[(arg >> (i * 4)) & 0x0F]) | (((decimal & (1 << i)) > 0));
			ct++;
		}
	}
	tuxctl_ldisc_put(tty, buf, ct);

	// Store led value for resetting
	led = arg;
	
	// Success
	return 0;
}

