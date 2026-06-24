#include <arch/csr.h>

# this arcane "ax" flag specifies to the linker that this section
# is "a" for allocatable (whatever this means) and "x" for executable.
# (source: # https://github.com/rust-lang/rust/issues/117323)
# lld will throw a bunch of errors without this
.section .init.text, "ax"

.type init, @function
.global init
init:
    # the init loader starts in M-mode; we have to prepare everything
    # for entering S-mode and handing control over to kstart, which is
    # the de-facto kernel entry point living in higher half

    ##########################
    # initial register setup #
    ##########################

    # set paging mode to Bare (i.e. disable paging)
    csrw satp, zero

    # set MPP=01 (S-mode) in mstatus[12:11]
    # this will make the hart enter S-mode upon an mret instruction

    # clear bits 12:11 with a bitmask
    # mstatus[12:11] -> 0b0001'1000'0000'0000 = 0x1800
    li t0, 0x1800
    csrc mstatus, t0
    # set mstatus[12:11] = 0b01 (S-mode)
    li t0, 0x800
    csrs mstatus, t0

    # delegate all exceptions/interrupts to S-mode
    li t0, -1
    csrw medeleg, t0
    csrw mideleg, t0

    # enable the Sstc extension (activating the stimecmp register,
    # otherwise stimecmp will be read-only zero)
    # https://docs.riscv.org/reference/isa/priv/machine.html#sec:menvcfg
    li t0, (1 << 63)
    csrs menvcfg, t0
    # allow S-mode to access the stimecmp register, otherwise we get an
    # illegal instruction fault on access
    # https://docs.riscv.org/reference/isa/priv/machine.html#mcounteren
    li t0, 0x2
    csrs mcounteren, t0

    # give S-mode access to all physical memory
    li t0, 0x3fffffffffffffff
    csrw pmpaddr0, t0
    li t0, 0xf
    csrw pmpcfg0, t0

    # jump to kmain when calling mret;
    # in order to load a higher-half address, we need to use
    # absolute addressing as illustrated in the
    # RISC-V Assembly Programmer's Manual, Chapter 12 (Absolute addressing)
    .option push
    .option norelax
    .global kstart
        lui t0, %hi(kstart)
        addi t0, t0, %lo(kstart)
    .option pop
    csrw mepc, t0

    ###################
    # console message #
    ###################

    # print message to console before changing page tables just so that we
    # know that the init code is alive and running

    # serial address; by default, each byte (char) that is written to this
    # address will be printed to stdout by QEMU
    li t0, 0x10000000
    # safe to use la here since the symbol is nearby, i.e., within 2G of our
    # current address, unlike the kstart higher half address; refer to
    # the "medany" code model detailed here:
    # https://www.sifive.com/blog/all-aboard-part-4-risc-v-code-models
    la t1, _init_msg
    init_print_msg_loop:
        lb t2, 0(t1)
        sb t2, 0(t0)
        addi t1, t1, 1
        bnez t2, init_print_msg_loop

    ######################
    # init pgtable setup #
    ######################

    # prepare an early page table to map the kernel to adress VA_END - 1G

    # satp:
    # 63    60 59            44 43                       0
    #   MODE         ASID                   PPN

    # calculate PPN=satp[43:0] for the address of the initial page table
    la t0, _init_ptb
    srli t0, t0, 12
    # set MODE=satp[63:60]=8
    li t1, 0x8000000000000000
    add t0, t0, t1

    # change page tables and flush the TLB for the current hart
    csrw satp, t0
    sfence.vma

    csrr a0, mhartid

    # switch to S-mode
    mret

    #############
    # init data #
    #############

    .section .init.data

    # first we identity map the first 3G of memory:
    # - all peripherals (rtc, serial, clint, plic, virtio mmio regions,
    #   as well as the PCI controller) live below the 0x40000000 (1G)
    #   physical address boundary; you can verify this yourself by
    #   dumping the device tree:
    #     $ qemu-system-riscv64 -machine virt -machine dumpdtb=riscv64.dtb
    #     $ dtc -qI dtb riscv64.dtb
    # - the range 1G - 2G is not particularly useful so we don't map it
    # - the kernel is mapped to 0x80000000 (2G), so we identity map it
    #
    # then we map the kernel to the higher half, to the last 2G of memory:
    # - VA 0xffffffff80000000 (VA_END - 2G) -> PA 0x80000000 (2G)
    # the location itself is arbitrary, but it's where the Linux kernel
    # loads itself in the virtual address space as well:
    # https://docs.kernel.org/arch/riscv/vm-layout.html#risc-v-linux-kernel-sv39
    .align 12
    _init_ptb:
    .dword 0x000000000000000f
    .dword 0x0000000000000000
    .dword 0x000000002000000f
    .rep 512 - 5
        .dword 0x0000000000000000
    .endr
    .dword 0x000000002000000f
    .dword 0x0000000000000000

    _init_msg:
    .string "init: relocating to 0xffffffff80000000 and jumping to S-mode\n"
.end
