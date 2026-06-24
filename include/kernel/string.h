#ifndef __STRING_H__
#define __STRING_H__

#include <kernel/types.h>

void memset(void *dest, int val, size_t count);
void memset64(uint64_t *dest, uint64_t val, size_t count);
void *memcpy(void *dest, const void *src, size_t size);
char *strncpy(char *dst, const char *src, size_t n);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);
size_t strlen(const char *str);
char *strstr(char *str, const char *substring);
u64 strtou64(char *str, size_t base);

#endif
