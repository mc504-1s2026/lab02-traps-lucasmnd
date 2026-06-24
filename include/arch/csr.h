#ifndef __CSR_H__
#define __CSR_H__

#ifndef __ASSEMBLER__
#include <kernel/types.h>
#endif // __ASSEMBLER__

/*
 * this "stringifies" the CSR_* macros so that they can be
 * appended to the asm string inside the __asm__ sections below
 * (C moment)
 * https://stackoverflow.com/questions/2751870/how-exactly-does-the-double-stringize-trick-work
 */
#ifdef __ASSEMBLER__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

/* Unprivileged registers */
#define CSR_TIME		0xC01

/* M-mode registers */
#define CSR_MSTATUS		0x300
#define CSR_MSTATUS_MPP		(1ULL << 12) | (1ULL << 11)
#define CSR_MSTATUS_MPP_S	(1ULL << 11)

#define CSR_MEPC		0x341
#define CSR_MEDELEG		0x302
#define CSR_MIDELEG		0x303

/* S-mode registers */
#define CSR_PMPCFG0		0x3a0
#define CSR_PMPADDR0		0x3b0

#define CSR_SIE			0x104
#define CSR_SIE_SSIE		(1 << 1)
#define CSR_SIE_STIE		(1 << 5)
#define CSR_SIE_SEIE		(1 << 9)

#define CSR_SIP			0x144
#define CSR_SIP_SSIP		(1 << 1)
#define CSR_SIP_STIP		(1 << 5)
#define CSR_SIP_SEIP		(1 << 9)

#define CSR_STVEC		0x105
#define CSR_SCAUSE		0x142
#define CSR_STVAL		0x143

#define CSR_SSTATUS		0x100
#define CSR_SSTATUS_SIE		(1UL << 1);

#define CSR_SEPC		0x141

#define CSR_SATP		0x180
#define CSR_SATP_MODE_SHIFT	60
#define CSR_SATP_MODE_SV39	8UL  << CSR_SATP_MODE_SHIFT
#define CSR_SATP_MODE_SV48	9UL  << CSR_SATP_MODE_SHIFT
#define CSR_SATP_MODE_SV57	10UL << CSR_SATP_MODE_SHIFT

#define CSR_STIMECMP		0x14D

#ifndef __ASSEMBLER__
/* CSR access macros, very heavily inspired by arch/riscv/asm/csr.h in Linux */
#define csr_read(csr)						\
({								\
	register u64 __v;					\
	__asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)	\
			      : "=r" (__v)			\
			      :					\
			      : "memory");			\
	__v;							\
})

#define csr_write(csr, val)					\
({								\
	register u64 __v = (u64) val;				\
	__asm__ __volatile__ ("csrw " __ASM_STR(csr) ", %0"	\
			      :					\
			      : "rK" (__v)			\
			      : "memory");			\
})

#define csr_set(csr, mask)					\
({								\
	register u64 __mask = (u64) mask;			\
	__asm__ __volatile__ ("csrs " __ASM_STR(csr) ", %0"	\
			      :					\
			      : "rK" (__mask)			\
			      : "memory");			\
})

#define csr_clear(csr, mask)					\
({								\
	register u64 __mask = (u64) mask;			\
	__asm__ __volatile__ ("csrc " __ASM_STR(csr) ", %0"	\
			      :					\
			      : "rK" (__mask)			\
			      : "memory");			\
})

#define csr_read_clear(csr, mask)					\
({								\
	register u64 __mask = (u64) mask;			\
	register u64 __v;					\
	__asm__ __volatile__ ("csrrc %0, " __ASM_STR(csr) ", %1"	\
			      : "=r" (__v)			\
			      : "rK" (__mask)			\
			      : "memory");			\
	__v;							\
})

#endif // __ASSEMBLER__

#endif
