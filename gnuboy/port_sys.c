#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "syscode.h"
#include "input.h"
#include "emu.h"
#include "loader.h"

extern uint16_t *framebuf;

void* framebuffer_init(unsigned size1);
extern void (*app_pal_update)(int i, int r, int g, int b);
extern void (*app_scr_update)(uint8_t *src, void *dest);

#include "sys.h"

#include <stdlib.h>

#include "fb.h"
#include "input.h"
#include "rc.h"

struct fb fb;

rcvar_t vid_exports[] = {
	RCV_END
};

void vid_resize() {}
void vid_preinit() {}

void vid_init() {
	fb.w = 160;
	fb.h = 144;
	fb.pelsize = 1;
	fb.pitch = fb.w * fb.pelsize;
	fb.indexed = 1;
	fb.enabled = 1;
	fb.dirty = 0;
	fb.ptr = framebuffer_init(160 * 144);
	sys_start();
}

void vid_close() {}
void vid_end();
void vid_settitle(char *title) {}

void vid_setpal(int i, int r, int g, int b) {
	app_pal_update(i, r, g, b);
}

void vid_begin() {}
void vid_end() {
	sys_wait_refresh();
	app_scr_update(fb.ptr, framebuf);
	sys_start_refresh();
}

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

enum {
	KEY_SELECT = K_SPACE, KEY_START = K_ENTER,
	KEY_A = 'd', KEY_B = 's',

	KEY_UP = K_UP,
	KEY_LEFT = K_LEFT,
	KEY_RIGHT = K_RIGHT,
	KEY_DOWN = K_DOWN,

	KEY_RESET = 0x200, KEY_EXIT, KEY_SAVE,
};

void keytrn_init(void) {
	uint8_t keymap[64]; uint16_t rkeymap[64];
	static const uint16_t keys[] = {
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

		KEY(1, KEY_B)
		KEY(3, KEY_B)
		KEY(7, KEY_B)
		KEY(8, KEY_B)
		KEY(9, KEY_B)

		KEY(CENTER, KEY_A)
		KEY(DIAL, KEY_A)
		KEY(STAR, KEY_A)
		KEY(0, KEY_A)
		KEY(HASH, KEY_A)

		KEY(VOLUP, KEY_A)
		KEY(VOLDOWN, KEY_B)
		KEY(PLUS, KEY_START)
	};
	static const uint16_t keys_power[] = {
		KEY(LSOFT, KEY_START)
		KEY(DIAL, KEY_EXIT)
		KEY(UP, KEY_RESET)
		KEY(DOWN, KEY_EXIT)
		KEY(LEFT, KEY_SELECT)
		KEY(RIGHT, KEY_START)
		KEY(CENTER, KEY_B)
		KEY(0, KEY_SAVE)
#undef KEY
	};
	int i, flags = sys_getkeymap(keymap);

#define FILL_RKEYMAP(keys) \
	memset(rkeymap, 0, sizeof(rkeymap)); \
	for (i = 0; i < (int)sizeof(keys) / 2; i += 2) \
		rkeymap[keys[i]] = keys[i + 1];

	FILL_RKEYMAP(keys)

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

static unsigned key_timer[2];
static unsigned key_flags = 0;

void ev_poll(int wait) {
	event_t ev;
	int type, key;
	(void)wait;
	for (;;) {
		unsigned key2;
		type = sys_event(&key);
		switch (type) {
		case EVENT_KEYDOWN:
			key2 = key - KEY_RESET;
			if (key2 < 3) {
				if (key2 == KEY_SAVE - KEY_RESET) {
					sram_save();
       	} else {
					if (key_flags & 1 << key2) break;
					key_flags |= 1 << key2;
					key_timer[key2] = sys_timer_ms();
				}
				break;
			}
			ev.type = EV_PRESS;
			goto post;
		case EVENT_KEYUP:
			key2 = key - KEY_RESET;
			if (key2 < 3) {
				key_flags &= ~(1 << key2); break;
			}
			ev.type = EV_RELEASE;
post:
			ev.code = key;
			/* returns 0 if queue is full */
			if (!ev_postevent(&ev)) goto end;
			break;
		case EVENT_END: goto end;
#if EMBEDDED == 1
		case EVENT_QUIT:
			ev.type = EV_PRESS;
			key = K_ESC;
			goto post;
#endif
		}
	}
end:
	if (key_flags) {
		unsigned i, time = sys_timer_ms();
		for (i = 0; i < 2; i++) {
			if (key_flags >> i & 1)
			if (time - key_timer[i] > 1000) {
				key_flags &= ~(1 << i);
				if (i) {
					ev.type = EV_PRESS;
					ev.code = K_ESC;
					ev_postevent(&ev);
				} else emu_reset();
			}
		}
	}
}

#if defined(APP_MEM_RESERVE) && defined(APP_DATA_EXCEPT)

#define ROMMAP_ADDR 0x8000000
#define ROMMAP_SIZE (8 << 20)
#define ROMMAP_EXTRA 0

struct {
	uint32_t *tab2;
	uint16_t *map;
	uint32_t pages, npages, last;
	uint32_t fileoffs, rompages;
	FILE *file;
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
	memset(p, 0, res0 + res1);
	rommap.map = (uint16_t*)p; p += res0;
	rommap.tab2 = (uint32_t*)p;
	rommap.pages = (uint32_t)(p + res1);
	memset(p + res1, -1, 0x1000);
	for (i = 0; i < ROMMAP_EXTRA; i++)
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

static void rommap_page_update(unsigned i, uint32_t val) {
	uint32_t *tab2 = rommap.tab2;
	tab2[i] = val;
	clean_dcache();
	invalidate_tlb_mva(ROMMAP_ADDR + (i << 12));
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
	i = pos - ROMMAP_EXTRA;
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
	rommap.fileoffs = pos;
	rommap.file = f;
	size &= ~0xfff;
	if (size > 4 << 20) size = 4 << 20;
	rommap.rompages = size >> 12;
	{
		uint32_t *tab2 = rommap.tab2;
		uint32_t pages = rommap.pages;
		unsigned i = ROMMAP_EXTRA + (size >> 12), cb = 3 << 2;
		while (i < ROMMAP_SIZE >> 12)
			tab2[i++] = pages | cb | 2;
		clean_dcache();
		invalidate_tlb();
	}
	return (uint8_t*)ROMMAP_ADDR;
}
#endif
