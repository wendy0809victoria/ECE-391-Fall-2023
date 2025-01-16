/* assembly_linkage.h - Defines for various assembly linkage functions
*/
#ifndef ASSERMBLY_LINKAGE_H
#define ASSERMBLY_LINKAGE_H
#include "types.h"
#include "rtc.h"
#include "keyboard.h"
#include "idt.h"
#include "system_calls.h"
#include "pit.h"
// interrupt linkage
extern void rtc_handler_linkage();
extern void keyboard_handler_linkage();
extern void pit_handler_linkage();
// exception linkage
extern void divided_error_handler_linkage();
extern void debug_handler_linkage();
extern void nmi_handler_linkage();
extern void breakpoint_handler_linkage();
extern void overflow_handler_linkage();
extern void bounds_check_handler_linkage();
extern void invalid_opcode_handler_linkage();
extern void device_not_available_handler_linkage();
extern void double_fault_handler_linkage();
extern void coprocessor_segment_overrun_handler_linkage();
extern void invalid_tss_handler_linkage();
extern void segment_not_present_handler_linkage();
extern void stack_exception_handler_linkage();
extern void general_protection_fault_handler_linkage();
extern void page_fault_handler_linkage();
extern void reserved_handler_linkage();
extern void floating_point_error_handler_linkage();
extern void alignment_check_handler_linkage();
extern void machine_check_handler_linkage();
extern void simd_floating_point_handler_linkage();

// system call linkage
extern void system_call_handler_linkage();


#endif
