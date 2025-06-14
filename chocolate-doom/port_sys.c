#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "d_loop.h"
#include "deh_str.h"
#include "doomtype.h"
#include "i_input.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "v_diskicon.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "syscode.h"

extern uint16_t *framebuf;

int screenheight = 200, patch_yadd = 0;

static boolean initialized = false;
int png_screenshots = 0;
int fullscreen = 0;
pixel_t *I_VideoBuffer = NULL;
static boolean noblit;
int usegamma = 0;
unsigned int joywait = 0;

boolean screensaver_mode = false;
boolean screenvisible = true;

extern void (*app_pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
extern void (*app_scr_update)(uint8_t *src, void *dest);

void* framebuffer_init(unsigned size1);



void I_SetGrabMouseCallback(grabmouse_callback_t func) {}
void I_DisplayFPSDots(boolean dots_on) {}

void I_StartFrame(void) {}

#define KEY_RSHIFT	(0x80+0x36)
extern char gamekeydown[256];

#ifdef GAME_DOOM
#include "doom/doomstat.h"
static void player_message(const char *s) {
	players[consoleplayer].message = s;
}
#elif defined(GAME_HERETIC)
#include "heretic/doomdef.h"
#include "heretic/p_local.h"
static void player_message(const char *s) {
	P_SetMessage(&players[consoleplayer], s, false);
}
#elif defined(GAME_HEXEN)
#include "hexen/h2def.h"
#include "hexen/p_local.h"
static void player_message(const char *s) {
	P_SetMessage(&players[consoleplayer], s, false);
}
#elif defined(GAME_STRIFE)
#include "strife/doomstat.h"
static void player_message(const char *s) {
	players[consoleplayer].message = s;
}
#else
#error
#endif

void I_GetEvent(void) {
	int type, key;
	event_t event;
	for (;;) {
		type = sys_event(&key);
		switch (type) {
		case EVENT_KEYDOWN:
			if (key == KEY_RSHIFT) {
				if (gamekeydown[KEY_RSHIFT]) {
					player_message("AUTO RUN OFF.");
					goto keyup;
				}
				player_message("AUTO RUN ON.");
			}
			event.type = ev_keydown;
			event.data1 = key;
			event.data2 = key;
			event.data3 = key;
			D_PostEvent(&event);
			break;
		case EVENT_KEYUP:
			if (key == KEY_RSHIFT) continue;
keyup:
			event.type = ev_keyup;
			event.data1 = key;
			event.data2 = 0;
			event.data3 = 0;
			D_PostEvent(&event);
			break;
		case EVENT_END: return;
#if EMBEDDED == 1
		case EVENT_QUIT:
			if (0) I_Quit();
			else {
				event_t event;
				event.type = ev_quit;
				D_PostEvent(&event);
			}
#endif
		}
	}
}

void I_StartTic(void) {
	if (!initialized) return;
	I_GetEvent();
}

void I_UpdateNoBlit(void) {}

void I_FinishUpdate(void) {
	if (!initialized || noblit) return;

	// Draw disk icon before blit, if necessary.
	V_DrawDiskIcon();

	sys_wait_refresh();
	app_scr_update(I_VideoBuffer, framebuf);
	sys_start_refresh();

	// Restore background and undo the disk indicator, if it was drawn.
	V_RestoreDiskBackground();
}

void I_ReadScreen(pixel_t* scr) {
	memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}

void I_SetPalette(byte *doompalette) {
	app_pal_update(doompalette, framebuf, gammatable[usegamma]);
}

void I_SetWindowTitle(const char *title) {}
void I_InitWindowTitle(void) {}

void I_RegisterWindowIcon(const unsigned int *icon, int width, int height) {}
void I_InitWindowIcon(void) {}

void I_GraphicsCheckCommandLine(void) {
	noblit = M_CheckParm("-noblit");
}

void I_CheckIsScreensaver(void) {}

void I_InitGraphics(void) {
	byte *doompal;
	int w, h;

	I_VideoBuffer = framebuffer_init(SCREENWIDTH * SCREENHEIGHT);

	{
		byte *doompal = W_CacheLumpName(DEH_String("PLAYPAL"), PU_CACHE);
		I_SetPalette(doompal);
	}

	V_RestoreBuffer();
	memset(I_VideoBuffer, 0, SCREENWIDTH * SCREENHEIGHT);

	sys_start();

	initialized = true;
}

void I_BindVideoVariables(void) {
	M_BindIntVariable("usegamma", &usegamma);
}

int vanilla_keyboard_mapping = true;

void I_StartTextInput(int x1, int y1, int x2, int y2) {}
void I_StopTextInput(void) {}
void I_BindInputVariables(void) {}

static unsigned basetime = 0;

int I_GetTime(void) {
#if 1
	static unsigned last1 = 0, last2 = 0;
	unsigned t1 = sys_timer_ms();
	unsigned t2, t3, t4 = last2;

	t2 = t1 - last1;
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
		last2 = t4;
		last1 = t1 - t2;
	}
#if TICRATE == 35
	return t4 + (t2 * 0x11eb9 >> 21);
#else
	return t4 + (t2 * TICRATE) / 1000;
#endif
#else
	unsigned ticks = sys_timer_ms();
	if (!basetime) basetime = ticks;
	ticks -= basetime;
	return (ticks * TICRATE) / 1000;    
#endif
}

