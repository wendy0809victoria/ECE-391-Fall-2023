/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* i8259_init
 *   DESCRIPTION: Initialize the 8259 PIC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void i8259_init(void) 
{
    outb(0xFF,MASTER_8259_PORT+1);
    outb(0xFF,SLAVE_8259_PORT+1);
    // ICW1 for master
    outb(ICW1, MASTER_8259_PORT); 
    // ICW1 for slave
    outb(ICW1, SLAVE_8259_PORT); 
    // ICW2 (vector offset) for master to the data port
    outb(ICW2_MASTER, MASTER_8259_PORT + 1); 
    // ICW2 (vector offset) for slave to the data port
    outb(ICW2_SLAVE, SLAVE_8259_PORT + 1); 
    // ICW3 for master to the data port, tell Master PIC that there is a slave PIC at IRQ2
    outb(ICW3_MASTER, MASTER_8259_PORT + 1); 
    // ICW3 for slave to the data port, tell Slave PIC its cascade identity
    outb(ICW3_SLAVE, SLAVE_8259_PORT + 1); 
    // ICW4 for master to the data port to set 8086 mode for pics
    outb(ICW4, MASTER_8259_PORT + 1); 
    // ICW4 for slave to the data port to set 8086 mode for pics
    outb(ICW4, SLAVE_8259_PORT + 1);
    // initialize the mask
    master_mask = 0xFF;
    slave_mask = 0xFF;
    enable_irq(2); //enable IRQ2 on master PIC to enable the cascade to slave PIC
}

/* enable_irq
 *   DESCRIPTION: Enable (unmask) the specified IRQ
 *   INPUTS: irq_num -- the irq number to be enabled
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void enable_irq(uint32_t irq_num) 
{
    uint8_t mask;
    // check if the irq_num is valid
    if (irq_num < 0 || irq_num > 15)
    {
        return;
    }    
    if(irq_num < 8)
    {
        // irq_num is in master PIC
        mask = ~(1 << irq_num);
        master_mask = master_mask & mask;
    }
    else
    {
        // irq_num is in slave PIC
        mask = ~(1 << (irq_num - 8));
        slave_mask = slave_mask & mask;
    }
    outb(master_mask, MASTER_8259_PORT + 1);
    outb(slave_mask, SLAVE_8259_PORT + 1);
}

/* 
* disable_irq
*   DESCRIPTION: Disable (mask) the specified IRQ
*   INPUTS: irq_num -- the irq number to be disabled
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: none
*/
void disable_irq(uint32_t irq_num) 
{
    uint8_t mask;
    // check if the irq_num is valid
    if (irq_num < 0 || irq_num > 15)
    {
        return;
    }    
    if(irq_num < 8)
    {
        // irq_num is in master PIC
        mask = (1 << irq_num);
        master_mask = master_mask | mask;
    }
    else
    {
        // irq_num is in slave PIC
        mask = (1 << (irq_num - 8));
        slave_mask = slave_mask | mask;
    }
    outb(master_mask, MASTER_8259_PORT + 1);
    outb(slave_mask, SLAVE_8259_PORT + 1);
}

/* 
* send_eoi
*   DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
*   INPUTS: irq_num -- the irq number to be sent EOI
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: none
*/
void send_eoi(uint32_t irq_num) 
{
    // check if the irq_num is valid
    if (irq_num < 0 || irq_num > 15)
    {
        return;
    }    
    if(irq_num < 8)
    {
        // send EOI to master PIC
        outb(EOI | irq_num, MASTER_8259_PORT);
    }
    else
    {
        // send EOI to slave PIC
        outb(EOI | (irq_num - 8), SLAVE_8259_PORT);
        // send EOI to master PIC to tell it that the interrupt of slave PIC is finished
        outb(EOI | 2, MASTER_8259_PORT);
    }
}
