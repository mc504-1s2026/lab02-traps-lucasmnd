#ifndef __PLIC_H__
#define __PLIC_H__

#include <kernel/types.h>

void plic_irq_set_priority(u32 irq, u32 prio);
void plic_hart_enable_irq(u32 hart, u32 irq);
void plic_hart_set_threshold(u32 hart, u32 threshold);
u32 plic_hart_claim_irq(u32 hart);
void plic_hart_complete_irq(u32 hart, u32 irq);

#endif
