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

static void printf_body(void (*put)(void*, int), void *param, const char *fmt, va_list va) {
	int a;
	for (;;) {
		const char *fmt1;
		int width, prec, fill;

#define PUT(a) put(param, a)

		a = *fmt++;
		if (!a) break;
		if (a != '%') {
			PUT(a);
			continue;
		}
		width = 0; prec = -1; fill = ' ';
		
		fmt1 = fmt;
		for (;;) {
			const char *str;
			unsigned len, hex_alpha, bin_alt;
			uintptr_t num;
			a = *fmt++;
			if (a == 's') {
				str = va_arg(va, const char*);
				if (!str) str = "(null)";
str1:
				len = 0;
				while (len < (unsigned)prec && str[len]) len++;
				while (width > (int)len) { PUT(' '); width--; }
				while (len) { PUT(*str++); len--; }
				break;

			} else if (a == 'u' || a == 'i' || a == 'd') {
				char buf[10]; // 32bit = 10, 64bit = 20
				int sign = 0;
				if (width < prec) width = prec;
				num = va_arg(va, unsigned);
				if (a != 'u') {
					sign = (int)num >> 31;
					num = ((int)num + sign) ^ sign;
					width += sign;
				}
				len = 0;
				do {
					a = num + '0'; num /= 10;
					buf[len++] = a - num * 10;
				} while (num);
				while (width > (int)len) {
					a = width > prec ? fill : '0';
					PUT(a); width--;
				}
				if (sign) PUT('-');
				while (len) PUT(buf[--len]);
				break;

			} else if (a == 'c') {
				while (width > 1) { PUT(' '); width--; }
				a = va_arg(va, int);
				PUT(a);
				break;

			} else if (a == 'x' || a == 'X') {
				bin_alt = 0;
				if (width < prec) width = prec;
				hex_alpha = 'a' - '0' - 10 - 'x' + a;
				num = va_arg(va, unsigned);
do_hex:
				for (len = sizeof(void*) * 2; len > 1; len--) {
					if (num >> (sizeof(num) * 8 - 4)) break;
					num <<= 4;
				}
				if (bin_alt) len += 2;
				while (width > (int)len) {
					a = width > prec ? fill : ' ';
					PUT(a); width--;
				}
				if (bin_alt) { PUT('0'); PUT('x'); len -= 2; }
				while (len) {
					a = num >> (sizeof(num) * 8 - 4);
					num <<= 4;
					if (a >= 10) a += hex_alpha;
					PUT(a + '0');
					len--;
				}
				break;

			} else if (a == 'p') {
				void *ptr = va_arg(va, void*);
				hex_alpha = 'a' - '0' - 10;
				if (!ptr) { str = "(nil)"; goto str1; }
				bin_alt = 1;
				num = (uintptr_t)ptr;
				goto do_hex;

			} else if ((unsigned)(a - '0') < 10) {
				a -= '0';
				if (prec < 0) {
					if (!a && width == 0) fill = '0';
					width = width * 10 + a;
					if (width >> 16) goto bad;
				} else {
					prec = prec * 10 + a;
					if (prec >> 16) goto bad;
				}
			} else if (a == '%') {
				PUT(a);
				break;
			} else if (a == '.') {
				if (prec >= 0) goto bad;
				prec = 0;
			} else goto bad;
		}
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

