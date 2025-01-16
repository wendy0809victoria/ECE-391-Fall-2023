/* rtc.h - Defines used in interactions with the RTC (real time clock)
 */
#ifndef RTC_H
#define RTC_H
#include "types.h"

// RTC_PORT to specify an index or "register number", and to disable NMI
#define RTC_PORT 0x70
// RTC_CMOS_PORT to read or write from/to the CMOS
#define RTC_CMOS_PORT 0x71
// The IRQ number of the RTC is 8
#define RTC_IRQ 8
//  RTC Status Register A, B, and C at offset 0xA, 0xB, and 0xC in the CMOS RAM
// disable NMI(non-maskable-interrupt) by setting the 0x80 bit, 
#define RTC_REG_A 0x8A
#define RTC_REG_B 0x8B
#define RTC_REG_C 0x8C

#define RTC_RATE_2      0xf // 2 Hz
#define RTC_RATE_1024   0x6 // 1024 Hz

// initialize the RTC
extern void rtc_init(void);
// RTC interrupt handler
extern void rtc_handler(void);
// 3.2
extern int32_t RTC_open(const uint8_t * filename);
extern int32_t RTC_read(int32_t fd, void * buf, int32_t nbytes);
extern int32_t RTC_write(int32_t fd, const void * buf, int32_t nbytes);
extern int32_t RTC_close(int32_t fd);
extern void RTC_set_freq(int32_t rate);
extern int32_t freq_to_rate(int32_t freq);

extern int32_t freq[3];
extern int32_t intr[3];

#endif
