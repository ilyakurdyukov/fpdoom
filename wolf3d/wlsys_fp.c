#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define WLSYS_MAIN
#include "wlsys_def.h"

static void *framebuf_mem = NULL;
uint16_t *framebuf;

void lcd_appinit(void) {
	static const uint16_t dim[] = {
		320, 240,  160, 128,  480, 320,  400, 240,
	};
	struct sys_display *disp = &sys_data.display;
	unsigned scaler = sys_data.scaler - 1;
	unsigned w = disp->w1, h = disp->h1;
	if (h <= 68) {
		scaler = GS_MODE;
		if (h == 68) scaler = GS_MODE + 1;
		if (h == 48) scaler = GS_MODE + 2;
	} else {
		if (scaler >= GS_MODE) {
			switch (w) {
			case 400: scaler = 3; break;
			case 480: scaler = 2; break;
			case 240: case 320:
				scaler = 0; break;
			case 128: case 160:
			case 176: case 220:
				scaler = 1; break;
			default:
				fprintf(stderr, "!!! unsupported resolution (%dx%d)\n", w, h);
				exit(1);
			}
		}
		w = dim[scaler * 2];
		h = dim[scaler * 2 + 1];
	}
	sys_data.scaler = scaler;
	disp->w2 = w;
	disp->h2 = h;
}

void wlsys_end(void) {
	if (framebuf_mem) free(framebuf_mem);
}

uint8_t* wlsys_init(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w2, h = disp->h2;
	unsigned size, size1, size2; uint8_t *p;
	static const uint8_t pal_size[] = { 2, 4, 6, 6, 1, 1, 1 };
	int scaler = sys_data.scaler;
	int w1 = w, h1 = h;
	if (scaler == 1) {
		w1 <<= 1; h1 <<= 1;
	} else if (h <= 68) {
		w1 = 320; h1 = 224;
		if (h == 68) h1 = 226;
		if (h == 48) h1 = 240;
	}
	frameWidth = w;
	frameHeight = h;
	screenWidth = w1;
	screenHeight = h1;
	size = w * h;
	size1 = w1 * h1;
	size2 = pal_size[scaler] << 8;
	p = malloc(size * 2 + size2 + size1 + 31);
	framebuf_mem = p;
	p += -(intptr_t)p & 31;
	p += size2;
	framebuf = (uint16_t*)p;
	p += size * 2;

	initFizzle();
	sys_framebuffer(framebuf);
	sys_start();

	return p;
}

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

void keytrn_init(void) {
	uint8_t keymap[64], rkeymap[64];
	static const uint8_t keys[] = {
#define KEY(name, val) KEYPAD_##name, val,
		KEY(UP, sc_UpArrow)
		KEY(LEFT, sc_LeftArrow)
		KEY(RIGHT, sc_RightArrow)
		KEY(DOWN, sc_DownArrow)
		KEY(2, sc_UpArrow)
		KEY(4, sc_LeftArrow)
		KEY(6, sc_RightArrow)
		KEY(5, sc_DownArrow)
		KEY(LSOFT, sc_Return)
		KEY(RSOFT, sc_Escape)
		KEY(CENTER, sc_LControl) /* fire */
		KEY(DIAL, sc_CapsLock) /* run toggle */
		KEY(1, 'q') /* strafe left */
		KEY(3, 'e') /* strafe right */
		KEY(7, '[') /* prev weapon */
		KEY(9, ']') /* next weapon */
		KEY(PLUS, sc_O) /* map */
	};
	static const uint8_t keys_power[] = {
		KEY(UP, sc_CapsLock)
		KEY(LEFT, '[')
		KEY(RIGHT, ']')
		KEY(DOWN, sc_Escape)
		KEY(CENTER, sc_Return)
		KEY(DIAL, sc_O) /* map */
		KEY(4, sc_G) /* G+Y: god mode */
		KEY(5, sc_Y)
		KEY(6, sc_M) /* M+Y: all items */
#undef KEY
	};
	int i, flags = sys_getkeymap(keymap);

#define FILL_RKEYMAP(keys) \
	memset(rkeymap, 0, sizeof(rkeymap)); \
	for (i = 0; i < (int)sizeof(keys); i += 2) \
		rkeymap[keys[i]] = keys[i + 1];

	FILL_RKEYMAP(keys)

#define KEY(name) rkeymap[KEYPAD_##name]
	// no center key
	if (!(flags & 1)) {
		KEY(UP) = sc_LControl;	/* fire */
		KEY(DOWN) = sc_LControl;	/* fire */
	}
#undef KEY

#define FILL_KEYTRN(j) \
	for (i = 0; i < 64; i++) { \
		unsigned a = keymap[i]; \
		sys_data.keytrn[j][i] = a < 64 ? rkeymap[a] : 0; \
	}

	FILL_KEYTRN(0)
	FILL_RKEYMAP(keys_power)
	FILL_KEYTRN(1)
#undef FILL_RKEYMAP
#undef FILL_KEYTRN
}

char *strncat(char *dst, const char *src, size_t len) {
	char *d = dst;
	while (*d++);
	strncpy(d, src, len);
	return dst;
}

void __assert2(const char *file, int line, const char *func, const char *msg) {
	fprintf(stderr, "!!! %s:%i %s: assert(%s) failed\n", file, line, func, msg);
	exit(255);
}
