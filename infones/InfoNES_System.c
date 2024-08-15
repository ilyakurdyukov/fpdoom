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

static int LoadSRAM(void);
static int SaveSRAM(void);

WORD NesPalette[64] = { /* ARGB1555 */
  0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
  0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
  0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
  0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
  0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
  0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000
};

#if NES_DISP_WIDTH != 256 || NES_DISP_HEIGHT != 240
#error
#endif

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w1, h = disp->h1, sh;
	int scaler = sys_data.scaler - 1;
	unsigned crop = 0;
	if (h > w) h = w;
	if (scaler >= 99) crop = 8, scaler -= 100;
	if ((unsigned)scaler > 2) scaler = h <= 128 ? 1 : 0;
	sys_data.scaler = scaler | crop << 3;
	sh = scaler == 1 ? 1 : 0;
	disp->w2 = 256 >> sh;
	disp->h2 = (240 - crop * 2) >> sh;
}

static void *framebuf_mem;
static uint16_t *framebuf;

static void framebuf_init(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w2, h = disp->h2;
	unsigned size = w * h; uint8_t *p;

	p = malloc(size * 2 + 31);
	framebuf_mem = p;
	p += -(intptr_t)p & 31;
	framebuf = (uint16_t*)p;
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

#ifdef USE_ASM
void scr_update_1d1_asm(void *src, uint16_t *dst, unsigned n);
void scr_update_1d2_asm(void *src, uint16_t *dst, int32_t w, int32_t h);
#define scr_update_1d1 scr_update_1d1_asm
#define scr_update_1d2 scr_update_1d2_asm
#else
void scr_update_1d1(void *src, uint16_t *dst, unsigned n) {
	uint32_t *s = (uint32_t*)src;
	uint32_t *d = (uint32_t*)dst;
	do {
		uint32_t a = *s++;
		a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
		*d++ = a;
	} while ((n -= 2));
}

void scr_update_1d2(void *src, uint16_t *dst, int32_t w, int32_t h) {
	uint32_t *s = (uint32_t*)src;
	uint32_t a, b, c, st = w << 1;
	do {
		h -= w << 16;
		do {
			a = *s;
			b = *(uint32_t*)((uint8_t*)s + st);
			s++;
			c = (a & 0x03e07c1f) + (b & 0x03e07c1f);
			a = (a & 0x7c1f03e0) + (b & 0x7c1f03e0);
			a = c + (a >> 16 | a << 16);
			// rounding half to even
			a += (0x200401 & a >> 2) + 0x200401;
			a >>= 2; a &= 0x03e07c1f;
			a |= a >> 16;
			*dst++ = a + (a & ~0x1f);
		} while ((h += 2 << 16) < 0);
		s = (uint32_t*)((uint8_t*)s + st);
	} while ((h -= 2));
}
#endif

void InfoNES_LoadFrame(void) {
	unsigned h, scaler, crop;
	WORD *src;
	sys_wait_refresh();
	scaler = sys_data.scaler;
	crop = scaler >> 3;
	h = 240 - crop * 2;
	src = WorkFrame + crop * 256;
	if (!(scaler & 7))
		scr_update_1d1(src, framebuf, 256 * h);
	else
		scr_update_1d2(src, framebuf, 256, h);
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

