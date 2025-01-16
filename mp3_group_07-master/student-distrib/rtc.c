#include "rtc.h"
#include "lib.h"
#include "i8259.h"
#include "system_calls.h"

// the default frequency is 1024 Hz
#define FREQ 1024
// the default rate is 6
#define RATE 6

// the flag to check if rtc interrupt happens
static int32_t rtc_flag = 0;
volatile uint32_t rtc_counter;
// virtualization not implemented yet

// 3 - total number of terminal
int32_t freq[3] = {2, 2, 2};            // 2 - default frequency
int32_t intr[3] = {0, 0, 0};            // 0 - interrupt not happen yet

/*
* rtc_init
*   DESCRIPTION: initialize the rtc
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: 0
*   SIDE EFFECTS: none
 */
void rtc_init(void)
{
    // disable interrupts
    // cli();
    char prev;
    // select register B, and disable NMI
    outb(0x8B, RTC_PORT);
    // read the current value of register B
    prev = inb(RTC_CMOS_PORT);
    // reset the index again (a read will reset the index to register D)
    outb(0x8B, RTC_PORT);
    // write the previous value ORed with 0x40. This turns on bit 6 of register B
    outb(prev | 0x40, RTC_CMOS_PORT);
    // initialize rtc_counter
    rtc_counter = 0;
    // enable PIC's 8th irq
    enable_irq(RTC_IRQ);
    // enable interrupts
    // sti();
}

/*
*  rtc_handler
*   DESCRIPTION: RTC interrupt handler
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: none
*/
void rtc_handler(void)
{
    // disable interrupts
    cli();
    int32_t i;
    // increment rtc_counter by 1 each time an interrupt occurs
    rtc_counter++;
    
    //  test for RTC
    // For checkpoint 1, OS must execute the test interrupts handler when an RTC interrupt occurs.
    // test_interrupts();

    // 3 - total number of terminal
    for (i = 0; i < 3; i++) {
        if (rtc_counter % (1024 / freq[i]) == 0) {
            intr[i] = 1;
        }
    }

    // select register C, and enable NMI, if register C is not read after an IRQ 8, then the interrupt will not happen again
    outb(0x0C, RTC_PORT);
    // read from the register C and throw away contents
    inb(RTC_CMOS_PORT);
    rtc_flag = 1;
    // send EOI to PIC
    send_eoi(RTC_IRQ);
    // enable interrupts
    sti();
}

/*
*  RTC_set_freq
*   DESCRIPTION: RTC set frequence
*   INPUTS: rate
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: none
*/
// Refrence from: https://wiki.osdev.org/RTC 
void RTC_set_freq(int32_t rate)
{
    char prev;
    cli();
    rate &= 0x0F;
    if(rate < 6 )
    {
        rate = 6;
    }			                // rate must be above 6 and not over 15(max freq is 1024)
    outb(0x8A, RTC_PORT);		// set index to register A, disable NMI
    prev = inb(RTC_CMOS_PORT);	// get initial value of register A
    outb(0x8A, RTC_PORT);		// reset index to A
    outb((prev & 0xF0) | rate, RTC_CMOS_PORT); //write only our rate to A. Note, rate is the bottom 4 bits.
    sti();
}

/*
*  freq_to_rate
*   DESCRIPTION: change frequency to rate
*   INPUTS: frequency
*   OUTPUTS: rate
*   RETURN VALUE: rate
*   SIDE EFFECTS: none
*/
int32_t freq_to_rate(int32_t freq)
{
    switch (freq)
    {
        case 2:     return 0x0F;
        case 4:     return 0x0E;
        case 8:     return 0x0D;
        case 16:    return 0x0C;
        case 32:    return 0x0B;
        case 64:    return 0x0A;
        case 128:   return 0x09;
        case 256:   return 0x08;
        case 512:   return 0x07;
        case 1024:  return 0x06;
        default:    return -1;      // frequency out of range
    }
}

/*
*  RTC_open
*   DESCRIPTION: RTC_open
*   INPUTS: 
*        filename: file name
*   OUTPUTS: none
*   RETURN VALUE: 0 if success, -1 if fail
*   SIDE EFFECTS: none
*/
int32_t RTC_open(const uint8_t * filename)
{
    if(filename == NULL)
    {
        return -1;
    }

    freq[run_terminal] = 2; // set freq to 2
    intr[run_terminal] = 0; // 0 - interrupt not happen yet

    return 0;
}

/*
*  RTC_read
*   DESCRIPTION: RTC_read
*   INPUTS: 
*        fd: file descriptor
*        buf: buffer
*        nbytes: number of bytes
*   OUTPUTS: none
*   RETURN VALUE: 0
*   SIDE EFFECTS: none
*/
int32_t RTC_read(int32_t fd, void * buf, int32_t nbytes)
{
    rtc_flag = 0;
    while(intr[run_terminal] == 0) {/* stay here */}
	intr[run_terminal] = 0;
    return 0;
}

/*
*  RTC_write
*   DESCRIPTION: RTC_write
*   INPUTS:
*        fd: file descriptor
*        buf: buffer
*        nbytes: number of bytes
*   OUTPUTS: none
*   RETURN VALUE: 0 if success, -1 if fail
*   SIDE EFFECTS: none
*/
int32_t RTC_write(int32_t fd, const void * buf, int32_t nbytes)
{
    int32_t frequency;
    int32_t rate;
    if(buf == NULL)
    {
        return -1;
    }
    frequency = *((int32_t*)buf);    // get freq, must be power of 2
    rate = freq_to_rate(frequency);  // get rate
    if (rate == -1)
    {
        return -1; // fail
    }
    else
    {
        freq[run_terminal] = frequency;
        return 0;
    }
}    
/*
*  RTC_close
*   DESCRIPTION: RTC_close
*   INPUTS: 
*        fd: file descriptor
*   OUTPUTS: none
*   RETURN VALUE: 0 
*   SIDE EFFECTS: none
*/
int32_t RTC_close(int32_t fd)
{
    return 0;
}
