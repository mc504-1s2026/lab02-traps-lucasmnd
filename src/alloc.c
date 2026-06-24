#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/alloc.h>
#include <arch/pgtable.h>
#include <kernel/panic.h>

#define ALLOC_LIST_SIZE 4096 * 8

/**
 * struct alloc_t - The kernel physical memory allocator.
 * @freelist: Array of physical addresses of free physical pages.
 * @next: Index of the next free page within the freelist.
 */
struct alloc_t {
	phys_addr_t list[ALLOC_LIST_SIZE];
	size_t next;
	bool initialized;
} alloc;

int alloc_init(phys_addr_t start)
{
	if (!IS_ALIGNED(start)) {
		critical("alloc: tried to init allocator with available memory starting at misaligned address 0x%x!\n", start);
		BUG();
		return -1;
	}

	info("alloc: initializing physmem allocator at phys address %p\n", start);
	for (size_t i = 0; i < ALLOC_LIST_SIZE; i++) {
		alloc.list[i] = (u64)start + i * PAGE_SIZE;
	}
	alloc.next = 0;
	alloc.initialized = true;
	return 0;
}

phys_addr_t alloc_get_page()
{
	if (!alloc.initialized) {
		critical("alloc: tried to alloc pages before initializing the allocator!\n");
		BUG();
		return 0;
	}

	if (alloc.next == ALLOC_LIST_SIZE - 1)
	{
		critical("alloc: no more free pages to allocate.\n");
		BUG();
		return 0;
	}

	phys_addr_t pa = alloc.list[alloc.next];
	alloc.list[alloc.next++] = 0;
	trace("alloc: allocating physical page 0x%x\n", pa);

	return pa;
}

phys_addr_t alloc_zero_page()
{
	phys_addr_t addr = alloc_get_page();
	memset((void*) addr, 0, PAGE_SIZE);
	return addr;
}

int alloc_free_page(phys_addr_t pa)
{
	if (!alloc.initialized) {
		critical("alloc: tried to free pages before initializing the allocator!\n");
		BUG();
		return -1;
	}

	if (!IS_ALIGNED(pa)) {
		critical("alloc: tried to free page at misaligned physical address!\n");
		BUG();
		return -1;
	}

	if (alloc.next == 0) {
		critical("alloc: tried to free page when no pages have been allocated!\n");
		BUG();
		return -1;
	}

	alloc.list[--alloc.next] = pa;
	return 0;
}
