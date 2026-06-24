#!/usr/bin/env bash

if [[ "$#" -lt 1 || "$#" -gt 2 ]]; then
    echo "usage: qemu.sh <path-to-kernel-elf> [--gdb]" >&2
    exit 1
fi

GDB=(-s -S)
if [[ -z "$2" || "$2" != "--gdb" ]]; then
    unset GDB
fi

qemu-system-riscv64 -nographic -m 1G -machine virt -bios none -serial mon:stdio -kernel "$1" "${GDB[@]}"
