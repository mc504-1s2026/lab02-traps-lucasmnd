#ifndef __PANIC_H__
#define __PANIC_H__

#include <kernel/defs.h>
#include <kernel/printf.h>
#include <kernel/types.h>
#include <stdarg.h>

__noreturn void panic(const char *fmt, ...);

enum bug_behavior {
	BUG_PANIC,
	BUG_LOG,
};

#define BUG_DEFAULT_BEHAVIOR BUG_PANIC

void bug_set_behavior(enum bug_behavior);
enum bug_behavior bug_get_behavior();

#define BUG() do {						\
	if (bug_get_behavior() == BUG_PANIC) {			\
		panic("BUG() in function %s (%s:%d)\n",		\
		      __func__, __FILE__, __LINE__);		\
	} else {						\
		critical("BUG() in function %s (%s:%d)\n",	\
		      __func__, __FILE__, __LINE__);		\
	}							\
} while (0)

#endif
