#ifndef __MM_H__
#define __MM_H__

#include <kernel/types.h>
#include <arch/pgtable.h>
#include <kernel/sizes.h>

extern char _start[];
extern char _end[];
extern char _text_start[];
extern char _text_end[];
extern char _bss_start[];
extern char _bss_end[];
extern char _rodata_start[];
extern char _rodata_end[];
extern char _data_start[];
extern char _data_end[];

/**
 * struct memory_map_t - Contains the virtual address of the kernel sections.
 * @kernel_start: The first address of the kernel executable
 * @kernel_end: The last address reserved for the kernel
 * @kernel_size: Total size of the kernel executable
 * @*_start: The first address of a section
 * @*_end: The last address of a section
 *
 * Retrieve the addresses mapped by the linker to each session of the kernel.
 * The *_start is always page alligned and the end contains the last initialized
 * address of the session (not necessarily page alligned).
 */
struct memory_map_t {
	u64 phys_offset;
	phys_addr_t kernel_start;
	phys_addr_t kernel_end;
	size_t kernel_size;
	phys_addr_t text_start;
	phys_addr_t text_end;
	phys_addr_t bss_start;
	phys_addr_t bss_end;
	phys_addr_t rodata_start;
	phys_addr_t rodata_end;
	phys_addr_t data_start;
	phys_addr_t data_end;
};

/* This is the start of the kernel virtual address space, right after
 * the hole in the middle of the Sv39 address space (bits 63-39) */
#define KERNEL_VA_START 0xffffffc000000000UL
#define KERNEL_VA_END 0xffffffffffffffffUL

/* This is the offset between the kernel virtual and physical load address */
#define KERNEL_PHYS_OFFSET 0xffffffff00000000

/* The kernel defines a virtual memory area called the "direct mapping"
 * that maps directly to physical memory; i.e., in order to access the
 * physical memory address X, you write to an X-byte offset within this
 * direct map region. This technique is many kernels, such as Linux and
 * the BSDs. */

/* The start of the direct map area. This needs to be 1GiB-aligned, since
 * we map it using gigapages (i.e. leaf nodes on the root page table).
 */
#define KERNEL_DIRECT_MAP_START 0xffffffc000000000UL
#define KERNEL_DIRECT_MAP_SIZE (32 * SIZE_1G)
#define KERNEL_DIRECT_MAP_END (KERNEL_DIRECT_MAP_START + KERNEL_DIRECT_MAP_SIZE)

/*
 * va_to_pa(): convert a (direct mapping) virtual address into a physical one
 *
 * This is done by calculating the offset of the virtual address into the
 * start of the direct map area.
 */
phys_addr_t __always_inline va_to_pa(void *va)
{
	if (KERNEL_DIRECT_MAP_START <= (u64)va &&
	    (u64) va <= KERNEL_DIRECT_MAP_END) {
		return (phys_addr_t) va - KERNEL_DIRECT_MAP_START;
	} else {
		return 0;
	}
}

/*
 * pa_to_va(): convert a physical address into a virtual one (within the direct map area)
 */
u64 __always_inline pa_to_va(phys_addr_t pa)
{
	return pa + KERNEL_DIRECT_MAP_START;
}

/**
 * mm_init_kmap() - Initialize the kernel memory layout map
 *
 * This initializes the global kmap struct containing information about
 * the kernel memory layout, such as the kernel virtual start/end addresses and
 * size, as well as the start/end of each section of the kernel.
 */
void mm_init_kmap();

/**
 * mm_init() - Initialize the memory management.
 *
 * At this first implementation we are going to map the physical addresses
 * to the virtual addresses of the same number.
 *
 * Map all the kernel sessions using the suitable flags for each session,
 * map the serial to address 0x10000000, map all the remaining addresses to
 * read and write pages
 */
void mm_init();

/**
 * vm_init() - Initialize the memory abstraction.
 *
 * Initialize the memory management and write the root PTB to the satp register
 */
void vm_init();

/**
 * vm_map_page() - Map a single virtual address page to a physical address page.
 * @va: Virtual address
 * @pa: Physical address
 * @flags: PTE flags
 *
 * Example:
 *
 *	int ret;
 *	ret = vm_map_page(0x10000, 0x10000, PTE_READ | PTE_WRITE);
 *	if (ret)
 *		panic("failed to map page!\n");
 *
 * Return: 0 on success and -1 on failure.
 */
int vm_map_page(u64 va, phys_addr_t pa, u64 flags);

/**
 * dump_pte() - Dump the state of a single PTE.
 */
void dump_pte(pte_t pte);

/**
 * dump_ptbs() - Dump the state of all PTBs starting from the kernel root PTB.
 *
 * This walks through all PTBs, printing the PTEs along the way.
 */
void dump_ptbs();

#endif
