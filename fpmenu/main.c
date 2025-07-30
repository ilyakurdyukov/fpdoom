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
	if (!(sys_data.mac & 0x100)) {
		w -= w % FONT_W;
		h -= h % FONT_H;
	}
	disp->w2 = w;
	disp->h2 = h;
}

typedef struct {
	uint8_t *framebuf_mem; uint16_t *framebuf;
	char *menu0, *menu;
	unsigned st, flags;
	unsigned cols, rows, sel, msel, mrows;
	union { uint16_t u16[2]; uint32_t u32; } pal[2];
	struct { unsigned clust, size; } fatfile;
} draw_t;

static void draw_text(draw_t *draw, unsigned y0, const char *str) {
	unsigned a, x, y, st = draw->st, n = draw->cols;
	unsigned sel = y0 == draw->sel - draw->msel;
	unsigned pos = y0 * st * FONT_H;
	do {
		const uint8_t *bm; uint32_t c = sel;
		if ((a = *str)) str++; else a = 0x20, c = 0;
		if (a - 0x20 >= 0x60) a = '?';
		bm = font_data + (a - 0x20) * FONT_H;
		if (sys_data.mac & 0x100) {
			uint8_t *p = (uint8_t*)draw->framebuf + pos;
			c = (c - 1) & 0xff;
			for (y = 0; y < FONT_H; y++, p += st)
			for (a = bm[y] ^ c, x = 0; x < FONT_W; x++, a <<= 1)
				p[x] = a & 0x80;
		} else {
			uint16_t *p = draw->framebuf + pos;
			c = draw->pal[c].u32;
			for (y = 0; y < FONT_H; y++, p += st)
			for (a = bm[y], x = 0; x < FONT_W; x++, a <<= 1)
				p[x] = c >> ((a & 0x80) >> 3);
		}
		pos += FONT_W;
	} while (--n);
}

static void draw_msel(draw_t *draw, unsigned msel) {
	unsigned msel1 = draw->msel;
	char *p = draw->menu;
	int skip = msel - msel1;
	draw->msel = msel;
	if (skip < 0) p = draw->menu0, skip = msel;
	while (skip--) {
		uint32_t next = *(uint32_t*)p;
		if (!next) break;
		p += next;
	}
	draw->menu = p;
	draw->flags |= 1;
}

static void draw_menu_adv(draw_t *draw, int adv) {
	if (adv < 0) {
		if (draw->sel > 0) {
			if (--draw->sel < draw->msel)
			draw_msel(draw, draw->sel);
		}
	} else {
		if (draw->sel + 1 < draw->mrows) {
			if (++draw->sel - draw->msel >= draw->rows)
				draw_msel(draw, draw->msel + 1);
		}
	}
}

/* 888 -> 565 */
#define RGB565(v) \
	(((v) >> 8 & 0xf800) | ((v) >> 5 & 0x7e0) | ((v) >> 3 & 0x1f))

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

	args = menu_file(draw->menu, draw->sel - draw->msel);
	if (!args) return 0; // should never happen
	name = get_first_arg(args);
	if (!name) return 0; // caption/separator
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
	unsigned key_time[2] = { 0 };
	unsigned scroll_len = 0;

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
	draw.pal[1].u16[0] = RGB565(0xffffff); // sel_bg
	draw.pal[1].u16[1] = RGB565(0x000000); // sel_text
	{
		char *vars = menu + *(uint32_t*)(menu + 4);
		char *args = find_var("_colors", vars);
		if (args) {
			char *d = (char*)CHIPRAM_ADDR;
			extract_args_t x = { 0, 0, d + 0x1000 };
			if (!extract_args(0, &x, args, d + 6)) {
				DBG_LOG("too many args\n");
			} else if (x.argc != 4) {
				DBG_LOG("wrong number of args (%d)\n", x.argc);
			} else {
				unsigned i, a; char *p = d + 6;
				for (i = 0; i < 4; i++) {
					a = strtol(p, NULL, 0);
					((uint16_t*)draw.pal)[i] = RGB565(a);
					while (*p++);
				}
			}
		}
	}

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

	draw.flags = 1;
	draw.sel = 0;
	draw.msel = 0;
	draw.mrows = menu_count(draw.menu, ~0);
	if (draw.mrows <= 0) return 1;

	if (sys_data.mac & 0x100)
		sys_data.mac &= ~1; // disable irq refresh
	sys_framebuffer(draw.framebuf);
	if (sys_data.mac & 0x100) {
		struct sys_display *disp = &sys_data.display;
		unsigned w = disp->w2, h = disp->h2;
		draw.framebuf = (uint16_t*)((uint8_t*)draw.framebuf +
				((h % FONT_H) >> 1) * w);
	}
	sys_start();
	// flush events
	{
		int key;
		while (sys_event(&key) != EVENT_END);
	}

	for (;;) {
		sys_wait_refresh();
		if (draw.flags & 1) {
			unsigned size = draw.st * draw.rows * FONT_H;
			uint16_t *p = draw.framebuf;
			draw.flags &= ~1;
			if (sys_data.mac & 0x100) {
				memset(p, 0x80, size);
			} else {
				unsigned a = draw.pal[0].u16[0];
				do *p++ = a; while (--size);
			}
		}
		{
			unsigned i;
			char *p = draw.menu;
			for (i = 0; i < draw.rows; i++) {
				uint32_t next = *(uint32_t*)p;
				if (!next) break;
				p += next;
				draw_text(&draw, i, p + 4);
			}
		}
		sys_start_refresh();
		sys_wait_ms(10);
		if (!(draw.flags & 6)) scroll_len = 0;
		for (;;) {
			int type, key;
			type = sys_event(&key);
			if (type == EVENT_END) break;
			if (type == EVENT_KEYUP) {
				switch (key) {
				case 0x32: // 2
				case 0x04: // UP
					draw.flags &= ~2; break;
				case 0x35: // 5
				case 0x05: // DOWN
					draw.flags &= ~4; break;
				}
				continue;
			}
			if (type == EVENT_KEYDOWN) {
				DBG_LOG("key = 0x%02x\n", key);
				switch (key) {
				case 0x32: // 2
				case 0x04: // UP
					draw.flags |= 2; draw_menu_adv(&draw, -1);
					key_time[0] = sys_timer_ms();
					break;
				case 0x35: // 5
				case 0x05: // DOWN
					draw.flags |= 4; draw_menu_adv(&draw, 1);
					key_time[1] = sys_timer_ms();
					break;
				//case 0x34: // 4
				//case 0x06: // LEFT
				// TODO: page up
				//case 0x36: // 6
				//case 0x07: // RIGHT
				// TODO: page down
				case 0x01: // DIAL
				case 0x08: // LSOFT
				case 0x09: // RSOFT
				// some phones don't have a center key
				case 0x0d: // CENTER
					if (run_binary(&draw)) goto loop_end;
					break;
				}
			}
		}
		{
			unsigned i, time = sys_timer_ms();
			unsigned scroll_delay = scroll_len ? 50 : 350;
			for (i = 0; i < 2; i++)
			if (draw.flags >> i & 2)
			if (time - key_time[i] > scroll_delay) {
				key_time[i] += scroll_delay;
				draw_menu_adv(&draw, i ? 1 : -1);
				if (scroll_len < 20) scroll_len++;
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

	for (i = 0; i < 64; i++)
		sys_data.keytrn[0][i] = keymap[i];
	// ignore combinations with the power key
	for (i = 0; i < 64; i++)
		sys_data.keytrn[1][i] = 0;
}

