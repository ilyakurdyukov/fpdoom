#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "InfoNES.h"
#include "InfoNES_System.h"
#include "InfoNES_pAPU.h"

#include <stdint.h>
#include "syscode.h"

static const char *szRomName;
static char szSaveName[256];
static char nSRAM_SaveFlag;
static char app_quit = 0;
static BYTE SRAM_COPY[SRAM_SIZE];

// 112: Darkwing Duck
// 113: Battle City
// 113: Lode Runner
unsigned STEP_PER_SCANLINE = 112;
// 1: fix right border
unsigned NES_Hacks = 0;

#define FRAMERATE 60
#define TIMER_MUL (((1000 << 12) + FRAMERATE - 1) / FRAMERATE)

static unsigned timerlast, timertick, timertick_ms;
static unsigned scaler_crop;

static int LoadSRAM(void);
static int SaveSRAM(void);

/* 888 -> 555 */
#define RGB555(v) \
	(((v) >> 9 & 0x7c00) | ((v) >> 6 & 0x3e0) | ((v) >> 3 & 0x1e))

WORD NesPalette[64] = { /* ARGB1555 */
#define X(a, b, c, d) RGB555(a), RGB555(b), RGB555(c), RGB555(d),
X(0x737373, 0x21188c, 0x0000ad, 0x42009c) /* 00 */
X(0x8c0073, 0xad0010, 0xa50000, 0x7b0800)
X(0x422900, 0x004200, 0x005200, 0x003910)
X(0x18395a, 0x000000, 0x000000, 0x000000)
X(0xbdbdbd, 0x0073ef, 0x2139ef, 0x8400f7) /* 10 */
X(0xbd00bd, 0xe7005a, 0xde2900, 0xce4a08)
X(0x8c7300, 0x009400, 0x00ad00, 0x009439)
X(0x00848c, 0x000000, 0x000000, 0x000000)
X(0xffffff, 0x39bdff, 0x5a94ff, 0xce8cff) /* 20 */
X(0xf77bff, 0xff73b5, 0xff7363, 0xff9c39)
X(0xf7bd39, 0x84d610, 0x4ade4a, 0x5aff9c)
X(0x00efde, 0x7b7b7b, 0x000000, 0x000000)
X(0xffffff, 0xade7ff, 0xc6d6ff, 0xd6ceff) /* 30 */
X(0xffc6ff, 0xffc6de, 0xffbdb5, 0xffdead)
X(0xffe7a5, 0xe7ffa5, 0xadf7bd, 0xb5ffce)
X(0x9cfff7, 0xc6c6c6, 0x000000, 0x000000)
#undef X
};

#if NES_DISP_WIDTH != 256 || NES_DISP_HEIGHT != 240
#error
#endif

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w1, h = disp->h1;
	int scaler = sys_data.scaler - 1;
	unsigned crop = 0, wide = 0;
	if (h > w) h = w;
	if (scaler >= 99) crop = 8, scaler -= 100;
	if (scaler >= 49) wide = 1, scaler -= 50;
	if ((unsigned)scaler >= 6) {
		if (h >= 320) { // for 320x480 screens
			scaler = 2; wide = w >= 384;
		} else scaler = h <= 128;
	}
	if (scaler >= 2) {
		h = 320;
		if (scaler == 2) {
			w = wide ? 384 : 342;
			if (crop) h -= 21;
		} else {
			w = wide ? 480 : 384;
			crop = 13; // 240 - 13 * 2 = 214
		}
	} else {
		int sh = scaler == 1;
		w = (wide ? 320 : 256) >> sh;
		h = (240 - crop * 2) >> sh;
	}
	sys_data.scaler = scaler * 2 | wide;
	scaler_crop = crop;
	disp->w2 = w;
	disp->h2 = h;
}

static void *framebuf_mem;
static uint16_t *framebuf;

static void framebuf_init(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w2, h = disp->h2;
	unsigned size = w * h; uint8_t *p;
	if (sys_data.scaler >= 3 * 2) size += w;
	p = malloc(size * 2 + 31);
	framebuf_mem = p;
	p += -(intptr_t)p & 31;
	framebuf = (uint16_t*)p;
	if (sys_data.scaler == 2 * 2) {
		p -= 2;
		do p += w * 2, *(uint16_t*)p = 0;
		while (--h);
	}
}

