# TODO

- Split tests in ktest accross different files, maybe store all suites in a
  global struct
- Maybe a simple tty abstraction? Make printf use the serial driver instead
  of writing directly to memory-mapped registers? `early_printk` for printing
  before the serial driver is setup?
- Rework the cmdline parsing logic to make it a little bit cleaner and more
  useful
    - Support specifying multiple test suites to run, maybe implement strsep to
      help with this
- Implement a dump of registers when panicking (turn this later into a proper
  stack trace)
  - Hopefully the stack trace part won't require parsing the ELF headers?
    Although implementing this _might_ be useful for loading programs when we
    implement userspace later
