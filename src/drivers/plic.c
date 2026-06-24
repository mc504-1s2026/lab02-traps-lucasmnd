#include <arch/plic.h>
#include <arch/io.h>
#include <kernel/mm.h>

/* https://wiki.osdev.org/PLIC */
#define PLIC_BASE		(0xc000000UL + KERNEL_DIRECT_MAP_START)
#define PLIC_IRQ_BASE		PLIC_BASE
/* TODO(nukelet): explain that we're skipping the first entries because we're
 * skipping the M-mode context */
#define PLIC_ENABLE_BASE	(PLIC_BASE + 0x2000 + 0x80)
#define PLIC_THRESH_BASE	(PLIC_BASE + 0x200000 + 0x1000)

#define PLIC_IRQ_PRIORITY(irq)		((u32*)(PLIC_IRQ_BASE + 4 * (irq)))
/* Calculate enable register address for a given hart on S-mode */
#define PLIC_HART_ENABLE(hart)		((u32*)(PLIC_ENABLE_BASE + (hart) * 0x100))
#define PLIC_HART_THRESHOLD(hart)	((u32*)(PLIC_THRESH_BASE + (hart) * 0x1000))
#define PLIC_HART_CLAIM(hart)		((u32*)(PLIC_THRESH_BASE + 0x4 + (hart) * 0x1000))

void plic_irq_set_priority(u32 irq, u32 prio)
{
	iowrite32(prio, PLIC_IRQ_PRIORITY(irq));
}

void plic_hart_enable_irq(u32 hart, u32 irq)
{
	u32 offset = irq/32;
	u32 val = (1 << (irq % 32));
	iowrite32(val, PLIC_HART_ENABLE(hart) + offset);
}

void plic_hart_set_threshold(u32 hart, u32 threshold)
{
	trace("plic: writing threshold=%d for hart %d (%p)\n", threshold, hart, PLIC_HART_THRESHOLD(hart));
	iowrite32(threshold, PLIC_HART_THRESHOLD(hart));
}

u32 plic_hart_claim_irq(u32 hart)
{
	trace("plic: claiming irq at reg %p\n", PLIC_HART_CLAIM(0));
	return ioread32(PLIC_HART_CLAIM(hart));
}

void plic_hart_complete_irq(u32 hart, u32 irq)
{
	iowrite32(irq, PLIC_HART_CLAIM(hart));
}

