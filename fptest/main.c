#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "syscode.h"

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w1, h = disp->h1;
	disp->w2 = w;
	disp->h2 = h;
}

static void *framebuf_mem = NULL;
static uint16_t *framebuf = NULL;

static void framebuf_alloc(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w2, h = disp->h2;
	size_t size = w * h;
	uint8_t *p;
	framebuf_mem = p = malloc(size * 2 + 31);
	p += -(intptr_t)p & 31;
	framebuf = (void*)p;
}

static void gen_image(uint16_t *p, unsigned w, unsigned h, unsigned st) {
	unsigned x, y;

	for (y = 0; y < h; y++)
	for (x = 0; x < w; x++) {
		unsigned a = x * 32 / w;
		if (y < h >> 2) a <<= 11;
		else if (y < h >> 1) a <<= 5 + 1;
		else if (y >= 3 * h >> 2) a *= 1 << 11 | 1 << (5 + 1) | 1;
		p[y * st + x] = a;
	}
	p[0] = -1; p[w - 1] = -1;
	p += (h - 1) * st;
	p[0] = -1; p[w - 1] = -1;
}

static void test_refresh() {
	uint32_t t0 = sys_timer_ms(), t1, n = 0, t2, t3;
	do {
		CHIP_FN2(sys_start_refresh)();
		CHIP_FN2(sys_wait_refresh)();
		t1 = sys_timer_ms();
		n++;
	} while (t1 - t0 < 1000);
	t1 -= t0;
	printf("%d frames in %d ms\n", n, t1);
	t2 = t1 / n;
	t3 = (t1 - t2 * n) * 1000 / n;
	printf("1 frame in %d.%03d ms\n", t2, t3);
	t3 = n * 1000;
	t2 = t3 / t1;
	t3 = (t3 - t2 * t1) * 1000 / t1;
	printf("%d.%03d frames per second\n", t2, t3);
}

static void test_keypad() {
	int type, key, ret = 0;

	for (;;) {
		type = CHIP_FN2(sys_event)(&key);
		switch (type) {
		case EVENT_KEYUP:
		case EVENT_KEYDOWN: {
			const char *s = "???";
			switch (key) {
#define X(val, name) case val: s = #name; break;
			KEYPAD_ENUM(X)
#undef X
			}
			printf("0x%02x (\"%s\") %s\n", key, s,
					type == EVENT_KEYUP ? "release" : "press");
	    break;
		}
		case EVENT_END: sys_wait_ms(10); break;
		case EVENT_QUIT: goto end;
		}
	}
end:;
}

int main(int argc, char **argv) {
	int i;

	if (1) {
		printf("argc = %i\n", argc);
		for (i = 0; i < argc; i++)
			printf("[%i] = %s\n", i, argv[i]);
	}

	if (1) {
		struct sys_display *disp = &sys_data.display;
		unsigned w = disp->w2, h = disp->h2;
		framebuf_alloc();
		CHIP_FN2(sys_framebuffer)(framebuf);
		CHIP_FN2(sys_start)();
		gen_image(framebuf, w, h, w);
		test_refresh();
		free(framebuf_mem);
	}

	if (1) {
		test_keypad();
	}

	return 0;
}

void keytrn_init(void) {
	uint8_t keymap[64];
	int i, flags = sys_getkeymap(keymap);

#define FILL_KEYTRN(j) \
	for (i = 0; i < 64; i++) { \
		sys_data.keytrn[j][i] = keymap[i]; \
	}

	FILL_KEYTRN(0)
	FILL_KEYTRN(1)
#undef FILL_KEYTRN
}

