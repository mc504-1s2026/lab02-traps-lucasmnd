#ifndef __BITS_H__
#define __BITS_H__

#include <kernel/types.h>

#define UINT64_MAX ((uint64_t) (~0))

/* Use this definition to set '1' to all bits in range: low-high */
#define BITMASK64(high, low) (uint64_t) ((UINT64_MAX << (low)) & (UINT64_MAX >> (64 - 1 - (high))))

#endif
