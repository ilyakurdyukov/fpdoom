#ifndef EDUKE32_COMPAT_H
#define EDUKE32_COMPAT_H

#include "../jfbuild/include/compat.h"

// Based on github.com/BSzili/NBlood-Amiga

// save-game compatibility
#undef BMAX_PATH
#ifdef EMBEDDED
#define BMAX_PATH 64
#else
#define BMAX_PATH 260
#endif

#ifdef __cplusplus
//#include "baselayer.h"
#include "build.h"
#include "cache1d.h"
#include "pragmas.h"
#include "osd.h"
#include "vfs.h"
#include "types.h"
#include "keyboard.h"
#include "control.h"
#include "assert.h"
#include "fx_man.h"
#include "scriptfile.h"

#include <math.h>
#include <stdlib.h>

#if !USE_OPENGL
#undef USE_OPENGL
#endif

#ifdef __GNUC__
#define UNUSED(x) UNUSED_##x __attribute__((unused))
#else
#define UNUSED(x) x
#endif

#if __GNUC__ >= 7
#define fallthrough__ __attribute__((fallthrough))
#else
#define fallthrough__
#endif

#define UNREFERENCED_PARAMETER(...)
#define UNREFERENCED_CONST_PARAMETER(...)
#define EDUKE32_FUNCTION __FUNCTION__
#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))
#define ARRAY_SSIZE(arr) (ssize_t)ARRAY_SIZE(arr)
#define EDUKE32_STATIC_ASSERT(...)
#define EDUKE32_PREDICT_FALSE(...) __VA_ARGS__

#define initprintf buildprintf
#define ERRprintf buildprintf

typedef struct { int x, y; } vec2_t;
typedef struct { int x, y, z; } vec3_t;
typedef struct { int16_t x, y; } vec2_16_t;

typedef int32_t fix16_t;
#define fix16_one (1<<16)
static inline int fix16_to_int(fix16_t a) { return (a + 0x8000) >> 16; }
static inline fix16_t fix16_from_int(int a) { return a << 16; }
#define F16(x) fix16_from_int((x))

#define fix16_sadd(a,b) ((a)+(b))
#define fix16_ssub(a,b) ((a)-(b))
#define fix16_clamp(x, lo, hi) clamp((x), (lo), (hi))
#define fix16_min(a,b) min((fix16_t)(a), (fix16_t)(b))
#define fix16_max(a,b) max((fix16_t)(a), (fix16_t)(b))

#define DO_FREE_AND_NULL(var) do { Xfree(var); (var) = NULL; } while (0)
#define Xcalloc calloc
#define Xfree free
#define Xstrdup strdup
#define Xmalloc malloc

#define Bfstat fstat
#define Bexit exit
#define Bfflush fflush
#define Bassert assert
#define Bchdir chdir

#if USE_POLYMOST && USE_OPENGL
#define tileInvalidate invalidatetile
#else
#define tileInvalidate(x,y,z)
#endif



#define ClockTicks long
//#define timerInit inittimer
//#define timerSetCallback installusertimercallback
#define timerInit(tickspersecond) { int tps = (tickspersecond);
#define timerSetCallback(callback) inittimer(tps, (callback)); }
#define timerGetTicks getticks
#define timerGetPerformanceCounter getusecticks
#define timerGetPerformanceFrequency() (1000*1000)
static inline int32_t BGetTime(void) { return (int32_t)totalclock; }

#include "palette.h"

#define in3dmode() (1)
enum rendmode_t {
	REND_CLASSIC,
	REND_POLYMOST = 3,
	REND_POLYMER
};

#define videoGetRenderMode() REND_CLASSIC
#define videoSetRenderMode(mode)
#define videoNextPage nextpage
 extern char inpreparemirror;
#define renderDrawRoomsQ16(daposx, daposy, daposz, daang, dahoriz, dacursectnum) ({drawrooms((daposx), (daposy), (daposz), fix16_to_int((daang)), fix16_to_int((dahoriz)), (dacursectnum)); inpreparemirror;})
#define renderDrawMasks drawmasks
#define videoCaptureScreen(x,y) screencapture((char *)(x),(y))
#define videoCaptureScreenTGA(x,y) screencapture((char *)(x),(y))
#define engineFPSLimit() 1
#define enginePreInit preinitengine
#define PrintBuildInfo() // already part of preinitengine
#define windowsCheckAlreadyRunning() 1
#define enginePostInit() 0 // TODO
//#define engineLoadBoard loadboard
//#define engineLoadBoardV5V6 loadoldboard
#define artLoadFiles(a,b) loadpics(const_cast<char*>((a)), (b))
#define engineUnInit uninitengine
#define engineInit initengine
#define tileCreate allocatepermanenttile
#define tileLoad loadtile
#define tileDelete(x) // nothing to do here
#define calc_sector_reachability()

