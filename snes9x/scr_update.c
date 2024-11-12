#include <stdint.h>

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

#if USE_16BIT
DEF(void, scr_update16_1d1, (uint16_t *dst, const uint16_t *src, unsigned h)) {
	uint32_t *s = (uint32_t*)src, *d = (uint32_t*)dst;
	do {
		h -= 256 << 16;
		do {
			uint32_t a0, a1, a2, a3;
			a0 = *s++; a1 = *s++; a2 = *s++; a3 = *s++;
			*d++ = a0; *d++ = a1; *d++ = a2; *d++ = a3;
		} while ((int)(h += 8 << 16) < 0);
		s += 128;
	} while ((h -= 1));
}

// pixel aspect ratio 5:4
DEF(void, scr_update16_5x4d4, (uint16_t *d, const uint16_t *s, unsigned h)) {
	uint32_t m = 0x07e0f81f, r = m & ~(m << 1);
	uint32_t a, b, c;
	do {
		h -= 256 << 16;
		do {
			*d++ = s[0];
			*d++ = a = s[1]; b = s[2];
			a = a | b << 16;
			c = (a >> 16 | a << 16) & m; c += a & m;
			c += r & c >> 1; c = c >> 1 & m;
			*d++ = c | c >> 16;
			*d++ = b;
			*d++ = s[3];
			s += 4;
		} while ((int32_t)(h += 4 << 16) < 0);
		s += 256;
	} while (--h);
}

DEF(void, scr_update16_1d2, (uint16_t *d, const uint16_t *src, unsigned h)) {
	uint32_t *s = (uint32_t*)src;
	uint32_t m = 0x07e0f81f, r = m & ~(m << 1);
	uint32_t a, b, c;
	h -= 1;
	do {
		h -= 256 << 16;
		do {
			b = s[256]; a = *s++;
			c = (a & m) + (b & m);
			c += (a >> 16 | a << 16) & m;
			c += (b >> 16 | b << 16) & m;
			// rounding half to even
			c += (r & c >> 2) + r;
			c = c >> 2 & m;
			*d++ = c | c >> 16;
		} while ((int)(h += 2 << 16) < 0);
		s += 128 * 3;
	} while ((int)(h -= 2) > 0);
	if (h) return;
	h = 256;
	do {
		a = *s++;
		b = a >> 16 | a << 16;
		a = (a & m) + (b & m);
		a += (r & a >> 2) + r;
		a = a >> 2 & m;
		*d++ = a | a >> 16;
	} while ((h -= 2));
}

// 01234567 8 * 2.5 / 4
// 23233232
// 0011 1223 3344 4556 6677

// pixel aspect ratio 5:4
DEF(void, scr_update16_5x4d8, (uint16_t *d, const uint16_t *s, unsigned h)) {
	unsigned m = 0x07e0f81f, r = m & ~(m << 1);
	unsigned a, b, c, t0, t1, w = 256, n;
	h -= 1;
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
#define X0(i) (t0 = s[i], t1 = s[w * 2 + i], \
	((t0 | t0 << 16) & m) + ((t1 | t1 << 16) & m))
	do { X1 s += w * 3; } while ((int)(h -= 2) > 0);
#undef X0
	if (h) return;
#define X0(i) (t0 = s[i], (t0 | t0 << 16) & m)
	X1
#undef X0
#undef X1
#undef X2
#undef X3
}

#undef DEF
#ifdef USE_ASM
#define scr_update16_1d1 scr_update16_1d1_asm
#define scr_update16_5x4d4 scr_update16_5x4d4_asm
#define scr_update16_1d2 scr_update16_1d2_asm
#define scr_update16_5x4d8 scr_update16_5x4d8_asm
#endif

typedef void (*scr_update16_t)(uint16_t *d, const uint16_t *s, unsigned h);

const scr_update16_t scr_update16_fn[] = {
	scr_update16_1d1, scr_update16_5x4d4,
	scr_update16_1d2, scr_update16_5x4d8,
};

#else

DEF(void, pal_update8, (uint16_t *s, uint16_t *d)) {
	uint8_t *c8 = (uint8_t*)d - 256;
	unsigned a, i, r, g, b;
	for (i = 0; i < 256; i++) {
		a = s[i]; // BGR555
		r = a << 3 & 0xf8; r |= r >> 5;
		g = a >> 2 & 0xf8; g |= g >> 5;
		b = a >> 7 & 0xf8; b |= b >> 5;
		c8[i] = (r * 4899 + g * 9617 + b * 1868 + 0x2000) >> 14;
	}
}

