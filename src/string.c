#include <kernel/string.h>
#include <stdint.h>

void memset(void *dest, int val, size_t count)
{
	uint8_t *ptr = (uint8_t*)dest;
	while (count--) {
		*ptr++ = (uint8_t)val;
	}
}

void memset64(u64 *dest, u64 val, size_t count)
{
	while (count--) {
		*dest++ = val;
	}
}

void *memcpy(void *dest, const void *src, size_t count)
{
	uint8_t *ptr = (uint8_t*)dest;
	while (count--) {
		*ptr++ = *(uint8_t*)src++;
	}

	return dest;
}

char *strncpy(char *dst, const char *src, size_t n)
{
	while (n > 1 && *src != '\0') {
		*dst++ = *src++;
		n--;
	}

	dst[n] = 0;

	return dst;
}

int strcmp(const char *str1, const char *str2)
{
	while (*str1 != '\0') {
		if (*str1 != *str2)
			break;
		str1++; str2++;
	}

	return *(unsigned char*)str1 - *(unsigned char*)str2;
}

int strncmp(const char *str1, const char *str2, size_t n)
{
	while (n > 0 && *str1 != '\0') {
		if (--n == 0)
			break;
		if (*str1 != *str2)
			break;
		str1++;
		str2++;
	}

	return *(unsigned char*)str1 - *(unsigned char*)str2;
}

size_t strlen(const char *str)
{
	size_t len = 0;
	while (*str++ != '\0') len++;
	return len;
}

char *strstr(char *str, const char *substring)
{
	size_t i, j;
	bool match;

	for (i = 0; str[i] != '\0'; i++) {
		match = true;
		for (j = 0; substring[j] != '\0' && str[i + j] != '\0'; j++) {
			if (str[i + j] != substring[j]) {
				match = false;
				break;
			}
		}

		if (match)
			return &str[i];
	}

	return NULL;
}

u64 strtou64(char *str, size_t base)
{
	char *ptr = str;
	char c;
	u64 res, digit;
	bool neg = false;

	if (strncmp(str, "-", 1) == 0) {
		neg = true;
		str++;
	}

	if (strncmp(str, "0b", 2) == 0 || strncmp(str, "0B", 2) == 0) {
		if (base != 2) {
			return 0;
		} else {
			str += 2;
		}
	} else if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
		if (base != 16) {
			return 0;
		} else {
			str += 2;
		}
	}

	res = 0;
	while (*str != '\0') {
		c = *str++;
		digit = (u64) (c - '0');
		if (digit >= base) {
			return 0;
		}

		/* TODO: check overflow */

		res *= base;
		res += digit;
	}

	return res;
}
