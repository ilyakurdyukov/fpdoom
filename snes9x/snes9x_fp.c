#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "syscode.h"

#define SNES_WIDTH 256
#define SNES_HEIGHT 224
#define SNES_HEIGHT_EXTENDED 239

int emu_init(const char *rom_fn);
void emu_start(void);
void emu_loop(void);
void emu_exit(void);
void emu_save(int);
void emu_reset(void);

unsigned emu_getpad0(void);
uint32_t joypad0_state = 0x80000000;
static int app_quit = 0;
uint32_t SNES_crc, SNES_hacks;

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w1, h = disp->h1;
	int scaler = sys_data.scaler - 1;
	unsigned crop = 0, wide = 0;
	if (h > w) h = w;
	if (scaler >= 99) crop = 1, scaler -= 100;
	if (scaler >= 49) wide = 1, scaler -= 50;
	if (h <= 68) {
#if !USE_16BIT
		scaler = 2; crop = 1; wide = h == 68;
		if (h == 48) {
#else
		if (1) {
#endif
			fprintf(stderr, "!!! unsupported resolution (%dx%d)\n", w, h);
			exit(1);
		}
	} else {
		if ((unsigned)scaler >= 2) {
			scaler = h <= 128;
		}
		w = wide ? 320 : 256;
		h = crop ? 224 : 239;
		if (scaler == 1)
			w >>= 1, h = (h + 1) >> 1;
	}
	sys_data.user[0] = crop;
	sys_data.scaler = scaler << 1 | wide;
	disp->w2 = w;
	disp->h2 = h;
}

static void *framebuf_mem;
uint16_t *framebuf;

typedef void (*pal_update_t)(uint16_t *s, uint16_t *d);
extern const pal_update_t pal_update_fn[];
void (*app_pal_update)(uint16_t *s, uint16_t *d);

static void framebuf_init(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w2, h = disp->h2;
	unsigned size = w * h; uint8_t *p;
	unsigned size2 = 0;
#if !USE_16BIT
	static const uint8_t pal_size[] = { 2, 4, 1 };
	unsigned scaler = sys_data.scaler;
	size2 = pal_size[scaler >> 1] << 8;
	app_pal_update = pal_update_fn[scaler >> 1];
#endif
	p = malloc(size * 2 + size2 + 28);
	framebuf_mem = p;
	p += -(intptr_t)p & 31;
	p += size2;
	framebuf = (uint16_t*)p;
	sys_framebuffer(framebuf);
	sys_start();
}

int main(int argc, char **argv) {

	while (argc > 1) {
		if (argc > 2 && !strcmp(argv[1], "--crc")) {
			SNES_crc = strtol(argv[2], NULL, 0);
			SNES_hacks |= 1;
			argc -= 2; argv += 2;
		} else if (!strcmp(argv[1], "--")) {
			argc -= 1; argv += 1;
			break;
		} else break;
	}

	if (argc != 2) return 1;
	if (!emu_init(argv[1])) return 1;

	framebuf_init();
	emu_start();
	do {
		emu_loop();
	} while (!app_quit);
	emu_exit();
	free(framebuf_mem);
	return 0;
}

typedef void (*scr_update_t)(uint16_t *d, const uint8_t *s, uint16_t *c16, unsigned h);
typedef void (*scr_update16_t)(uint16_t *d, const uint16_t *s, unsigned h);

extern const scr_update_t scr_update_fn[];
extern const scr_update16_t scr_update16_fn[];