DEF(void, pal_update16, (uint16_t *s, uint16_t *d)) {
	uint16_t *c16 = d - 256;
	unsigned a, i;
	for (i = 0; i < 256; i++) {
		a = s[i] & 0x7fff;
    c16[i] = a >> 10 | (a << 1 & 0x7c0) | a << 11;
	}
}

DEF(void, pal_update32, (uint16_t *s, uint16_t *d)) {
	uint32_t *c32 = (uint32_t*)d - 256;
	unsigned a, i;
	for (i = 0; i < 256; i++) {
		a = s[i] & 0x7fff;
    c32[i] = a >> 10 | (a & 0x3e0) << 17 | (a & 0x1f) << 11;
	}
}

#ifdef USE_ASM
#define pal_update16 pal_update16_asm
#define pal_update32 pal_update32_asm
#endif

typedef void (*pal_update_t)(uint16_t *s, uint16_t *d);

const pal_update_t pal_update_fn[] = {
	pal_update16, pal_update32,
	pal_update8, pal_update8,
};

DEF(void, scr_update_1d1, (uint16_t *dst, const uint8_t *s, uint16_t *c16, unsigned h)) {
	uint32_t *d = (uint32_t*)dst;
	unsigned n = h << 8;
	do {
		uint32_t a0, a1, a2, a3;
		a0 = *s++; a1 = *s++; a2 = *s++; a3 = *s++;
		a0 = c16[a0] | c16[a1] << 16;
		a2 = c16[a2] | c16[a3] << 16;
		*d++ = a0; *d++ = a2;
	} while ((n -= 4));
}

// pixel aspect ratio 5:4
DEF(void, scr_update_5x4d4, (uint16_t *d, const uint8_t *s, uint16_t *c16, unsigned h)) {
	unsigned m = 0x07c0f81f, r = m & ~(m << 1);
	unsigned n = h << 8;
	do {
		uint32_t a, b, aa;
		*d++ = c16[s[0]];
		a = c16[s[1]]; b = c16[s[2]];
		aa = a | b << 16;
		aa = (aa >> 16 | aa << 16) & m; aa += aa & m;
		aa += r & aa >> 1; aa = aa >> 1 & m;
		*d++ = a;
		*d++ = aa | aa >> 16;
		*d++ = b;
		*d++ = c16[s[3]];
		s += 4;
	} while ((n -= 4));
}

DEF(void, scr_update_1d2, (uint16_t *d, const uint8_t *s, uint16_t *c16, unsigned h)) {
	unsigned m = 0x07c0f81f, r = m & ~(m << 1);
	unsigned a, x, w = 256;
	uint32_t *c32 = (uint32_t*)(c16 - 256);
	h -= 1;
	do {
		for (x = 0; x < w; x += 2, s += 2) {
			a = c32[s[0]] + c32[s[1]];
			a += c32[s[w]] + c32[s[w + 1]];
			a += (r & a >> 2) + r;
			a = a >> 2 & m;
			*d++ = a | a >> 16;
		}
		s += w;
	} while ((int)(h -= 2) > 0);
	if (h) return;
	for (x = 0; x < w; x += 2, s += 2) {
		a = c32[s[0]] + c32[s[1]];
		a += (r & a >> 2) + r;
		a = a >> 2 & m;
		*d++ = a | a >> 16;
	}
}

// pixel aspect ratio 5:4
DEF(void, scr_update_5x4d8, (uint16_t *d, const uint8_t *s, uint16_t *c16, unsigned h)) {
	unsigned m = 0x07c0f81f, r = m & ~(m << 1);
	unsigned a, b, c, w = 256, n;
	uint32_t *c32 = (uint32_t*)(c16 - 256);
	h -= 1;
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
#define X0(i) (c32[s[i]] + c32[s[w + i]])
	do { X1 s += w; } while ((int)(h -= 2) > 0);
#undef X0
	if (h) return;
#define X0(i) c32[s[i]]
	X1
#undef X0
#undef X1
#undef X2
#undef X3
}

