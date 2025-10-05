#include "../pctest/syscode.h"
#include "doomkey.h"

#include "SDL.h"
#include "SDL_keyboard.h"

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
			if (key >= 0x100)
			switch (key) {
#define X(sdl, game) \
	case SDLK_##sdl: key = game; break;
			X(RETURN, KEY_ENTER)
			X(ESCAPE, KEY_ESCAPE)
			X(BACKSPACE, KEY_BACKSPACE)
			X(TAB, KEY_TAB)
			X(LALT, KEY_RALT)
			X(RALT, KEY_RALT)
			X(LCTRL, KEY_RCTRL)
			X(RCTRL, KEY_RCTRL)
#if 0
			X(LSHIFT, KEY_RSHIFT)
			X(RSHIFT, KEY_RSHIFT)
			X(CAPSLOCK, KEY_CAPSLOCK)
#else
			X(CAPSLOCK, KEY_RSHIFT)
#endif
			X(UP, KEY_UPARROW)
			X(DOWN, KEY_DOWNARROW)
			X(LEFT, KEY_LEFTARROW)
			X(RIGHT, KEY_RIGHTARROW)
			X(PAGEUP, KEY_PGUP)
			X(PAGEDOWN, KEY_PGDN)
			X(INSERT, KEY_INS)
			X(DELETE, KEY_DEL)
			X(HOME, KEY_HOME)
			X(END, KEY_END)
			X(F1, KEY_F1)
			X(F2, KEY_F2)
			X(F3, KEY_F3)
			X(F4, KEY_F4)
			X(F5, KEY_F5)
			X(F6, KEY_F6)
			X(F7, KEY_F7)
			X(F8, KEY_F8)
			X(F9, KEY_F9)
			X(F10, KEY_F10)
			X(F11, KEY_F11)
#undef X
			default: goto next;
			}
			*rkey = key;
			return ret;
		}
next:
	}
}