int main(int argc, char **argv) {

	while (argc > 1) {
		if (argc > 2 && !strcmp(argv[1], "--scanline_step")) {
			unsigned a = atoi(argv[2]);
			if (a - 110 < 10) STEP_PER_SCANLINE = a;
			argc -= 2; argv += 2;
		} else if (argc > 2 && !strcmp(argv[1], "--hacks")) {
			NES_Hacks = atoi(argv[2]);
			argc -= 2; argv += 2;
		} else if (!strcmp(argv[1], "--")) {
			argc -= 1; argv += 1;
			break;
		} else break;
	}

	if (argc != 2) return 1;

	szRomName = argv[1];
	if (InfoNES_Load(szRomName)) {
		fprintf(stderr, "!!! InfoNES_Load failed\n");
		return 1;
	}
	printf("nes: mapper = %03u\n", MapperNo);
	{
		const char *name = szRomName;
		const char *s = strrchr(name, '.');
		unsigned n = s ? (unsigned)(s - name) : strlen(name);
		snprintf(szSaveName, sizeof(szSaveName), "%.*s.srm", n, name);
	}
	LoadSRAM();
	framebuf_init();
	sys_framebuffer(framebuf);
	sys_start();

	timerlast = sys_timer_ms();
	timertick = 0; timertick_ms = 0;

	InfoNES_Main();
	SaveSRAM();
	return 0;
}

static int LoadSRAM(void) {
	FILE *f; int pos, tag;

	nSRAM_SaveFlag = 0;
	if (!ROM_SRAM) return 0;
	nSRAM_SaveFlag = 1;

	f = fopen(szSaveName, "rb");
	if (!f) return -1;

	pos = 0; tag = fgetc(f);
	if (tag != EOF)
	while (pos < SRAM_SIZE) {
		int a, k;
		if ((a = fgetc(f)) == EOF) break;
		if (a != tag) {
			SRAM[pos++] = a;
			continue;
		}
		if ((a = fgetc(f)) == EOF) break;
		if ((k = fgetc(f)) == EOF) break;
		if (++k > SRAM_SIZE - pos) break;
		do SRAM[pos++] = a; while (--k);
	}
	fclose(f);
	memcpy(SRAM_COPY, SRAM, SRAM_SIZE);
	return pos < SRAM_SIZE ? -1 : 0;
}

static int SaveSRAM(void) {
	FILE *f; int i, tag, next;
	unsigned short *hist;
	unsigned char *dst, *dst_buf;

	if (!nSRAM_SaveFlag) return 0;
	if (!memcmp(SRAM_COPY, SRAM, SRAM_SIZE)) return 0;
	memcpy(SRAM_COPY, SRAM, SRAM_SIZE);

	dst_buf = malloc(SRAM_SIZE + (SRAM_SIZE >> 7) + 1);
	hist = (unsigned short*)dst_buf;

	memset(hist, 0, 256 * sizeof(*hist));
	next = -1;
	for (i = 0; i < SRAM_SIZE; i++) {
		int prev = next;
		next = SRAM[i];
		if (prev != next) hist[next]++;
	}
	for (i = 1, next = hist[tag = 0]; i < 256; i++)
		if (hist[i] < next) tag = i, next = hist[i];

	dst = dst_buf;
	*dst++ = tag;
	i = 0; next = SRAM[i++];
	do {
		int prev = next, run = 0;
		do next = i < SRAM_SIZE ? SRAM[i++] : -1;
		while (++run < 256 && prev == next);
		if (run > 3 || prev == tag) {
			*dst++ = tag; *dst++ = prev; *dst++ = run - 1;
		} else do *dst++ = prev; while (--run);
	} while (next >= 0);

	f = fopen(szSaveName, "wb");
	if (f) {
		fwrite(dst_buf, 1, dst - dst_buf, f);
		fclose(f);
	}
	free(dst_buf);
	return f ? 0 : -1;
}

int InfoNES_Menu(void) {
	if (app_quit) return -1;
	return 0;
}

int InfoNES_ReadRom(const char *fn) {
	unsigned n; int ret = -1;
	FILE *f;

	f = fopen(fn, "rb");
	if (!f) return ret;
	do {
		n = sizeof(NesHeader);
		if (fread(&NesHeader, 1, n, f) != n) break;
		if (memcmp(NesHeader.byID, "NES\x1a", 4)) break;
		memset(SRAM, 0, SRAM_SIZE);

		/* If trainer presents Read Triner at 0x7000-0x71ff */
		if (NesHeader.byInfo1 & 4) {
			n = 512;
			if (fread(SRAM + 0x1000, 1, n, f) != n) break;
		}

		n = NesHeader.byRomSize << 14;
		ROM = (BYTE*)malloc(n);
		if (fread(ROM, 1, n, f) != n) break;

		if (NesHeader.byVRomSize) {
			n = NesHeader.byVRomSize << 13;
			VROM = (BYTE*)malloc(n);
			if (fread(VROM, 1, n, f) != n) break;
		}
		ret = 0;
	} while (0);

	fclose(f);
	return ret;
}

