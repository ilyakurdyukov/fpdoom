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
	uint32_t a, b, a0, a1, b0, b1;
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

#define X2(a0, a1, i) a = a0 + a1; \
	a += r & a >> 1; a = a >> 1 & m; X1(a, i)
#define X3(a0, a1, a2, i) a = a0 + (a1 << 1) + a2; \
	a += (r & a >> 2) + r; a = a >> 2 & m; X1(a, i)
// 0123456789abcde
// 333333323333333
// 0001 1122 2333 4445 5566 6778 8899 9aaa bbbc ccdd deee
#define X4 /* 11d15 */ \
	X0(a0, 0) X0(a1, 1) \
	X3(a0, a0, a1, 0) /* 0001 */ \
	X0(a2, 2) X0(a3, 3) \
	X2(a1, a2, 1) /* 1122 */ \
	X3(a2, a3, a3, 2) /* 2333 */ \
	X0(a0, 4) X0(a1, 5) X0(a2, 6) \
	X3(a0, a0, a1, 3) /* 4445 */ \
	X2(a1, a2, 4) /* 5566 */ \
	X0(a3, 7) X0(a0, 8) \
	X3(a2, a3, a0, 5) /* 6778 */ \
	X0(a1, 9) X0(a2, 10) \
	X2(a0, a1, 6) /* 8899 */ \
	X3(a1, a2, a2, 7) /* 9aaa */ \
	X0(a3, 11) X0(a0, 12) \
	X3(a3, a3, a0, 8) /* bbbc */ \
	X0(a1, 13) X0(a2, 14) \
	X2(a0, a1, 9) /* ccdd */ \
	X3(a1, a2, a2, 10) /* deee */

// 255*11/15+1 = 188, 240*11/15 = 176
DEF(void, scr_update_11d15, (void *src, uint16_t *d, unsigned h)) {
	uint16_t *s = (uint16_t*)src;
	uint32_t m = 0x07c0f81f, r = m & ~(m << 1);
	unsigned k, w = 256, w2 = 188;
	uint32_t a, a0, a1, a2, a3;
	uint32_t buf[11 * 15], *t;
	do {
		h -= 17 << 10;
		do {
			uint16_t *s2 = s;
			for (t = buf, k = 0; k < 15; k++, s2 += w, t += 11) {
#define X0(a, i) a = s2[i] & ~0x20; a = (a | a << 16) & m;
#define X1(a, i) t[i] = a & m;
				X4
#undef X0
#undef X1
			}
			s += 15;
			for (t = buf, k = 0; k < 11; k++, d++, t++) {
				uint16_t *d2 = d;
#define X0(a, i) a = t[i * 11];
#define X1(a, i) a &= m; *d2 = a | a >> 16; d2 += w2;
				X4
#undef X0
#undef X1
			}
		} while ((int)(h += 1 << 10) < 0);
#define X0(a, i) a = *s & ~0x20; s += w; a = (a | a << 16) & m;
#define X1(a, i) a &= m; *d = a | a >> 16; d += w2;
		X4
#undef X0
#undef X1
		s += 1 - w; d += 1 - w2;
	} while ((int)(h -= 15) > 0);
}

// 2+252*6/7+2 = 220
// pixel aspect ratio 90:77
DEF(void, scr_update_90x77d105, (void *src, uint16_t *d, unsigned h)) {
	uint16_t *s = (uint16_t*)src;
	uint32_t m = 0x07c0f81f, r = m & ~(m << 1);
	unsigned k, w = 256, w2 = 220;
	uint32_t a, a0, a1, a2, a3;
	uint32_t buf[6 * 15], *t;
	do {
		for (k = 0; k < 2; k++, s++, d++) {
			uint16_t *s2 = s, *d2 = d;
#define X0(a, i) a = s2[i] & ~0x20; s2 += w; a = (a | a << 16) & m;
#define X1(a, i) a &= m; *d2 = a | a >> 16; d2 += w2;
			X4
#undef X0
#undef X1
		}
		h -= 36 << 10;
		do {
			uint16_t *s2 = s;
			for (t = buf, k = 0; k < 15; k++, s2 += w, t += 6) {
#define X0(a, i) a = s2[i] & ~0x20; a = (a | a << 16) & m;
#define X1(a, i) t[i] = a & m;
// 0123456
// 3434343
// 0001 1112 2233 3344 4555 5666
				X0(a0, 0) X0(a1, 1)
				X3(a0, a0, a1, 0) /* 0001 */
				X0(a2, 2) X0(a3, 3)
				X3(a1, a1, a2, 1) /* 1112 */
				X2(a2, a3, 2) /* 2233 */
				X0(a0, 4) X0(a1, 5) X0(a2, 6)
				X2(a3, a0, 3) /* 3344 */
				X3(a0, a1, a1, 4) /* 4555 */
				X3(a1, a2, a2, 5) /* 5666 */
#undef X0
#undef X1
			}
			s += 7;
			for (t = buf, k = 0; k < 6; k++, d++, t++) {
				uint16_t *d2 = d;
#define X0(a, i) a = t[i * 6];
#define X1(a, i) a &= m; *d2 = a | a >> 16; d2 += w2;
				X4
#undef X0
#undef X1
			}
		} while ((int)(h += 1 << 10) < 0);
		for (k = 0; k < 2; k++, s++, d++) {
			uint16_t *s2 = s, *d2 = d;
#define X0(a, i) a = s2[i] & ~0x20; s2 += w; a = (a | a << 16) & m;
#define X1(a, i) a &= m; *d2 = a | a >> 16; d2 += w2;
			X4
#undef X0
#undef X1
		}
		s += w * 14; d += w2 * 10;
	} while ((int)(h -= 15) > 0);
}
#undef X2
#undef X3
#undef X4

