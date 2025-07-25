#include <stdlib.h>
#include <stdio.h>

#include "syscode.h"

void (*app_pal_update)(int i, int r, int g, int b);
void (*app_scr_update)(uint8_t *src, void *dest);

uint16_t *framebuf;

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

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

#undef DEF
#ifdef USE_ASM
// TODO
#endif

void* framebuffer_init(unsigned size1) {
	static const uint8_t pal_size[] = { 2, 2 };
	static const struct {
		void (*pal_update)(int i, int r, int g, int b);
		void (*scr_update)(uint8_t *src, void *dest);
	} fn[] = {
		{ pal_update16, scr_update_6x5d3 },
		{ pal_update16, scr_update_9x8d9 },
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

void lcd_appinit(void) {
	static const uint16_t dim[] = {
		320, 240,  160, 128
	};
	struct sys_display *disp = &sys_data.display;
	unsigned mode = sys_data.scaler - 1;
	unsigned w = disp->w1, h = disp->h1;
	if (h <= 68) {
		fprintf(stderr, "!!! unsupported resolution (%dx%d)\n", w, h);
		exit(1);
	} else if (mode >= 2) {
		switch (w) {
		case 400: // TODO
		case 480: // TODO
		case 240: case 320:
			mode = 0; break;
		case 176: case 220: // TODO
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
