#include <stdarg.h>
#include <kernel/printf.h>
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/defs.h>
#include <stddef.h>

/* This is all heavily inspired by Linux, especially lib/vsprintf.c */

// TODO: we need to implement some sort of locking/buffering mechanism
// in order to avoid interleaved prints across harts. we will purposefully
// ignore this for now

char *uart = (char *)0x10000000;
void putchar(char c)
{
	*uart = c;
	return;
}

void print(const char *str)
{
	while (*str != '\0') {
		putchar(*str);
		str++;
	}
	return;
}

#define FMT_SIGNED		(1U << 0)
#define FMT_ZERO_PADDING	(1U << 1)
#define FMT_UPPERCASE		(1U << 2)
#define FMT_PREFIX		(1U << 3)

struct fmt_specifier {
	u8 flags;
	u8 base;
	u8 width;
	u8 length;
};

// this is very inspired by kernel/printk/printk.c
// within the Linux kernel

enum fmt_state {
	FMT_STATE_NONE,
	FMT_STATE_FLAG,
	FMT_STATE_WIDTH,
	FMT_STATE_LENGTH,
	FMT_STATE_SPECIFIER,
	FMT_STATE_CHAR,
	FMT_STATE_NUMBER,
	FMT_STATE_STR,
	FMT_STATE_PTR,
	FMT_STATE_INVALID,
	FMT_STATE_END,
};

struct fmt {
	const char *str;
	enum fmt_state state;
};

static char *number(char *buf, size_t size, u64 number, struct fmt_specifier *spec)
{
	static char *upper = "0123456789ABCDEF";
	static char *lower = "0123456789abcdef";
	static char *digits;
	char tmp[64] = {0};
	size_t i = 0;

	/* add minus sign for negative numbers (if signed) */
	if (spec->flags & FMT_SIGNED && (i64)number < 0 && size > 1) {
		number = (u64)((i64)number * -1);
		*buf++ = '-';
		size--;
	}

	/* add 0x prefix for hex numbers */
	if (spec->flags & FMT_PREFIX && size > 2) {
		*buf++ = '0';
		*buf++ = 'x';
		size -= 2;
	}

	if (number == 0) {
		*buf++ = '0';
		return buf;
	}


	/* write the number in inverse digit order to a buffer */
	digits = (spec->flags & FMT_UPPERCASE) ? upper : lower;
	while (number > 0) {
		tmp[i++] = digits[number % spec->base];
		number /= spec->base;
	}

	/* recalculate the amount of zero padding and insert zeroes */
	if (spec->flags & FMT_ZERO_PADDING && spec->width > i) {
		spec->width -= i;
		while (spec->width-- > 0 && --size > 0) {
			*buf++ = '0';
		}
	}

	/* uninvert the digits of the number */
	while (i-- > 0 && --size > 0) {
		*buf++ = tmp[i];
	}

	return buf;
}

static char *string(char *buf, size_t size, char *str)
{
	while (size > 0 && *str != 0) {
		*buf++ = *str++;
		size--;
	}

	return buf;
}

static char *character(char *buf, size_t size, u8 c)
{
	if (--size > 0) {
		*buf++ = (char)c;
	}

	return buf;
}

/* for now this is just a no-op of sorts, but i plan on implementing
 * something similar to Linux's %pX so that we can print symbol names
 * and offsets later (will help implementing a nice and readable stack
 * trace later once/if we implement ELF parsing)
 * */
static char *pointer(char *buf, size_t size, u64 ptr, struct fmt_specifier *spec)
{
	return number(buf, size, ptr, spec);

}

static int is_digit(char c)
{
	return '0' <= c && c <= '9';
}

/* yet another trick stolen from Linux */
static u64 number_truncate(u64 number, const struct fmt_specifier *spec)
{
	size_t shift = 64 - 8 * spec->length;
	number <<= shift;
	if (spec->flags & FMT_SIGNED) {
		return (u64)((i64) number >> shift);
	} else {
		return number >> shift;
	}
}

const char *parse_flag(const char *str, struct fmt_specifier *spec)
{
	switch (*str) {
	case '0':
		spec->flags |= FMT_ZERO_PADDING;
		str += 1;
		break;
	case '#':
		spec->flags |= FMT_PREFIX;
		str += 1;
		break;
	default:
		break;
	}

	return str;
}

const char *parse_width(const char *str, struct fmt_specifier *spec)
{
	u8 width = 0, digit;

	while (is_digit(*str)) {
		digit = *str++ - '0';
		width *= 10;
		width += digit;
	}

	spec->width = width;
	return str;
}

