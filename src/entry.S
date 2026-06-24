#include <arch/csr.h>

.section .text.init

.option norvc

.type start, @function
.global kstart
kstart:
    .cfi_startproc

.option push
.option norelax
    la gp, _global_pointer
.option pop
    # setup early stack
    la sp, _stack_top

    # clear .bss
    la t5, _bss_start
    la t6, _bss_end

# clear the kernel's .bss section
bss_clear:
    sd zero, (t5)
    addi t5, t5, 8
    bltu t5, t6, bss_clear

# save flattened device tree address
save_dtb:
    .option push
    .option norelax
    .extern _kernel_dtb
        la t0, _kernel_dtb
        sw a1, (t0)
    .option pop

# save hartid
    .option push
    .option norelax
        la t0, _hartid
        sw a0, (t0)
    .option pop

    j kmain

    .cfi_endproc
.end
