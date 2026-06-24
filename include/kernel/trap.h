#ifndef __TRAP_H__
#define __TRAP_H__

#include <kernel/types.h>

/* https://docs.riscv.org/reference/isa/priv/supervisor.html#scause
 *
 * Bit 63 in the `scause` register defines what kind of trap
 * just happened: exceptions have scause[63] == 0, while
 * interrupts have scause[63] == 1 */
#define TRAP_IRQ_BIT		(1UL << 63)
#define TRAP_TIMER_IRQ		(TRAP_IRQ_BIT | 0x5)
#define TRAP_EXTERNAL_IRQ	(TRAP_IRQ_BIT | 0x9)

/* Exception codes that are of interest to us at the moment */
#define EXCEPTION_INST_ACCESS_FAULT	1UL
#define EXCEPTION_LOAD_ACCESS_FAULT	5UL
#define EXCEPTION_STORE_ACCESS_FAULT	7UL
#define EXCEPTION_INST_PAGE_FAULT	12UL
#define EXCEPTION_LOAD_PAGE_FAULT	13UL
#define EXCEPTION_STORE_PAGE_FAULT	15UL

/**
 * trap_setup(): configure traps
 *
 * Write the trap handler address to `stvec`, disable interrupts, etc
 */
void trap_setup();

/**
 * hart_irq_enable(): enable all interrupts for the current hart
 *
 * Enable interrupts (sstatus[SIE]).
 */
void hart_irq_enable();

/**
 * hart_irq_disable(): disable interrupts for the current hart
 *
 * Disable interrupts (sstatus[SIE]).
 */
void hart_irq_disable();

/**
 * hart_irq_save(): save state and disable interrupts for the current hart
 *
 * This is useful for temporarily disabling interrupts (and reenabling them
 * later with hart_irq_restore()).
 *
 * Returns: an u64 containing the value of sstatus[SIE] before disabling
 * interrupts.
 */
u64 hart_irq_save();

/**
 * hart_irq_restore(): restore previous interrupt enabled/disabled state
 * @flags: previous interrupt state
 *
 * Restore the interrupt state that was saved with hart_irq_save().
 */
void hart_irq_restore(u64 flags);

#endif