void emu_refresh(const void *src, int h) {
	static char clean = 0;
	uint16_t *d; const uint8_t *s;
	int crop;
	sys_wait_refresh();

	crop = sys_data.user[0];
	s = src; d = framebuf;
	if (h > 224) {
		int smul = USE_16BIT ? 4 : 1;
		if (crop) s += 8 * 256 * smul, h = 224;
		clean = 0;
	} else if (!crop) {
		static const unsigned tab[][3] = {
#define X(w, h, c1, c2) \
	{ c1 * w * 2, (h - c2) * w * 2, c2 * w * 2 },
			X(256, 239, 8, 7)
			X(320, 239, 8, 7)
			X(128, 120, 4, 4)
			X(160, 120, 4, 4)
#undef X
		};
		const unsigned *p = tab[sys_data.scaler];
		if (!clean) {
			clean = 1;
			memset(d, 0, p[0]);
			memset((uint8_t*)d + p[1], 0, p[2]);
		}
		d = (uint16_t*)((uint8_t*)d + p[0]);
	}
#if USE_16BIT
	scr_update16_fn[sys_data.scaler](d, (uint16_t*)s, h);
#else
	scr_update_fn[sys_data.scaler](d, s, framebuf - 256, h);
#endif
	sys_start_refresh();
}

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

enum {
	KEY_RESET = 16, KEY_EXIT, KEY_SAVE,
	KEY_TR = 4, KEY_TL, KEY_X, KEY_A,
	KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UP,
	KEY_START, KEY_SELECT, KEY_Y, KEY_B
};

static unsigned key_timer[2];
static unsigned key_flags = 0;

unsigned emu_getpad0(void) {
	for (;;) {
		int key, event;
		event = sys_event(&key);
		switch (event) {
		case EVENT_KEYDOWN:
			if (key < 16) {
				joypad0_state |= 1 << key;
			} else {
				if (key == KEY_SAVE) emu_save(-1);
				else if ((key -= KEY_RESET) < 2) {
					key_flags |= 1 << key;
					key_timer[key] = sys_timer_ms();
				}
			}
			break;
		case EVENT_KEYUP:
			if (key < 16)
				joypad0_state &= ~(1 << key);
			break;

		case EVENT_END: goto end;
		case EVENT_QUIT:
			app_quit = 1;
			goto end;
		}
	}
end:
	if (key_flags) {
		unsigned i, time = sys_timer_ms();
		for (i = 0; i < 2; i++) {
			if (key_flags >> i & 1)
			if (time - key_timer[i] > 1000) {
				key_flags &= ~(1 << i);
				if (i) app_quit = 1;
				else emu_reset();
			}
		}
	}
	return joypad0_state;
}

void keytrn_init(void) {
	uint8_t keymap[64], rkeymap[64];
	static const uint8_t keys[] = {
#define KEY(name, val) KEYPAD_##name, val,
		KEY(LSOFT, KEY_SELECT)
		KEY(RSOFT, KEY_START)

		KEY(UP, KEY_UP)
		KEY(LEFT, KEY_LEFT)
		KEY(RIGHT, KEY_RIGHT)
		KEY(DOWN, KEY_DOWN)

		KEY(2, KEY_UP)
		KEY(4, KEY_LEFT)
		KEY(6, KEY_RIGHT)
		KEY(5, KEY_DOWN)

		KEY(1, KEY_TL)
		KEY(3, KEY_TR)

		KEY(CENTER, KEY_B)
		KEY(DIAL, KEY_A)

		KEY(7, KEY_X)
		KEY(8, KEY_Y)
		KEY(9, KEY_X)

		KEY(STAR, KEY_A)
		KEY(0, KEY_B)
		KEY(HASH, KEY_A)
	};
	static const uint8_t keys_power[] = {
		KEY(LSOFT, KEY_RESET)
		KEY(DIAL, KEY_EXIT)
		KEY(UP, KEY_RESET)
		KEY(DOWN, KEY_EXIT)
		KEY(LEFT, KEY_SELECT)
		KEY(RIGHT, KEY_START)
		KEY(CENTER, KEY_B)
		KEY(0, KEY_SAVE)
	};
	int i, flags = sys_getkeymap(keymap);
	(void)flags;

#define FILL_RKEYMAP(keys) \
	memset(rkeymap, 0, sizeof(rkeymap)); \
	for (i = 0; i < (int)sizeof(keys); i += 2) \
		rkeymap[keys[i]] = keys[i + 1];

#define FILL_KEYTRN(j) \
	for (i = 0; i < 64; i++) { \
		unsigned a = keymap[i]; \
		sys_data.keytrn[j][i] = a < 64 ? rkeymap[a] : 0; \
	}

	FILL_RKEYMAP(keys)
	FILL_KEYTRN(0)
	FILL_RKEYMAP(keys_power)
	FILL_KEYTRN(1)
#undef FILL_RKEYMAP
#undef FILL_KEYTRN
}

