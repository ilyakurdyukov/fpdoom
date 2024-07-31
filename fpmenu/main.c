#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "syscode.h"
#include "sdio.h"

#if !LIBC_SDIO
#define FAT_READ_SYS \
	if (sdio_read_block(sector, buf)) break;
#include "microfat.h"
fatdata_t fatdata_glob;
#endif
#include "fatfile.h"

#undef DBG_LOG
#if MENU_DEBUG
#define DBG_LOG(...) printf(__VA_ARGS__)
#define DBG_EXTRA_ARG(x) , x
#else
#define DBG_LOG(...) (void)0
#define DBG_EXTRA_ARG(x)
#endif

#include "readconf.h"

extern uint8_t __image_start[];

typedef int (*printf_t)(const char *fmt, ...);
typedef void (*readbin_t)(unsigned clust, unsigned size, uint8_t *ram,
		fatdata_t *fatdata DBG_EXTRA_ARG(printf_t printf));

#define FONT_W 8
#define FONT_H 16
static const uint8_t font_data[] = {
#include "font8x16.h"
};

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	unsigned w = disp->w1, h = disp->h1;
	disp->w2 = w - w % FONT_W;
	disp->h2 = h - h % FONT_H;
}

typedef struct {
	uint8_t *framebuf_mem; uint16_t *framebuf;
	char *menu0, *menu;
	unsigned cols, rows, st, sel, mrows;
	union { uint16_t u16[2]; uint32_t u32; } pal[2];
	struct { unsigned clust, size; } fatfile;
} draw_t;

static void draw_text(draw_t *draw, unsigned y0, const char *str) {
	unsigned a, x, y, st = draw->st, n = draw->cols;
	uint16_t *p0 = draw->framebuf, *p;
	unsigned sel = y0 == draw->sel;
	p0 += y0 * st * FONT_H;
	do {
		const uint8_t *bm; uint32_t c = sel;
		if ((a = *str)) str++; else a = 0x20, c = 0;
		if (a - 0x20 >= 0x60) a = '?';
		bm = font_data + (a - 0x20) * FONT_H;
		c = draw->pal[c].u32;
		for (p = p0, y = 0; y < FONT_H; y++, p += st)
		for (a = bm[y], x = 0; x < FONT_W; x++, a <<= 1)
			p[x] = c >> ((a & 0x80) >> 3);
		p0 += FONT_W;
	} while (--n);
}

/* 888 -> 565 */
#define RGB565(v) \
	(((v) >> 8 & 0xf800) | ((v) >> 5 & 0x7e0) | ((v) >> 3 & 0x1e))

static unsigned menu_count(char *p, unsigned lim) {
	unsigned i = 0;
	do {
		uint32_t next = *(uint32_t*)p;
		if (!next) break;
		p += next;
	} while (i++ < lim);
	return i;
}

static char* menu_file(char *p, unsigned sel) {
	for (;;) {
		uint32_t next = *(uint32_t*)p;
		if (!next) break;
		p += next;
		if (!sel) {
			p += 4; while (*p++);
			return p;
		}
		sel--;
	};
	return NULL;
}

static void do_fat_init(void) {
#if !LIBC_SDIO
	static int init_done = 0;
	if (init_done) return;
	init_done = 1;
	sdio_init();
	if (sdcard_init()) {
		printf("!!! sdcard_init failed\n");
		exit(1);
	}
	fatdata_glob.buf = (uint8_t*)CHIPRAM_ADDR + 0x9000;
	if (fat_init(&fatdata_glob, 0)) {
		printf("!!! fat_init failed\n");
		exit(1);
	}
#endif
}

static int run_binary(draw_t *draw) {
	fat_entry_t *p; char *args, *name;
	char *d = (char*)CHIPRAM_ADDR;
	int argc;

	args = menu_file(draw->menu, draw->sel);
	if (!args) return 0;
	name = get_first_arg(args);
	if (!strcmp(name, "exit")) return 1;

	DBG_LOG("bin: \"%s\"\n", name);
	{
		extract_args_t x = { 0, 1, d + 0x1000 };
		if (!extract_args(0, &x, args, d + 6)) {
			DBG_LOG("too many args\n");
			return 0;
		}
		argc = x.argc;
	}
	*(short*)(d + 4) = argc;
#if MENU_DEBUG
	d += 6; printf("args:");
	while (argc--) {
		printf(" \"%s\"", d);
		while (*d++);
	}
	printf("\n");
#endif
	do_fat_init();
	p = fat_find_path(&fatdata_glob, name);
	if (!p || (p->entry.attr & FAT_ATTR_DIR)) {
		DBG_LOG("file not found\n");
		return 0;
	}
	{
		unsigned clust = fat_entry_clust(p);
		uint32_t size = p->entry.size;
		DBG_LOG("start = 0x%x, size = 0x%x\n", clust, size);
		if (size > (2 << 20) - 0x10000) {
			DBG_LOG("binary is too big\n");
			return 0;
		}
		draw->fatfile.clust = clust;
		draw->fatfile.size = size;
	}
	return 1;
}

