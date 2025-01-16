#define ASM 1
// use one macro to define all the interrupt linkage
#define INTR_LINK(name,func) \
    .globl name              ;\
    name:                    ;\
        pushal               ;\
        pushfl               ;\
        call func            ;\
        popfl                ;\
        popal                ;\
        iret                 ;\

// jump table for system call
jump_table:
    .long 0  // system call is 1-indexed, thus 0 is not used
    .long halt
    .long execute
    .long read
    .long write
    .long open
    .long close
    .long getargs
    .long vidmap
    .long set_handler
    .long sigreturn
// define all the interrupt linkage
INTR_LINK(rtc_handler_linkage, rtc_handler);
INTR_LINK(keyboard_handler_linkage, keyboard_handler);
INTR_LINK(pit_handler_linkage, pit_handler);
// define all the exception linkage
INTR_LINK(divided_error_handler_linkage, exception_divided_error);
INTR_LINK(debug_handler_linkage, exception_debug);
INTR_LINK(nmi_handler_linkage, exception_nmi_interrupt);
INTR_LINK(breakpoint_handler_linkage, exception_breakpoint);
INTR_LINK(overflow_handler_linkage, exception_overflow);
INTR_LINK(bounds_check_handler_linkage, exception_bound_range_exceeded);
INTR_LINK(invalid_opcode_handler_linkage, exception_invalid_opcode);
INTR_LINK(device_not_available_handler_linkage, exception_device_not_available);
INTR_LINK(double_fault_handler_linkage, exception_double_fault);
INTR_LINK(coprocessor_segment_overrun_handler_linkage, exception_coprocessor_segment_overrun);
INTR_LINK(invalid_tss_handler_linkage, exception_invalid_tss);
INTR_LINK(segment_not_present_handler_linkage, exception_segment_not_present);
INTR_LINK(stack_exception_handler_linkage, exception_stack_segment_fault);
INTR_LINK(general_protection_fault_handler_linkage, exception_general_protection);
INTR_LINK(page_fault_handler_linkage, exception_page_fault);
INTR_LINK(reserved_handler_linkage, exception_reserved);
INTR_LINK(floating_point_error_handler_linkage, exception_x87_floating_point_exception);
INTR_LINK(alignment_check_handler_linkage, exception_alignment_check);
INTR_LINK(machine_check_handler_linkage, exception_machine_check);
INTR_LINK(simd_floating_point_handler_linkage, exception_simd_floating_point_exception);
// define the system call linkage
// eax is reserved for return value (not pushed and popped)
.globl system_call_handler_linkage
system_call_handler_linkage:
    pushl %ecx
    pushl %edx
    pushl %ebx
    pushl %esp
    pushl %ebp
    pushl %esi
    pushl %edi
    pushfl
    pushl %edx         // push the 3rd argument
    pushl %ecx         // push the 2nd argument
    pushl %ebx         // push the 1st argument
    cmpl $0, %eax
    jle invalid_syscall
    cmpl $10, %eax         // 10 is the total number of system calls implemented
    jg invalid_syscall
    call *jump_table(,%eax,4)   // call the corresponding system call
    jmp system_call_handler_linkage_end
invalid_syscall:
    movl $-1, %eax      // return -1 if the system call num is invalid
system_call_handler_linkage_end:
    addl $12, %esp  // pop the 3 arguments
    popfl
    popl %edi
    popl %esi
    popl %ebp
    popl %esp
    popl %ebx
    popl %edx
    popl %ecx
    iret
