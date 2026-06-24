#include <kernel/trap.h>
#include <kernel/panic.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq()
{
	/* not implemented */
	BUG();
}

void handle_exception()
{
	/* not implemented */
	BUG();
}

void trap_setup()
{
	/* not implemented */
	BUG();
}

void handle_trap()
{
	/* not implemented */
	BUG();
}

void hart_irq_enable()
{
	/* not implemented */
	BUG();
}

u64 hart_irq_save()
{
	/* not implemented */
	BUG();
}

void hart_irq_restore(u64 flags)
{
	/* not implemented */
	BUG();
}

void hart_irq_disable()
{
	/* not implemented */
	BUG();
}