#if defined(APP_MEM_RESERVE) && defined(APP_DATA_EXCEPT)

#define ROMMAP_ADDR 0x8000000
#define ROMMAP_SIZE (7 << 20)

struct {
	uint32_t *tab2;
	uint16_t *map;
	uint32_t pages, npages, last;
	uint32_t fileoffs, rompages;
	FILE *file;
	uint8_t superfx;
} rommap;

void invalidate_tlb(void);
void invalidate_tlb_mva(uint32_t);

size_t app_mem_reserve(void *start, size_t size) {
	uint32_t ram_addr = (uint32_t)start & 0xfc000000;
	uint32_t ram_size = *(volatile uint32_t*)ram_addr;
	uint8_t *p = (uint8_t*)start;
	uint32_t *tab1 = (uint32_t*)(p + size);
	unsigned i, domain = 2 << 5, cb = 3 << 2;
	uint32_t res1 = ROMMAP_SIZE >> (12 - 2);
	uint32_t res2 = 0x180000, res0;

	if (ram_size >= 8 << 20) res2 = 4 << 20;
	res0 = res2 >> 12;
	rommap.npages = res0 - 1; res0 <<= 1;
	res2 += res1 + res0;
	if (size < res2) {
		fprintf(stderr, "!!! app_memres failed\n");
		exit(1);
	}
	size -= res2; p += size;
	memset(p, 0, res0 + res1 + 0x1000);
	rommap.map = (uint16_t*)p; p += res0;
	rommap.tab2 = (uint32_t*)p;
	rommap.pages = (uint32_t)(p + res1);
	for (i = 0; i < (0x8000 >> 12); i++)
		((uint32_t*)p)[i] = (uint32_t)(p + res1) | cb | 2;
	tab1 += ROMMAP_ADDR >> 20;
	// coarse pages
	for (i = 0; i < ROMMAP_SIZE >> 20; i++, p += 0x400)
		tab1[i] = (uint32_t)p | domain | 0x11;
	invalidate_tlb();
	return size;
}

#if LIBC_SDIO < 3
#define ROMMAP_ERR(...) fprintf(stderr, __VA_ARGS__)
#else
#define ROMMAP_ERR(...) (void)0
#endif

#if 0
#define ROMMAP_LOG ROMMAP_ERR
#else
#define ROMMAP_LOG(...) (void)0
#endif

unsigned rommap_size(void) { return rommap.rompages << 12; }
void rommap_superfx(void) {
	uint32_t *tab2 = rommap.tab2;
	unsigned i, n = rommap.rompages;
	if (n > 0x200) n = 0x200;
	for (i = 0; i < n; i++) {
		uint32_t val = tab2[8 + i];
		i = 0x208 + (i << 1) - (i & 7);
		tab2[i] = val;
		tab2[i + 8] = val;
	}
	clean_dcache();
	invalidate_tlb();
	rommap.superfx = 1;
}

static void rommap_page_update(unsigned i, uint32_t val) {
	uint32_t *tab2 = rommap.tab2;
	tab2[i] = val;
	clean_dcache();
	invalidate_tlb_mva(ROMMAP_ADDR + (i << 12));
	if (!rommap.superfx) return;
	i -= 8;
	if (i >= 0x200) return;
	i = 0x208 + (i << 1) - (i & 7);
	tab2[i] = val;
	tab2[i + 8] = val;
	{
		uint32_t a = ROMMAP_ADDR + (i << 12);
		clean_dcache();
		invalidate_tlb_mva(a);
		invalidate_tlb_mva(a + 0x8000);
	}
}

