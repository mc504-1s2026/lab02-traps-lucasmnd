#include <kernel/serial.h>
#include <kernel/panic.h>
#include "arch/csr.h"
#include "arch/io.h"
#include "arch/plic.h"
#include "arch/spinlock.h"

#define SERIAL_BUF_SIZE 256

struct serialdev {
    /* buffer storing received serial data */
    char buf[SERIAL_BUF_SIZE]; 
    /* number of bytes stored in this buffer */
    size_t len;
	struct spinlock sl;
} dev;

void serial_init()
{
	spin_init(&dev.sl);
	dev.len = 0;
	void *addr_queue = SERIAL_BASE + SERIAL_FCR;
	u8 flags = SERIAL_FCR_FIFO_ENABLE | SERIAL_FCR_RX_FIFO_CLEAR | SERIAL_FCR_TX_FIFO_CLEAR;
	iowrite8(flags, addr_queue);
}

void serial_irq_enable()
{
	iowrite8(SERIAL_IER_ERBFI, SERIAL_BASE + SERIAL_IER);
	plic_irq_set_priority(IRQ_SERIAL, 1);
	plic_hart_set_threshold(0, 0);
	plic_hart_enable_irq(0, IRQ_SERIAL);
	csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{
	iowrite8(0, SERIAL_BASE + SERIAL_IER);
	plic_irq_set_priority(IRQ_SERIAL, 0);
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	spin_lock(&dev.sl);
	if(dev.len < SERIAL_BUF_SIZE){
		dev.buf[dev.len++] = ioread8(SERIAL_BASE + SERIAL_RBR);
	}
	spin_unlock(&dev.sl);
}

size_t serial_read(char *buf)
{
	u64 temp = spin_lock_irqsave(&dev.sl);

	for(unsigned int i = 0; i < dev.len; i++){
		buf[i] = dev.buf[i];
	}

	size_t ret = dev.len;
	dev.len = 0;

	spin_unlock_irqrestore(&dev.sl, temp);

	return ret;
}

void serial_puts(char *str)
{
	while(str[0] != '\0'){
		serial_putc(str[0]);
		str++;
	}
}

void serial_putc(char c)
{
	while((ioread8(SERIAL_BASE + SERIAL_LSR) & SERIAL_LSR_THRE) == 0);

	iowrite8(c, SERIAL_BASE + SERIAL_THR);
}
