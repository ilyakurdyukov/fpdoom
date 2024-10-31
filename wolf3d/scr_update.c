#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define WLSYS_MAIN
#include "wlsys_def.h"

unsigned frameWidth, frameHeight;

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

DEF(void, pal_update8, (SDL_Color *pal, void *dest)) {
	int i, a;
	uint8_t *pal8 = (uint8_t*)dest - 256;
	for (i = 0; i < 256; i++) {
		int r = pal[i].r, g = pal[i].g, b = pal[i].b;
		a = (r * 4899 + g * 9617 + b * 1868 + 0x2000) >> 14;
		pal8[i] = a;
	}
}

DEF(void, pal_update16, (SDL_Color *pal, void *dest)) {
	int i, a;
	uint16_t *pal16 = (uint16_t*)dest - 256;
	for (i = 0; i < 256; i++) {
		a = pal[i].r << 8 & 0xf800;
		a |= pal[i].g << 3 & 0x7e0;
		a |= pal[i].b >> 3;
		pal16[i] = a;
	}
}

DEF(void, pal_update32, (SDL_Color *pal, void *dest)) {
	int i, a; uint32_t *pal32;
	pal32 = (uint32_t*)dest - 256;
	for (i = 0; i < 256; i++) {
		a = pal[i].r << 22 | pal[i].b << 11 | pal[i].g << 1;
		pal32[i] = a;
	}
}

DEF(uint16_t*, scr_repeat, (uint16_t *dest, uint32_t v, unsigned n)) {
	uint32_t *d = (uint32_t*)dest;
	do {
		*d++ = v; *d++ = v; *d++ = v; *d++ = v;
	} while ((n -= 8));
	return (uint16_t*)d;
}

DEF(uint16_t*, scr_update_1d1, (uint16_t *d, const uint8_t *s, uint16_t *c16, unsigned n)) {
	do *d++ = c16[*s++]; while (--n);
	return d;
}

DEF(uint16_t*, scr_update_1d2, (uint16_t *d, const uint8_t *s, uint32_t *c32, unsigned h)) {
	unsigned x, y, w = 320;
	unsigned a, b = 0x00400802;
	for (y = 0; y < h; y += 2, s += w)
	for (x = 0; x < w; x += 2, s += 2) {
		a = c32[s[0]] + c32[s[1]];
		a += c32[s[w]] + c32[s[w + 1]];
		a += (b & a >> 2) + b;
		a &= 0xf81f07e0;
		*d++ = a | a >> 16;
	}
	return d;
}

DEF(uint16_t*, scr_update_3d2, (uint16_t *d, const uint8_t *s, uint16_t *c16, unsigned h)) {
	uint32_t *c32 = (uint32_t*)c16 - 256;
	unsigned x, y, w = 320, w2 = 480;
	uint32_t a, a0, a1, a2, a3;
	uint32_t b = 0x00400802 << 1, m = 0xf81f07e0;
	for (y = 0; y < h; y += 2, s += w2 * 2 - w, d += w2 * 2)
	for (x = 0; x < w; x += 2, s += 2, d += 3) {
		a0 = c32[s[0]];
		a1 = c32[s[1]];
		a2 = c32[s[w2]];
		a3 = c32[s[w2 + 1]];

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
	return d;
}

// pixel aspect ratio 25:24
DEF(uint16_t*, scr_update_25x24d20, (uint16_t *d, const uint8_t *s, uint16_t *c16, unsigned h)) {
	uint32_t *c32 = (uint32_t*)c16 - 256;
	unsigned x, y, w = 320, w2 = 400;
	uint32_t a, a0, a1, a2, a3;
	uint32_t b = 0x00400802 << 1, m = 0xf81f07e0;
	do {
		for (y = 0; y < 3; y++, s += w2 - w)
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
		if (!(h -= 3)) break;

		for (x = 0; x < w; x += 4, s += 4, d += 5) {
			a0 = c32[s[0]];
			a2 = c32[s[w2]];
			X1(a0, 0)
			X2(a0, a2, w2)
			X1(a2, w2 * 2)

			a0 = c32[s[1]];
			a1 = c32[s[2]];
			a2 = c32[s[w2 + 1]];
			a3 = c32[s[w2 + 2]];

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
			a2 = c32[s[w2 + 3]];
			X1(a0, 4)
			X2(a0, a2, w2 + 4)
			X1(a2, w2 * 2 + 4)
		}
		s += w2 * 2 - w; d += w2 * 2;
	} while ((h -= 2));
	return d;
}
#undef X1
#undef X2

/* display aspect ratio 7:5, LCD pixel aspect ratio 7:10 */
DEF(uint8_t*, scr_update_128x64, (uint8_t *d, const uint8_t *s, uint8_t *c8, unsigned h)) {
	unsigned x, y;
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
			if (h == 1) continue;
			X(t, 3, 0) d[126] = a; d[127] = b;
		}
		s += 320 * 6; d += 128;
	} while ((int)(h -= 2) > 0);