void def_data_except(uint32_t fsr, uint32_t far, uint32_t pc);
void app_data_except(uint32_t fsr, uint32_t far, uint32_t pc) {
	uint32_t pos, buf;
	unsigned i, j, k, npages, cb = 3 << 2;
	uint16_t *map;
	// 0x7 : Translation Page
	// 0xf : Permission Page
	if ((fsr & 0xf7) != 0x27) goto err;
	pos = far - ROMMAP_ADDR;
	if (pos >= ROMMAP_SIZE) goto err;
	pos >>= 12;
	if (rommap.superfx) {
		buf = pos - 0x208;
		if (buf < 0x400)
			pos = ((buf >> 1 & ~7) | (buf & 7)) + 8;
	}
	map = rommap.map;
	if (fsr & 8) {
		uint32_t *tab2 = rommap.tab2;
		buf = tab2[pos];
		i = ((buf - rommap.pages) >> 12) - 1;
		if ((int)i >= 0) {
			map[i] = -1; // lock page
			ROMMAP_LOG("!!! page lock: pos = 0x%x, buf = 0x%x\n", pos, buf & ~0xfff);
			rommap_page_update(pos, buf | 0xff0);
			return;
		}
	}
	k = i = rommap.last;
	npages = rommap.npages;
	do {
		if (++i >= npages) i = 0;
		j = map[i];
		if (!(j >> 15)) goto found;
	} while (i != k);
	goto err_locked;
found:
	map[i] = fsr & 8 ? -1 : pos + 1;
	rommap.last = i;
	buf = rommap.pages + ((i + 1) << 12);
	if (fsr & 8)
	ROMMAP_LOG("!!! page map%s: pos = 0x%x, buf = 0x%x\n",
		fsr & 8 ? "+lock" : "", pos, buf);
	i = pos - 8;
	if (i < rommap.rompages) {
		if (fseek(rommap.file, (i << 12) + rommap.fileoffs, SEEK_SET)) {
			ROMMAP_ERR("!!! rommap: fseek failed\n");
			goto err;
		}
		if (fread((void*)buf, 1, 0x1000, rommap.file) != 0x1000) {
			ROMMAP_ERR("!!! rommap: fread failed\n");
			goto err;
		}
	} else {
		if (!(fsr & 8)) goto err;
		memset((void*)buf, 0, 0x1000);
	}
	if ((int)--j >= 0) rommap_page_update(j, 0);
	if (fsr & 8) buf |= 0xff0;
	rommap_page_update(pos, buf | cb | 2);
	return;

err_locked:
	ROMMAP_ERR("!!! rommap: all pages locked\n");
err:
	def_data_except(fsr, far, pc);
}

uint8_t* init_rommap(const char *fn) {
	size_t size, pos = 0;
	FILE *f;
	f = fopen(fn, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	if ((size & 0x1fff) == 512) pos = 512;
	rommap.fileoffs = pos;
	rommap.file = f;
	size &= ~0x1fff;
	if (size > 6 << 20) size = 6 << 20;
	rommap.rompages = size >> 12;
	{
		uint32_t *tab2 = rommap.tab2;
		uint32_t pages = rommap.pages;
		unsigned i = 8 + (size >> 12), cb = 3 << 2;
		while (i < ROMMAP_SIZE >> 12)
			tab2[i++] = pages | cb | 2;
		clean_dcache();
		invalidate_tlb();
	}
	return (uint8_t*)ROMMAP_ADDR;
}
#endif

time_t time(time_t *tloc) {
	// Groundhog Day reference
	// $ date +%s -d 1993-02-02T06:00+UTC
	time_t t = sys_timer_ms() / 1000 + 728632800;
	if (tloc) *tloc = t;
	return t;
}

