#include <stdint.h>

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

DEF(void, scr_update_1d1, (void *src, uint16_t *dst, unsigned h)) {
	uint32_t *s = (uint32_t*)src;
	uint32_t *d = (uint32_t*)dst;
	uint32_t a0, a1, a2, a3, m = ~0x200020;
	h <<= 8;
	do {
		a0 = *s++ & m; a1 = *s++ & m;
		a2 = *s++ & m; a3 = *s++ & m;
		*d++ = a0; *d++ = a1;
		*d++ = a2; *d++ = a3;
	} while ((h -= 8));
}

// pixel aspect ratio 5:4
DEF(void, scr_update_5x4d4, (void *src, uint16_t *d, unsigned h)) {
	uint16_t *s = (uint16_t*)src;
	uint32_t m = 0x07c0f81f, r = m & ~(m << 1);
	uint32_t a, b, m1 = ~0x200020;
	h <<= 8;
	do {
		a = s[0]; *d++ = a & m1;
		a = s[1] | s[2] << 16;
		a &= m1;
		b = (a >> 16 | a << 16) & m; b += a & m;
		b += r & b >> 1; b = b >> 1 & m;
		*d++ = a;
		*d++ = b | b >> 16;
		*d++ = a >> 16;
		a = s[3]; *d++ = a & m1;
		s += 4;
	} while ((h -= 4));
}

DEF(void, scr_update_1d2, (void *src, uint16_t *d, unsigned h)) {
	uint32_t *s = (uint32_t*)src;
	uint32_t m = 0x07c0f81f, r = m & ~(m << 1);
	uint32_t a, b, c;
	do {
		h -= 256 << 16;
		do {
			b = s[128]; a = *s++;
			c = (a & m) + (b & m);
			c += (a >> 16 | a << 16) & m;
			c += (b >> 16 | b << 16) & m;
			// rounding half to even
			c += (r & c >> 2) + r;
			c = c >> 2 & m;
			*d++ = c | c >> 16;
		} while ((int)(h += 2 << 16) < 0);
		s += 128;
	} while ((h -= 2));
}

// 01234567 8 * 2.5 / 4
// 23233232
// 0011 1223 3344 4556 6677

// pixel aspect ratio 5:4
DEF(void, scr_update_5x4d8, (void *src, uint16_t *d, unsigned h)) {
	uint16_t *s = (uint16_t*)src;
	unsigned m = 0x07c0f81f, r = m & ~(m << 1);
	unsigned a, b, c, t0, t1, w = 256, n;
#define X2(i, a) a += (r & a >> 2) + r; \
	a = a >> 2 & m; *d++ = a | a >> 16;
#define X3(i, a) a += (r & a >> 3) + r + (r << 1); \
	a = a >> 3 & m; *d++ = a | a >> 16;
#define X1 n = w; do { \
	a = X0(0); a += b = X0(1); X2(0, a) /* 0011 */ \
	b += X0(2) << 1; b += c = X0(3); X3(1, b) /* 1223 */ \
	c += a = X0(4); X2(2, c) /* 3344 */ \
	a += X0(5) << 1; a += b = X0(6); X3(3, a) /* 4556 */ \
	b += X0(7); X2(4, b) /* 6677 */ \
	s += 8; } while (n -= 8);
#define X0(i) (t0 = s[i], t1 = s[w + i], \
	((t0 | t0 << 16) & m) + ((t1 | t1 << 16) & m))
	do { X1 s += w; } while ((int)(h -= 2) > 0);
#undef X0
#undef X1
#undef X2
#undef X3
}

#define X1(inc, a0, a1) \
	a = s[0] & ~0x20; b = s[256] & ~0x20; s++; \
	d[-w2] = a; d[w2] = b; a |= b << 16; \
	a0 = a & m; a1 = (a >> 16 | a << 16) & m; \
	a = a0 + a1; a += r & a >> 1; a = a >> 1 & m; \
	*d = a | a >> 16; d += inc;
