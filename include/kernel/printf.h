#ifndef __PRINTF_H__
#define __PRINTF_H__

#include <kernel/types.h>
#include <stdarg.h>

#define LOG_CRIT	0
#define LOG_ERR		1
#define LOG_WARN	2
#define LOG_INFO	3
#define LOG_DEBUG	4
#define LOG_TRACE	5
#define LOG_DEFAULT	LOG_INFO

void snprintf(char *buf, size_t size, const char *fmt_str, ...);

void print(const char *str);

void printk_set_level(int level);
void vprintk(int level, const char *fmt, va_list args);
void printk(int level, const char *fmt, ...);

#ifndef fmt_prefix
#define fmt_prefix(fmt) fmt
#endif

#define trace(fmt, ...) printk(LOG_TRACE, fmt_prefix(fmt), ##__VA_ARGS__)
#define debug(fmt, ...) printk(LOG_DEBUG, fmt_prefix(fmt), ##__VA_ARGS__)
#define info(fmt, ...) printk(LOG_INFO, fmt_prefix(fmt), ##__VA_ARGS__)
#define warn(fmt, ...) printk(LOG_WARN, fmt_prefix(fmt), ##__VA_ARGS__)
#define error(fmt, ...) printk(LOG_ERR, fmt_prefix(fmt), ##__VA_ARGS__)
#define critical(fmt, ...) printk(LOG_CRIT, fmt_prefix(fmt), ##__VA_ARGS__)

#endif
