#ifndef __DEFS_H__
#define __DEFS_H__

/*
 *  https://clang.llvm.org/docs/AttributeReference.html#always-inline-force-inline
 */
#define __always_inline 	inline __attribute__((__always_inline__))
#define __noreturn		__attribute__((__noreturn__))
#define __aligned(x)		__attribute__((__aligned__(x)))
#define __packed		__attribute__((__packed__))

#define DIV_ROUND_UP(num, div) (((num) - 1 + (div))/(div))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#endif

