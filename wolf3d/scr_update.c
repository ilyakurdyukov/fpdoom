#include <stdio.h>
#include <stdint.h>
#define WLSYS_MAIN
#include "wlsys_def.h"

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

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

DEF(uint16_t*, scr_update_3d2, (uint16_t *d, const uint8_t *s, uint32_t *c32, unsigned h)) {
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
#undef X1
#undef X2
	}
	return d;
}

DEF(uint16_t*, scr_update_5x4d4, (uint16_t *d, const uint8_t *s, uint16_t *c16, unsigned h)) {
	unsigned x, y, a, b, c;
	uint32_t r = 0x08210821, m = ~(r >> 1);
	for (y = 0; y < h; y++, s += 80)
	for (x = 0; x < 320; x += 4, s += 4) {
		*d++ = c16[s[0]];
		a = c16[s[1]];
		b = c16[s[2]];
		c = ((a >> 1) & m) + ((b >> 1) & m);
		// rounding half to even
		c += ((c & (a | b)) | (a & b)) & r;
		*d++ = a;
		*d++ = c;
		*d++ = b;
		*d++ = c16[s[3]];
	}
	return d;
}

#undef DEF

#ifdef USE_ASM
#define pal_update16 pal_update16_asm
#define pal_update32 pal_update32_asm
#define scr_repeat scr_repeat_asm
#define scr_update_1d1 scr_update_1d1_asm
#define scr_update_1d2 scr_update_1d2_asm
#define scr_update_3d2 scr_update_3d2_asm
#define scr_update_5x4d4 scr_update_5x4d4_asm
#endif

void wlsys_setpal(SDL_Color *pal) {
	uint16_t *d = framebuf;
	int scaler = sys_data.scaler;
#if EMBEDDED == 1
	if (scaler >= 4) scaler = 0;
#endif
	if (scaler == 0 || scaler == 3) {
		pal_update16(pal, d);
	} else if (scaler == 1) {
		pal_update32(pal, d);
	} else if (scaler == 2) {
		pal_update16(pal, d);
		pal_update32(pal, d - 256);
	}
}

static void wlsys_refresh0(const uint8_t *src, int type) {
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

static void wlsys_refresh1(const uint8_t *src, int type) {
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

static void wlsys_refresh2(const uint8_t *src, int type) {
	uint16_t *d = framebuf;
	uint16_t *pal16 = (uint16_t*)d - 256;
	uint32_t *pal32 = (uint32_t*)pal16 - 256;
	unsigned w = 480, h = 320, skip, v;
	if (type == 0) {
		v = pal16[src[0]]; v |= v << 16;
		skip = 10 * w;
		d = scr_repeat(d, v, skip);
		d = scr_update_3d2(d, src, pal32, 200);
		scr_repeat(d, v, skip);
	} else if (type == 2) {
		d = scr_update_3d2(d, src, pal32, 188);
		// need to keep 38..39 pixels
		scr_update_1d1(d, src + (h - 38) * w, pal16, w * 38);
	} else {
		scr_update_1d1(d, src, pal16, w * h);
	}
}

static void wlsys_refresh3(const uint8_t *src, int type) {
	uint16_t *d = framebuf;
	uint16_t *pal16 = (uint16_t*)d - 256;
	unsigned w = 400, h = 240, skip, v;
	if (type == 0) {
		v = pal16[src[0]]; v |= v << 16;
		skip = 20 * w;
		d = scr_repeat(d, v, skip);
		d = scr_update_5x4d4(d, src, pal16, 200);
		scr_repeat(d, v, skip);
	} else if (type == 2) {
		d = scr_update_5x4d4(d, src, pal16, 240 - 39);
		// need to keep 38..39 pixels
		scr_update_1d1(d, src + (h - 39) * w, pal16, w * 39);
	} else {
		scr_update_1d1(d, src, pal16, w * h);
	}
}

typedef void (*wlsys_refresh_t)(const uint8_t *src, int type);
static const wlsys_refresh_t wlsys_refresh_fn[] = {
	wlsys_refresh0, wlsys_refresh1, wlsys_refresh2, wlsys_refresh3 };

int force_refresh_type;
int last_refresh_type;

void wlsys_refresh(const uint8_t *src, int type) {
	if (!type) type = force_refresh_type;
	if (!(type & 16)) sys_wait_refresh();
	type &= 15; last_refresh_type = type;

#if EMBEDDED == 1
	if (sys_data.scaler >= 4) {
		uint16_t *pal16 = (uint16_t*)framebuf - 256;
		scr_update_1d1(framebuf, src, pal16, screenWidth * screenHeight);
	} else
#endif
	wlsys_refresh_fn[sys_data.scaler](src, type);

	sys_start_refresh();
}

