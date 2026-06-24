#include <arch/csr.h>
#include <arch/pgtable.h>

#include <kernel/alloc.h>
#include <kernel/defs.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <kernel/string.h>
#include <kernel/types.h>

#include <ktest/test.h>
#include <stdbool.h>

#define TRAP_IRQ_BIT			(1ULL << 63)
#define TRAP_CODE_MASK			(TRAP_IRQ_BIT - 1)
#define TRAP_INST_ACCESS_FAULT		1
#define TRAP_LOAD_ACCESS_FAULT		5
#define TRAP_STORE_ACCESS_FAULT		7
#define TRAP_INST_PAGE_FAULT		12
#define TRAP_LOAD_PAGE_FAULT		13
#define TRAP_STORE_PAGE_FAULT		15

extern void trap_entry();
static bool ktest_fault_occurred = false;
static bool ktest_fault_expected = false;
static u64 ktest_fault_return_addr = 0;
void handle_trap()
{
	static u64 scause, stval, sepc;

	scause = csr_read(CSR_SCAUSE);
	stval = csr_read(CSR_STVAL);
	sepc = csr_read(CSR_SEPC);

	switch (scause) {
		case TRAP_INST_ACCESS_FAULT:
			error("instruction access fault at address 0x%x, sepc = 0x%x\n", stval, sepc);
			break;
		case TRAP_LOAD_ACCESS_FAULT:
			error("load access fault at address 0x%x, sepc = 0x%x\n", stval, sepc);
			break;
		case TRAP_STORE_ACCESS_FAULT:
			error("store access fault at address 0x%x, sepc = 0x%x\n", stval, sepc);
			break;
		case TRAP_INST_PAGE_FAULT:
			error("instruction page fault at address 0x%x, sepc = 0x%x\n", stval, sepc);
			break;
		case TRAP_LOAD_PAGE_FAULT:
			error("load page fault at address 0x%x, sepc = 0x%x\n", stval, sepc);
			break;
		case TRAP_STORE_PAGE_FAULT:
			error("store page fault at address 0x%x, sepc = 0x%x\n", stval, sepc);
			break;
		default:
			error("uncaught exception! cause: 0x%x, sepc = \n", scause, sepc);
	}	

	if (!ktest_fault_expected)
		panic("unexpected fault!\n");

	ktest_fault_occurred = true;
	ktest_fault_expected = false;

	csr_write(CSR_SEPC, ktest_fault_return_addr);
}

void trap_enable()
{
	csr_write(CSR_STVEC, trap_entry);
	csr_set(CSR_SIE, CSR_SIE_SEIE);
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void trap_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
	csr_write(CSR_STVEC, 0);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void test_is_aligned(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, IS_ALIGNED(0x00001000UL), true);
	KTEST_EXPECT_EQ(test, IS_ALIGNED(0x00001010UL), false);
	KTEST_EXPECT_EQ(test, IS_ALIGNED(0xabcab000UL), true);
	KTEST_EXPECT_EQ(test, IS_ALIGNED(0xabcabcabUL), false);
	KTEST_EXPECT_EQ(test, IS_ALIGNED(0x80000000UL), true);
	KTEST_EXPECT_EQ(test, IS_ALIGNED(0x80000400UL), false);
	KTEST_EXPECT_EQ(test, IS_ALIGNED(0xfffff000UL), true);
	KTEST_EXPECT_EQ(test, IS_ALIGNED(0x7fffffffUL), false);
}

void test_ppn_down(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, PPN_DOWN(0x00001000UL), 0x00001UL);
	KTEST_EXPECT_EQ(test, PPN_DOWN(0x00001010UL), 0x00001UL);
	KTEST_EXPECT_EQ(test, PPN_DOWN(0xabcab000UL), 0xabcabUL);
	KTEST_EXPECT_EQ(test, PPN_DOWN(0xabcabcabUL), 0xabcabUL);
	KTEST_EXPECT_EQ(test, PPN_DOWN(0x80000000UL), 0x80000UL);
	KTEST_EXPECT_EQ(test, PPN_DOWN(0x80000400UL), 0x80000UL);
	KTEST_EXPECT_EQ(test, PPN_DOWN(0x7fffffffUL), 0x7ffffUL);
	KTEST_EXPECT_EQ(test, PPN_DOWN(0xfffff000UL), 0xfffffUL);
}

