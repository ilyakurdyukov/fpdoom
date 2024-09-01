//#include <SDL.h>
#ifndef SDL_MAJOR_VERSION
typedef struct { uint8_t r, g, b, a; } SDL_Color;
typedef uint8_t SDL_Surface;
#define SDL_ALPHA_OPAQUE 255
#define SDL_GetTicks sys_timer_ms
#define SDL_Delay sys_wait_ms
typedef struct { int dummy; } SDL_Joystick;
#endif

int CalcAngle(int32_t y, int32_t x);
int ProjectionAngle(int32_t y, int32_t x);

uint8_t *wlsys_init(void);
void wlsys_end(void);
void wlsys_setpal(SDL_Color *pal);
void wlsys_refresh(const uint8_t *src, int type);

FILE *fopen_fix(const char *path, const char *mode);

#if !defined(WLSYS_MAIN) && LINUX_FN_FIX
#define fopen fopen_fix
#endif

extern int force_refresh_type;
extern int last_refresh_type;

extern int RunInvert;
extern int CalcRotateMult;

extern uint16_t *framebuf;

enum {
	sc_None = 0,
	sc_LControl,
	sc_LShift,
	sc_LAlt,
	sc_UpArrow,
	sc_DownArrow,
	sc_LeftArrow,
	sc_RightArrow,

	sc_BackSpace = 8,
	sc_Tab = 9,
	sc_CapsLock,
	sc_Return = 13,
	sc_Escape = 27,
	sc_Space = ' ',

	sc_1 = '1', sc_2, sc_3, sc_4,

	sc_G = 'g',
	sc_M = 'm',
	sc_N = 'n',
	sc_O = 'o',
	sc_P = 'p',
	sc_Y = 'y',

	sc_Delete = 127,
	sc_Last
};

#ifndef WLSYS_MAIN
enum {
	EVENT_KEYDOWN, EVENT_KEYUP, EVENT_END, EVENT_QUIT
};
int sys_event(int *rkey);
void sys_wait_refresh(void);
void sys_start_refresh(void);
unsigned sys_timer_ms(void);
void sys_wait_ms(uint32_t delay);
#else
extern int16_t screenWidth, screenHeight;

#include "syscode.h"
#endif

