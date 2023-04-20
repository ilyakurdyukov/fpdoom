#include "common.h"
#include <stdlib.h>

#define FN_ALIAS(fn, copy) \
	__asm__("\t.global " #copy "\n\t.set " #copy ", " #fn);

#if 0
unsigned fastchk16(unsigned crc, const void *src, int len) {
	uint8_t *s = (uint8_t*)src;

	while (len > 1) {
		crc += s[1] << 8 | s[0]; s += 2;
		len -= 2;
	}
	if (len) crc += *s;
	crc = (crc >> 16) + (crc & 0xffff);
	crc += crc >> 16;
	return crc & 0xffff;
}
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
uint16_t swap_be16(uint16_t v) {
	return v >> 8 | v << 8;
}

uint32_t swap_be32(uint32_t v) {
	uint32_t t = v >> 24 | v << 24, m = 0xff00;
	return t | (v >> 8 & m) | (v & m) << 8;
}
#endif

#if 0
void *memcpy(void *dst, const void *src, size_t len) {
	uint8_t *d = (uint8_t*)dst;
	const uint8_t *s = (const uint8_t*)src;

	/* copy aligned data faster */
	if (!(((uintptr_t)s | (uintptr_t)d) & 3)) {
		unsigned len4 = len >> 2;		
		uint32_t *d4 = (uint32_t*)d;
		const uint32_t *s4 = (const uint32_t*)s;
		while (len4--) *d4++ = *s4++;
		len &= 3;
		d = (uint8_t*)d4;
		s = (const uint8_t*)s4;
	}
	while (len--) *d++ = *s++;
	return dst;
}
#endif

#if 0
void *memset(void *dst, int c, size_t len) {
	uint8_t *d = (uint8_t*)dst;
	while (len--) *d++ = c;
	return dst;
}
#endif

void *memmove(void *dst, const void *src, size_t len) {
	uint8_t *d = (uint8_t*)dst;
	const uint8_t *s = (const uint8_t*)src;
	if ((uintptr_t)d - (uintptr_t)s >= len)
		return memcpy(dst, src, len);
	while (len--) d[len] = s[len];
	return dst;
}

size_t strlen(const char *src) {
	const char *s = src;
	while (*s++);
	return s - src - 1;
}

char *strcpy(char *dst, const char *src) {
	char *d = (char*)dst; int a;
	do *d++ = a = *src++; while (a);
	return dst;
}

char *strcat(char *dst, const char *src) {
	char *d = (char*)dst; int a;
	while (*d++);
	d--;
	do *d++ = a = *src++; while (a);
	return dst;
}

char *strncpy(char *dst, const char *src, size_t len) {
	char *d = (char*)dst; int a;
	while (len--) {
		a = *src++;
		if (!a) {
			do *d++ = a; while (len--);
			break;
		}
		*d++ = a;
	}
	return dst;
}

int strcmp(const char *src1, const char *src2) {
	const uint8_t *s1, *s2;
	int a, b;
	s1 = (const uint8_t*)src1;
	s2 = (const uint8_t*)src2;
	do {
		a = *s1++; b = *s2++; a -= b;
	} while (!a && b);
	return a;
}

int strncmp(const char *src1, const char *src2, size_t len) {
	const uint8_t *s1, *s2;
	int a = 0, b;
	s1 = (const uint8_t*)src1;
	s2 = (const uint8_t*)src2;
	while (len--) {
		a = *s1++; b = *s2++;
		a -= b;
		if (a || !b) break;
	}
	return a;
}

int strcasecmp(const char *src1, const char *src2) {
	const uint8_t *s1, *s2;
	int a, b, c;
	s1 = (const uint8_t*)src1;
	s2 = (const uint8_t*)src2;
	do {
		a = *s1++; b = *s2++;
		c = a - b;
		if (c) {
			if ((unsigned)(a - 'A') <= 'z' - 'a') a += 'a' - 'A';
			if ((unsigned)(b - 'A') <= 'z' - 'a') b += 'a' - 'A';
			c = a - b;
		}
	} while (!c && b);
	return c;
}

int strncasecmp(const char *src1, const char *src2, size_t len) {
	const uint8_t *s1, *s2;
	int a, b, c = 0;
	s1 = (const uint8_t*)src1;
	s2 = (const uint8_t*)src2;
	while (len--) {
		a = *s1++; b = *s2++;
		c = a - b;
		if (c) {
			if ((unsigned)(a - 'A') <= 'z' - 'a') a += 'a' - 'A';
			if ((unsigned)(b - 'A') <= 'z' - 'a') b += 'a' - 'A';
			c = a - b;
		}
		if (c || !b) break;
	}
	return c;
}

int memcmp(const void *src1, const void *src2, size_t len) {
	const uint8_t *s1, *s2;
	int a = 0, b;
	s1 = (const uint8_t*)src1;
	s2 = (const uint8_t*)src2;
	while (len--) {
		a = *s1++; b = *s2++; a -= b;
		if (a) break;
	}
	return a;
}

char* strchr(const char *str, int ch) {
	char a;
	for (; (a = *str); str++)
		if (a == ch) return (char*)str;
	return NULL;
}

char* strrchr(const char *str, int ch) {
	char a, *found = NULL;
	for (; (a = *str); str++)
		if (a == ch) found = (char*)str;
	return found;
}

int toupper(int ch) {
	if ((unsigned)(ch - 'a') <= 'z' - 'a')
		ch -= 'a' - 'A';
	return ch;
}

int tolower(int ch) {
	if ((unsigned)(ch - 'A') <= 'z' - 'a')
		ch += 'a' - 'A';
	return ch;
}

long strtol(const char *s, char **end, int base) {
	unsigned a; unsigned long val = 0, sign = 0;
	do a = *s++;
	while (a == ' ' || a == '\t' || a == '\n' || a == '\r');

	if (a == '-') sign--, a = *s++;
	else if (a == '+') a = *s++;

	if (!base) {
		base = 10;
		if (a == '0') {
			base = 8;
			a = *s++;
			if ((a | ('a' - 'A')) == 'x') {
				base = 16;
				a = *s++;
			}
		}
	}
	for (;;) {
		a -= '0';
		if (a > 10) {
			a -= 'A' - '0';
			a &= ~('a' - 'A');
			a += 10;
		}
		if (a >= (unsigned)base) break;
		val = val * base + a;
		a = *s++;
	}
	if (end) *end = (char*)s - 1;
	return (val ^ sign) - sign;
}

int atoi(const char *s) {
	return strtol(s, 0, 10);
}

FN_ALIAS(atoi, atol)

char* strdup(const char *str) {
	size_t n = strlen(str) + 1;
	char *p = malloc(n);
	return p ? memcpy(p, str, n) : p;
}


#if RAND_MAX & (RAND_MAX + 1)
#error
#endif

#if RAND_MAX == 0x7fff
#define RAND_BITS 15
#elif RAND_MAX == 0x7fffffff
#define RAND_BITS 31
#else
#error
#endif

static uint64_t rand_seed = 0;

void srand(unsigned seed) {
	rand_seed = seed;
}

int rand(void) {
	uint64_t m = 0x6765c793fa10079d; // 5^27
	return (rand_seed = rand_seed * m + 1) >> (64 - RAND_BITS);
}

