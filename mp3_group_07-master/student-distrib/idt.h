/* idt.h - Defines used in working with the IDT (interrupt descriptor table)
*/
#ifndef IDT_H
#define IDT_H

extern void idt_init();

extern void exception_divided_error();
extern void exception_debug();
extern void exception_nmi_interrupt();
extern void exception_breakpoint();
extern void exception_overflow();
extern void exception_bound_range_exceeded();
extern void exception_invalid_opcode();
extern void exception_device_not_available();
extern void exception_double_fault();
extern void exception_coprocessor_segment_overrun();
extern void exception_invalid_tss();
extern void exception_segment_not_present();
extern void exception_stack_segment_fault();
extern void exception_general_protection();
extern void exception_page_fault();
extern void exception_x87_floating_point_exception();
extern void exception_alignment_check();
extern void exception_machine_check();
extern void exception_simd_floating_point_exception();
extern void system_call_handler();

#endif
