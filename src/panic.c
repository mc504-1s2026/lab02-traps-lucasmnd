#include <kernel/panic.h>
#include <kernel/printf.h>

__noreturn void panic(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	error("----- start of kernel panic -----\n");

	vprintk(LOG_ERR, fmt, args);

	/* TODO: should disable interrupts here, etc */

	error("----- end of kernel panic -----\n");

	while (1) {
		__asm__("nop");
	}
}

static enum bug_behavior bug_behavior;

void bug_set_behavior(enum bug_behavior behavior)
{
	bug_behavior = behavior;
}

enum bug_behavior bug_get_behavior()
{
	return bug_behavior;
}