void test_ppn_up(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, PPN_UP(0x00001000UL), 0x00001UL);
	KTEST_EXPECT_EQ(test, PPN_UP(0x00001010UL), 0x00002UL);
	KTEST_EXPECT_EQ(test, PPN_UP(0xabcab000UL), 0xabcabUL);
	KTEST_EXPECT_EQ(test, PPN_UP(0xabcabcabUL), 0xabcacUL);
	KTEST_EXPECT_EQ(test, PPN_UP(0x80000000UL), 0x80000UL);
	KTEST_EXPECT_EQ(test, PPN_UP(0x80000400UL), 0x80001UL);
	KTEST_EXPECT_EQ(test, PPN_UP(0x7fffffffUL), 0x80000UL);
	KTEST_EXPECT_EQ(test, PPN_UP(0xfffff000UL), 0xfffffUL);
}

struct ktest_case page_tests[] = {
	KTEST_CASE(test_is_aligned),
	KTEST_CASE(test_ppn_down),
	KTEST_CASE(test_ppn_up),
	{ },
};

struct ktest_suite page_unit_tests = {
	.name = "page_unit_tests",
	.tests = page_tests,
};

void test_va_get_index(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, va_get_index(0x0000000fff, 2), 0x000);
	KTEST_EXPECT_EQ(test, va_get_index(0x0000000fff, 1), 0x000);
	KTEST_EXPECT_EQ(test, va_get_index(0x0000000fff, 0), 0x000);
	KTEST_EXPECT_EQ(test, va_get_index(0x7ffffff000, 2), 0x1ff);
	KTEST_EXPECT_EQ(test, va_get_index(0x7ffffff000, 1), 0x1ff);
	KTEST_EXPECT_EQ(test, va_get_index(0x7ffffff000, 0), 0x1ff);
	KTEST_EXPECT_EQ(test, va_get_index(0x7ffc000000, 2), 0x1ff);
	KTEST_EXPECT_EQ(test, va_get_index(0x7ffc000000, 1), 0x1e0);
	KTEST_EXPECT_EQ(test, va_get_index(0x7ffc000000, 0), 0x000);
	KTEST_EXPECT_EQ(test, va_get_index(0xfffffffffffff000, 2), 0x1ff);
	KTEST_EXPECT_EQ(test, va_get_index(0xfffffffffffff000, 1), 0x1ff);
}

struct ktest_case virt_addr_tests[] = {
	KTEST_CASE(test_va_get_index),
	{ },
};

struct ktest_suite virt_addr_unit_tests = {
	.name = "virt_addr_unit_tests",
	.tests = virt_addr_tests,
};

void test_phys_to_ppn(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, phys_to_ppn(0x00001000UL), 0x00001UL);
	KTEST_EXPECT_EQ(test, phys_to_ppn(0x00001010UL), 0x00001UL);
	KTEST_EXPECT_EQ(test, phys_to_ppn(0xabcab000UL), 0xabcabUL);
	KTEST_EXPECT_EQ(test, phys_to_ppn(0xabcabcabUL), 0xabcabUL);
	KTEST_EXPECT_EQ(test, phys_to_ppn(0x80000000UL), 0x80000UL);
	KTEST_EXPECT_EQ(test, phys_to_ppn(0x80000400UL), 0x80000UL);
	KTEST_EXPECT_EQ(test, phys_to_ppn(0x7fffffffUL), 0x7ffffUL);
	KTEST_EXPECT_EQ(test, phys_to_ppn(0xfffff000UL), 0xfffffUL);
}

void test_ppn_to_phys(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, ppn_to_phys(0x00001UL), 0x00001000UL);
	KTEST_EXPECT_EQ(test, ppn_to_phys(0xabcabUL), 0xabcab000UL);
	KTEST_EXPECT_EQ(test, ppn_to_phys(0x80000UL), 0x80000000UL);
	KTEST_EXPECT_EQ(test, ppn_to_phys(0x7ffffUL), 0x7ffff000UL);
	KTEST_EXPECT_EQ(test, ppn_to_phys(0xfffffUL), 0xfffff000UL);
}

