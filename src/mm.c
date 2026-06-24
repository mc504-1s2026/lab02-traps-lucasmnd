#include <arch/pgtable.h>
#include <arch/csr.h>
#include <kernel/printf.h>
#include <kernel/bits.h>
#include <kernel/types.h>
#include <kernel/defs.h>
#include <kernel/string.h>
#include <kernel/mm.h>
#include <kernel/alloc.h>
#include <kernel/panic.h>
#include <kernel/sizes.h>

struct memory_map_t kmap;

void mm_init_kmap() {
	debug("\nreading kernel virtual memory layout map...\n");

	kmap.phys_offset = KERNEL_PHYS_OFFSET;
	debug("phys_offset = %#x\n", kmap.phys_offset);

	kmap.kernel_start = (phys_addr_t) _start;
	kmap.kernel_end = (phys_addr_t) _end;
	kmap.kernel_size = kmap.kernel_end - kmap.kernel_start;
	kmap.text_start = (phys_addr_t) _text_start;
	kmap.text_end = (phys_addr_t) _text_end;
	kmap.bss_start = (phys_addr_t) _bss_start;
	kmap.bss_end = (phys_addr_t) _bss_end;
	kmap.rodata_start = (phys_addr_t) _rodata_start;
	kmap.rodata_end = (phys_addr_t) _rodata_end;
	kmap.data_start = (phys_addr_t) _data_start;
	kmap.data_end = (phys_addr_t) _data_end;

	debug("kernel_start = %p\n", kmap.kernel_start);
	debug("kernel_end = %p\n", kmap.kernel_end);
	debug("kernel_size = %p\n", kmap.kernel_size);
	debug("text_start = %p\n", kmap.text_start);
	debug("text_end = %p\n", kmap.text_end);
	debug("bss_start = %p\n", kmap.bss_start);
	debug("bss_end = %p\n", kmap.bss_end);
	debug("rodata_start = %p\n", kmap.rodata_start);
	debug("rodata_end = %p\n", kmap.rodata_end);
	debug("data_start = %p\n", kmap.data_start);
	debug("data_end = %p\n", kmap.data_end);

	debug("\n");
}

struct pgtable *kernel_root_ptb;

void vm_init_kernel_direct_map()
{
	struct pgtable *ptb = kernel_root_ptb;
	u64 va;
	phys_addr_t pa;
	pte_t pte;
	size_t idx;
	for (u64 i = 0; i < KERNEL_DIRECT_MAP_SIZE / SIZE_1G; i++) {
		pa = i * SIZE_1G;
		va = KERNEL_DIRECT_MAP_START + pa;
		idx = va_get_index(va, 2);
		pte = pte_from_ppn(phys_to_ppn(pa));
		pte |= (PTE_READ | PTE_WRITE | PTE_EXEC | PTE_VALID);

		ptb->entries[idx] = pte;
	}
}

void vm_init_kernel()
{
	/* the addresses in kmap are virtual; we need to calculate the actual
	 * physical addresses by subtracting the offset between the start of
	 * the kernel in physical memory and virtual memory, e.g.
	 * 0xffffffff80000000 - 0x80000000 = 0xffffffff00000000
	 * the kmap.phys_offset is a statically defined symbol in linker.ld */

	info("setting up kernel page tables...\n");
	kernel_root_ptb = (struct pgtable *) alloc_get_page();
	debug("kernel_root_ptb: 0x%x\n", kernel_root_ptb);

	/* map each section of the kernel executable */

	debug("mapping .text: %p -> %p, size %lu\n",
	      kmap.text_start, kmap.text_start - kmap.phys_offset,
	      kmap.text_end - kmap.text_start);
	for (size_t addr = kmap.text_start; addr < kmap.text_end; addr += PAGE_SIZE) {
		vm_map_page(addr, addr - kmap.phys_offset, PTE_READ | PTE_EXEC);
	}

	debug("mapping .bss: %p -> %p, size %lu\n",
	      kmap.bss_start, kmap.bss_start - kmap.phys_offset,
	      kmap.bss_end - kmap.bss_start);
	for (size_t addr = kmap.bss_start; addr < kmap.bss_end; addr += PAGE_SIZE) {
		vm_map_page(addr, addr - kmap.phys_offset, PTE_READ | PTE_WRITE);
	}

	debug("mapping .rodata: %p -> %p, size %lu\n",
	      kmap.rodata_start, kmap.rodata_start - kmap.phys_offset,
	      kmap.rodata_end - kmap.rodata_start);
	for (size_t addr = kmap.rodata_start; addr < kmap.rodata_end; addr += PAGE_SIZE) {
		vm_map_page(addr, addr - kmap.phys_offset, PTE_READ);
	}

	debug("mapping .data: %p -> %p, size %lu\n",
	      kmap.data_start, kmap.data_start - kmap.phys_offset,
	      kmap.data_end - kmap.data_start);
	for (size_t addr = kmap.data_start; addr < kmap.data_end; addr += PAGE_SIZE) {
		vm_map_page(addr, addr - kmap.phys_offset, PTE_READ | PTE_WRITE);
	}

	debug("mapping serial device: %p -> %p, size 4096\n", 0x10000000, 0x10000000);
	vm_map_page(0x10000000, 0x10000000, PTE_READ | PTE_WRITE);

	vm_init_kernel_direct_map();

	/* map the zero page as RWX=0 in order to catch null-pointer exceptions.
	 * note that finding a page with RWX=0 at the last level PTB will make the
	 * processor raise a page fault (instead of e.g. thinking it's a non-leaf
	 * page that points to another PTB)
	 */
	debug("mapping the zero page as RWX=0 to catch null-pointer accesses\n");
	vm_map_page(0x0, 0x0, 0);
	info("kernel page tables ready!\n");
}

