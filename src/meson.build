add_project_arguments([
  # tell the compiler to be very thorough and emit warnings for lots
  # of different things (some of which are nitpicky, but nevertheless
  # important to keep in mind when working with C)
  '-Wall', '-Wextra',
  # tell the compiler to emit warnings for potentially problematic
  # integer conversions (e.g. signed-unsigned, etc.)
  '-Wconversion', '-Wsign-conversion',
  # tell the compiler that this is a freestanding project, e.g., does
  # *not* run on top of an operating system and cannot link against libc
  # or use the C standard library
  '-ffreestanding', '-nostdlib',
  # use the C11 standard for the project (same as the Linux kernel)
  '-std=c11',
  '-fno-pie', '-fno-pic',
  ],
  language : 'c')

common_sources = [
  'boot/init.S',
  'printf.c',
  'mm.c',
  'string.c',
  'panic.c',
  'alloc.c',
  'dtb.c',
  'timer.c',
  'trap_entry.S',
  'trap.c',
  'entry.S',
  'drivers/serial.c',
  'drivers/plic.c',
]

kernel_sources = common_sources + [
  'kmain.c',
]

ktest_sources = common_sources + [
  'ktest/kmain.c',
  'ktest/trap.S',
  'ktest/test.c',
]

kernel_img = executable('kernel.elf',
  kernel_sources,
  link_args : ['-T' + linker_script, '-z', 'norelro'],
  link_depends: [linker_script],
  include_directories: kernel_include_dir,
)

# test_img = executable('ktest.elf',
#   ktest_sources,
#   link_args : ['-T' + linker_script],
#   link_depends: [linker_script],
#   include_directories: kernel_include_dir,
# )
