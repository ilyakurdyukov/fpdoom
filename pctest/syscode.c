#include <stdlib.h>
#include <string.h>
#include <unistd.h> // usleep

#include "SDL.h"
#if SDL_MAJOR_VERSION >= 2
#include "SDL_keycode.h"
#else
#include "SDL_keysym.h"
#endif

#include "syscode.h"

#if SDL_MAJOR_VERSION >= 2
static SDL_Window *sdl_window;
#endif
static SDL_Surface *sdl_surface;

struct sys_data sys_data;
static uint16_t *framebuf_base;
static int scale = 0;

void sys_framebuffer(void *base) {
	framebuf_base = (uint16_t*)base;
}

void sys_start(void) {
	int w, h;
	const char *name = "phone screen";

	w = sys_data.display.w2 * scale;
	h = sys_data.display.h2 * scale;

#if SDL_MAJOR_VERSION < 2
	sdl_surface = SDL_SetVideoMode(w, h, 32, SDL_SWSURFACE);
	if (!sdl_surface) {
		fprintf(stderr, "!!! SDL_SetVideoMode failed: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_WM_SetCaption(name, 0);
#else
	sdl_window = SDL_CreateWindow(name,
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			w, h, 0);
	if (!sdl_window) {
		fprintf(stderr, "!!! SDL_CreateWindow failed: %s\n", SDL_GetError());
		exit(1);
	}
	// sdl_surface = SDL_GetWindowSurface(sdl_window);
	sdl_surface = SDL_CreateRGBSurface(0, w * scale, h * scale, 32, 0, 0, 0, 0);
	if (!sdl_surface) {
		fprintf(stderr, "!!! SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
		exit(1);
	}
#endif
}

void sys_wait_refresh(void) {}

void sys_start_refresh(void) {
	unsigned w = sys_data.display.w2, h = sys_data.display.h2;
	unsigned a, k, x, y, pitch;
	uint8_t *dst; uint16_t *s;

	if (SDL_LockSurface(sdl_surface)) {
		fprintf(stderr, "!!! SDL_LockSurface failed: %s\n", SDL_GetError());
		return;
	}
	dst = (uint8_t*)sdl_surface->pixels;
	if (sys_data.mac & 0x100) {
		uint8_t *s = (uint8_t*)framebuf_base;
		pitch = sdl_surface->pitch;
		for (y = 0; y < h; y++, s += w, dst += pitch * scale) {
			uint32_t *d = (uint32_t*)dst;
#define CONV \
	a = s[x], a = a * 2 - (a >> 7), a = a << 16 | a << 8 | a | 0xff000000
			if (scale == 1) {
				for (x = 0; x < w; x++) {
					CONV, *d++ = a;
				}
			} else if (scale == 2) {
				for (x = 0; x < w; x++)
					CONV, *d++ = a, *d++ = a;
			} else if (scale == 3) {
				for (x = 0; x < w; x++)
					CONV, *d++ = a, *d++ = a, *d++ = a;
			} else if (scale > 3) {
				for (x = 0; x < w; x++) {
					CONV; k = scale; do *d++ = a; while (--k);
				}
			}
#undef CONV
			for (x = 1; x < scale; x++)
				memcpy(dst + pitch * x, dst, w * 4 * scale);
		}
	} else {
		uint16_t *s = framebuf_base;
		pitch = sdl_surface->pitch;
		for (y = 0; y < h; y++, s += w, dst += pitch * scale) {
			uint32_t *d = (uint32_t*)dst;
#define CONV \
	a = s[x], a = (a >> 11) << 19 | (a & 0x7e0) << 5 | (a & 0x1f) << 3 | 0xff000000
			if (scale == 1) {
				for (x = 0; x < w; x++) {
					CONV, *d++ = a;
				}
			} else if (scale == 2) {
				for (x = 0; x < w; x++)
					CONV, *d++ = a, *d++ = a;
			} else if (scale == 3) {
				for (x = 0; x < w; x++)
					CONV, *d++ = a, *d++ = a, *d++ = a;
			} else if (scale > 3) {
				for (x = 0; x < w; x++) {
					CONV; k = scale; do *d++ = a; while (--k);
				}
			}
#undef CONV
			for (x = 1; x < scale; x++)
				memcpy(dst + pitch * x, dst, w * 4 * scale);
		}
	}
	SDL_UnlockSurface(sdl_surface);
#if SDL_MAJOR_VERSION < 2
	SDL_Flip(sdl_surface);
#else
	{
		SDL_Surface *winsurface = SDL_GetWindowSurface(sdl_window);
		if (!winsurface) {
			fprintf(stderr, "!!! SDL_GetWindowSurface failed: %s\n", SDL_GetError());
			return;
		}
		SDL_BlitSurface(sdl_surface, NULL, winsurface, NULL);
		SDL_UpdateWindowSurface(sdl_window);
	}
#endif
}

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

int sys_getkeymap(uint8_t *dest) {
	int i;
	for (i = 0; i < 64; i++) dest[i] = i;
	return 1;
}

#if !NO_SYSEVENT
#define POWER 0x40
static const int pc_keys[] = {
#define X(fp, sdl) KEYPAD_##fp, SDLK_##sdl,
	X(LSOFT, RETURN)
	X(LSOFT, SPACE)
	X(RSOFT, ESCAPE)
	X(UP, UP)
	X(DOWN, DOWN)
	X(LEFT, LEFT)
	X(RIGHT, RIGHT)
	X(CENTER, LCTRL)
	X(DIAL, CAPSLOCK) /* run toggle */
	X(1, COMMA) // ','
	X(3, PERIOD) // '.'
	X(1, q) X(2, w) X(3, e)
	X(4, a) X(5, s) X(6, d)
	X(7, SEMICOLON) // ';'
	X(8, RIGHTBRACKET) // ']'
	X(9, QUOTE) // '\''
	X(0, SPACE) // ' '
	X(HASH, PAGEUP) /* fly up */
	X(STAR, INSERT) /* fly down */
	X(LSOFT + POWER, EQUALS)
	X(RSOFT + POWER, MINUS)
	X(DIAL + POWER, TAB) /* map */
	X(0 + POWER, F11) /* gamma */
#undef X
	0, 0 };
#undef POWER

int sys_event(int *rkey) {
	const int *keys = pc_keys;
	SDL_Event event;
	for (;;) {
		int ret, key, i, x;
		if (!SDL_PollEvent(&event)) return EVENT_END;
		switch (event.type) {
		case SDL_QUIT: return EVENT_QUIT;
		case SDL_KEYDOWN: ret = EVENT_KEYDOWN; goto mapkey;
		case SDL_KEYUP: ret = EVENT_KEYUP;
mapkey:
			key = event.key.keysym.sym;
			for (i = 0; (x = keys[i + 1]); i += 2) if (x == key) {
				x = keys[i];
				x = sys_data.keytrn[x >> 6][x & 63];
				if (!x) break;
				*rkey = x;
				return ret;
			}
		}
	}
}
#endif

unsigned sys_timer_ms(void) { return SDL_GetTicks(); }
void sys_wait_ms(uint32_t delay) { SDL_Delay(delay); }
void sys_wait_us(uint32_t delay) { usleep(delay); }
extern int screenheight;

#define ERR_EXIT(...) do { \
	fprintf(stderr, "!!! " __VA_ARGS__); \
	exit(1); \
} while (0)

int __real_main(int argc, char **argv);
int __wrap_main(int argc, char **argv) {
	unsigned i, w = 320, h = 240; int ret;
	static const uint16_t res[] = {
		240, 320,  128, 160, // common
		240, 240,  128, 128, // square
		// uncommon
		320, 480,  240, 400,  176, 220,
		// monochrome
		128, 64,  96, 68,  64, 48,
	};

	sys_data.rotate = 3;
	sys_data.mac = 0xd0;
	sys_data.scaler = 0;

	if (argc) argc--, argv++;
	while (argc) {
		if (argc >= 2 && !strcmp(argv[0], "--scaler")) {
			sys_data.scaler = atoi(argv[1]) + 1;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--scale")) {
			scale = atoi(argv[1]);
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--mac")) {
			unsigned a = strtol(argv[1], NULL, 0);
			sys_data.mac = a & ~0x100;
			argc -= 2; argv += 2;
		} else if (argc >= 3 && !strcmp(argv[0], "--res")) {
			w = atoi(argv[1]);
			h = atoi(argv[2]);
			argc -= 3; argv += 3;
		} else if (!strcmp(argv[0], "--")) {
			argc -= 1; argv += 1;
			break;
		} else if (argv[0][0] == '-') {
			ERR_EXIT("unknown option \"%s\"\n", argv[0]);
		} else break;
	}

	for (i = 0; res[i]; i += 2) {
		if (w == res[i] && h == res[i + 1]) break;
		if (res[i + 1] > 68 && w == res[i + 1] && h == res[i]) break;
	}
	if (!res[i])
		ERR_EXIT("unsupported resolution (%d, %d)\n", w, h);

	sys_data.display.w1 = w;
	sys_data.display.h1 = h;
	if (h <= 68) sys_data.mac |= 0x100;

	if (scale <= 0) {
		unsigned j = 0, k = 1;
		while ((j += w) < 640) k++;
		scale = k;
	}

	lcd_appinit();
	keytrn_init();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
		fprintf(stderr, "!!! SDL_Init failed: %s\n", SDL_GetError());
		exit(1);
	}

	ret = __real_main(argc, argv);
	if (sdl_surface) SDL_FreeSurface(sdl_surface);
#if SDL_MAJOR_VERSION >= 2
	if (sdl_window) SDL_DestroyWindow(sdl_window);
#endif
	return ret;
}

#if !defined(__SANITIZE_ADDRESS__) && defined(__has_feature)
#if __has_feature(address_sanitizer)
#define __SANITIZE_ADDRESS__ 1
#endif
#endif

#ifdef __SANITIZE_ADDRESS__
const char* __asan_default_options() { return "detect_leaks=0"; }
#endif

