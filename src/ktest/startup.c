#include <kernel/dtb.h>
#include <kernel/printf.h>
#include <kernel/types.h>
#include <arch/csr.h>

// the kernel entry point once we get to S-mode, implemented in kernel.c
extern void kmain();

/* device tree address, initialized in entry.S */
extern phys_addr_t kernel_dtb;
/* boot arguments passed through QEMU using the -append option */
extern char *bootargs;

void mmode_startup(void)
{
	info("[M] booting in M-mode\n");
	info("[M] starting early kernel boot\n");

	/* parse the kernel cmdline arguments from the device tree */
	info("[M] kernel_dtb: 0x%x\n", kernel_dtb);
	struct dtb_node node = dtb_match_node_name((struct dtb*)kernel_dtb, "chosen");
	struct dtb_prop prop = dtb_node_match_property((struct dtb*)kernel_dtb, &node, "bootargs");

	if (prop.name != NULL) {
		bootargs = prop.value;
	} else {
		bootargs = "";
	}

	info("[M] bootargs: %s\n", bootargs);

	info("[M] setting mstatus[MPP] = 01 to enter S-mode upon mret\n");
	u64 val;
	val = csr_read(CSR_MSTATUS);
	val &= ~CSR_MSTATUS_MPP;
	val |= CSR_MSTATUS_MPP_S;
	csr_write(CSR_MSTATUS, val);

	// disable paging
	info("[M] setting SATP = 0 to disable paging before entering S-mode\n");
	csr_write(CSR_SATP, 0);

	// delegate all exceptions/interrupts to S-mode
	// "setting a bit in medeleg or mideleg will delegate the
	// corresponding trap, when occurring in S-mode or U-mode,
	// to the S-mode trap handler"
	// https://riscv.github.io/riscv-isa-manual/snapshot/privileged/#_machine_trap_delegation_medeleg_and_mideleg_registers
	info("[M] delegating all traps to S-mode\n");
	csr_write(CSR_MEDELEG, 0xFFFFFFFFFFFFFFFFULL);
	csr_write(CSR_MIDELEG, 0xFFFFFFFFFFFFFFFFULL);

	// give S-mode to the entire physical memory
	// TODO: figure out how this actually works
	info("[M] giving S-mode access to the entire physical memory\n");
	csr_write(CSR_PMPADDR0, 0x3fffffffffffffffULL);
	csr_write(CSR_PMPCFG0, 0xf);

	csr_write(CSR_MEPC, kmain);

	info("[M] calling mret to enter S-mode\n");
	__asm__ __volatile__("mret");

	while (1) {

	}
	return;
}
