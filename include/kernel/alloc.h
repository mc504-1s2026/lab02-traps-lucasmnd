#ifndef __ALLOC_H__
#define __ALLOC_H__

#include <kernel/types.h>

/**
 * alloc_init() - Populate the allocator freelist with all physical memory pages.
 * @start: Physical address of the first free page in memory.
 *
 * Return: 0 on success, -1 on failure.
 */
int alloc_init(phys_addr_t start);

/**
 * alloc_get_page() - Get a free page from the allocator.
 *
 * Note that this should return 0 to signal that the allocation failed, since
 * it should be impossible to allocate a page at physical address 0.
 *
 * Return: The physical address of the allocated physical memory page, or 0 if the
 * allocation failed.
 */
phys_addr_t alloc_get_page();


/**
 * alloc_zero_page() - Get a free zeroed page from the allocator.
 *
 * Note that this should return 0 to signal that the allocation failed, since
 * it should be impossible to allocate a page at physical address 0.
 *
 * Return: The physical address of the allocated and zeroed physical memory page.
 */
phys_addr_t alloc_zero_page();


/**
 * alloc_free_page() - Free a physical memory page.
 * @pa: Physical address of the page to be freed.
 *
 * Return: 0 on success, -1 on failure.
 */
int alloc_free_page(phys_addr_t pa);

#endif