void mm_init()
{
	phys_addr_t freemem;
	/* Fill out the memory_map */
	mm_init_kmap();

	/* initialize the "early" physical memory allocator: the kernel
	 * init function identity mapped the bottom 3G of the physical
	 * address space, which is enough to bootstrap our page tables */
	info("initializing early physmem allocator\n");
	alloc_init(kmap.kernel_end - kmap.phys_offset);

	vm_init_kernel();

	/* use a proper virtual address from the direct map */
	kernel_root_ptb = (struct pgtable*)pa_to_va((phys_addr_t)kernel_root_ptb);

	/* "reinit" the allocator, this time pointing to the first currently
	 * free page, using the direct mapping */
	info("initializing direct-map kernel physmem allocator\n");
	freemem = alloc_get_page();
	alloc_init(pa_to_va(freemem));
}

int vm_map_page(u64 va, phys_addr_t pa, u64 flags)
{
	struct pgtable *ptb = kernel_root_ptb;
	phys_addr_t next_ptb_addr;
	pte_t *ptep, pte;
	u8 level = PTB_LEVELS - 1;

	trace("vm_map_page: %p -> %p, flags: %#x\n", va, pa, flags);

	if (!IS_ALIGNED(va) || !IS_ALIGNED(pa)) {
		critical("vm_map_page: attempted to map misaligned address ([%p -> %p])!\n", va, pa);
		BUG();
		return -1;
	}


	/* FIXME: once we move away from having a simple identity map we will want
	 * to actually validate whether @va is a valid kernel/user address; for now
	 * we just refuse to map a @va that doesn't have va[63:38] all equal. */
	if (va > 0x3fffffffff && va < 0xffffffc000000000) {
		critical("vm_map_page: attempted to map non-canonical virtual address %p (bits[63:38] aren't all equal)\n");
		BUG();
		return -1;
	}

	do {
		ptep = ptb_get_ptep(ptb, va_get_index(va, level));
		pte = *ptep;
		trace("vm_map_page: ptb=%p, level=%d, pte=%p\n", PPN_DOWN(ptep), level, pte);

		/* we reached the last-level PTB */
		if (level == 0) {
			/* if this PTE is already valid, we're trying to remap the VA. bail out */
			if (pte_valid(pte)) {
				critical("vm_map_page: attempted to remap VA 0x%x, PTE: 0x%x\n", va, pte);
				BUG();
				return -1;
			}

			/* time to turn the PTE into a leaf to map the VA -> PA */
			pte = pte_from_ppn(phys_to_ppn(pa));
			pte |= flags;
			pte |= PTE_VALID;

			trace("vm_map_page: mapping page, PTE=0x%x\n", pte);

			/* TODO: this should be atomic (call vma.fence) */
			*ptep = pte;

			break;
		}

		/* we're not at the last level yet and the PTE is invalid --
		 * we need to create the next-level PTB before proceeding */
		if (!pte_valid(pte)) {
			next_ptb_addr = alloc_zero_page();
			pte = pte_from_ppn(phys_to_ppn(next_ptb_addr));
			pte &= ~PTE_LEAF;
			pte |= PTE_VALID;

			trace("vm_map_page: allocated PTB at %p, PTE=%p\n", next_ptb_addr, pte);

			/* TODO: this should be atomic (call vma.fence) */
			*ptep = pte;
		}

		/* jump to the next-level PTB */
		ptb = pte_next_ptb(pte);

	} while(level-- > 0);
	
	return 0;
}

void dump_ptbs()
{
	/* stack of PTBs to visit */
	struct pgtable *allocated_ptbs[512];
	/* top of the stack */
	u64 top = 0;
	/* counter for the total number of PTBs */
	size_t total_ptbs = 0;
	pte_t pte;
	struct pgtable *ptbp;

	allocated_ptbs[top++] = kernel_root_ptb;

	while (top > 0) {
		total_ptbs++;
		ptbp = allocated_ptbs[--top];
		for (size_t i = 0; i < PTB_ENTRY_COUNT; i++) {
			/* skip non-leaf PTEs that point to PPN=0
			 * TODO: look further into this */
			if (!ptbp)
				continue;

			pte = *ptb_get_ptep(ptbp, i);

			if (!pte_valid(pte))
				continue;

			if(pte_leaf(pte)) {
				trace("(leaf) ");
			} else {
				trace("(non-leaf) ");
				allocated_ptbs[top++] = pte_next_ptb(pte);
			}

			trace("PPN=0x%x, V=%d, R=%d, W=%d, X=%d\n",
			       pte_get_ppn(pte), pte_valid(pte),
			       pte_readable(pte), pte_writable(pte), pte_executable(pte));
		}
	}

	trace("ptbdump: %d PTBs allocated in total\n", total_ptbs);
}

void dump_pte(pte_t pte)
{
	if (pte_leaf(pte)) {
		trace("(leaf) ");
	} else {
		trace("(non-leaf) ");
	}

	trace("PPN=0x%x, V=%d, R=%d, W=%d, X=%d\n",
	       pte_get_ppn(pte), pte_valid(pte),
	       pte_readable(pte), pte_writable(pte), pte_executable(pte));
}

void vm_init()
{
	phys_addr_t ptb;
	mm_init();
	ptb = va_to_pa(kernel_root_ptb);
	csr_write(CSR_SATP, CSR_SATP_MODE_SV39 | phys_to_ppn(ptb));
	__asm__("sfence.vma");
}