#define X2(inc) \
	a0 += b0; a1 += b1; a = a0 + a1; \
	a0 += r & a0 >> 1; a0 = a0 >> 1 & m; \
	a1 += r & a1 >> 1; a1 = a1 >> 1 & m; \
	a0 |= a1 >> 16 | a1 << 16; \
	d[-w2] = a0; d[w2] = a0 >> 16; \
	a += (r & a >> 2) + r; a = a >> 2 & m; \
	*d = a | a >> 16; d += inc;

// 012
// 466
// 0000 1111 1122 2222

// There is no perfect solution for this scaling.
// Any method will cause distortion on scrolling.
DEF(void, scr_update_4d3, (void *src, uint16_t *d, unsigned h)) {
	uint16_t *s = (uint16_t*)src;
	uint32_t m = 0x07c0f81f, r = m & ~(m << 1);
	uint32_t a, b, a0, a1, b0, b1;
	unsigned w2 = 342;
	do {
		h -= 255 << 16; d += w2;
		do {
			X1(1, a0, a1)
			X1(2, a0, a1)
			X1(-1, b0, b1)
			X2(2)
		} while ((int)(h += 3 << 16) < 0);
		X1(2, a0, a1)

		// 240 - 16 = 74 * 3 + 2
		if (!(h -= 2)) break;

		s += 256; d += w2;
		h -= 255 << 16;
		do {
			d[0] = *s++ & ~0x20;
			a = *s++ & ~0x20; b = *s++ & ~0x20;
			d[1] = a; d[3] = b; a |= b << 16;
			a = (a & m) + ((a >> 16 | a << 16) & m);
			a += r & a >> 1; a = a >> 1 & m;
			d[2] = a | a >> 16;
			d += 4;
		} while ((int)(h += 3 << 16) < 0);
		*d = *s++ & ~0x20; d += 2;
	} while (--h);
}

// 012
// 686
// 0000 0011 1111 1122 2222

// pixel aspect ratio 5:4
DEF(void, scr_update_5x4d3, (void *src, uint16_t *d, unsigned h)) {
	uint16_t *s = (uint16_t*)src;
	uint32_t m = 0x07c0f81f, r = m & ~(m << 1);
	uint32_t a, b, a0, a1, b0, b1, m2 = ~0x200020;
	unsigned w2 = 426;
	do {
		h -= 255 << 16; d += w2;
		do {
			X1(2, a0, a1)
			X1(-1, b0, b1)
			X2(3)
			X1(-1, a0, a1)
			X2(2)
		} while ((int)(h += 3 << 16) < 0);
		X1(1, a0, a1)

		// 240 - 16 = 74 * 3 + 2
		if (!(h -= 2)) break;

		s += 256; d += w2;
		h -= 255 << 16;
		do {
			a = *s++ & ~0x20; b = *s++ & ~0x20;
			d[0] = a; d[2] = b; a |= b << 16;
			a = (a & m) + ((a >> 16 | a << 16) & m);
			a += r & a >> 1; a = a >> 1 & m;
			d[1] = a | a >> 16;
			a = *s++ & ~0x20;
			d[4] = a; a |= b << 16;
			a = (a & m) + ((a >> 16 | a << 16) & m);
			a += r & a >> 1; a = a >> 1 & m;
			d[3] = a | a >> 16;
			d += 5;
		} while ((int)(h += 3 << 16) < 0);
		*d++ = *s++ & ~0x20;
	} while (--h);
}
#undef X1
#undef X2

#undef DEF
#ifdef USE_ASM
#define scr_update_1d1 scr_update_1d1_asm
#define scr_update_5x4d4 scr_update_5x4d4_asm
#define scr_update_1d2 scr_update_1d2_asm
#define scr_update_5x4d8 scr_update_5x4d8_asm
#define scr_update_4d3 scr_update_4d3_asm
#define scr_update_5x4d3 scr_update_5x4d3_asm
#endif

typedef void (*scr_update_t)(void *src, uint16_t *dst, unsigned h);

const scr_update_t scr_update_fn[] = {
	scr_update_1d1, scr_update_5x4d4,
	scr_update_1d2, scr_update_5x4d8,
	scr_update_4d3, scr_update_5x4d3
};


