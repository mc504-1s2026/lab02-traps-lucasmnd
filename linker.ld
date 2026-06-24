ENTRY(init);

_RAM_START = 0x80000000;
_KERNEL_VIRT_OFFSET = 0xffffffff00000000;


SECTIONS {
    /*
      The special linker variable dot `.' always contains the current output location counter.
      https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_chapter/ld_3.html
    */
    . = _RAM_START;

    PROVIDE(_start = . + _KERNEL_VIRT_OFFSET);
    .init : ALIGN(4K) {
        PROVIDE(_init_start = .);
        *(.init.text);
        *(.init.data);
        PROVIDE(_init_end = .);
    }

    /* The init loader will map the entire binary (init loader + kernel) to
     * 0xffffffff80000000, so we  */
    . += _KERNEL_VIRT_OFFSET;

    /* include entry point at the start of the binary */
    .text : AT(ADDR(.text) - _KERNEL_VIRT_OFFSET) ALIGN(4K) {
        PROVIDE(_text_start = .);
        *(.text);
        PROVIDE(_text_end = .);
    }

    .bss : AT(ADDR(.bss) - _KERNEL_VIRT_OFFSET) ALIGN(4K) {
        PROVIDE(_bss_start = .);
        *(.bss);
        *(.sbss);
        PROVIDE(_global_pointer = .);
        PROVIDE(_hartid = .);
        PROVIDE(_kernel_dtb = .);
        PROVIDE(_bootargs = .);
        /* define a 32KB stack */
        . += 32K;
        PROVIDE(_stack_top = .);
        /* define a 4096-byte space for global vars */
        . += 4096;
        PROVIDE(_bss_end = .);
    }

    .rodata : AT(ADDR(.rodata) - _KERNEL_VIRT_OFFSET) ALIGN(4K) {
        PROVIDE(_rodata_start = .);
        *(.rodata);
        *(.rodata.*);
        *(.srodata);
        PROVIDE(_rodata_end = .);
    }

    .data : AT(ADDR(.data) - _KERNEL_VIRT_OFFSET) ALIGN(4K) {
        PROVIDE(_data_start = .);
        *(.data);
        *(.sdata);
        PROVIDE(_data_end = .);
    }

    /*
     *  These are special compiler-generated sections that we don't really
     *  care about, but place at the end of the executable so it doesn't interfere
     *  with other sections
     */

    .data.rel.ro : { *(.data.rel.ro); }
    .eh_frame : { *(.eh_frame); }
    .comment : { *(.comment); }
    .riscv.attributes : { *(.riscv.attributes); }
    .symtab : { *(.symtab); }
    .shstrtab : { *(.shstrtab); }
    .strtab : { *(.strtab); }

    /*
     *  Ensure that the kernel image is page-aligned (makes things easier
     *  when setting up a linear mapping for physical memory)
     */
    . = ALIGN(4K);
    PROVIDE(_end = .);
}