/* display aspect ratio 7:5, LCD pixel aspect ratio 7:10 */
DEF(void, scr_update_128x64, (void *src, uint16_t *dst, unsigned h)) {
	uint8_t *s = (uint8_t*)src, *d = (uint8_t*)dst;
	unsigned x, y;
	uint32_t a, b, t;
// 00112 23344
#define X(a0, n, a1) \
	for (a = a0, y = 0; y < n; y++) { \
		t = s2[1] + s2[3]; \
		t += (s2[5] + s2[7]) << 16; \
		s2 += 512; a += t << 1; \
	} \
	a -= a1; b = a >> 16; a &= 0xffff; \
	a = a * 0x924a + 0x3c000; /* div14 */ \
	b = b * 0x924a + 0x3c000; \
	a = (a + (a >> 5 & 0x4000) + 0x80000) >> (19 + 1); \
	b = (b + (b >> 5 & 0x4000) + 0x80000) >> (19 + 1);
	do {
		for (x = 0; x < 128; x += 2, s += 8) {
			const uint8_t *s2 = s;
			X(0, 4, t) *d++ = a; *d++ = b;
			X(t, 3, 0) d[126] = a; d[127] = b;
		}
		s += 512 * 6; d += 128;
	} while ((h -= 7));
#undef X
}

// 00011122 23334445 55666777
DEF(void, scr_update_96x68, (void *src, uint16_t *dst, unsigned h)) {
	uint8_t *s = (uint8_t*)src, *d = (uint8_t*)dst;
	unsigned x, y = 0;
	uint32_t a0, a1, a2, t0, t1, t2, t3;
	do {
		for (x = 0; x < 96; x += 3, s += 16, d += 3) {
			const uint8_t *s2 = s;
			a0 = a1 = a2 = 0;
			do {
				y += 56;
				do {
					t0 = s2[1] + s2[3];
					t1 = s2[7] + s2[9]; t1 += t1 << 1;
					t2 = s2[13] + s2[15];
					t1 += t3 = s2[5]; t0 += (t0 + t3) << 1;
					t1 += t3 = s2[11]; t2 += (t2 + t3) << 1;
					s2 += 512;
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
		s += 512 * (56 - 1); d += 96 * (17 - 1);
	} while ((h -= 56));
}

#if LIBC_SDIO == 0
DEF(void, scr_update_64x48, (void *src, uint16_t *dst, unsigned h)) {
	uint8_t *s = (uint8_t*)src, *d = (uint8_t*)dst;
	unsigned x, y = 0;
	do {
		for (x = 0; x < 64; x++, s += 8, d++) {
			const uint8_t *s2 = s;
			uint32_t a0 = 0, t0;
			do {
				y += 14;
				do {
					t0 = s2[1] + s2[3] + s2[5] + s2[7];
					a0 += t0 + (t0 << 1); s2 += 512;
				} while ((int)(y -= 3) > 0);
				a0 += t0 *= y;
				a0 = (a0 * 0x4925 + (3 << 18)) >> 20; /* div56 */
				*d = a0; a0 = -t0; d += 64;
			} while (y);
			d -= 64 * 3;
		}
		s += 512 * (14 - 1); d += 64 * (3 - 1);
	} while ((h -= 14));
}
#endif

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
	scr_update_4d3, scr_update_5x4d3,
	scr_update_11d15, scr_update_90x77d105,
	scr_update_128x64, scr_update_96x68,
#if LIBC_SDIO == 0
	scr_update_64x48, scr_update_64x48,
#endif
};