DEF(void, scr_update_128x64, (uint16_t *dst, const uint8_t *s, uint16_t *c16, unsigned h)) {
	uint8_t *d = (uint8_t*)dst, *c8 = (uint8_t*)c16 + 256;
	unsigned x, y;
	uint32_t a, b, t;
// 00112 23344
#define X(a0, n, a1) \
	for (a = a0, y = 0; y < n; y++) { \
		t = c8[s2[0]] + c8[s2[1]]; \
		t += (c8[s2[2]] + c8[s2[3]]) << 16; \
		s2 += 256; a += t << 1; \
	} \
	a -= a1; b = a >> 16; a &= 0xffff; \
	a = a * 0x924a + 0x3c000; /* div14 */ \
	b = b * 0x924a + 0x3c000; \
	a = (a + (a >> 5 & 0x4000) + 0x80000) >> (19 + 1); \
	b = (b + (b >> 5 & 0x4000) + 0x80000) >> (19 + 1);
	do {
		for (x = 0; x < 128; x += 2, s += 4) {
			const uint8_t *s2 = s;
			X(0, 4, t) *d++ = a; *d++ = b;
			X(t, 3, 0) d[126] = a; d[127] = b;
		}
		s += 256 * 6; d += 128;
	} while ((h -= 7));
#undef X
}

// 00011122 23334445 55666777
DEF(void, scr_update_96x68, (uint16_t *dst, const uint8_t *s, uint16_t *c16, unsigned h)) {
	uint8_t *d = (uint8_t*)dst, *c8 = (uint8_t*)c16 + 256;
	unsigned x, y = 0;
	uint32_t a0, a1, a2, t0, t1, t2, t3;
	do {
		for (x = 0; x < 96; x += 3, s += 8, d += 3) {
			const uint8_t *s2 = s;
			a0 = a1 = a2 = 0;
			do {
				y += 56;
				do {
					t0 = c8[s2[0]] + c8[s2[1]];
					t1 = c8[s2[3]] + c8[s2[4]]; t1 += t1 << 1;
					t2 = c8[s2[6]] + c8[s2[7]];
					t1 += t3 = c8[s2[2]]; t0 += (t0 + t3) << 1;
					t1 += t3 = c8[s2[5]]; t2 += (t2 + t3) << 1;
					s2 += 256;
					a0 += t0 + (t0 << 4);
					a1 += t1 + (t1 << 4);
					a2 += t2 + (t2 << 4);
				} while ((int)(y -= 17) > 0);
#define X(i) a##i += t##i *= y; \
	a##i = (a##i * 0x4925 + (3 << 22)) >> 24; /* div448 */ \
	d[i] = a##i; a##i = -t##i;
				X(0) X(1) X(2) d += 96;
#undef X
			} while (y);
			d -= 96 * 17;
		}
		s += 256 * (56 - 1); d += 96 * (17 - 1);
	} while ((h -= 56));
}

#if LIBC_SDIO == 0
DEF(void, scr_update_64x48, (uint16_t *dst, const uint8_t *s, uint16_t *c16, unsigned h)) {
	uint8_t *d = (uint8_t*)dst, *c8 = (uint8_t*)c16 + 256;
	unsigned x, y = 0;
	do {
		for (x = 0; x < 64; x++, s += 4, d++) {
			const uint8_t *s2 = s;
			uint32_t a0 = 0, t0;
			do {
				y += 14;
				do {
					t0 = c8[s2[0]] + c8[s2[1]] + c8[s2[2]] + c8[s2[3]];
					a0 += t0 + (t0 << 1); s2 += 256;
				} while ((int)(y -= 3) > 0);
				a0 += t0 *= y;
				a0 = (a0 * 0x4925 + (3 << 18)) >> 20; /* div56 */
				*d = a0; a0 = -t0; d += 64;
			} while (y);
			d -= 64 * 3;
		}
		s += 256 * (14 - 1); d += 64 * (3 - 1);
	} while ((h -= 14));
}
#endif

#undef DEF
#ifdef USE_ASM
#define scr_update_1d1 scr_update_1d1_asm
#define scr_update_5x4d4 scr_update_5x4d4_asm
#define scr_update_1d2 scr_update_1d2_asm
#define scr_update_5x4d8 scr_update_5x4d8_asm
#endif

typedef void (*scr_update_t)(uint16_t *d, const uint8_t *s, uint16_t *c16, unsigned h);

const scr_update_t scr_update_fn[] = {
	scr_update_1d1, scr_update_5x4d4,
	scr_update_1d2, scr_update_5x4d8,
	scr_update_128x64, scr_update_96x68,
#if LIBC_SDIO == 0
	scr_update_64x48, scr_update_64x48,
#endif
};

#endif
