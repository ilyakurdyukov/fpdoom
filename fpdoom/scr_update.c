
#include "common.h"
#include "syscode.h"

#include <stdlib.h>
#include <stdio.h>

#define SCREENWIDTH 320
#define SCREENHEIGHT 200

void (*app_pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
void (*app_scr_update)(uint8_t *src, void *dest);

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

#define PAL_UPDATE(load, process) \
	for (i = 0; i < 256; i++) { \
		r = load; g = load; b = load; process; \
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

#ifdef USE_ASM
#define pal_update32 pal_update32_asm
#endif

DEF(void, pal_update32_8d9, (uint8_t *pal, void *dest, const uint8_t *gamma)) {
	int i, r, g, b;
	uint32_t *d = (uint32_t*)dest - 256 * 2;
#define X(i) ((i) * 8 + 4) / 9
#define X4(i) X(i), X(i + 1), X(i + 2), X(i + 3)
#define X16(i) X4(i), X4(i + 4), X4(i + 8), X4(i + 12)
#define X64(i) X16(i), X16(i + 16), X16(i + 32), X16(i + 48)
	static const uint8_t lut[256] = { X64(0), X64(64), X64(128), X64(192) };
#undef X64
#undef X16
#undef X4
#undef X

	pal_update32(pal, dest, gamma);
	if (!gamma)
		pal_update32(pal, (uint32_t*)dest - 256, lut);
	else
		PAL_UPDATE(lut[gamma[*pal++]],
				*d++ = r << 22 | b << 11 | g << 1)
}

#undef PAL_UPDATE

DEF(void, scr_update_1d1, (uint8_t *src, void *dest)) {
	uint8_t *s = src;
	uint16_t *d = (uint16_t*)dest;
	uint16_t *c16 = (uint16_t*)dest - 256;
	int x, y, w = SCREENWIDTH, h = SCREENHEIGHT;
	for (y = 0; y < h; y++)
	for (x = 0; x < w; x++)
		*d++ = c16[*s++];
}

// 00rrrrrr rr000bbb bbbbb00g ggggggg0
// rrrrr000 000bbbbb 00000ggg ggg00000
// rounding half to even

DEF(void, scr_update_1d2_custom, (uint8_t *s, uint16_t *d, uint32_t *c32, int h)) {
	int x, y, w = SCREENWIDTH;
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

#ifdef USE_ASM
#define scr_update_1d2_custom scr_update_1d2_custom_asm
#endif

DEF(void, scr_update_1d2, (uint8_t *src, void *dest)) {
	uint8_t *s = src;
	uint16_t *d = (uint16_t*)dest;
	uint32_t *c32 = (uint32_t*)dest - 256;
	int h = SCREENHEIGHT;
	scr_update_1d2_custom(s, d, c32, h);
}

DEF(void, scr_update_3d2, (uint8_t *src, void *dest)) {
	uint8_t *s = src;
	uint16_t *d = (uint16_t*)dest;
	uint32_t *c32 = (uint32_t*)dest - 256;
	int x, y, w = SCREENWIDTH, h = SCREENHEIGHT;
	int w2 = SCREENWIDTH * 3 / 2;
	uint32_t a, a0, a1, a2, a3;
	uint32_t b = 0x00400802 << 1, m = 0xf81f07e0;
	for (y = 0; y < h; y += 2, s += w, d += w2 * 2)
	for (x = 0; x < w; x += 2, s += 2, d += 3) {
		a0 = c32[s[0]];
		a1 = c32[s[1]];
		a2 = c32[s[w]];
		a3 = c32[s[w + 1]];

#define X1(a0, o) \
	a = a0 << 2 & m; \
	d[o] = a | a >> 16;

#define X2(a0, a1, o) \
	a = a0 + a1; \
	a = (b & a) + (a << 1); a &= m; \
	d[o] = a | a >> 16;

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
}

// pixel aspect ratio 25:24
DEF(void, scr_update_25x24d20, (uint8_t *src, void *dest)) {
	uint8_t *s = src;
	uint16_t *d = (uint16_t*)dest;
	uint32_t *c32 = (uint32_t*)dest - 256;
	int x, y, w = SCREENWIDTH, h = SCREENHEIGHT;
	int w2 = SCREENWIDTH * 5 / 4;
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

typedef enum {false, true} boolean;
extern boolean menuactive, viewactive;
extern int screenblocks;

DEF(void, scr_update_2d3_crop, (uint8_t *src, void *dest)) {
	uint8_t *s = src;
	uint16_t *d = (uint16_t*)dest;
	uint32_t *c32 = (uint32_t*)dest - 256 * 2;
	int x, y, w = SCREENWIDTH, h = SCREENHEIGHT;
	uint32_t a, a0, a1, a2, a3, a4, a5;
	uint32_t b = 0x00400802, m = 0xf81f07e0;

	// fallback to 1d2
	if (menuactive || !viewactive) {
		uint32_t i, n = 14 * w / 4;
		uint32_t *p = (uint32_t*)d;
		for (i = 0; i < n; i++) p[i] = 0;
		p = (uint32_t*)(d + 114 * w / 2);
		for (i = 0; i < n; i++) p[i] = 0;
		scr_update_1d2_custom(s, d + 14 * w / 2, c32 + 256, 200);
		return;
	}

	y = (h - 32) / 3;
	if (screenblocks >= 11)
			s += w * 4, y = 128 / 2;

	for (s += 40; y; y--, s += w * 2 + 80, d += w / 2)
	for (x = w / 4; x; x--, s += 3, d += 2) {
		a0 = c32[s[0]] << 1;
		a  = c32[s[1]];
		a1 = c32[s[2]] << 1;
		a0 += a; a1 += a;

		a2 = c32[s[w]];
		a  = c32[s[w + 1]];
		a3 = c32[s[w + 2]];
		a = (a & ~b) >> 1;
		a0 += a2 += a;
		a1 += a3 += a;

		a4 = c32[s[w * 2]] << 1;
		a  = c32[s[w * 2 + 1]];
		a5 = c32[s[w * 2 + 2]] << 1;
		a2 += a4 + a;
		a3 += a5 + a;

#define X(a, i) \
	a += (b & a >> 2) + b; a &= m; \
	d[i] = a | a >> 16;
		X(a0, 0) X(a1, 1) X(a2, w / 2)
		X(a3, w / 2 + 1)
#undef X
	}
	if (screenblocks < 11)
		scr_update_1d2_custom(s - 40, d, c32 + 256, 32);
}

#undef DEF
#ifdef USE_ASM
#define pal_update16 pal_update16_asm
#define scr_update_1d1 scr_update_1d1_asm
#define scr_update_1d2 scr_update_1d2_asm
#define scr_update_3d2 scr_update_3d2_asm
#define scr_update_25x24d20 scr_update_25x24d20_asm
#endif

uint8_t* framebuffer_init(void) {
	static const uint8_t pal_size[] = { 2, 4, 4, 8, 4 };
	static const struct {
		void (*pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
		void (*scr_update)(uint8_t *src, void *dest);
	} fn[] = {
		{ pal_update16, scr_update_1d1 },
		{ pal_update32, scr_update_1d2 },
		{ pal_update32, scr_update_3d2 },
		{ pal_update32_8d9, scr_update_2d3_crop },
		{ pal_update32, scr_update_25x24d20 },
	};
	int mode = sys_data.scaler;
	size_t size, size2 = pal_size[mode] << 8;
	uint8_t *dest;

	size = sys_data.display.w2 * sys_data.display.h2;
	dest = malloc(size * 2 + size2 + 31);
	dest = (void*)(dest + (-(intptr_t)dest & 31));
	dest += size2;

	app_pal_update = fn[mode].pal_update;
	app_scr_update = fn[mode].scr_update;
	return dest;
}

void lcd_appinit(void) {
	static const uint16_t dim[] = {
		320, 200,  160, 100,  480, 300,
		160, 128,  400, 240
	};
	struct sys_display *disp = &sys_data.display;
	unsigned mode = sys_data.scaler - 1;
	if (mode >= 5) {
		int w = disp->w1, h = disp->h1;
		switch (w) {
		case 400: mode = 4; break;
		case 480: mode = 2; break;
		case 240: case 320:
			mode = 0; break;
		case 128: case 160:
			mode = 1; break;
		default:
			fprintf(stderr, "!!! unsupported resolution (%dx%d)\n", w, h);
			exit(1);
		}
	}
	sys_data.scaler = mode;
	disp->w2 = dim[mode * 2];
	disp->h2 = dim[mode * 2 + 1];
}