// common.h
void G_AddGroup(const char *buffer);
void G_AddPath(const char *buffer);
void G_AddDef(const char *buffer);
int32_t G_CheckCmdSwitch(int32_t argc, char const * const * argv, const char *str);


// palette

#define renderEnableFog() // TODO
#define renderDisableFog() // TODO
#define numshades numpalookups

extern char pow2char[8];
#define renderDrawLine drawline256
#define renderSetAspect setaspect
typedef walltype *uwallptr_t;
#define tspritetype spritetype
typedef spritetype *tspriteptr_t;
#define videoSetCorrectedAspect() // newaspect not needed
#define renderDrawMapView drawmapview
#define videoClearViewableArea clearview
#define videoSetGameMode(davidoption, daupscaledxdim, daupscaledydim, dabpp, daupscalefactor) setgamemode(davidoption, daupscaledxdim, daupscaledydim, dabpp)
#define videoSetViewableArea setview
//#define videoSetViewableArea(x1, y1, x2, y2) do { setview((x1),(y1),(x2),(y2)); setaspect(65536, yxaspect); } while(0) // TODO HACK!
#define videoClearScreen clearallviews
#define mouseLockToWindow(a) grabmouse((a)-2)
#define videoResetMode resetvideomode
#define whitecol 31
#define blackcol 0
#define kopen4loadfrommod kopen4load
#define g_visibility visibility

enum {
	CSTAT_SPRITE_BLOCK = 1,
	CSTAT_SPRITE_TRANSLUCENT = 2,
	CSTAT_SPRITE_YFLIP = 8,
	CSTAT_SPRITE_BLOCK_HITSCAN = 0x100,
	CSTAT_SPRITE_TRANSLUCENT_INVERT = 0x200,
	CSTAT_SPRITE_INVISIBLE = 0x8000,
	CSTAT_SPRITE_ALIGNMENT_FACING = 0,
	CSTAT_SPRITE_ALIGNMENT_FLOOR = 32,
	CSTAT_SPRITE_ONE_SIDED = 64,
	CSTAT_WALL_BLOCK = 1,
	CSTAT_WALL_MASKED = 16,
	CSTAT_WALL_BLOCK_HITSCAN = 64,
};

#define clipmove_old clipmove
#define getzrange_old getzrange
#define pushmove_old pushmove


// controls

#ifndef MAXMOUSEBUTTONS // TODO
#define MAXMOUSEBUTTONS 6
#endif
#define CONTROL_ProcessBinds()
#define CONTROL_ClearAllBinds()
#define CONTROL_FreeMouseBind(x)
extern int CONTROL_BindsEnabled; // TODO
#define KB_UnBoundKeyPressed KB_KeyPressed
extern kb_scancode KB_LastScan;

template <typename T, typename X, typename Y>
static inline T clamp(T in, X min, Y max) {
	return in <= (T)min ? (T)min : (in >= (T)max ? (T)max : in);
}
#undef min
#undef max
template <typename T> static inline T min(T a, T b) { return a < b ? a : b; }
template <typename T> static inline T max(T a, T b) { return a > b ? a : b; }
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define tabledivide32_noinline(n, d) ((n)/(d))
#define tabledivide32(a,b) ((a)/(b))

// defs.c
extern const char *G_DefaultDefFile(void);
extern const char *G_DefFile(void);
extern char *g_defNamePtr;
enum {
	T_EOF = -2,
	T_ERROR = -1,
};
typedef struct { char *text; int tokenid; } tokenlist;
static int getatoken(scriptfile *sf, const tokenlist *tl, int ntokens) {
	char *tok;
	int i;

	if (!sf) return T_ERROR;
	tok = scriptfile_gettoken(sf);
	if (!tok) return T_EOF;

	for (i = 0; i < ntokens; i++) {
		if (!Bstrcasecmp(tok, tl[i].text))
			return tl[i].tokenid;
	}

	return T_ERROR;
}
#define clearDefNamePtr() Xfree(g_defNamePtr)
#define G_AddDefModule(x) // TODO

