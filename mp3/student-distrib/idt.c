#include "idt.h"
#include "lib.h"
#include "x86_desc.h"
#include "assembly_linkage.h"
#include "system_calls.h"
// number of vectors in IDT
#define Divided_Error 0
#define Debug_Exception 1
#define NMI_Interrupt 2
#define Breakpoint 3
#define Overflow 4
#define Bound_Range_Exceeded 5
#define Invalid_Opcode 6
#define Device_Not_Available 7
#define Double_Fault 8
#define Coprocessor_Segment_Overrun 9
#define Invalid_TSS 10
#define Segment_Not_Present 11
#define Stack_Segment_Fault 12
#define General_Protection 13
#define Page_Fault 14
#define Reserved 15
#define x87_Floating_Point_Exception 16
#define Alignment_Check 17
#define Machine_Check 18
#define SIMD_Floating_Point_Exception 19
// helper function to handle exceptions
void handle_exception(uint8_t vector, const char *msg)
{
    printf("Exception %d: %s\n", vector, msg);\
    // halt the system
    // while(1);
    halt(1);
}
// exception handlers for each exception
void exception_divided_error()
{
    handle_exception(Divided_Error, "Divided Error");
}
void exception_debug()
{
    handle_exception(Debug_Exception, "Debug Exception");
}
void exception_nmi_interrupt()
{
    handle_exception(NMI_Interrupt, "NMI Interrupt");
}
void exception_breakpoint()
{
    handle_exception(Breakpoint, "Breakpoint");
}
void exception_overflow()
{
    handle_exception(Overflow, "Overflow");
}
void exception_bound_range_exceeded()
{
    handle_exception(Bound_Range_Exceeded, "Bound Range Exceeded");
}
void exception_invalid_opcode()
{
    handle_exception(Invalid_Opcode, "Invalid Opcode");
}
void exception_device_not_available()
{
    handle_exception(Device_Not_Available, "Device Not Available");
}
void exception_double_fault()
{
    handle_exception(Double_Fault, "Double Fault");
}
void exception_coprocessor_segment_overrun()
{
    handle_exception(Coprocessor_Segment_Overrun, "Coprocessor Segment Overrun");
}
void exception_invalid_tss()
{
    handle_exception(Invalid_TSS, "Invalid TSS");
}
void exception_segment_not_present()
{
    handle_exception(Segment_Not_Present, "Segment Not Present");
}
void exception_stack_segment_fault()
{
    handle_exception(Stack_Segment_Fault, "Stack Segment Fault");
}
void exception_general_protection()
{
    handle_exception(General_Protection, "General Protection");
}
void exception_page_fault()
{
    handle_exception(Page_Fault, "Page Fault");
}
void exception_reserved()
{
    handle_exception(Reserved, "Reserved");
}
void exception_x87_floating_point_exception()
{
    handle_exception(x87_Floating_Point_Exception, "x87 Floating Point Exception");
}
void exception_alignment_check()
{
    handle_exception(Alignment_Check, "Alignment Check");
}
void exception_machine_check()
{
    handle_exception(Machine_Check, "Machine Check");
}
void exception_simd_floating_point_exception()
{
    handle_exception(SIMD_Floating_Point_Exception, "SIMD Floating Point Exception");
}

// system call handler
void system_call_handler()
{
    printf("a system call was called\n");
}

/*
*  idt_init
*     Desciption: init the IDT
*     Inputs: none
*     Outputs: none
*     Return Value: none
*     Side Effect: none
*/
void idt_init()
{
    // set up the IDT
    int i;
    for(i = 0; i < NUM_VEC; i++)
    {
        // 
        idt[i].seg_selector = KERNEL_CS;
        // 8 reserved bits, just set to 0
        idt[i].reserved4 = 0;
        // reserved 0,1,2,3 for gate type
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1;
        // set to 1 for 32-bit interrupt gate or trap gate
        idt[i].size = 1;
        // 1 reserved bit(bit 44), just set to 0
        idt[i].reserved0 = 0;
        // if it is system call(with entry 0x80)
        if (i == 0x80)
        {
            // set gate type to 0xF for system call
            idt[i].reserved3 = 1;
            // system call has DPL 3
            idt[i].dpl = 3;
        }
        else
        {
            // set gate type to 0xE for interrupt gate
            idt[i].reserved3 = 0;
            // all other entries has DPL 0
            idt[i].dpl = 0;
        }
        // all entries has not been set up, will be set to 1 in SET_IDT_ENTRY
        idt[i].present = 0;
    }
    // set up the exceptions
    SET_IDT_ENTRY(idt[Divided_Error],divided_error_handler_linkage);
    SET_IDT_ENTRY(idt[Debug_Exception], debug_handler_linkage);
    SET_IDT_ENTRY(idt[NMI_Interrupt], nmi_handler_linkage);
    SET_IDT_ENTRY(idt[Breakpoint], breakpoint_handler_linkage);
    SET_IDT_ENTRY(idt[Overflow], overflow_handler_linkage);
    SET_IDT_ENTRY(idt[Bound_Range_Exceeded], bounds_check_handler_linkage);
    SET_IDT_ENTRY(idt[Invalid_Opcode], invalid_opcode_handler_linkage);
    SET_IDT_ENTRY(idt[Device_Not_Available], device_not_available_handler_linkage);
    SET_IDT_ENTRY(idt[Double_Fault], double_fault_handler_linkage);
    SET_IDT_ENTRY(idt[Coprocessor_Segment_Overrun], coprocessor_segment_overrun_handler_linkage);
    SET_IDT_ENTRY(idt[Invalid_TSS], invalid_tss_handler_linkage);
    SET_IDT_ENTRY(idt[Segment_Not_Present], segment_not_present_handler_linkage);
    SET_IDT_ENTRY(idt[Stack_Segment_Fault], stack_exception_handler_linkage);
    SET_IDT_ENTRY(idt[General_Protection], general_protection_fault_handler_linkage);
    SET_IDT_ENTRY(idt[Page_Fault], page_fault_handler_linkage);
    SET_IDT_ENTRY(idt[Reserved], reserved_handler_linkage);
    SET_IDT_ENTRY(idt[x87_Floating_Point_Exception], floating_point_error_handler_linkage);
    SET_IDT_ENTRY(idt[Alignment_Check], alignment_check_handler_linkage);
    SET_IDT_ENTRY(idt[Machine_Check], machine_check_handler_linkage);
    SET_IDT_ENTRY(idt[SIMD_Floating_Point_Exception], simd_floating_point_handler_linkage);
    // set up the interrupts
    // pit is connected to IRQ0 (0x20)
    SET_IDT_ENTRY(idt[0x20], pit_handler_linkage);
    // keyboard is connected to IRQ1 (0x21)
    SET_IDT_ENTRY(idt[0x21], keyboard_handler_linkage);
    // RTC is connected to IRQ8 (0x28)
    SET_IDT_ENTRY(idt[0x28], rtc_handler_linkage);
    // system call is connected to 0x80
    SET_IDT_ENTRY(idt[0x80], system_call_handler_linkage);
    lidt(idt_desc_ptr);
}
