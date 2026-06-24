#ifndef __IO_H__
#define __IO_H__

#include <kernel/types.h>
#include <kernel/defs.h>

/* TODO(nukelet): we should technically add memory barriers/fences,
 * as well as properly map these I/O regions as non-cacheable, but
 * that's a problem for another day */
__always_inline u64 ioread64(void *addr)
{
	return *(volatile u64 *)addr;
}

static void __always_inline iowrite64(u64 val, void *addr)
{
	*(volatile u64 *)addr = val;
}

static u32 __always_inline ioread32(void *addr)
{
	return *(volatile u32 *)addr;
}

static void __always_inline iowrite32(u32 val, void *addr)
{
	*(volatile u32 *)addr = val;
}

static u16 __always_inline ioread16(void *addr)
{
	return *(volatile u16 *)addr;
}

static void __always_inline iowrite16(u16 val, void *addr)
{
	*(volatile u16 *)addr = val;
}

static u8 __always_inline ioread8(void *addr)
{
	return *(volatile u8 *)addr;
}

static void __always_inline iowrite8(u8 val, void *addr)
{
	*(volatile u8 *)addr = val;
}

#endif
