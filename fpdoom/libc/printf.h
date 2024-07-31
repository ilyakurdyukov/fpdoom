typedef struct {
	char *buf;
	unsigned count, lim;
} sprintf_param_t;

typedef struct {
	FILE *file;
	unsigned count;
} fprintf_param_t;

static void sprintf_put(void *param, int ch) {
	sprintf_param_t *x = param;
	unsigned pos = x->count;
	if (pos < x->lim)
		x->buf[pos] = ch;
	x->count = pos + 1;
}

static void fprintf_put(void *param, int ch) {
	fprintf_param_t *x = param;
	x->count++;
	fputc(ch, x->file);
}

static void printf_putn(void (*put)(void*, int), void *param, int ch, int n) {
	while (n > 0) { put(param, ch); n--; }
}

static void printf_body(void (*put)(void*, int), void *param, const char *fmt, va_list va) {
	int a;
	for (;;) {
		const char *fmt1;
		const char *str;
		unsigned len, hex_alpha, flags = 0;
		int width, prec;
		uintptr_t num;

#define PUT(a) put(param, a)

		a = *fmt++;
		if (!a) break;
		if (a != '%') {
			PUT(a);
			continue;
		}

		width = 0; prec = -1;
		fmt1 = fmt;
		a = *fmt++;
		// flags
		for (;;) {
			if (a == '0') flags |= 1;
			else if (a == '#') flags |= 2;
			else if (a == '-') flags |= 4;
			else if (a == '+') flags |= 8;
			else if (a == ' ') flags |= 16;
			else break;
			a = *fmt++;
		}
		// width
		if (a == '*') {
			width = va_arg(va, int);
			if (width >> 16) goto bad;
			a = *fmt++;
		} else
		while ((unsigned)a - '0' < 10) {
			width = width * 10 + (a - '0');
			if (width >> 16) goto bad;
			a = *fmt++;
		}
		// prec
		if (a == '.') {
			a = *fmt++;
			prec = 0;
			if (a == '*') {
				prec = va_arg(va, int);
				if (prec >> 16) goto bad;
				a = *fmt++;
			} else
			while ((unsigned)a - '0' < 10) {
				prec = prec * 10 + (a - '0');
				if (prec >> 16) goto bad;
				a = *fmt++;
			}
		}
		// length
		if (a == 'z') {	// size_t
			a = *fmt++;
		} else if (a == 'l') {	// long
			a = *fmt++;
		}
		// type
		if (a == 's') {
			str = va_arg(va, const char*);
			if (!str) str = "(null)";
str1:
			len = 0;
			while (len < (unsigned)prec && str[len]) len++;
			width -= len;
			if (!(flags & 4)) printf_putn(put, param, ' ', width);
			while (len) { PUT(*str++); len--; }

		} else if (a == 'u' || a == 'i' || a == 'd') {
			char buf[10]; // 32bit = 10, 64bit = 20
			int sign = 0;
			if (width < prec) width = prec;
			num = va_arg(va, unsigned);
			if (a != 'u') {
				sign = (int)num >> 31;
				num = ((int)num + sign) ^ sign;
				width += sign;
				sign &= '-';
				if (!sign && (flags & (8 | 16))) {
					width -= 1;
					sign = flags & 8 ? '+' : ' ';
				}
			}
			len = 0;
			do {
				a = num + '0';
#ifndef __thumb__
				num /= 10;
#else // help the compiler not to call __aeabi_uidiv
				num = (num * 0xcccccccdull) >> (32 + 3);
#endif
				buf[len++] = a - num * 10;
			} while (num);
			if (prec < (int)len) prec = len;
			if ((flags & 5) == 1 && width > prec) prec = width;
			width -= prec;
			if (!(flags & 4)) printf_putn(put, param, ' ', width);
			if (sign) PUT(sign);
			printf_putn(put, param, '0', prec - len);
			while (len) PUT(buf[--len]);

		} else if (a == 'c') {
			a = va_arg(va, int);
			width -= 1;
			if (!(flags & 4)) printf_putn(put, param, ' ', width);
			PUT(a);

		} else if (a == 'x' || a == 'X') {
			if (width < prec) width = prec;
			hex_alpha = 'a' - '0' - 10 - 'x' + a;
			num = va_arg(va, unsigned);
do_hex:
			if (flags & 2) width -= 2;
			for (len = sizeof(void*) * 2; len > 1; len--) {
				if (num >> (sizeof(num) * 8 - 4)) break;
				num <<= 4;
			}
			if (prec < (int)len) prec = len;
			if ((flags & 5) == 1 && width > prec) prec = width;
			width -= prec;
			if (!(flags & 4)) printf_putn(put, param, ' ', width);
			if (flags & 2) {
				PUT('0'); PUT('x' - 'a' + '0' + 10 + hex_alpha);
			}
			printf_putn(put, param, '0', prec - len);
			while (len) {
				a = num >> (sizeof(num) * 8 - 4);
				num <<= 4;
				if (a >= 10) a += hex_alpha;
				PUT(a + '0');
				len--;
			}

		} else if (a == 'p') {
			void *ptr = va_arg(va, void*);
			hex_alpha = 'a' - '0' - 10;
			if (!ptr) { str = "(nil)"; goto str1; }
			flags |= 2;
			num = (uintptr_t)ptr;
			goto do_hex;

		} else if (a == '%') {
			PUT(a);
			continue;
		} else goto bad;

		if (flags & 4) printf_putn(put, param, ' ', width);
		continue;
bad:
		fmt = fmt1;
		PUT('%');
	}
}

#undef PUT

#define PRINTF_PARAM_s(a1, a2) \
	sprintf_param_t param = { a1, 0, a2 }
#define PRINTF_PARAM_f(a1, a2) \
	fprintf_param_t param = { a1, 0 }

#define PRINTF_END_s \
	if (param.lim) \
		param.buf[param.count < param.lim ? param.count : param.lim - 1] = 0; \
	return param.count;
#define PRINTF_END_f \
	return param.count;

#define PRINTF_DEF(name, type, a1, a2, ...) \
int PREFIX(name)(__VA_ARGS__, ...) { \
	PRINTF_PARAM_##type(a1, a2); \
	va_list va; \
	va_start(va, fmt); \
	printf_body(type##printf_put, &param, fmt, va); \
	va_end(va); \
	PRINTF_END_##type; \
} \
\
int PREFIX(v##name)(__VA_ARGS__, va_list va) { \
	PRINTF_PARAM_##type(a1, a2); \
	printf_body(type##printf_put, &param, fmt, va); \
	PRINTF_END_##type; \
}

PRINTF_DEF(printf, f, stdout, 0, const char *fmt)
PRINTF_DEF(sprintf, s, buf, 0x7fffffff, char *buf, const char *fmt)
PRINTF_DEF(snprintf, s, buf, size, char *buf, size_t size, const char *fmt)
PRINTF_DEF(fprintf, f, file, 0, FILE *file, const char *fmt)

#undef PRINTF_DEF

