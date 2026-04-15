#include "syscode.h"

#include "SDL.h"
#include "SDL_keyboard.h"

enum {
	KEY_END = 0,
	KEY_ROTL, KEY_ROTR,
	KEY_DOWN, KEY_LEFT, KEY_RIGHT,
	KEY_DROP,
	KEY_PAUSE, KEY_RESTART, KEY_QUIT,
	KEY_REWIND
};

int sys_event(int *rkey) {
	SDL_Event event;
	for (;;) {
		int ret, key;
		if (!SDL_PollEvent(&event)) return EVENT_END;
		switch (event.type) {
		case SDL_QUIT: return EVENT_QUIT;
		case SDL_KEYDOWN: ret = EVENT_KEYDOWN; goto mapkey;
		case SDL_KEYUP: ret = EVENT_KEYUP;
mapkey:
			key = event.key.keysym.sym;
			//if (key >= 0x100)
			switch (key) {
#define X(sdl, game) \
	case SDLK_##sdl: key = game; break;
			X(RETURN, KEY_DROP)
			X(ESCAPE, KEY_QUIT)
			X(BACKSPACE, KEY_REWIND)

			X(UP, KEY_ROTR)
			X(DOWN, KEY_DOWN)
			X(LEFT, KEY_LEFT)
			X(RIGHT, KEY_RIGHT)
			X(SPACE, KEY_ROTR)

			X(w, KEY_ROTR)
			X(s, KEY_DOWN)
			X(a, KEY_LEFT)
			X(d, KEY_RIGHT)

			X(p, KEY_PAUSE)
			X(r, KEY_RESTART)
#undef X
			default: goto next;
			}
			*rkey = key;
			return ret;
		}
next:;
	}
}