int I_GetTimeMS(void) {
	unsigned ticks;
	ticks = sys_timer_ms();
	if (!basetime) basetime = ticks;
	return ticks - basetime;
}

void I_Sleep(int ms) { sys_wait_ms(ms); }
void I_WaitVBL(int count) { sys_wait_ms((count * 1000) / 70); }
void I_InitTimer(void) {}

boolean I_CheckAbortHR(void) { return false; }

#include "doomkeys.h"

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

void keytrn_init(void) {
	uint8_t keymap[64], rkeymap[64];
	static const uint8_t keys[] = {
#define KEY(name, val) KEYPAD_##name, val,
		KEY(UP, KEY_UPARROW)
		KEY(LEFT, KEY_LEFTARROW)
		KEY(RIGHT, KEY_RIGHTARROW)
		KEY(DOWN, KEY_DOWNARROW)
		KEY(2, KEY_UPARROW)
		KEY(4, KEY_LEFTARROW)
		KEY(6, KEY_RIGHTARROW)
		KEY(5, KEY_DOWNARROW)
		KEY(LSOFT, KEY_ENTER)
		KEY(RSOFT, KEY_ESCAPE)
		KEY(CENTER, KEY_RCTRL) /* fire */
		KEY(DIAL, KEY_RSHIFT) /* run toggle */
		KEY(1, ',') /* strafe left */
		KEY(3, '.') /* strafe right */
		KEY(7, ';') /* prev weapon */
		KEY(9, '\'') /* next weapon */
		KEY(0, ']') /* next item */
		KEY(8, ' ') /* use item */
		KEY(HASH, KEY_PGUP) /* fly up */
		KEY(STAR, KEY_INS) /* fly down */
		KEY(VOLUP, KEY_EQUALS)
		KEY(VOLDOWN, KEY_MINUS)
		KEY(PLUS, KEY_TAB) /* map */
	};
	static const uint8_t keys_power[] = {
		KEY(UP, KEY_RSHIFT)
		KEY(LEFT, ';')
		KEY(RIGHT, '\'')
		KEY(DOWN, KEY_ESCAPE)
		KEY(CENTER, KEY_ENTER)

		KEY(LSOFT, KEY_EQUALS)
		KEY(RSOFT, KEY_MINUS)
		KEY(DIAL, KEY_TAB) /* map */
		KEY(0, KEY_F11) /* gamma */
		/* for cheats */
		KEY(1, 'i')
		KEY(2, 'd')
		KEY(3, 'q')
		KEY(4, 'k')
		KEY(5, 'f')
		KEY(6, 'a')
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
		KEY(UP) = KEY_RCTRL;	/* fire */
		KEY(DOWN) = KEY_RCTRL;	/* fire */
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

