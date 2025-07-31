#include <stdlib.h>
#include <stdio.h>

#include "syscode.h"

void (*app_pal_update)(int i, int r, int g, int b);
void (*app_scr_update)(uint8_t *src, void *dest);

uint16_t *framebuf;

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

DEF(void, pal_update8, (int i, int r, int g, int b)) {
	uint8_t *pal = (uint8_t*)framebuf - 256;
	pal[i] = (r * 4899 + g * 9617 + b * 1868 + 0x2000) >> 14;
}

DEF(void, pal_update16, (int i, int r, int g, int b)) {
	uint16_t *pal = (uint16_t*)framebuf - 256;
	pal[i] = (r >> 3) << 11 | (g >> 2) << 5 | b >> 3;
}

DEF(void, pal_update32, (int i, int r, int g, int b)) {
	uint32_t *pal = (uint32_t*)framebuf - 256;
	pal[i] = r << 22 | b << 11 | g << 1;
}

// 012
// 334
// 00 01 11 22 22

DEF(void, scr_update_6x5d3, (uint8_t *s, void *dest)) {
	uint32_t *d = (uint32_t*)dest;
	uint16_t *c16 = (uint16_t*)dest - 256;
	uint32_t m = 0x07e0f81f, r = m & ~(m << 1);
	uint32_t x, y, a, b;
	for (y = 240; y; y -= 5, s += 160 * 2, d += 160 * 4)
	for (x = 160; x; x--) {
		a = c16[*s++]; a |= a << 16;
		b = c16[s[160 - 1]]; b |= b << 16;
		*d++ = a;
		a = (a & m) + (b & m);
		a += r & a >> 1; a = a >> 1 & m;
		a |= (a >> 16 | a << 16);
		d[160 - 1] = a;
		d[160 * 2 - 1] = b;
		a = c16[s[160 * 2 - 1]]; a |= a << 16;
		d[160 * 3 - 1] = a;
		d[160 * 4 - 1] = a;
	}
}

// rough scaling
// 012345678
// 222222211
// 00 11 22 33 44 55 66 78

DEF(void, scr_update_9x8d9, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint16_t *c16 = (uint16_t*)dest - 256;
	uint32_t m = 0x07e0f81f, r = m & ~(m << 1);
	uint32_t x, y, a, b;
	for (y = 128; y; y -= 8, s += 160) {
		for (x = 160 * 7; x; x--) *d++ = c16[*s++];
		for (x = 160; x; x--) {
			a = c16[*s++];
			b = c16[s[160 - 1]];
			a |= b << 16;
			a = (a & m) + ((a >> 16 | a << 16) & m);
			a += r & a >> 1; a = a >> 1 & m;
			*d++ = a | a >> 16;
		}
	}
}

// 012345678
// 455445544

DEF(void, scr_update_27x20d9, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint16_t *c16 = (uint16_t*)dest - 256;
	uint32_t m = 0x07e0f81f, r = m & ~(m << 1);
	uint32_t x, y, k, a, b;

	y = 144 / 9 * 2 + 1;
	k = 1; goto skip;
	do {
		for (x = 160; x; x--, d += 3) {
			a = c16[*s++]; b = c16[s[160 - 1]];
			d[0] = a; d[1] = a; d[2] = a; d += 480;
			d[0] = a; d[1] = a; d[2] = a; d += 480;
			a |= b << 16;
			a = (a & m) + ((a >> 16 | a << 16) & m);
			a += r & a >> 1; a = a >> 1 & m;
			a |= a >> 16;
			d[0] = a; d[1] = a; d[2] = a; d += 480;
			d[0] = b; d[1] = b; d[2] = b; d += 480;
			d[0] = b; d[1] = b; d[2] = b; d -= 480 * 4;
		}
		s += 160; d += 480 * 4;
		k = 2; if (y != 1) k += y & 1;
skip:
		do {
			for (x = 160; x; x--, d += 3) {
				a = c16[*s++];
				d[0] = a, d[1] = a, d[2] = a; d += 480;
				d[0] = a, d[1] = a, d[2] = a; d -= 480;
			}
			d += 480;
		} while (--k);
	} while (--y);
}

// 01234567 11/8 x
// 23323333
// 00 11 12 22 33 44 45 55 66 67 77

// 012345678 11/9 y
// 233223322
// 00 11 12 22 33 44 55 56 66 77 88

