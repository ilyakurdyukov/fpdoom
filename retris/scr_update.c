#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syscode.h"

uint16_t *framebuf;
static uint8_t *tilesbuf;

#include "tiles.h"
#include "resize.h"

void scr_tile_update(unsigned x, unsigned y, unsigned rgb, unsigned t, void *dest) {
	uint16_t *d = (uint16_t*)dest;
	unsigned n = sys_data.user[0];
	d += (y * 20 * n + x) * n;
	if (!t) {
		unsigned r = rgb >> (16 + 3);
		unsigned g = rgb >> (8 + 2) & 63;
		unsigned b = rgb >> 3 & 31;
		unsigned c2 = r << 11 | g << 5 | b;
		for (x = 0; x < n; x++) d[x] = c2;
		for (x = 1; x < n; x++)
			memcpy(d + x * 20 * n, d, n * 2);
	} else {
		unsigned rb = rgb & 0xff00ff;
		unsigned g1 = rgb - rb;
		const uint8_t *s = tilesbuf + (t - 1) * n * n;
		for (y = 0; y < n; y++, d += n * 20)
		for (x = 0; x < n; x++) {
			unsigned a = *s++, a1 = 0x100 - a;
			unsigned rb2 = rb * a1 + (a << 24 | a << 8);
			unsigned g2 = g1 * a1 + (a << 16);
			unsigned r = rb2 >> (24 + 3);
			unsigned g = g2 >> (16 + 2) & 63;
			unsigned b = rb2 >> (8 + 3) & 31;
			d[x] = r << 11 | g << 5 | b;
		}
	}
}

void framebuffer_init(void) {
	size_t size, size2 = 0;
	uint8_t *p;
	unsigned w = sys_data.user[0], h = w * 11;
	unsigned size1 = h * w;

	size = sys_data.display.w2 * sys_data.display.h2 * 2;
	p = malloc(size + size1 + size2 + 28);
	p = (void*)(p + (-(intptr_t)p & 31));
	p += size2;
	framebuf = (uint16_t*)p;
	p += size;
	sys_framebuffer(framebuf);
	tilesbuf = p;
	image_resize(scr_tiles.data, scr_tiles.w, scr_tiles.h, p, w, h);
}

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	unsigned mode = sys_data.scaler - 1;
	unsigned w = disp->w1, h = disp->h1;
	static const uint8_t dotsize[] = { 12, 6, 16, 8 };
	if (mode >= 4) {
		if (w > h) w = h;
		switch (w) {
		case 240: mode = 0; break; // 12
		case 128: mode = 1; break; // 6
		case 320: mode = 2; break; // 16
		case 176: mode = 3; break; // 8
		default:
			fprintf(stderr, "!!! unsupported resolution (%dx%d)\n", w, h);
			exit(1);
		}
	}
	sys_data.scaler = mode;
	w = dotsize[mode];
	sys_data.user[0] = w;
	h = w *= 20;
	disp->w2 = w;
	disp->h2 = h;
}