void InfoNES_ReleaseRom(void) {
	if (ROM) { free(ROM); ROM = NULL; }
	if (VROM) { free(VROM); VROM = NULL; }
}

static void wait_frame(void) {
	unsigned t0 = timertick, t1, t2;
	t2 = FrameSkip + 1;
	do t0 += TIMER_MUL;
	while (--t2);
	t2 = t0 >> 12;
	t1 = t2 - timertick_ms;
	if (t2 >= 1000) t0 -= TIMER_MUL * 60, t2 -= 1000;
	timertick = t0;
	timertick_ms = t2;
	t1 += timerlast; // time to next frame
	timerlast = t1;
	t0 = sys_timer_ms();
	t1 -= t0;
	if ((int)t1 < 0) {
		t1 = -t1;
		if (t1 > 500 / 60 && FrameSkip < 3) FrameSkip++;
	} else {
		if (FrameSkip) FrameSkip--;
		else if (t1 > 500 / 60) sys_wait_ms(t1);
	}
}

typedef void (*scr_update_t)(void *src, uint16_t *dst, unsigned h);

extern const scr_update_t scr_update_fn[];

void InfoNES_LoadFrame(void) {
	unsigned h, crop;
	WORD *src;
	sys_wait_refresh();
	crop = scaler_crop;
	h = 240 - crop * 2;
	src = WorkFrame + crop * 256;
	scr_update_fn[sys_data.scaler](src, framebuf, h);
	sys_start_refresh();
	wait_frame();
}

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

enum {
	KEY_RESET = 1, KEY_EXIT, KEY_SAVE,
	KEY_A = 8, KEY_B, KEY_SELECT, KEY_START,
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
};

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

static unsigned key_timer[2];
static unsigned key_flags = 0;

void InfoNES_PadState(DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem) {
	int type, key;
	(void)pdwPad2;
	if (key_flags) {
		unsigned i, time = sys_timer_ms();
		for (i = 0; i < 2; i++) {
			if (key_flags >> i & 1)
			if (time - key_timer[i] > 1000) {
				key_flags &= ~(1 << i);
				if (i) {
					*pdwSystem |= PAD_SYS_QUIT;
					app_quit = 1;
				} else InfoNES_Reset();
			}
		}
	}
	for (;;) {
		type = sys_event(&key);
		switch (type) {
		case EVENT_KEYUP:
			if (key >= 8) *pdwPad1 &= ~(1 << (key - 8));
			else if ((key -= KEY_RESET) < 2) key_flags &= ~(1 << key);
			break;
		case EVENT_KEYDOWN:
			if (key >= 8) *pdwPad1 |= 1 << (key - 8);
			else if (key == KEY_SAVE) SaveSRAM();
			else if ((key -= KEY_RESET) < 2) {
				key_flags |= 1 << key;
				key_timer[key] = sys_timer_ms();
			}
			break;

		case EVENT_END: goto end;
		case EVENT_QUIT:
			*pdwSystem |= PAD_SYS_QUIT;
			app_quit = 1;
			goto end;
		}
	}
end:
	return;
}

void *InfoNES_MemoryCopy(void *dst, const void *src, int n) {
	return memcpy(dst, src, n);
}

void *InfoNES_MemorySet(void *dst, int val, int n) {
	return memset(dst, val, n);
}

void InfoNES_DebugPrint(char *pszMsg) {
	fprintf(stderr, "%s\n", pszMsg);
}

#include <stdarg.h>

void InfoNES_MessageBox(char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
}

void InfoNES_Wait(void) {}

void InfoNES_SoundInit(void) {}
int InfoNES_SoundOpen(int samples_per_sync, int sample_rate) {
	(void)samples_per_sync; (void)sample_rate;
	return 1;
}
void InfoNES_SoundClose(void) {}
void InfoNES_SoundOutput(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5) {
	(void)samples; (void)wave1; (void)wave2; (void)wave3; (void)wave4; (void)wave5;
}

