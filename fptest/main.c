#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "syscode.h"
#include "cmd_def.h"
#include "usbio.h"
#include "sdio.h"

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

static void usbio_bench(int arg, unsigned len, unsigned n) {
	void *mem = malloc(len + 32);
	uint8_t *buf = (uint8_t*)(((intptr_t)mem + 31) & ~31);
	uint32_t t0, t1; unsigned i;

	{
		union { uint16_t u16[5]; uint32_t u32[2]; } buf;
		unsigned tmp = CMD_USBBENCH | arg << 8;
		buf.u16[0] = tmp; tmp += CHECKSUM_INIT;
		buf.u16[1] = len; tmp += len;
		buf.u32[1] = n; tmp += (uint16_t)n + (n >> 16);
		buf.u16[4] = tmp + (tmp >> 16);
		usb_write(&buf, 10); // sizeof(buf) == 12
	}

	memset(buf, 0, len);
	t0 = sys_timer_ms();
	if (arg == 0)
	for (i = 0; i < n; i += len)
		usb_read(buf, len, USB_WAIT);
	else
	for (i = 0; i < n; i += len)
		usb_write(buf, len);
	t1 = sys_timer_ms();
	free(mem);
	t1 -= t0;
	printf("usbio raw %s: %u bytes in %u ms\n",
			arg ? "write" : "read", i, t1);
}

static void sdio_bench(uint32_t size) {
	void *mem = malloc(512 + 32);
	uint8_t *buf = (uint8_t*)(((intptr_t)mem + 31) & ~31);
	unsigned i, n = size >> 9;
	uint32_t t0, t1;

	t0 = sys_timer_ms();
	for (i = 0; i < n; i++)
		if (sdio_read_block(i, buf)) break;
	t1 = sys_timer_ms();
	free(mem);
	t1 -= t0;
	printf("sdio raw read: %u bytes in %u ms\n", i << 9, t1);
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
		usbio_bench(0, 1024, 1 << 20);
		usbio_bench(1, 1024, 1 << 20);
	}

	if (1) {
		sdio_init();
		if (!sdcard_init()) {
			SDIO_VERBOSITY(0);
			sdio_bench(1 << 20);
		}
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