void test_pte_get_ppn(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, pte_get_ppn(0x00000403UL), 0x00001);
	KTEST_EXPECT_EQ(test, pte_get_ppn(0x3ffffc07UL), 0xfffff);
	KTEST_EXPECT_EQ(test, pte_get_ppn(0x2af2ac0bUL), 0xabcab);
	KTEST_EXPECT_EQ(test, pte_get_ppn(0x1ffffc0fUL), 0x7ffff);
	KTEST_EXPECT_EQ(test, pte_get_ppn(0x20000001UL), 0x80000);
}

void test_pte_from_ppn(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, pte_from_ppn(0x00001), 0x00000400UL);
	KTEST_EXPECT_EQ(test, pte_from_ppn(0xfffff), 0x3ffffc00UL);
	KTEST_EXPECT_EQ(test, pte_from_ppn(0xabcab), 0x2af2ac00UL);
	KTEST_EXPECT_EQ(test, pte_from_ppn(0x7ffff), 0x1ffffc00UL);
	KTEST_EXPECT_EQ(test, pte_from_ppn(0x80000), 0x20000000UL);
}

struct ktest_case ppn_tests[] = {
	KTEST_CASE(test_phys_to_ppn),
	KTEST_CASE(test_ppn_to_phys),
	KTEST_CASE(test_pte_get_ppn),
	KTEST_CASE(test_pte_from_ppn),
	{ },
};

struct ktest_suite ppn_unit_tests = {
	.name = "ppn_unit_tests",
	.tests = ppn_tests,
};

void test_pte_valid(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, pte_valid(0x00000402UL), false);
	KTEST_EXPECT_EQ(test, pte_valid(0x00000403UL), true);
	KTEST_EXPECT_EQ(test, pte_valid(0x3ffffc00UL), false);
	KTEST_EXPECT_EQ(test, pte_valid(0x3ffffc07UL), true);
	KTEST_EXPECT_EQ(test, pte_valid(0x2af2ac00UL), false);
	KTEST_EXPECT_EQ(test, pte_valid(0x2af2ac0bUL), true);
	KTEST_EXPECT_EQ(test, pte_valid(0x1ffffc00UL), false);
	KTEST_EXPECT_EQ(test, pte_valid(0x1ffffc0fUL), true);
	KTEST_EXPECT_EQ(test, pte_valid(0x20000000UL), false);
	KTEST_EXPECT_EQ(test, pte_valid(0x20000001UL), true);
}

void test_pte_readable(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, pte_readable(0x00000403UL), true);
	KTEST_EXPECT_EQ(test, pte_readable(0x3ffffc07UL), true);
	KTEST_EXPECT_EQ(test, pte_readable(0x2af2ac0bUL), true);
	KTEST_EXPECT_EQ(test, pte_readable(0x1ffffc0fUL), true);
	KTEST_EXPECT_EQ(test, pte_readable(0x20000001UL), false);
}

void test_pte_writable(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, pte_writable(0x00000403UL), false);
	KTEST_EXPECT_EQ(test, pte_writable(0x3ffffc07UL), true);
	KTEST_EXPECT_EQ(test, pte_writable(0x2af2ac0bUL), false);
	KTEST_EXPECT_EQ(test, pte_writable(0x1ffffc0fUL), true);
	KTEST_EXPECT_EQ(test, pte_writable(0x20000001UL), false);
}

void test_pte_executable(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, pte_executable(0x00000403UL), false);
	KTEST_EXPECT_EQ(test, pte_executable(0x3ffffc07UL), false);
	KTEST_EXPECT_EQ(test, pte_executable(0x2af2ac0bUL), true);
	KTEST_EXPECT_EQ(test, pte_executable(0x1ffffc0fUL), true);
	KTEST_EXPECT_EQ(test, pte_executable(0x20000001UL), false);
}

