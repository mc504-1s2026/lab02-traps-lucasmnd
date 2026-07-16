#include "arch/plic.h"
#include "arch/timer.h"
#include "kernel/printf.h"
#include <kernel/trap.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include "kernel/serial.h"

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq()
{
	u64 cause = csr_read(CSR_SCAUSE);

	if(cause == TRAP_TIMER_IRQ){
		timer_irq();
	} else if(cause == TRAP_EXTERNAL_IRQ){
		u32 irq = plic_hart_claim_irq(0);
		if(irq == IRQ_SERIAL) {
			serial_irq();
			plic_hart_complete_irq(0, irq);
		}
	}
}

void handle_exception()
{
	u64 r_scause, r_stval, r_sepc;
	r_scause = csr_read(CSR_SCAUSE);
	r_stval = csr_read(CSR_STVAL);
	r_sepc = csr_read(CSR_SEPC);

	error("Uncaught Exception\n");
	error("scause: %d\n", r_scause);
	error("stval: %d\n", r_stval);
	error("sepc: %d\n", r_sepc);
}

void trap_setup()
{
	csr_write(CSR_STVEC, trap_entry);
}

void handle_trap()
{
	u64 cause = csr_read(CSR_SCAUSE) & BIT(63);

	if(cause) { handle_irq();
	} else handle_exception();
}

void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
	u64 status = csr_read(CSR_SSTATUS);
	hart_irq_disable();
	return status;
}

void hart_irq_restore(u64 flags)
{
	csr_write(CSR_SSTATUS, flags);
}

void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}
