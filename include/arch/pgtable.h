#ifndef __PGTABLE_H__
#define __PGTABLE_H__

#include <kernel/types.h>
#include <kernel/bits.h>
#include <kernel/defs.h>
#include <kernel/printf.h>
#include <kernel/panic.h>

/* 
 * RISC-V Sv39 virtual memory layout
 *
 * Virtual address:
 *    [ VPN[2] ]  [ VPN[1] ]  [ VPN[0] ]  [ page offset ]
 *   38  ... 30 29  ...  21 20  ...  12 11      ...    0
 *
 * Physical address:
 *    [ PPN[2] ]  [ PPN[1] ]  [ PPN[0] ]  [ page offset ]
 *   55  ... 30 29  ...  21 20  ...  12 11      ...    0
 * 
 * Page table entry
 *    [ ... ]  [ PPN[2] ]  [ PPN[1] ]  [ PPN[0] ]  [ RSW ] [D] [A] [G] [U] [X] [W] [R] [V]
 *    63   54 53  ...  28 27  ...  19 18  ...  10  9 ... 8  7   6   5   4   3   2   1   0
 *
 */

#define PAGE_SHIFT 12					/* 12-bit page offset field */
#define PAGE_SIZE (1UL << PAGE_SHIFT)			/* 2**12 = 4096 byte pages */
#define PAGE_MASK (~(PAGE_SIZE - 1))			/* 0xfffffffffffff000 */

/**
 * IS_ALIGNED(): Macro to check if a given address is page-aligned.
 * @addr: The address to check.
 *
 * Return: 1 if the address is aligned and 0 otherwise.
 */
#define IS_ALIGNED(addr) (!((u64) (addr) & ~PAGE_MASK))

/**
 * PPN_DOWN(): Round address down to the nearest page boundary.
 * @addr: Address to round down.
 *
 * Returns: ddress rounded down to the nearest page boundary.
 */
#define PPN_DOWN(addr) ((u64) (addr) >> PAGE_SHIFT)

/**
 * PPN_UP(): Round address up to the nearest page boundary.
 * @addr: Address to round up.
 *
 * Returns: the address rounded up to the nearest page boundary.
 */
#define PPN_UP(addr) (((u64) (addr) + PAGE_SIZE - 1) >> PAGE_SHIFT)

/* Helper macros for extractings page table indices */

#define VPN_SHIFT_L2 30			/* VPN[2] = VA[38:30] */
#define VPN_MASK_L2 BITMASK64(38, 30)
#define VPN_SHIFT_L1 21			/* VPN[1] = VA[29:21] */
#define VPN_MASK_L1 BITMASK64(29, 21)
#define VPN_SHIFT_L0 12			/* VPN[0] = VA[20:12] */
#define VPN_MASK_L0 BITMASK64(20, 12)

/**
 * va_get_index(): Extract the index within a pagetable from a virtual address.
 * @va: Virtual address to extract the index from.
 * @level: Page table level.
 *
 * Returns: The index of VA @va within a page table of level @level.
 */
static __always_inline u64 va_get_index(u64 va, u64 level)
{
	switch (level) {
		case 2:
			return (va & VPN_MASK_L2) >> (VPN_SHIFT_L2);
		case 1:
			return (va & VPN_MASK_L1) >> (VPN_SHIFT_L1);
		case 0:
			return (va & VPN_MASK_L0) >> (VPN_SHIFT_L0);
		default:
			panic("ERROR: va_get_index: page table level %d doesn't exist!\n", level);
	}
	return 0;
}

#define PTB_LEVELS 3					/* total PTB levels */
#define PTB_INDEX_BITS 9				/* number of bits to map each index */
#define PTE_SHIFT 10					/* pagetable entry shift/offset */
#define PTE_PPN_MASK BITMASK64(53, 10)			/* mask to get only PPN from a pte */
#define PTB_ENTRY_COUNT (1UL << (PTB_INDEX_BITS))	/* the number of mappable entries is 2**n_bits in va */

/**
 * Page table typedefs
 *
 * pte_t: page table entry (one line of a page table)
 * ppn_t: physical page number (such as the PPN field in a PTE)
 *
 * Both are u64, but this definition of a type serves as a hint for the developer.
 */
typedef u64	pte_t;
typedef u64	ppn_t;

/**
 * struct pgtable - A page table represented as an array of PTEs.
 * @entries: array of PTEs
 *
 * This struct is used to represent a page table in memory. It is not
 * meant to be allocated, but rather typecasted to an address. For example:
 *
 *	struct pgtable *ptb;
 *	ptb = (struct pgtable *) alloc_get_page();
 */
struct pgtable {
	pte_t entries[PTB_ENTRY_COUNT];	/* 512 entries */
};

/**
 * Inline function helpers for manipulating PTBs, PTEs and PPNs
 */

/**
 * phys_to_ppn() - Extract the PPN from a physical address.
 */
static __always_inline ppn_t phys_to_ppn(phys_addr_t phys)
{
	return (ppn_t) phys >> PAGE_SHIFT;
}

/**
 * ppn_to_phys() - Convert a PPN to a physical address.
 */
static __always_inline phys_addr_t ppn_to_phys(ppn_t ppn)
{
	return (phys_addr_t) ppn << PAGE_SHIFT;
}

/**
 * pte_get_ppn() - Extract the PPN from a PTE.
 */
static __always_inline ppn_t pte_get_ppn(pte_t pte)
{
	return (ppn_t) (pte & PTE_PPN_MASK) >> PTE_SHIFT;
}

/*
 * pte_from_ppn() - Construct a PTE from a given PPN.
 * @ppn: Physical page number
 *
 * Return: a PTE with the PPN field set to @ppn, and all other fields cleared.
 */
static __always_inline pte_t pte_from_ppn(ppn_t ppn)
{
	return (pte_t) ppn << PTE_SHIFT;
}


/**
 * pte_next_ptb() - Get the next level pagetable from a PTE.
 * @pte: Page table entry.
 *
 * Return: Pointer to the struct pgtable at the address encoded in the PPN field of the PTE.
 */
static __always_inline struct pgtable *pte_next_ptb(pte_t pte)
{
	return (struct pgtable *) ppn_to_phys(pte_get_ppn(pte));
}

/**
 * ptb_get_ptep() - Get a pointer to the PTE at a given index within a pagetable.
 * @ptb: Pointer to a page table.
 * @index: Index within the page table.
 *
 * Return: Pointer to the PTE entry at index @index within @ptb.
 */
static __always_inline pte_t *ptb_get_ptep(struct pgtable *ptb, u64 index)
{
	return &ptb->entries[index];
}

#define PTE_VALID	(1UL << 0)
#define PTE_READ	(1UL << 1)
#define PTE_WRITE	(1UL << 2)
#define PTE_EXEC	(1UL << 3)
#define PTE_LEAF	(PTE_READ | PTE_WRITE | PTE_EXEC)

/* Inline functions to help manipulate PTEs. */

static __always_inline bool pte_valid(pte_t pte)
{
	return pte & PTE_VALID;
}

static __always_inline bool pte_readable(pte_t pte)
{
	return pte & PTE_READ;
}

static __always_inline bool pte_writable(pte_t pte)
{
	return pte & PTE_WRITE;
}

static __always_inline bool pte_executable(pte_t pte)
{
	return pte & PTE_EXEC;
}

static __always_inline bool pte_leaf(pte_t pte)
{
	return pte & PTE_LEAF;
}

#endif