#undef X
	return d - ((h & 1) << 7);
}

DEF(uint8_t*, scr_update_96x68, (uint8_t *d, const uint8_t *s, uint8_t *c8, unsigned h)) {
	unsigned x, y = 0;
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

	h -= 2;
	do {
		for (x = 0; x < 32; x++, s += 10, d += 3 - 96 * 3) {
			const uint8_t *s2 = s;
			a0 = a1 = 0;
			do { X1 } while (y);
		}
		s += 320 * 9; d += 96 * 2;
	} while ((int)(h -= 3) > 0);
	if (h) return d;
	for (x = 0; x < 32; x++, s += 10, d += 3 - 96 * 2) {
		const uint8_t *s2 = s;
		a0 = a1 = 0;
		for (;;) {
			X1 if (!y) break;
			y -= 2; t0 = 255 * 10; t0 |= t0 << 16;
			a0 += t0 << 1; a1 += t0;
		}
	}
	return d + 96;
#undef X1
#undef X
}

#if LIBC_SDIO == 0
DEF(uint8_t*, scr_update_64x48, (uint8_t *d, const uint8_t *s, uint8_t *c8, unsigned h)) {
	unsigned x, y;
	do {
		for (x = 64; x; x--, s += 5) {
			const uint8_t *s2 = s; uint32_t a = 0;
			for (y = 5; y; y--, s2 += 320)
				a += c8[s2[0]] + c8[s2[1]] +
						c8[s2[2]] + c8[s2[3]] + c8[s2[4]];
			*d++ = (a * 0x147b + (3 << 16)) >> 18; /* div25 */
		}
		s += 320 * 4;
	} while (--h);
	return d;
}
#endif

#undef DEF
#ifdef USE_ASM
#define pal_update16 pal_update16_asm
#define pal_update32 pal_update32_asm
#define scr_repeat scr_repeat_asm
#define scr_update_1d1 scr_update_1d1_asm
#define scr_update_1d2 scr_update_1d2_asm
#define scr_update_3d2 scr_update_3d2_asm
#define scr_update_25x24d20 scr_update_25x24d20_asm
#endif

void wlsys_setpal(SDL_Color *pal) {
	uint16_t *d = framebuf;
	int scaler = sys_data.scaler;
#if EMBEDDED == 1
	if (scaler >= GS_MODE) scaler = 0;
#endif
	if (scaler == 0) {
		pal_update16(pal, d);
	} else if (scaler == 1) {
		pal_update32(pal, d);
	} else if (scaler >= GS_MODE) {
		pal_update8(pal, d);
	} else if (scaler >= 2) {
		pal_update16(pal, d);
		pal_update32(pal, d - 256);
	}
}

static void wlsys_refresh_1d1(const uint8_t *src, int type) {
	uint16_t *d = framebuf;
	uint16_t *pal16 = (uint16_t*)d - 256;
	unsigned w = 320, h = 240, skip, v;
	if (type == 0) {
		v = pal16[src[0]]; v |= v << 16;
		skip = 20 * w;
		d = scr_repeat(d, v, skip);
		d = scr_update_1d1(d, src, pal16, w * 200);
		scr_repeat(d, v, skip);
	} else {
		scr_update_1d1(d, src, pal16, w * h);
	}
}

static void wlsys_refresh_1d2(const uint8_t *src, int type) {
	uint16_t *d = framebuf;
	uint32_t *pal32 = (uint32_t*)d - 256;
	unsigned w = 320, w2 = w / 2, h = 256, skip, v;
	if (type == 0) {
		v = pal32[src[0]] << 2 & 0xf81f07e0;
		v |= v << 16 | v >> 16;
		skip = 14 * w2;
		d = scr_repeat(d, v, skip);
		d = scr_update_1d2(d, src, pal32, 200);
		scr_repeat(d, v, skip);
	} else {
		scr_update_1d2(d, src, pal32, h);
	}
}

static void wlsys_refresh_3d2(const uint8_t *src, int type) {
	uint16_t *d = framebuf;
	uint16_t *pal16 = (uint16_t*)d - 256;
	unsigned w = 480, h = 320, skip, v;
	if (type == 0) {
		v = pal16[src[0]]; v |= v << 16;
		skip = 10 * w;
		d = scr_repeat(d, v, skip);
		d = scr_update_3d2(d, src, pal16, 200);
		scr_repeat(d, v, skip);
	} else if (type == 2) {
		d = scr_update_3d2(d, src, pal16, 188);
		// need to keep 38..39 pixels
		scr_update_1d1(d, src + (h - 38) * w, pal16, w * 38);
	} else {
		scr_update_1d1(d, src, pal16, w * h);
	}
}

static void wlsys_refresh_25x24d20(const uint8_t *src, int type) {
	uint16_t *d = framebuf;
	uint16_t *pal16 = (uint16_t*)d - 256;
	unsigned w = 400, h = 240, skip, v;
	if (type == 0) {
		scr_update_25x24d20(d, src, pal16, 200);
	} else if (type == 2) {
		d = scr_update_25x24d20(d, src, pal16, 168);
		scr_update_1d1(d, src + (h - 39) * w, pal16, w * 39);
	} else {
		scr_update_1d1(d, src, pal16, w * h);
	}
}