#define CONSTEXPR constexpr

static inline char *dup_filename(const char *fn) {
	char *buf = (char*)Xmalloc(BMAX_PATH);
	return Bstrncpy(buf, fn, BMAX_PATH);
}

// ODS stuff

typedef const osdfuncparm_t *osdcmdptr_t;
#define system_getcvars() // TODO 
#define OSD_SetLogFile buildsetlogfile

// fnlist

struct strllist {
	struct strllist *next;
	char *str;
};

typedef struct {
	BUILDVFS_FIND_REC *finddirs, *findfiles;
	int32_t numdirs, numfiles;
} fnlist_t;

#define FNLIST_INITIALIZER { NULL, NULL, 0, 0 }

void fnlist_clearnames(fnlist_t *fnl);
int32_t fnlist_getnames(fnlist_t *fnl, const char *dirname, const char *pattern, int32_t dirflags, int32_t fileflags);

extern const char *s_buildRev;

extern int32_t clipmoveboxtracenum;

#define MUSIC_Update()
#define OSD_Cleanup() // done via atexit
#define OSD_GetCols() (60) // TODO

// keyboard.h
static inline void keyFlushScans(void) {
	keyfifoplc = keyfifoend = 0;
}
#define KB_FlushKeyboardQueueScans keyFlushScans

static inline char keyGetScan(void) {
	char c;
	if (keyfifoplc == keyfifoend) return 0;
	c = keyfifo[keyfifoplc];
	keyfifoplc = (keyfifoplc+2) & (KEYFIFOSIZ-1);
	return c;
}
#define keyFlushChars KB_FlushKeyboardQueue

extern int32_t win_priorityclass;
#define MAXUSERTILES (MAXTILES-16)  // reserve 16 tiles at the end
#define tileGetCRC32(tile) (0) // TODO

//#include "kplib.h"
static inline int32_t check_file_exist(const char *fn) {
	int fh;
	if ((fh = openfrompath(fn, BO_BINARY | BO_RDONLY, BS_IREAD))) {
		close(fh);
		return 1;
	}
	return 0;
}

static inline int32_t testkopen(const char *filename, char searchfirst) {
	buildvfs_kfd fd = kopen4load(filename, searchfirst);
	if (fd != buildvfs_kfd_invalid) {
		kclose(fd);
		return 1;
	}
	return 0;
}

//char * OSD_StripColors(char *outBuf, const char *inBuf)
#define OSD_StripColors strcpy
#define Bstrncpyz Bstrncpy
#define Bstrstr strstr
//#define Bstrtolower 
#define maybe_append_ext(wbuf, wbufsiz, fn, ext) \
	snprintf(wbuf, wbufsiz, "%s%s", fn, ext) // TODO

extern int32_t Numsprites; // only used by the editor

#define roundscale scale // TODO rounding?
#define rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, daalpha, dablend, cx1, cy1, cx2, cy2) \
	rotatesprite(sx, sy, z, a, picnum, dashade, dapalnum, (dastat) & 0xff, cx1, cy1, cx2, cy2)
#define rotatesprite_fs_alpha(sx, sy, z, a, picnum, dashade, dapalnum, dastat, alpha) \
	rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, alpha, 0, 0, 0, xdim-1, ydim-1)
#define rotatesprite_fs(sx, sy, z, a, picnum, dashade, dapalnum, dastat) \
	rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, 0, 0, xdim-1, ydim-1)
static inline void rotatesprite_eduke32(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
		int8_t dashade, char dapalnum, int32_t dastat,
		int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2) {
	rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, cx1, cy1, cx2, cy2);
}
#define rotatesprite rotatesprite_eduke32

#define RS_LERP 0
#define RS_AUTO 2

extern char const g_keyAsciiTable[128];
extern char const g_keyAsciiTableShift[128];

extern int hitscangoalx;
extern int hitscangoaly;

#define videoEndDrawing()
#define videoBeginDrawing()

#define yax_preparedrawrooms()
#define yax_drawrooms(a, b, c, d)
#define renderPrepareMirror renderPrepareMirror_new
static inline
void renderPrepareMirror(int32_t dax, int32_t day, int32_t daz, fix16_t daang,
		fix16_t dahoriz, int16_t dawall, int32_t *tposx, int32_t *tposy, fix16_t *tang) {
	short ang;
	preparemirror(dax, day, daz, fix16_to_int(daang), fix16_to_int(dahoriz), dawall, 0, tposx, tposy, &ang);
	*tang = fix16_from_int(ang);
}
#define renderCompleteMirror completemirror