void test_pte_leaf(struct ktest *test)
{
	KTEST_EXPECT_EQ(test, pte_leaf(0x00000403UL), true);
	KTEST_EXPECT_EQ(test, pte_leaf(0x3ffffc07UL), true);
	KTEST_EXPECT_EQ(test, pte_leaf(0x2af2ac01UL), false);
	KTEST_EXPECT_EQ(test, pte_leaf(0x2af2ac0bUL), true);
	KTEST_EXPECT_EQ(test, pte_leaf(0x1ffffc01UL), false);
	KTEST_EXPECT_EQ(test, pte_leaf(0x1ffffc0fUL), true);
	KTEST_EXPECT_EQ(test, pte_leaf(0x20000001UL), false);
	KTEST_EXPECT_EQ(test, pte_leaf(0x20000005UL), true);
}


struct ktest_case pte_tests[] = {
	KTEST_CASE(test_pte_valid),
	KTEST_CASE(test_pte_readable),
	KTEST_CASE(test_pte_writable),
	KTEST_CASE(test_pte_executable),
	KTEST_CASE(test_pte_leaf),
	{ },
};

struct ktest_suite pte_unit_tests = {
	.name = "pte_unit_tests",
	.tests = pte_tests,
};

int alloc_tests_init(struct ktest_suite *suite)
{
	trap_enable();
	mm_init_kmap();
	return 0;
}

int alloc_tests_exit(struct ktest_suite *suite)
{
	trap_disable();
	return 0;
}

void alloc_tests_fail_without_init(struct ktest *test)
{
	phys_addr_t page_addr = 0;
	extern struct memory_map_t kmap;
	int ret = 0;

	bug_set_behavior(BUG_LOG);

	/* test that allocating/freeing pages before initializing
	 * the allocator doesn't work */

	page_addr = alloc_get_page();
	KTEST_ASSERT(test, page_addr == 0);

	ret = alloc_free_page(page_addr);
	KTEST_ASSERT(test, ret == -1);

	bug_set_behavior(BUG_PANIC);

	bug_set_behavior(BUG_LOG);

	alloc_init(kmap.kernel_end);
	page_addr = alloc_get_page();
	KTEST_ASSERT(test, page_addr != 0);

	ret = alloc_free_page(page_addr);
	KTEST_ASSERT(test, ret == 0);

	bug_set_behavior(BUG_PANIC);
}

void alloc_tests_sanity_checks(struct ktest *test)
{
	extern struct memory_map_t kmap;
	phys_addr_t page_addr = 0;
	int ret;

	/* initialize the allocator and then alloc a page */
	alloc_init(kmap.kernel_end);
	KTEST_EXPECT_NO_FAULT(test, {
		page_addr = alloc_get_page();
	});

	/* ensure the allocation didn't fail */
	KTEST_ASSERT(test, page_addr != 0);

	/* confirm that the address is aligned to 4K */
	KTEST_ASSERT(test, IS_ALIGNED(page_addr));

	/* make sure that the allocated physical page is located
	 * after the end of the kernel image in memory */
	KTEST_ASSERT(test, page_addr >= kmap.kernel_end);

	ret = alloc_free_page(page_addr);
	KTEST_ASSERT(test, ret == 0);
}

struct ktest_case alloc_test_cases[] = {
	KTEST_CASE(alloc_tests_fail_without_init),
	KTEST_CASE(alloc_tests_sanity_checks),
	{ },
};

struct ktest_suite alloc_tests = {
	.name = "alloc_tests",
	.tests = alloc_test_cases,
	.init = alloc_tests_init,
	.exit = alloc_tests_exit,
};

int vm_map_tests_init(struct ktest_suite *suite)
{
	trap_enable();
	return 0;
}

