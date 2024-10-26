
#include "common.h"
#include "syscode.h"

#include <stdlib.h>
#include <stdio.h>

#define SCREENWIDTH 320
#define SCREENHEIGHT 200
#define USE_ASM

void (*app_pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
void (*app_scr_update)(uint8_t *src, void *dest);

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

#define PAL_UPDATE(load, process) \
	for (i = 0; i < 256; i++) { \
		r = load; g = load; b = load; process; \
	}

DEF(void, pal_update8, (uint8_t *pal, void *dest, const uint8_t *gamma)) {
	int i, r, g, b;
	uint8_t *d = (uint8_t*)dest - 256;
	if (!gamma)
		PAL_UPDATE(*pal++,
				*d++ = (r * 4899 + g * 9617 + b * 1868 + 0x2000) >> 14)
	else
		PAL_UPDATE(gamma[*pal++],
				*d++ = (r * 4899 + g * 9617 + b * 1868 + 0x2000) >> 14)
}

DEF(void, pal_update16, (uint8_t *pal, void *dest, const uint8_t *gamma)) {
	int i, r, g, b;
	uint16_t *d = (uint16_t*)dest - 256;
	if (!gamma)
		PAL_UPDATE(*pal++,
				*d++ = (r >> 3) << 11 | (g >> 2) << 5 | b >> 3)
	else
		PAL_UPDATE(gamma[*pal++],
				*d++ = (r >> 3) << 11 | (g >> 2) << 5 | b >> 3)
}

DEF(void, pal_update32, (uint8_t *pal, void *dest, const uint8_t *gamma)) {
	int i, r, g, b;
	uint32_t *d = (uint32_t*)dest - 256;
	if (!gamma)
		PAL_UPDATE(*pal++,
				*d++ = r << 22 | b << 11 | g << 1)
	else
		PAL_UPDATE(gamma[*pal++],
				*d++ = r << 22 | b << 11 | g << 1)
}
#undef PAL_UPDATE

DEF(void, scr_update_1d1, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint16_t *c16 = (uint16_t*)dest - 256;
	int x, y, w = 320, h = 240;
	for (y = 0; y < h; y++)
	for (x = 0; x < w; x++)
		*d++ = c16[*s++];
}

// 00rrrrrr rr000bbb bbbbb00g ggggggg0
// rrrrr000 000bbbbb 00000ggg ggg00000
// rounding half to even

DEF(void, scr_update_1d2, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint32_t *c32 = (uint32_t*)dest - 256;
	int x, y, w = 320, h = 256;
	unsigned a, b = 0x00400802;
	for (y = 0; y < h; y += 2, s += w)
	for (x = 0; x < w; x += 2, s += 2) {
		a = c32[s[0]] + c32[s[1]];
		a += c32[s[w]] + c32[s[w + 1]];
		a += (b & a >> 2) + b;
		a &= 0xf81f07e0;
		*d++ = a | a >> 16;
	}
}

#define X1(a0, o) \
	a = a0 << 2 & m; \
	d[o] = a | a >> 16;

#define X2(a0, a1, o) \
	a = a0 + a1; \
	a = (b & a) + (a << 1); a &= m; \
	d[o] = a | a >> 16;

DEF(void, scr_update_3d2, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint32_t *c32 = (uint32_t*)dest - 256;
	unsigned x, y, w = 320, w2 = 480, h = 212;
	uint32_t a, a0, a1, a2, a3;
	uint32_t b = 0x00400802 << 1, m = 0xf81f07e0;

	for (x = 0; x < w; x += 2, s += 2, d += 3) {
		a0 = c32[s[0]];
		a1 = c32[s[1]];
		X1(a0, 0)
		X2(a0, a1, 1)
		X1(a1, 2)
	}

	for (y = 0; y < h; y += 2, s += w, d += w2 * 2)
	for (x = 0; x < w; x += 2, s += 2, d += 3) {
		a0 = c32[s[0]];
		a1 = c32[s[1]];
		a2 = c32[s[w]];
		a3 = c32[s[w + 1]];

		X1(a0, 0)
		X2(a0, a1, 1)
		X1(a1, 2)

		X2(a0, a2, w2)
		a = a0 + a1 + a2 + a3;
		a += ((b & a >> 1) + b) >> 1;
		a &= m;
		d[w2 + 1] = a | a >> 16;
		X2(a1, a3, w2 + 2)

		X1(a2, w2 * 2)
		X2(a2, a3, w2 * 2 + 1)
		X1(a3, w2 * 2 + 2)
	}

	for (x = 0; x < w; x += 2, s += 2, d += 3) {
		a0 = c32[s[0]];
		a1 = c32[s[1]];
		X1(a0, 0)
		X2(a0, a1, 1)
		X1(a1, 2)
	}
}

// pixel aspect ratio 25:24
DEF(void, scr_update_25x24d20, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint32_t *c32 = (uint32_t*)dest - 256;
	unsigned x, y, w = 320, w2 = 400, h = 200;
	uint32_t a, a0, a1, a2, a3;
	uint32_t b = 0x00400802 << 1, m = 0xf81f07e0;
	do {
		for (y = 0; y < 3; y++)
		for (x = 0; x < w; x += 4, s += 4, d += 5) {
			a0 = c32[s[0]];
			X1(a0, 0)
			a0 = c32[s[1]];
			a1 = c32[s[2]];
			X1(a0, 1)
			X2(a0, a1, 2)
			X1(a1, 3)
			a0 = c32[s[3]];
			X1(a0, 4)
		}

		for (x = 0; x < w; x += 4, s += 4, d += 5) {
			a0 = c32[s[0]];
			a2 = c32[s[w]];
			X1(a0, 0)
			X2(a0, a2, w2)
			X1(a2, w2 * 2)

			a0 = c32[s[1]];
			a1 = c32[s[2]];
			a2 = c32[s[w + 1]];
			a3 = c32[s[w + 2]];

			X1(a0, 1)
			X2(a0, a1, 2)
			X1(a1, 3)

			X2(a0, a2, w2 + 1)
			a = a0 + a1 + a2 + a3;
			a += ((b & a >> 1) + b) >> 1;
			a &= m;
			d[w2 + 2] = a | a >> 16;
			X2(a1, a3, w2 + 3)

			X1(a2, w2 * 2 + 1)
			X2(a2, a3, w2 * 2 + 2)
			X1(a3, w2 * 2 + 3)

			a0 = c32[s[3]];
			a2 = c32[s[w + 3]];
			X1(a0, 4)
			X2(a0, a2, w2 + 4)
			X1(a2, w2 * 2 + 4)
		}
		s += w; d += w2 * 2;
	} while ((h -= 5));
}
#undef X1
#undef X2

// 0123456789abcdef 16 * 2.75 / 4
// 2333233323332333
// 0011 1222 3334 4555 6667 7788 999a aabb bccd ddee efff
#define X2(a0, a1, i) a = a0 + a1; \
	a = (b & a) + (a << 1); X1(a, i)
#define X3(a0, a1, a2, i) a = a0 + (a1 << 1) + a2; \
	a += ((b & a >> 1) + b) >> 1; X1(a, i)
#define X4 \
	X0(a0, 0) X0(a1, 1) X0(a2, 2) \
	X2(a0, a1, 0) /* 0011 */ \
	X3(a1, a2, a2, 1) /* 1222 */ \
	X0(a3, 3) X0(a0, 4) X0(a1, 5) \
	X3(a3, a3, a0, 2) /* 3334 */ \
	X3(a0, a1, a1, 3) /* 4555 */ \
	X0(a2, 6) X0(a3, 7) X0(a0, 8) \
	X3(a2, a2, a3, 4) /* 6667 */ \
	X2(a3, a0, 5) /* 7788 */ \
	X0(a1, 9) X0(a2, 10) X0(a3, 11) \
	X3(a1, a1, a2, 6) /* 999a */ \
	X2(a2, a3, 7) /* aabb */ \
	X0(a0, 12) X0(a1, 13) \
	X3(a3, a0, a1, 8) /* bccd */ \
	X0(a2, 14) X0(a3, 15) \
	X2(a1, a2, 9) /* ddee */ \
	X3(a2, a3, a3, 10) /* efff */

DEF(void, scr_update_11d16, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint32_t *c32 = (uint32_t*)dest - 256;
	unsigned k, w = 320, w2 = 220, h = 256;
	uint32_t a, a0, a1, a2, a3;
	uint32_t b = 0x00400802 << 1, m1 = 0x3fc7f9fe, m2 = 0xf81f07e0;
	uint32_t buf[11 * 16], *t;
	do {
		h -= 20 << 10;
		do {
			uint8_t *s2 = s;
			for (t = buf, k = 0; k < 16; k++, s2 += w, t += 11) {
#define X0(a, i) a = c32[s2[i]];
#define X1(a, i) t[i] = a >> 2 & m1;
				X4
#undef X0
#undef X1
			}
			s += 16;
			for (t = buf, k = 0; k < 11; k++, d++, t++) {
				uint16_t *d2 = d;
#define X0(a, i) a = t[i * 11];
#define X1(a, i) a &= m2; *d2 = a | a >> 16; d2 += w2;
				X4
#undef X0
#undef X1
			}
		} while ((int)(h += 1 << 10) < 0);
		s += w * 15; d += w2 * 10;
	} while ((h -= 16));
}
#undef X2
#undef X3
#undef X4

/* display aspect ratio 7:5, LCD pixel aspect ratio 7:10 */
DEF(void, scr_update_128x64, (uint8_t *s, void *dest)) {
	uint8_t *d = (uint8_t*)dest, *c8 = d - 256;
	unsigned x, y, h = 64;
	uint32_t a, b, t;
// 00112 23344
#define X(a0, n, a1) \
	for (a = a0, y = 0; y < n; y++) { \
		t = c8[s2[2]]; t += t << 16; \
		t += (c8[s2[0]] + c8[s2[1]]) << 1; \
		t += (c8[s2[3]] + c8[s2[4]]) << 17; \
		s2 += 320; a += t << 1; \
	} \
	a -= a1; b = a >> 16; a &= 0xffff; \
	a = (a * 0xea0f + (3 << 20)) >> 22; /* div35 */ \
	b = (b * 0xea0f + (3 << 20)) >> 22;
	do {
		for (x = 0; x < 128; x += 2, s += 5) {
			const uint8_t *s2 = s;
			X(0, 4, t) *d++ = a; *d++ = b;
			X(t, 3, 0) d[126] = a; d[127] = b;
		}
		s += 320 * 6; d += 128;
	} while ((h -= 2));
#undef X
}

// 0001112223 3344455566 6777888999
DEF(void, scr_update_96x68, (uint8_t *s, void *dest)) {
	uint8_t *d = (uint8_t*)dest, *c8 = d - 256;
	unsigned x, y = 0, h = 68 / 3;
	uint32_t a0, a1, t0, t1, t2, t3;
#define X1 \
	y += 10; do { \
		t0 = c8[s2[0]] + c8[s2[1]] + c8[s2[2]]; \
		t0 += (c8[s2[7]] + c8[s2[8]] + c8[s2[9]]) << 16; \
		t2 = c8[s2[3]] + (c8[s2[6]] << 16); \
		t1 = c8[s2[4]] + c8[s2[5]]; s2 += 320; \
		t0 += (t0 << 1) + t2; \
		t1 += (t1 + t2) << 1; \
		a0 += t0 + (t0 << 1); \
		a1 += t1 + (t1 << 1); \
	} while ((int)(y -= 3) > 0); \
	t2 = a0 + (t0 *= y); a0 = -t0; \
	t3 = a1 + (t1 *= y); a1 = -t1; \
	t3 = (t3 + (t3 << 16)) >> 16; \
	t0 = t2 & 0xffff; t1 = t2 >> 16; \
	d[0] = (t0 * 0x147b + (3 << 18)) >> 20; /* div100 */ \
	d[1] = (t3 * 0x147b + (3 << 18)) >> 20; \
	d[2] = (t1 * 0x147b + (3 << 18)) >> 20; d += 96;

	do {
		for (x = 0; x < 32; x++, s += 10, d += 3 - 96 * 3) {
			const uint8_t *s2 = s;
			a0 = a1 = 0;
			do { X1 } while (y);
		}
		s += 320 * 9; d += 96 * 2;
	} while (--h);
	for (x = 0; x < 32; x++, s += 10, d += 3 - 96 * 2) {
		const uint8_t *s2 = s;
		a0 = a1 = 0;
		for (;;) {
			X1 if (!y) break;
			y -= 2; t0 = 255 * 10; t0 |= t0 << 16;
			a0 += t0 << 1; a1 += t0;
		}
	}
#undef X1
#undef X
}

#undef DEF
#ifdef USE_ASM
#define pal_update16 pal_update16_asm
#define pal_update32 pal_update32_asm
#define scr_update_1d1 scr_update_1d1_asm
#define scr_update_1d2 scr_update_1d2_asm
#define scr_update_3d2 scr_update_3d2_asm
#define scr_update_25x24d20 scr_update_25x24d20_asm
#define scr_update_128x64 scr_update_128x64_asm
#define scr_update_96x68 scr_update_96x68_asm
#endif

uint8_t* framebuffer_init(void) {
	static const uint8_t pal_size[] = { 2, 4, 4, 4, 4, 1, 1 };
	static const struct {
		void (*pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
		void (*scr_update)(uint8_t *src, void *dest);
	} fn[] = {
		{ pal_update16, scr_update_1d1 },
		{ pal_update32, scr_update_1d2 },
		{ pal_update32, scr_update_3d2 },
		{ pal_update32, scr_update_11d16 },
		{ pal_update32, scr_update_25x24d20 },
		{ pal_update8, scr_update_128x64 },
		{ pal_update8, scr_update_96x68 },
	};
	int mode = sys_data.scaler;
	size_t size, size2 = pal_size[mode] << 8;
	uint8_t *dest;

	size = sys_data.display.w2 * sys_data.display.h2;
	dest = malloc(size * 2 + size2 + 28);
	dest = (void*)(dest + (-(intptr_t)dest & 31));
	dest += size2;

	app_pal_update = fn[mode].pal_update;
	app_scr_update = fn[mode].scr_update;
	return dest;
}

/* replaces "i_main.c" */
extern int myargc;
extern char** myargv;
void D_DoomMain(void);
extern int screenheight;

int main(int argc, char **argv) {
	myargc = argc; myargv = argv;
	screenheight = *(int32_t*)sys_data.user;
	D_DoomMain();
}

void lcd_appinit(void) {
	static const uint16_t dim[] = {
		320, 240, 240,  160, 128, 256,  480, 320, 214,
		220, 176, 256,  400, 240, 200,
		128,  64, 224,   96,  68, 226,
	};
	struct sys_display *disp = &sys_data.display;
	unsigned mode = sys_data.scaler - 1;
	unsigned w = disp->w1, h = disp->h1;
	if (h <= 68) {
		mode = 5;
		if (h == 68) mode = 6;
	} else if (mode >= 5) {
		switch (w) {
		case 400: mode = 4; break;
		case 480: mode = 2; break;
		case 240: case 320:
			mode = 0; break;
		case 128: case 160:
			mode = 1; break;
		case 176: case 220:
			mode = 3; break;
		default:
			fprintf(stderr, "!!! unsupported resolution (%dx%d)\n", w, h);
			exit(1);
		}
	}
	sys_data.scaler = mode;
	disp->w2 = dim[mode * 3];
	disp->h2 = dim[mode * 3 + 1];
	*(int32_t*)sys_data.user = dim[mode * 3 + 2];
}