const char *parse_length(const char *str, struct fmt_specifier *spec)
{
	u8 length = 0;
	if (strncmp(str, "ll", 2) == 0) {
		str += 2;
		length = sizeof(long long);
	} else if (strncmp(str, "hh", 2) == 0) {
		str += 2;
		length = sizeof(char);
	} else if (strncmp(str, "l", 1) == 0) {
		str += 1;
		length = sizeof(long);
	} else if (strncmp(str, "h", 1) == 0) {
		str += 1;
		length = sizeof(short);
	}

	spec->length = length;
	return str;
}

const char *parse_type(const char *str, struct fmt *fmt, struct fmt_specifier *spec)
{
	enum fmt_state state;
	u8 flags = 0, base = 0;
	switch (*str) {
	case 'c':
		state = FMT_STATE_CHAR;
		break;
	case 'd':
		flags |= FMT_SIGNED;
		base = 10;
		state = FMT_STATE_NUMBER;
		break;
	case 'u':
		base = 10;
		state = FMT_STATE_NUMBER;
		break;
	case 'x':
		base = 16;
		state = FMT_STATE_NUMBER;
		break;
	case 'X':
		flags |= FMT_UPPERCASE;
		base = 16;
		state = FMT_STATE_NUMBER;
		break;
	case 's':
		state = FMT_STATE_STR;
		break;
	case 'p':
		flags |= FMT_PREFIX;
		base = 16;
		state = FMT_STATE_PTR;
		break;
	default:
		state = FMT_STATE_INVALID;
	}

	fmt->state = state;

	if (fmt->state != FMT_STATE_NONE)
		str += 1;

	spec->flags |= flags;
	spec->base = base;

	return str;
}

const char *parse_specifier(struct fmt *fmt, struct fmt_specifier *spec)
{
	const char *str = fmt->str;
	str = parse_flag(str, spec);
	str = parse_width(str, spec);
	str = parse_length(str, spec);
	str = parse_type(str, fmt, spec);
	fmt->str = str;
	return str;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
static void vsnprintf(char *buf, size_t size, const char *fmt_str, va_list args)
{
	struct fmt fmt = { .str = fmt_str, .state = FMT_STATE_NONE };
	struct fmt_specifier spec = {0};
	char *bufp = buf, *s;
	const char *percent;
	u8 c;
	size_t remaining = size;
	u64 num;
	while (fmt.state != FMT_STATE_END && size > 1) {
		remaining = size - (size_t) (bufp - buf);
		switch (fmt.state) {
		case FMT_STATE_NONE:
			if (*fmt.str == '\0') {
				fmt.state = FMT_STATE_END;
				continue;
			} else if (*fmt.str == '%') {
				memset(&spec, 0, sizeof(spec));
				percent = fmt.str;
				fmt.str++;
				fmt.state = FMT_STATE_SPECIFIER;
				continue;
			} else {
				*bufp++ = *fmt.str++;
				continue;
			}
			break;
		case FMT_STATE_SPECIFIER: {
			parse_specifier(&fmt, &spec);
			break;
		}
		case FMT_STATE_NUMBER:
			num = va_arg(args, u64);
			num = number_truncate(num, &spec);
			bufp = number(bufp, remaining, num, &spec);

			fmt.state = FMT_STATE_NONE;
			break;
		case FMT_STATE_STR:
			s = va_arg(args, char *);
			bufp = string(bufp, remaining, s);

			fmt.state = FMT_STATE_NONE;
			break;
		case FMT_STATE_CHAR:
			c = (u8)va_arg(args, u64);
			bufp = character(bufp, size, c);

			fmt.state = FMT_STATE_NONE;
			break;
		case FMT_STATE_PTR:
			num = va_arg(args, u64);
			bufp = pointer(bufp, size, num, &spec);

			fmt.state = FMT_STATE_NONE;
			break;
		case FMT_STATE_INVALID:
			if (--size > 0)
				*bufp++ = '%';
			fmt.str = percent + 1;
			fmt.state = FMT_STATE_NONE;
			break;
		default:
			fmt.state = FMT_STATE_NONE;
		}
	}

	*bufp = '\0';
}
#pragma clang diagnostic pop

#define PRINTK_BUF_SIZE 4096

static int current_level = LOG_INFO;

void printk_set_level(int level)
{
	current_level = level;
}

static char g_buf[PRINTK_BUF_SIZE];
void printk(int level, const char *fmt_str, ...)
{
	if (level > current_level)
		return;

	va_list args;
	va_start(args, fmt_str);
	vsnprintf(g_buf, PRINTK_BUF_SIZE, fmt_str, args);
	print(g_buf);
}

void vprintk(int level, const char *fmt_str, va_list args)
{
	if (level > current_level)
		return;

	vsnprintf(g_buf, PRINTK_BUF_SIZE, fmt_str, args);
	print(g_buf);
}

void snprintf(char *buf, size_t size, const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	vsnprintf(buf, size, fmt_str, args);
}
