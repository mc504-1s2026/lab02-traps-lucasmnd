#include "arch/csr.h"
#include <arch/timer.h>
#include <kernel/panic.h>
#include "kernel/serial.h"

u64 timer_read()
{
	return csr_read(CSR_TIME) / TIMER_FREQ;
}

void timer_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_STIE);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	csr_write(CSR_STIMECMP, csr_read(CSR_TIME) + secs * TIMER_FREQ);
}

void timer_irq()
{
	timer_irq_disable();
	serial_puts("alarm\r\n");
	serial_puts("> ");
}