DEF(void, scr_update_99x88d72, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint16_t *c16 = (uint16_t*)dest - 256;
	uint32_t m = 0x07e0f81f, r = m & ~(m << 1);
	uint32_t x, y, k, l, a, b;

#define X(a, t, a0, a1) \
	a = a0 | a1 << 16; \
	t = (a & m) + ((a >> 16 | a << 16) & m); \
	a = t + (r & t >> 1); a = a >> 1 & m; \
	a |= a >> 16;

	y = 144 / 9 * 2 + 1;
	k = 1; goto skip;
	do {
		x = 160 / 8 * 2;
		do {
			a = c16[*s++];
			b = c16[s[160 - 1]];
			d[0] = a; d += 220;
			a |= b << 16;
			a = (a & m) + ((a >> 16 | a << 16) & m);
			a += r & a >> 1; a = a >> 1 & m;
			a |= a >> 16;
			d[0] = a; d += 220;
			d[0] = b; d -= 220 * 2; d++;
			l = 1 + (x & 1);
			do {
				uint32_t a0, a1, b0, b1, t0, t1;
				a0 = c16[*s++];
				a1 = c16[*s++];
				b0 = c16[s[160 - 2]];
				b1 = c16[s[160 - 1]];
				X(a, a, a0, a1)
				d[0] = a0; d[1] = a; d[2] = a1; d += 220;
				X(a, t0, a0, b0) d[0] = a;
				X(a, t1, a1, b1) d[2] = a;
				a = t0 + t1;
				a += (r & a >> 2) + r;
				a = a >> 2 & m;
				d[1] = a | a >> 16; d += 220;
				X(b, b, b0, b1)
				d[0] = b0; d[1] = b; d[2] = b1; d -= 220 * 2; d += 3;
			}	while (--l);
		} while (--x);
		s += 160; d += 220 * 2;
		k = 2; if (y != 1) k += y & 1;
skip:
		do {
			x = 160 / 8 * 2;
			do {
				*d++ = c16[*s++];
				l = 1 + (x & 1);
				do {
					a = c16[*s++];
					b = c16[*s++];
					*d++ = a;
					X(a, a, a, b)
					*d++ = a;
					*d++ = b;
				} while (--l);
			} while (--x);
		} while (--k);
	} while (--y);
#undef X
}

// 00001 11122 22333 34444
DEF(void, scr_update_128x64, (uint8_t *s, void *dest)) {
	uint8_t *d = (uint8_t*)dest, *c8 = (uint8_t*)dest - 256;
	unsigned x, y = 0, h = 144;
	uint32_t a0, a1, t0, t1, t2, a, b;
	do {
		for (x = 0; x < 128; x += 4, s += 5, d += 4) {
			const uint8_t *s2 = s;
			a0 = a1 = 0;
			do {
				y += 9;
				do {
					t0 = c8[s2[0]] | c8[s2[4]] << 16;
					t1 = c8[s2[1]] | c8[s2[3]] << 16;
					t2 = c8[s2[2]]; t2 |= t2 << 16;
					t0 = t0 * 4 + t1;
					t2 += t1; t1 += t2 * 2;
					s2 += 160;
					a0 += t0 * 4;
					a1 += t1 * 4;
				} while ((int)(y -= 4) > 0);
#define X(i) a##i += t##i *= y; \
	b = a##i >> 16; a = a##i ^ (b << 16); \
	a = (a * 0x2d83 + (3 << 18)) >> 20; /* div45 */ \
	b = (b * 0x2d83 + (3 << 18)) >> 20; /* div45 */ \
	d[i] = a; d[3 - i] = b; a##i = -t##i;
				X(0) X(1) d += 128;
#undef X
			} while (y);
			d -= 128 * 4;
		}
		s += 160 * (9 - 1); d += 128 * (4 - 1);
	} while ((h -= 9));
}

