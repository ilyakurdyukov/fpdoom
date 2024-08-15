#include "build.h"
#include "baselayer_priv.h"
#include "types.h"
#include "keyboard.h"
#define TICRATE 120
void setvlinebpl(int dabpl);

#include "syscode.h"

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
		if (sys_data.display.h2 == 128) cachesize += 105 << 10;
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
	X(0x28, APOSTROPHE)
	X(0x2c, Z)
	X(0x33, COMMA)
	X(0x34, DOT)
	X(0x39, SPACE)
	X(0x3a, CAPSLOCK)
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
		KEY(9, KEY_APOSTROPHE) /* next weapon */
		KEY(HASH, KEY_A) /* jump */
		KEY(STAR, KEY_Z) /* crouch */
		KEY(0, KEY_RBRACKET) /* next item */
		KEY(VOLUP, KEY_EQUALS)
		KEY(VOLDOWN, KEY_MINUS)
		KEY(PLUS, KEY_TAB) /* map */
	};
	static const uint8_t keys_power[] = {
		KEY(UP, KEY_CAPSLOCK)
		KEY(LEFT, KEY_SEMICOLON)
		KEY(RIGHT, KEY_APOSTROPHE)
		KEY(DOWN, KEY_ESCAPE)
		KEY(CENTER, KEY_ENTER)

		KEY(LSOFT, KEY_EQUALS)
		KEY(RSOFT, KEY_MINUS)
		KEY(DIAL, KEY_TAB) /* map */
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
	if (sys_data.scaler) w <<= 1, h <<= 1;

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
	if (sys_data.scaler) x <<= 1, y <<= 1;
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

void (*app_pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
void (*app_scr_update)(uint8_t *src, void *dest);

#define PAL_UPDATE(load, process) \
	for (i = 0; i < 256; i++, pal++) { \
		r = load; g = load; b = load; process; \
	}

void pal_update16_asm(uint8_t *pal, void *dest, const uint8_t *gamma);
void pal_update16_ref(uint8_t *pal, void *dest, const uint8_t *gamma) {
	int i, r, g, b;
	uint16_t *d = (uint16_t*)dest - 256;
	PAL_UPDATE(*pal++,
			*d++ = (r >> 3) << 11 | (g >> 2) << 5 | b >> 3)
}

void pal_update32_asm(uint8_t *pal, void *dest, const uint8_t *gamma);
void pal_update32_ref(uint8_t *pal, void *dest, const uint8_t *gamma) {
	int i, r, g, b;
	uint32_t *d = (uint32_t*)dest - 256;
	PAL_UPDATE(*pal++,
			*d++ = r << 22 | b << 11 | g << 1)
}

#undef PAL_UPDATE

void scr_update_1d1_asm(uint8_t *src, void *dest);
void scr_update_1d1_ref(uint8_t *src, void *dest) {
	uint8_t *s = src;
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

void scr_update_1d2_asm(uint8_t *src, void *dest);
void scr_update_1d2_ref(uint8_t *src, void *dest) {
	uint8_t *s = src;
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

#ifdef USE_ASM
#define SEL(name) name##_asm
extern int scr_update_data[2];
#else
#define SEL(name) name##_ref
#endif

uint8_t *framebuffer_init(void) {
	static const uint8_t pal_size[] = { 2, 4 };
	static const struct {
		void (*pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
		void (*scr_update)(uint8_t *src, void *dest);
	} fn[] = {
		{ SEL(pal_update16), SEL(scr_update_1d1) },
		{ SEL(pal_update32), SEL(scr_update_1d2) },
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
	scr_update_data[0] = w << mode;
	scr_update_data[1] = h << mode;
#endif
	app_pal_update = fn[mode].pal_update;
	app_scr_update = fn[mode].scr_update;
	return dest;
}

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w1, h = disp->h1;
	if (h > w) h = w;
	sys_data.scaler = h <= 128;
	disp->w2 = w;
	disp->h2 = h;
}