extern unsigned char picsiz[MAXTILES];

#define EXTERN_INLINE static inline
#define MV_Lock()
#define MV_Unlock()

#define FX_PlayRaw(...) (-1)
#define FX_PlayLoopedRaw(...) (-1)

#define MOUSE_ClearAllButtons() MOUSE_ClearButton(0xFF)
#define JOYSTICK_GetButtons() 0
#define JOYSTICK_ClearAllButtons()

// TODO remove this
extern float curgamma;
#define g_videoGamma curgamma
#define DEFAULT_GAMMA 1.0f
//#define GAMMA_CALC ((int32_t)(min(max((float)((g_videoGamma - 1.0f) * 10.0f), 0.f), 15.f)))
#define GAMMA_CALC 0

extern int totalclocklock; // engine.c

// int const i = (int) totalclocklock >> (picanm[tilenum].sf & PICANM_ANIMSPEED_MASK);
// i = (totalclocklock>>((picanm[tilenum]>>24)&15));

enum {
	PICANM_ANIMTYPE_NONE = 0,
	PICANM_ANIMTYPE_OSC = 1 << 6,
	PICANM_ANIMTYPE_FWD = 2 << 6,
	PICANM_ANIMTYPE_BACK = 3 << 6,
	PICANM_ANIMTYPE_MASK = 3 << 6,
	PICANM_ANIMSPEED_MASK = 15,
};

#ifdef __cplusplus
extern "C" void crc32block(unsigned int *crcvar, unsigned char *blk, unsigned int len);

static inline unsigned Bcrc32(const void *data, unsigned length, unsigned crc) {
	unsigned temp = ~crc;
	crc32block(&temp, (unsigned char*)data, length);
	return ~temp;
}
#endif

// these are done in loadpalette
#define paletteInitClosestColorScale(r,g,b) 
#define paletteInitClosestColorGrid()
#define paletteInitClosestColorMap(pal)
#define palettePostLoadTables()
#define palettePostLoadLookups()
extern int paletteloaded; // TODO
enum {
	PALETTE_MAIN,
	PALETTE_SHADE,
	PALETTE_TRANSLUC
};
extern unsigned char *transluc;
// blendtable transluc
// transluc = kmalloc(65536L)
int getclosestcol(int r, int g, int b);
#define paletteGetClosestColor getclosestcol
#define RESERVEDPALS 4 // TODO

#ifndef MAXVOXMIPS
#define MAXVOXMIPS 5
#endif
extern intptr_t voxoff[MAXVOXELS][MAXVOXMIPS];
extern unsigned char voxlock[MAXVOXELS][MAXVOXMIPS];

#define maxspritesonscreen MAXSPRITESONSCREEN
//int animateoffs(short tilenum, short fakevar);
#define animateoffs_replace qanimateoffs


#define renderSetTarget setviewtotile
#define renderRestoreTarget setviewback

/*static inline int tilehasmodelorvoxel(int const tilenume, int pal)
{
    return
#ifdef USE_OPENGL
    (videoGetRenderMode() >= REND_POLYMOST && mdinited && usemodels && tile2model[Ptile2tile(tilenume, pal)].modelid != -1) ||
#endif
    (videoGetRenderMode() <= REND_POLYMOST && usevoxels && tiletovox[tilenume] != -1);
}*/
#define usevoxels (1)

#define CACHE1D_PERMANENT 255
#ifdef __cplusplus
extern int numtiles;
extern union picanm_t {
	struct {
		unsigned num : 6;
		unsigned type : 2;
		int xofs : 8;
		int yofs : 8;
		unsigned speed : 4;
		unsigned extra : 3;
		unsigned sign : 1;
	};
	int raw;
	struct {
		int raw;
		int operator&(int x) {
			if (x == 192) return raw & 192;
			if (x == 15) return raw >> 24 & 15;
			int error_picanm(void);
			return error_picanm();
		}
	} sf;
} picanm[MAXTILES];