// 00011 12223 33444
DEF(void, scr_update_96x68, (uint8_t *s, void *dest)) {
	uint8_t *d = (uint8_t*)dest, *c8 = (uint8_t*)dest - 256;
	unsigned x, y = 0, h = 144;
	uint32_t a0, a1, a2, t0, t1, t2, t3, t4;
	do {
		for (x = 0; x < 96; x += 3, s += 5, d += 3) {
			const uint8_t *s2 = s;
			a0 = a1 = a2 = 0;
			do {
				y += 36;
				do {
					t0 = c8[s2[0]];
					t1 = c8[s2[1]];
					t2 = c8[s2[2]];
					t3 = c8[s2[3]];
					t4 = c8[s2[4]];
					t0 = t0 * 3 + t1 * 2;
					t1 = t1 + t2 * 3 + t3;
					t2 = t3 * 2 + t4 * 3;
					s2 += 160;
					a0 += t0 + (t0 << 4);
					a1 += t1 + (t1 << 4);
					a2 += t2 + (t2 << 4);
				} while ((int)(y -= 17) > 0);
#define X(i) a##i += t##i *= y; \
	a##i = (a##i * 0x2d83 + (3 << 20)) >> 22; /* div180 */ \
	d[i] = a##i; a##i = -t##i;
				X(0) X(1) X(2) d += 96;
#undef X
			} while (y);
			d -= 96 * 17;
		}
		s += 160 * (36 - 1); d += 96 * (17 - 1);
	} while ((h -= 36));
}

#if LIBC_SDIO == 0
// 00112 23344
DEF(void, scr_update_64x48, (uint8_t *s, void *dest)) {
	uint8_t *d = (uint8_t*)dest, *c8 = d - 256;
	unsigned x, y, h = 48;
	for (; h; h--, s += 160 * 2)
	for (x = 64; x; x -= 2, s += 5) {
		const uint8_t *s2 = s;
		uint32_t a0 = 0, a1 = 0, a2 = 0;
		for (y = 3; y; y--, s2 += 160) {
			a0 += c8[s2[0]] + c8[s2[1]];
			a1 += c8[s2[2]];
			a2 += c8[s2[3]] + c8[s2[4]];
		}
		a0 = a0 * 2 + a1; a1 += a2 * 2;
#define X(i) *d++ = (a##i * 0x8889 + (3 << 18)) >> 20; /* div15 */
		X(0) X(1)
#undef X
	}
}
#endif

#undef DEF
#ifdef USE_ASM
#define scr_update_6x5d3 scr_update_6x5d3_asm
#define scr_update_9x8d9 scr_update_9x8d9_asm
#define scr_update_128x64 scr_update_128x64_asm
#endif

void* framebuffer_init(unsigned size1) {
	static const uint8_t pal_size[] = { 2, 2, 2, 2, 1, 1, 1 };
	static const struct {
		void (*pal_update)(int i, int r, int g, int b);
		void (*scr_update)(uint8_t *src, void *dest);
	} fn[] = {
		{ pal_update16, scr_update_6x5d3 },
		{ pal_update16, scr_update_9x8d9 },
		{ pal_update16, scr_update_27x20d9 },
		{ pal_update16, scr_update_99x88d72 },
		{ pal_update8, scr_update_128x64 },
		{ pal_update8, scr_update_96x68 },
#if LIBC_SDIO == 0
		{ pal_update8, scr_update_64x48 },
#endif
	};
	int mode = sys_data.scaler;
	size_t size, size2 = pal_size[mode] << 8;
	uint8_t *p;

	size = sys_data.display.w2 * sys_data.display.h2 * 2;
	p = malloc(size + size1 + size2 + 28);
	p = (void*)(p + (-(intptr_t)p & 31));
	p += size2;
	framebuf = (uint16_t*)p;
	p += size;
	sys_framebuffer(framebuf);

	app_pal_update = fn[mode].pal_update;
	app_scr_update = fn[mode].scr_update;
	return p;
}

#define GS_MODE 4

void lcd_appinit(void) {
	static const uint16_t dim[] = {
		320, 240,  160, 128,  480, 320,
		220, 176,
		// monochrome
		128, 64,  96, 68,  64, 48,
	};
	struct sys_display *disp = &sys_data.display;
	unsigned mode = sys_data.scaler - 1;
	unsigned w = disp->w1, h = disp->h1;
	if (h <= 68) {
		mode = GS_MODE;
		if (h == 68) mode = GS_MODE + 1;
		if (h == 48) mode = GS_MODE + 2;
	} else if (mode >= GS_MODE) {
		switch (w) {
		case 480: mode = 2; break;
		case 400:
		case 240: case 320:
			mode = 0; break;
		case 128: case 160:
			mode = 1; break;
		case 176: case 220:
			mode = 3; break;
		default:
err:
			fprintf(stderr, "!!! unsupported resolution (%dx%d)\n", w, h);
			exit(1);
		}
	}
	sys_data.scaler = mode;
	disp->w2 = dim[mode * 2];
	disp->h2 = dim[mode * 2 + 1];
}