int vm_map_tests_exit(struct ktest_suite *suite)
{
	trap_disable();
	return 0;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
/**
 * vm_map_tests_kernel() - Basic sanity tests for the kernel mappings
 *
 * Enable paging and check the permissions for different kernel sections.
 */
void vm_map_tests_kernel(struct ktest *test)
{
	u64 *ptr;
	int (*fptr)(void);
	u64 val;

	printk_set_level(LOG_DEBUG);
	vm_init();

	/* if we've gotten past vm_init at all then this probably
	 * didn't happen, but check anyway */
	KTEST_ASSERT(test, ktest_fault_occurred == 0);

	/* test that accessing a null-pointer will page fault */
	ptr = (u64 *)0;
	KTEST_EXPECT_FAULT(test, {
		*ptr = 0;
	});

	KTEST_EXPECT_FAULT(test, {
		val = *ptr;
	});

	extern struct memory_map_t kmap;

	/* .text is read-execute, writes should fault */
	ptr = (u64 *)kmap.text_start;
	KTEST_EXPECT_FAULT(test, { *ptr = 0; });

	/* .bss is read-write, exec should fault */
	fptr = (int (*)(void))kmap.bss_start;
	KTEST_EXPECT_FAULT(test, { fptr(); });

	/* .rodata is read-only, write and exec should fault */
	ptr = (u64*)kmap.rodata_start;
	fptr = (int (*)(void))kmap.rodata_start;
	KTEST_EXPECT_FAULT(test, { *ptr = 0; });
	KTEST_EXPECT_FAULT(test, { fptr(); });

	/* finally, .data is read-write, so exec should fault */
	fptr = (int (*)(void))kmap.data_start;
	KTEST_EXPECT_FAULT(test, { fptr(); });
}
#pragma clang diagnostic pop /* "-Wunused-but-set-variable" */

/**
 * vm_map_tests_basic - Test basic virtual address mapping functionality
 *
 * - Allocate a page
 * - Map an arbitrary virtual address to it
 * - Write to the entire page
 * - Check that no page faults have occurred
 * - Read the entire page and check that it mactches what was written
 * - Check again that no page faults have occurred
 */
void vm_map_tests_basic(struct ktest *test)
{
	int ret = 0;
	phys_addr_t page_addr;
	u64 va;
	u8 *ptr, val;

	page_addr = alloc_get_page();
	va = 0xabcde000;
	KTEST_EXPECT_NO_FAULT(test, {
		ret = vm_map_page(va, page_addr, PTE_READ | PTE_WRITE);
	});
	KTEST_ASSERT(test, ret == 0);

	ptr = (u8 *) va;

	for (size_t i = 0; i < PAGE_SIZE; i++) {
		KTEST_EXPECT_NO_FAULT(test, {
			ptr[i] = 0xab;

		});
	}

	for (size_t i = 0; i < PAGE_SIZE; i++) {
		KTEST_EXPECT_NO_FAULT(test, {
			val = ptr[i];
		});
		KTEST_EXPECT_EQ(test, val, 0xab);
	}
}

void vm_map_tests_refuse_malformed_va(struct ktest *test)
{
	int ret = 0;
	phys_addr_t page_addr;

	page_addr = alloc_get_page();

	KTEST_EXPECT_NO_FAULT(test, {
		bug_set_behavior(BUG_LOG);
		ret = vm_map_page(0xabcdabcd00000000, page_addr, PTE_READ | PTE_WRITE);
		bug_set_behavior(BUG_PANIC);
	});
	KTEST_ASSERT(test, ret == -1);

	KTEST_EXPECT_NO_FAULT(test, {
		bug_set_behavior(BUG_LOG);
		ret = vm_map_page(0xffffffff00000000, page_addr, PTE_READ | PTE_WRITE);
		bug_set_behavior(BUG_PANIC);
	});
	KTEST_ASSERT(test, ret == 0);

	alloc_free_page(page_addr);
}

void vm_map_tests_refuse_misaligned_va(struct ktest *test)
{
	int ret = 0;
	phys_addr_t page_addr;

	page_addr = alloc_get_page();

	KTEST_EXPECT_NO_FAULT(test, {
		bug_set_behavior(BUG_LOG);
		ret = vm_map_page(0xabcdabcdabcda100, page_addr, PTE_READ | PTE_WRITE);
		bug_set_behavior(BUG_PANIC);
	});
	KTEST_ASSERT(test, ret == -1);

	KTEST_EXPECT_NO_FAULT(test, {
		bug_set_behavior(BUG_LOG);
		ret = vm_map_page(0xffffffff10000000, page_addr, PTE_READ | PTE_WRITE);
		bug_set_behavior(BUG_PANIC);
	});
	KTEST_ASSERT(test, ret == 0);

	alloc_free_page(page_addr);
}

void vm_map_tests_refuse_misaligned_pa(struct ktest *test)
{
	int ret = 0;
	phys_addr_t page_addr;

	page_addr = alloc_get_page();

	KTEST_EXPECT_NO_FAULT(test, {
		bug_set_behavior(BUG_LOG);
		ret = vm_map_page(0xffffffff20000000, page_addr + 4, PTE_READ | PTE_WRITE);
		bug_set_behavior(BUG_PANIC);
	});
	KTEST_ASSERT(test, ret == -1);

	KTEST_EXPECT_NO_FAULT(test, {
		bug_set_behavior(BUG_LOG);
		ret = vm_map_page(0xffffffff30000000, page_addr, PTE_READ | PTE_WRITE);
		bug_set_behavior(BUG_PANIC);
	});
	KTEST_ASSERT(test, ret == 0);

	alloc_free_page(page_addr);
}

void vm_map_tests_refuse_remap(struct ktest *test)
{
	int ret = 0;
	phys_addr_t page_addr;

	page_addr = alloc_get_page();

	KTEST_EXPECT_NO_FAULT(test, {
		bug_set_behavior(BUG_LOG);
		ret = vm_map_page(0xffffffff40000000, page_addr, PTE_READ | PTE_WRITE);
		bug_set_behavior(BUG_LOG);
	});
	KTEST_ASSERT(test, ret == 0);

	KTEST_EXPECT_NO_FAULT(test, {
		bug_set_behavior(BUG_LOG);
		ret = vm_map_page(0xffffffff40000000, page_addr, PTE_READ | PTE_WRITE);
		bug_set_behavior(BUG_LOG);
	});
	KTEST_ASSERT(test, ret == -1);

	alloc_free_page(page_addr);
}
#pragma clang diagnostic pop /* "-Wunused-parameter" */

struct ktest_case vm_map_test_cases[] = {
	KTEST_CASE(vm_map_tests_kernel),
	KTEST_CASE(vm_map_tests_basic),
	KTEST_CASE(vm_map_tests_refuse_misaligned_va),
	KTEST_CASE(vm_map_tests_refuse_misaligned_pa),
	KTEST_CASE(vm_map_tests_refuse_malformed_va),
	KTEST_CASE(vm_map_tests_refuse_remap),
	{ },
};

struct ktest_suite vm_map_tests = {
	.name = "vm_map_tests",
	.tests = vm_map_test_cases,
	.init = vm_map_tests_init,
	.exit = vm_map_tests_exit,
};

extern char *bootargs;
#define KTEST_ARG_MAX_SIZE 64
char ktest_arg[KTEST_ARG_MAX_SIZE];
void parse_bootarg_ktest(char *bootargs, char *buf, size_t size)
{
	const char *prefix = "ktest_run=";
	const size_t prefix_len = strlen(prefix);
	if (strncmp(bootargs, prefix, prefix_len) != 0) {
		buf[0] = '\0';
		return;
	}

	strncpy(buf, bootargs + prefix_len, size);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
void kmain()
{
	bool ktest_single_test = false;
	struct ktest_suite *suites[] = {
		&page_unit_tests,
		&virt_addr_unit_tests,
		&ppn_unit_tests,
		&pte_unit_tests,
		&alloc_tests,
		&vm_map_tests,
		NULL
	};

	info("entered S-mode\n");

	parse_bootarg_ktest(bootargs, ktest_arg, KTEST_ARG_MAX_SIZE);
	if (strlen(ktest_arg) > 0)
		ktest_single_test = true;
	error("ktest_arg: \"%s\", strlen=%d\n", ktest_arg, strlen(ktest_arg));

	for (size_t i = 0; suites[i] != NULL; i++) {
		if (!ktest_single_test) {
			ktest_suite_run(suites[i]);
		} else {
			if (strcmp(ktest_arg, suites[i]->name) == 0)
				ktest_suite_run(suites[i]);
		}
	}

	info("nothing else to do for now; spinning indefinitely...");
	while (1) {}
}
#pragma clang diagnostic pop