static inline void tileSetSize(int32_t picnum, int16_t dasizx, int16_t dasizy) {
	int i, j;
	tilesizx[picnum] = dasizx;
	tilesizy[picnum] = dasizy;
	picanm[picnum].raw = 0;
	//tileUpdatePicSiz(picnum);

	for (i = 15; i > 1 && (1 << i) > dasizx; i--);
	for (j = 15; j > 1 && (1 << j) > dasizy; j--);
	picsiz[picnum] = i | j << 4;
}
#endif

extern int nextvoxid;

#define CLOCKTICKSPERSECOND 120

#define joyGetName getjoyname
#define JOYSTICK_SetDeadZone(axis, dead, satur) do { CONTROL_SetJoyAxisDead((axis), (dead)); CONTROL_SetJoyAxisSaturate((axis), (satur)); } while(0)
#define JOYSTICK_GetControllerButtons() (joyb)
#define CONTROLLER_BUTTON_A joybutton_A
#define CONTROLLER_BUTTON_B joybutton_B
#define CONTROLLER_BUTTON_START joybutton_Start
#define CONTROLLER_BUTTON_DPAD_UP joybutton_DpadUp
#define CONTROLLER_BUTTON_DPAD_DOWN joybutton_DpadDown
#define CONTROLLER_BUTTON_DPAD_LEFT joybutton_DpadLeft
#define CONTROLLER_BUTTON_DPAD_RIGHT joybutton_DpadRight

static inline
int setsprite(short spritenum, vec3_t *pos) {
	return setsprite(spritenum, pos->x, pos->y, pos->z);
}

static inline
void CONTROL_MapDigitalAxis(int32 whichaxis, int32 whichfunction, int32 direction) {
	CONTROL_MapDigitalAxis(whichaxis, whichfunction, direction, controldevice_joystick);
}

static inline
void CONTROL_MapAnalogAxis(int32 whichaxis, int32 whichanalog) {
	CONTROL_MapAnalogAxis(whichaxis, whichanalog, controldevice_joystick);
}

static inline
void CONTROL_SetAnalogAxisInvert(int32 whichaxis, int32 invert) {}

static inline
int OSD_Exec(const char *szScript) { return 0; }

static inline
vec2_16_t tileGetSize(int tile) {
	vec2_16_t size = { tilesizx[tile], tilesizy[tile] };
	return size;
}

typedef struct {
	union { struct { int x, y, z; }; vec3_t xyz; vec2_t xy; };
	short sprite, wall, sect;
} hitdata_t;

static inline
int hitscan(const vec3_t *sv, int sectnum, int vx, int vy, int vz,
		hitdata_t *hit, unsigned cliptype) {
	return hitscan(sv->x, sv->y, sv->z, sectnum, vx, vy, vz,
			&hit->sect, &hit->wall, &hit->sprite, &hit->x, &hit->y, &hit->z, cliptype);
}

#define KB_StringToScanCode(string) KB_StringToScanCode((char*)(string))

template <typename T, T &v>
struct compat_elem {
	compat_elem(const compat_elem&) = delete;
	compat_elem() {}
	operator int() { return v; }
	int operator=(int x) { return v = x; }
};

template <typename T, T *addr>
struct compat_array2 {
	T *p;
	compat_array2(size_t i) : p(addr + i) {}
	operator int() { return *p; }
	int operator=(int x) { return *p = x; }
};

// tilesiz[i].x -> tilesizx[i]
static struct {
	struct array {
		array(size_t i) : x(i), y(i) {}
		compat_array2<short, tilesizx> x;
		compat_array2<short, tilesizy> y;
	};
	array operator[](size_t i) const { return array(i); }
} tilesiz;

static struct {
	compat_elem<int, windowx1> x;
	compat_elem<int, windowy1> y;
} windowxy1;

static struct {
	compat_elem<int, windowx2> x;
	compat_elem<int, windowy2> y;
} windowxy2;

static struct {
	compat_elem<int, hitscangoalx> x;
	compat_elem<int, hitscangoaly> y;
} hitscangoal;

static struct {
	int operator=(int x) { return x; }
} g_loadedMapVersion;

#define ASS_AutoDetect 0

#if NO_NET
#include "mmulti.h"
#define numplayers numplayers1
static struct {
	int operator=(int x) { return x; }
	int operator-(int x) { return 1 - x; }
	bool operator==(int x) { return 1 == x; }
	bool operator!=(int x) { return 1 != x; }
	bool operator>(int x) { return 1 > x; }
	bool operator<(int x) { return 1 < x; }
} numplayers;
#endif

#endif // __cplusplus
#endif // EDUKE32_COMPAT_H