static void wlsys_refresh_128x64(const uint8_t *src, int type) {
	uint8_t *d = (uint8_t*)framebuf, *pal8 = d - 256;
	unsigned w = 128, v;
	if (type == 0) {
		v = pal8[src[0]]; v = (v + 1) >> 1;
		memset(d, v, w * 4); d += w * 4;
		d = scr_update_128x64(d, src, pal8, 57);
		memset(d, v, w * 3);
	} else {
		scr_update_128x64(d, src, pal8, 64);
	}
}

static void wlsys_refresh_96x68(const uint8_t *src, int type) {
	uint8_t *d = (uint8_t*)framebuf, *pal8 = d - 256;
	unsigned w = 96, v;
	if (type == 0) {
		v = pal8[src[0]]; v = (v + 1) >> 1;
		memset(d, v, w * 4); d += w * 4;
		d = scr_update_96x68(d, src, pal8, 60);
		memset(d, v, w * 4);
	} else {
		scr_update_96x68(d, src, pal8, 68);
	}
}

#if LIBC_SDIO == 0
static void wlsys_refresh_64x48(const uint8_t *src, int type) {
	uint8_t *d = (uint8_t*)framebuf, *pal8 = d - 256;
	unsigned w = 64, v;
	if (type == 0) {
		v = pal8[src[0]]; v = (v + 1) >> 1;
		memset(d, v, w * 4); d += w * 4;
		d = scr_update_64x48(d, src, pal8, 40);
		memset(d, v, w * 4);
	} else {
		scr_update_64x48(d, src, pal8, 48);
	}
}
#endif

void (*fizzlePixel)(unsigned x, unsigned y, uint8_t *src);

static void fizzlePixel_1d1(unsigned x, unsigned y, uint8_t *src) {
	uint16_t *d = framebuf, *pal = d - 256;
	unsigned pos = y * frameWidth + x;
	d[pos] = pal[src[pos]];
}

static void fizzlePixel_1d2(unsigned x, unsigned y, uint8_t *src) {
	uint16_t *d = framebuf;
	uint32_t *pal = (uint32_t*)d - 256;
	unsigned pos = y * screenWidth + x;
	unsigned a, b = 0x00400802;
	uint8_t *s = src + pos * 2;
	a = pal[s[0]] + pal[s[1]]; s += screenWidth;
	a += pal[s[0]] + pal[s[1]];
	a += (b & a >> 2) + b; a &= 0xf81f07e0;
	d[(pos + x) >> 1] = a | a >> 16;
}

static void fizzlePixel_gs(unsigned x, unsigned y, uint8_t *src) {
	uint8_t *d = (uint8_t*)framebuf, *pal = d - 256;
	unsigned x1 = x * screenWidth / frameWidth;
	unsigned y1 = y * screenHeight / frameHeight;
	unsigned x2 = ((x + 1) * screenWidth) / frameWidth;
	unsigned y2 = ((y + 1) * screenHeight) / frameHeight;
	unsigned a = 0, n, st = frameWidth;
	src += y1 * st + x1;
	y2 -= y1; x2 -= x1; n = x2 * y2;
	do {
		for (x1 = 0; x1 < x2; x1++) a += pal[src[x1]];
		src += st;
	} while (--y2);
	a = (a + n / 2) / n;
	d[y * frameWidth + x] = (a + 1) >> 1;
}

void initFizzle(void) {
	unsigned scaler = sys_data.scaler;
	if (scaler == 1) fizzlePixel = fizzlePixel_1d2;
	else if (scaler >= GS_MODE) fizzlePixel = fizzlePixel_gs;
	else fizzlePixel = fizzlePixel_1d1;
}

typedef void (*wlsys_refresh_t)(const uint8_t *src, int type);
static const wlsys_refresh_t wlsys_refresh_fn[] = {
	wlsys_refresh_1d1, wlsys_refresh_1d2,
	wlsys_refresh_3d2, wlsys_refresh_25x24d20,
	wlsys_refresh_128x64, wlsys_refresh_96x68,
#if LIBC_SDIO == 0
	wlsys_refresh_64x48,
#endif
};

int force_refresh_type;
int last_refresh_type;

void wlsys_refresh(const uint8_t *src, int type) {
	if (!type) type = force_refresh_type;
	if (!(type & 16)) sys_wait_refresh();
	type &= 15; last_refresh_type = type;

#if EMBEDDED == 1
	if (sys_data.scaler >= GS_MODE) {
		uint16_t *pal16 = (uint16_t*)framebuf - 256;
		scr_update_1d1(framebuf, src, pal16, screenWidth * screenHeight);
	} else
#endif
	wlsys_refresh_fn[sys_data.scaler](src, type);

	sys_start_refresh();
}

