#include "build.h"
#include "baselayer_priv.h"
#include "types.h"
#include "keyboard.h"
#define TICRATE 120
void setvlinebpl(int dabpl);

#include "syscode.h"

uint8_t* framebuffer_init(void);
void (*app_pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
void (*app_scr_update)(uint8_t *src, void *dest);

static uint8_t *frame, *frame_ptr;
static void shutdownvideo(void);

int wm_msgbox(const char *name, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
	return 0;
}

int wm_ynbox(const char *name, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
	puts("(assuming 'No')");
	return 0;
}

void wm_setapptitle(const char *name) {}
void wm_setwindowtitle(const char *name) {}

static uint8_t *framebuf = NULL;

#ifdef GAME_DUKE3D
// min=768
unsigned maxcache1dsize = (1648 - 64) << 10;
#elif defined(GAME_SW)
unsigned maxcache1dsize = (1552 - 64) << 10;
#elif defined(GAME_BLOOD)
unsigned maxcache1dsize = (1600 - 64) << 10;
#else
unsigned maxcache1dsize = 3 << 20;
#endif
char playanm_flag = 0;

int main(int argc, char **argv) {
	int i, j, ret;

	{
		uint32_t ram_addr = (uint32_t)&main & 0xfc000000;
		uint32_t ram_size = *(volatile uint32_t*)ram_addr;
		uint32_t cachesize = maxcache1dsize;
		cachesize += ram_size - (4 << 20);
		// 160x128 mode uses less memory for the framebuffer
		if (sys_data.display.h2 <= 128) cachesize += 105 << 10;
		if (cachesize > 4 << 20) cachesize = 4 << 20;
		maxcache1dsize = cachesize;
	}

	for (i = j = 1; i < argc;) {
		char *p = argv[i++];
		if (*p == '-') {
			if (argc > i && !strcmp(p + 1, "cachesize")) {
				maxcache1dsize = atoi(argv[i++]) << 10;
				continue;
			}
			if (argc > i && !strcmp(p + 1, "playanm")) {
				playanm_flag = atoi(argv[i++]);
				continue;
			}
		}
		argv[j++] = p;
  }
	argc = j;

	framebuf = framebuffer_init();
	sys_framebuffer(framebuf);

	_buildargc = argc;
	_buildargv = (const char**)argv;
	baselayer_init();

	srand(sys_timer_ms());

	ret = app_main(_buildargc, (char const * const*)_buildargv);
	return ret;
}

int initsystem(void) {
	atexit(uninitsystem);
	return 0;
}

void uninitsystem(void) {
	uninitinput();
	uninitmouse();
	uninittimer();
	shutdownvideo();
}

void initputs(const char *str) {}

void debugprintf(const char *f, ...) {
#ifdef DEBUGGINGAIDS
	va_list va;

	va_start(va,f);
	Bvfprintf(stderr, f, va);
	va_end(va);
#endif
	(void)f;
}

int initinput(void) {
	inputdevices = 1; // keyboard (1)
	return 0;
}

void uninitinput(void) {}

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

enum {
#define X(num, name) KEY_##name = num,
	X(0x01, ESCAPE)
	X(0x0c, MINUS)
	X(0x0d, EQUALS)
	X(0x0f, TAB)
	X(0x1a, LBRACKET)
	X(0x1b, RBRACKET)
	X(0x1c, ENTER)
	X(0x1d, LCTRL)
	X(0x1e, A)
	X(0x27, SEMICOLON)
	X(0x28, QUOTE)
	X(0x29, TILDE)
	X(0x2c, Z)
	X(0x2d, X)
	X(0x33, COMMA)
	X(0x34, DOT)
	X(0x39, SPACE)
	X(0x3a, CAPSLOCK)
	X(0x57, F11)
	X(0xc8, UP)
	X(0xcb, LEFT)
	X(0xcd, RIGHT)
	X(0xd0, DOWN)
#undef X
};

void keytrn_init(void) {
	uint8_t keymap[64], rkeymap[64];
	static const uint8_t keys[] = {
#define KEY(name, val) KEYPAD_##name, val,
		KEY(UP, KEY_UP)
		KEY(LEFT, KEY_LEFT)
		KEY(RIGHT, KEY_RIGHT)
		KEY(DOWN, KEY_DOWN)
		KEY(2, KEY_UP)
		KEY(4, KEY_LEFT)
		KEY(6, KEY_RIGHT)
		KEY(5, KEY_DOWN)
		KEY(LSOFT, KEY_ENTER)
		KEY(RSOFT, KEY_ESCAPE)
		KEY(CENTER, KEY_LCTRL) /* fire */
		KEY(DIAL, KEY_CAPSLOCK) /* run toggle */
		KEY(1, KEY_COMMA) /* strafe left */
		KEY(3, KEY_DOT) /* strafe right */
		KEY(8, KEY_SPACE) /* use */
		KEY(7, KEY_SEMICOLON) /* prev weapon */
		KEY(9, KEY_QUOTE) /* next weapon */
		KEY(HASH, KEY_A) /* jump */
		KEY(STAR, KEY_Z) /* crouch */
		KEY(0, KEY_RBRACKET) /* next item */
		KEY(VOLUP, KEY_EQUALS)
		KEY(VOLDOWN, KEY_MINUS)
		KEY(PLUS, KEY_TAB) /* map */
#ifdef GAME_DUKE3D
		KEY(CAMERA, KEY_TILDE) /* kick */
#elif defined(GAME_BLOOD)
		KEY(CAMERA, KEY_X) /* alt fire */
#endif
	};
	static const uint8_t keys_power[] = {
		KEY(UP, KEY_CAPSLOCK)
		KEY(LEFT, KEY_SEMICOLON)
		KEY(RIGHT, KEY_QUOTE)
		KEY(DOWN, KEY_ESCAPE)
		KEY(CENTER, KEY_ENTER)

		KEY(LSOFT, KEY_EQUALS)
		KEY(RSOFT, KEY_MINUS)
		KEY(DIAL, KEY_TAB) /* map */
		KEY(0, KEY_F11) /* gamma */
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
		KEY(UP) = KEY_LCTRL;	/* fire */
		KEY(DOWN) = KEY_LCTRL;	/* fire */
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

const char *getkeyname(int num) {
	return KB_ScanCodeToString(num);
}

const char *getjoyname(int what, int num) { return NULL; }
int initmouse(void) {	return 0; }
void uninitmouse(void) {}
void grabmouse(int a) {}
void readmousexy(int *x, int *y) {}
void readmousebstatus(int *b) {}
void releaseallbuttons(void) {}

static uint32_t timerlast = 0, timertick = 0;
static uint32_t timerticspersec = 0, timerlastsample = 0;
//static void (*usertimercallback)(void) = NULL;

int inittimer(int tickspersecond, void (*callback)(void)) {
	if (timerticspersec) return 0;    // already installed
	buildputs("Initialising timer\n");
	if (tickspersecond != TICRATE) {
		buildputs("!!! unexpected ticrate\n");
		exit(1);
	}
	timerticspersec = tickspersecond;
	timerlast = sys_timer_ms();
	if (callback) {
		buildputs("!!! unimplemented: timer callback\n");
		exit(1);
	}
	//usertimercallback = callback;
	return 0;
}

void uninittimer(void) {
	timerticspersec = 0;
}

void sampletimer(void) {
	unsigned t1 = sys_timer_ms();
	unsigned t2, t3, t4 = timertick;

	t2 = t1 - timerlast;
	if (t2 >= 1000) {
		if (t2 < 2000) {
			t2 -= 1000;
			t4 += TICRATE;
		} else {
			// slow path
			t3 = t2 / 1000;
			t2 -= t3 * 1000;
			t4 += t3 * TICRATE;
		}
		timertick = t4;
		timerlast = t1 - t2;
	}
#if TICRATE == 120
	t4 += (t2 * 0xf5c3 >> 19);
#else
	t4 += (t2 * TICRATE) / 1000;
#endif
	totalclock += t4 - timerlastsample;
	timerlastsample = t4;
}

unsigned int getticks(void) {
	return (unsigned int)sys_timer_ms();
}

unsigned int getusecticks(void) {
	return (unsigned int)sys_timer_ms() * 1000;
}

static char modeschecked = 0;

void getvalidmodes(void) {
	int i, w, h;
	if (modeschecked) return;
	modeschecked = 1;

	w = sys_data.display.w2;
	h = sys_data.display.h2;
	if (sys_data.scaler == 1) w <<= 1, h <<= 1;

	i = 0;
#define ADDMODE(f) if (i < MAXVALIDMODES) { \
	validmode[i].xdim = w; \
	validmode[i].ydim = h; \
	validmode[i].bpp = 8; \
	validmode[i].fs = f; \
	i++; \
}
	ADDMODE(0)
	ADDMODE(1)
#undef ADDMODE
	validmodecnt = i;
}

static void shutdownvideo(void) {
	if (frame) {
		free(frame_ptr);
		frame = NULL;
	}
}

int setvideomode(int x, int y, int c, int fs) {
	static int started = 0;
	struct sys_display *disp = &sys_data.display;
	if (started) return 0;

	started = 1;
	sys_start();

	getvalidmodes();
	x = disp->w2; y = disp->h2;
	if (y <= 68) {
		x = 320;
		if (y == 68) y = 226;
		else y = 224;
	}
	if (sys_data.scaler == 1) x <<= 1, y <<= 1;
	buildprintf("Setting video mode %dx%d\n", x, y);

	//if (baselayer_videomodewillchange) baselayer_videomodewillchange();

	{
		int i, j, st = x;
		uint8_t *p;
		frame_ptr = p = malloc(st * y + 8);
		frame = p += -(intptr_t)p & 7;
		frameplace = (intptr_t)p;
		bytesperline = st;
		imageSize = st * y;
		numpages = 1;

		setvlinebpl(st);
		for (i = j = 0; i <= y; i++)
			ylookup[i] = j, j += st;
	}

	xres = x; yres = y; bpp = c; fullscreen = fs;

	modechange = 1; videomodereset = 0;
	//if (baselayer_videomodedidchange) baselayer_videomodedidchange();
	setpalettefade(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);
	return 0;
}

void resetvideomode(void) {}

void showframe(void) {
	sys_wait_refresh();
	app_scr_update(frame, framebuf);
	sys_start_refresh();
}

int setpalette(int start, int num, unsigned char *dapal) {
	(void)start; (void)num; (void)dapal;
	app_pal_update((uint8_t*)curpalettefaded, framebuf, NULL);
	return 0;
}

int handleevents(void) {
	int type, key, ret = 0;

	for (;;) {
		type = sys_event(&key);
		switch (type) {
		case EVENT_KEYUP:
		case EVENT_KEYDOWN:
			if (type == EVENT_KEYDOWN) {
				if (!keystatus[key]) {
					keystatus[key] = 1;
					keyfifo[keyfifoend] = key;
					keyfifo[(keyfifoend+1)&(KEYFIFOSIZ-1)] = 1;
					keyfifoend = ((keyfifoend+2)&(KEYFIFOSIZ-1));
				}
			} else {
				keystatus[key] = 0;
			}
	    break;

		case EVENT_END: goto end;
		case EVENT_QUIT:
			quitevent = 1;
			ret = -1;
			goto end;
		}
	}
end:
	sampletimer();
	return ret;
}

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

#define PAL_UPDATE(load, process) \
	for (i = 0; i < 256; i++, pal++) { \
		r = load; g = load; b = load; process; \
	}

DEF(void, pal_update8, (uint8_t *pal, void *dest, const uint8_t *gamma)) {
	int i, r, g, b;
	uint8_t *d = (uint8_t*)dest - 256;
	PAL_UPDATE(*pal++,
			*d++ = (r * 4899 + g * 9617 + b * 1868 + 0x2000) >> 14)
}

DEF(void, pal_update16, (uint8_t *pal, void *dest, const uint8_t *gamma)) {
	int i, r, g, b;
	uint16_t *d = (uint16_t*)dest - 256;
	PAL_UPDATE(*pal++,
			*d++ = (r >> 3) << 11 | (g >> 2) << 5 | b >> 3)
}

DEF(void, pal_update32, (uint8_t *pal, void *dest, const uint8_t *gamma)) {
	int i, r, g, b;
	uint32_t *d = (uint32_t*)dest - 256;
	PAL_UPDATE(*pal++,
			*d++ = r << 22 | b << 11 | g << 1)
}

#undef PAL_UPDATE

DEF(void, scr_update_1d1, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint16_t *c16 = (uint16_t*)dest - 256;
	struct sys_display *disp = &sys_data.display;
	int x, y, w = disp->w2, h = disp->h2;
	int sadd = bytesperline - w;
	for (y = 0; y < h; y++, s += sadd)
	for (x = 0; x < w; x++)
		*d++ = c16[*s++];
}

// 00rrrrrr rr000bbb bbbbb00g ggggggg0
// rrrrr000 000bbbbb 00000ggg ggg00000
// rounding half to even

DEF(void, scr_update_1d2, (uint8_t *s, void *dest)) {
	uint16_t *d = (uint16_t*)dest;
	uint32_t *c32 = (uint32_t*)dest - 256;
	struct sys_display *disp = &sys_data.display;
	int x, y, w = disp->w2, h = disp->h2;
	int st = bytesperline, sadd = st * 2 - w * 2;
	unsigned a, b = 0x00400802;
	for (y = 0; y < h; y++, s += sadd)
	for (x = 0; x < w; x++, s += 2) {
		a = c32[s[0]] + c32[s[1]];
		a += c32[s[st]] + c32[s[st + 1]];
		a += (b & a >> 2) + b;
		a &= 0xf81f07e0;
		*d++ = a | a >> 16;
	}
}

/* display aspect ratio 7:5, LCD pixel aspect ratio 7:10 */
DEF(void, scr_update_128x64, (uint8_t *s, void *dest)) {
	uint8_t *d = (uint8_t*)dest, *c8 = dest - 256;
	unsigned x, y, h = 64;
	uint32_t a, b, t;
// 00112 23344
#define X \
	t = c8[s2[2]]; t += t << 16; \
	t += (c8[s2[0]] + c8[s2[1]]) << 1; \
	t += (c8[s2[3]] + c8[s2[4]]) << 17; \
	s2 += 320; a += t << 1;
	do {
		for (x = 0; x < 320; x += 5, s += 5) {
			const uint8_t *s2 = s;
			for (a = 0, y = 0; y < 4; y++) { X }
			a -= t;
			b = a >> 16; a &= 0xffff;
			a = (a + 35 / 2) * 0xea0f >> 21; // div35
			b = (b + 35 / 2) * 0xea0f >> 21;
			*d++ = (a + 1) >> 1;
			*d++ = (b + 1) >> 1;
			for (a = t, y = 0; y < 3; y++) { X }
			b = a >> 16; a &= 0xffff;
			a = (a + 35 / 2) * 0xea0f >> 21; // div35
			b = (b + 35 / 2) * 0xea0f >> 21;
			d[126] = (a + 1) >> 1;
			d[127] = (b + 1) >> 1;
		}
		s += 320 * 6; d += 128;
	} while ((h -= 2));
#undef X
}

DEF(void, scr_update_96x68, (uint8_t *s, void *dest)) {
	uint8_t *d = (uint8_t*)dest;
	uint8_t *c8 = (uint8_t*)dest - 256;
	unsigned x, y = 0, h = 68; // (3+3+4)*22+3+3
	uint32_t a, b, c, t0, t1;
// 0001112223 3344455566 6777888999
#define X(op) \
	a op (c8[s2[0]] + c8[s2[1]] + c8[s2[2]]) * 3; \
	a += t0 = c8[s2[3]]; \
	b op t0 * 2 + (c8[s2[4]] + c8[s2[5]]) * 3; \
	b += (t1 = c8[s2[6]]) * 2; \
	c op t1 + (c8[s2[7]] + c8[s2[8]] + c8[s2[9]]) * 3;
	// (3+3+4)*22+3+3
	do {
		for (x = 0; x < 320; x += 10, s += 10) {
			uint8_t *s2 = s; uint32_t m;
			X(=) s2 += 320; X(+=) s2 += 320; X(+=)
			m = 0x8889 * 2; // div30
			if (y == 2) {
				s2 += 320; X(+=)
				m = 0xcccd; // div40
			}
			a = a * m + 0xfc000;
			b = b * m + 0xfc000;
			c = c * m + 0xfc000;
			a = (a + (a >> 7 & 0x4000)) >> 21;
			b = (b + (b >> 7 & 0x4000)) >> 21;
			c = (c + (c >> 7 & 0x4000)) >> 21;
			*d++ = (a + 1) >> 1;
			*d++ = (b + 1) >> 1;
			*d++ = (c + 1) >> 1;
		}
		s += 320 * 2;
		if (++y == 3) s += 320, y = 0;
	} while ((h -= 1));
#undef X
}

#undef DEF
#ifdef USE_ASM
extern int scr_update_data[2];
#define pal_update16 pal_update16_asm
#define pal_update32 pal_update32_asm
#define scr_update_1d1 scr_update_1d1_asm
#define scr_update_1d2 scr_update_1d2_asm
#endif

uint8_t *framebuffer_init(void) {
	static const uint8_t pal_size[] = { 2, 4, 1, 1 };
	static const struct {
		void (*pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
		void (*scr_update)(uint8_t *src, void *dest);
	} fn[] = {
		{ pal_update16, scr_update_1d1 },
		{ pal_update32, scr_update_1d2 },
		{ pal_update8, scr_update_128x64 },
		{ pal_update8, scr_update_96x68 },
	};
	struct sys_display *disp = &sys_data.display;
	int w = disp->w2, h = disp->h2;
	int mode = sys_data.scaler;
	size_t size = w * h, size2 = pal_size[mode] << 8;
	uint8_t *dest;

	dest = malloc(size * 2 + size2 + 31);
	dest += -(intptr_t)dest & 31;
	dest += size2;

#ifdef USE_ASM
	if (mode == 1) w <<= 1, h <<= 1;
	scr_update_data[0] = w;
	scr_update_data[1] = h;
#endif
	app_pal_update = fn[mode].pal_update;
	app_scr_update = fn[mode].scr_update;
	return dest;
}

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	unsigned w = disp->w1, h = disp->h1, mode;
	if (w == 128 && h == 64) mode = 2;
	else if (w == 96 && h == 68) mode = 3;
	else {
		if (h > w) h = w;
		mode = h <= 128;
	}
	sys_data.scaler = mode;
	disp->w2 = w;
	disp->h2 = h;
}

