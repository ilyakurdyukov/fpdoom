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