int main(int argc, char **argv) {
	char *menu; draw_t draw;
	(void)argc; (void)argv;

	{
		FILE *fi;
		fi = fopen("fpbin/config.txt", "rb");
		if (!fi) return 1;
		menu = parse_config(fi, 0x10000);
		fclose(fi);
	}

	if (!menu) return 1;

	if (MENU_DEBUG) {
		char *p = menu; int i = 0;
		printf("Menu:\n");
		for (;;) {
			uint32_t next = *(uint32_t*)p;
			if (!next) break;
			p += next;
			printf("%u: \"%s\"\n", i++, p + 4);
		}
	}

	draw.menu0 = menu;
	draw.menu = menu;
	draw.fatfile.clust = 0;
	draw.fatfile.size = 0;

	draw.pal[0].u16[0] = RGB565(0x000000); // bg
	draw.pal[0].u16[1] = RGB565(0xffffff); // text
	// selected
	draw.pal[1].u16[0] = RGB565(0xffffff); // bgm
	draw.pal[1].u16[1] = RGB565(0x000000); // text

	{
		struct sys_display *disp = &sys_data.display;
		int w = disp->w2, h = disp->h2;
		unsigned size = w * h; uint8_t *p;

		draw.cols = w / FONT_W;
		draw.rows = h / FONT_H;
		draw.st = w;

		p = malloc(size * 2 + 31);
		draw.framebuf_mem = p;
		p += -(intptr_t)p & 31;
		draw.framebuf = (uint16_t*)p;
	}

	draw.sel = 0;
	draw.mrows = menu_count(draw.menu, draw.rows);
	if (draw.mrows <= 0) return 1;

	sys_framebuffer(draw.framebuf);
	sys_start();

	{
		struct sys_display *disp = &sys_data.display;
		unsigned size = disp->w2 * disp->h2;
		uint16_t *p = draw.framebuf, a = draw.pal[0].u16[0];
		do *p++ = a; while (--size);
	}

	for (;;) {
		sys_wait_refresh();
		{
			char *p = draw.menu; int i = 0;
			for (;;) {
				uint32_t next = *(uint32_t*)p;
				if (!next) break;
				p += next;
				draw_text(&draw, i++, p + 4);
			}
		}
		sys_start_refresh();
		sys_wait_ms(10);
		for (;;) {
			int type, key;
			type = sys_event(&key);
			if (type == EVENT_END) break;
			if (type == EVENT_KEYDOWN) {
				DBG_LOG("key = 0x%02x\n", key);
				switch (key) {
				case 0x32: // 2
				case 0x04: // UP
					if (draw.sel > 0) draw.sel--;
					break;
				case 0x35: // 5
				case 0x05: // DOWN
					if (draw.sel + 1 < draw.mrows) draw.sel++;
					break;
				// TODO
				//case 0x34: // 4
				//case 0x06: // LEFT
				//case 0x08: // LSOFT
				//case 0x36: // 6
				//case 0x07: // RIGHT
				//case 0x09: // RSOFT
				case 0x0d: // CENTER
					if (run_binary(&draw)) goto loop_end;
					break;
				}
			}
		}
	}
loop_end:
	free(draw.framebuf_mem);
	sys_brightness(0);
	if (draw.fatfile.size) {
		uint8_t *ram = (uint8_t*)((intptr_t)__image_start & 0xfc000000);
		uint32_t size = *(uint32_t*)(ram + 4) - 8;
		void *readbin = ram + 8;
		uint8_t *dst = (uint8_t*)CHIPRAM_ADDR + 0x4000;
		DBG_LOG("readbin = %p, size = 0x%x\n", readbin, size);
		memcpy(dst, readbin, size);
		clean_dcache();
		clean_icache();
		((readbin_t)dst)(draw.fatfile.clust, draw.fatfile.size, ram,
				&fatdata_glob DBG_EXTRA_ARG(&printf));
	}
	return 0;
}

void keytrn_init(void) {
	uint8_t keymap[64];
	int i, flags = sys_getkeymap(keymap);
	(void)flags;

#define FILL_KEYTRN(j) \
	for (i = 0; i < 64; i++) { \
		sys_data.keytrn[j][i] = keymap[i]; \
	}

	FILL_KEYTRN(0)
	FILL_KEYTRN(1)
#undef FILL_KEYTRN
}

