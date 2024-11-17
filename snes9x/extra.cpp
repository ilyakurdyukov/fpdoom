#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "snes9x.h"
#include "memmap.h"
#include "apu.h"
#include "soundux.h"
#include "display.h"

extern "C" {
unsigned sys_timer_ms(void);
void sys_wait_ms(uint32_t delay);
void emu_refresh(const void *src, int h);

extern uint16_t *framebuf;
}

void S9xMessage(int type, int number, const char *message) {
	puts(message);
}

extern "C"
char* osd_GetPackDir() { return (char*)""; }

bool8 S9xReadMousePosition(int which1, int &x, int &y, uint32 &buttons) {
	return 0;
}

bool8 S9xReadSuperScopePosition(int &x, int &y, uint32 &buttons) {
	return 0;
}

bool JustifierOffscreen() { return 0; }
void JustifierButtons(uint32& justifiers) {}

bool8 S9xDoScreenshot(int width, int height) { return FALSE; }

#if EMBEDDED == 2 && !defined(APP_DATA_EXCEPT)
int CMemory::MAX_ROM_SIZE;
#else
int CMemory::MAX_ROM_SIZE = 6 << 20;
#endif

extern "C" uint8_t* init_rommap(const char *fn);

extern "C"
int emu_init(const char *rom_fn) {
#ifdef APP_DATA_EXCEPT
	{
		uint8_t *p = init_rommap(rom_fn);
		if (!p) {
			fprintf(stderr, "!!! init_rommap(rom_fn) failed\n");
			return 0;
		}
		Memory.ROM = p;
	}
#elif EMBEDDED == 2
	{
		FILE *f; unsigned size;
		if (!(f = fopen(rom_fn, "rb"))) {
			fprintf(stderr, "!!! fopen(rom_fn) failed\n");
			return 0;
		}
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		CMemory::MAX_ROM_SIZE = size & ~0x1fff;
		fclose(f);
	}
#endif
	memset(&Settings, 0, sizeof(Settings));

	Settings.JoystickEnabled = FALSE;
	Settings.SoundPlaybackRate = 4;
	Settings.Stereo = FALSE;
	Settings.SoundBufferSize = 0;
	Settings.CyclesPercentage = 100;
	Settings.DisableSoundEcho = FALSE;
	Settings.APUEnabled = Settings.NextAPUEnabled = FALSE;
	Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
	Settings.SkipFrames = AUTO_FRAMERATE;
	Settings.ShutdownMaster = TRUE;
	Settings.FrameTimePAL = 20000;
	Settings.FrameTimeNTSC = 16667;
	Settings.FrameTime = Settings.FrameTimeNTSC;
	Settings.DisableSampleCaching = TRUE;
	Settings.DisableMasterVolume = TRUE;
	Settings.Mouse = FALSE;
	Settings.SuperScope = FALSE;
	Settings.MultiPlayer5 = FALSE;
	Settings.ControllerOption = SNES_JOYPAD;
	Settings.SupportHiRes = FALSE;
	Settings.ThreadSound = FALSE;
	Settings.Mute = TRUE;
	Settings.AutoSaveDelay = 30; // seconds
	Settings.ApplyCheats = FALSE;
	Settings.TurboMode = FALSE;
	Settings.TurboSkipFrames = 40;
	Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;

#if USE_16BIT
	Settings.Transparency = TRUE;
	Settings.SixteenBit = TRUE;
#else
	Settings.Transparency = FALSE;
	Settings.SixteenBit = FALSE;
#endif

	if (!Memory.Init() || !S9xInitAPU()) return 0;

	if (0) S9xInitSound(Settings.SoundPlaybackRate,
			Settings.Stereo, Settings.SoundBufferSize);
	S9xSetSoundMute(TRUE);

#ifdef GFX_MULTI_FORMAT
	S9xSetRenderPixelFormat(RGB565);
#endif

	{
		unsigned w = 256, h = 239, size = w * h;
#if USE_16BIT
		uint8_t *p = (uint8_t*)malloc(size * 6 + w + 8 + 28);
		if (!p) return 0;
		p += -(intptr_t)p & 31;
		// interleave buffers to save on pointers
		GFX.Pitch = w * 4;
		GFX.Screen = p;
		GFX.SubScreen = p + w * 2; p += size * 4;
		GFX.ZBuffer = p;
		GFX.SubZBuffer = p + w; // p += size * 2;
#else
		uint8_t *p = (uint8_t*)malloc(size * 2 + 8 + 28);
		if (!p) return 0;
		p += -(intptr_t)p & 31;
		GFX.Pitch = w;
		GFX.Screen = p;
		GFX.SubScreen = p; p += size;
		GFX.ZBuffer = p;
		GFX.SubZBuffer = NULL; // p += size;
#endif
	}

	if (!S9xGraphicsInit()) return 0;
	{
		uint32 saved_flags = CPU.Flags;
		if (!Memory.LoadROM(rom_fn)) return 0;
		Memory.LoadSRAM(S9xGetFilename(".srm"));
		CPU.Flags = saved_flags;
	}
	return 1;
}

static unsigned timerlast, timertick, timertick_ms;

extern "C"
void emu_start() {
	timerlast = sys_timer_ms();
	timertick = 0; timertick_ms = 0;
}

extern "C"
void emu_loop(void) { S9xMainLoop(); }

void S9xAutoSaveSRAM() {
	Memory.SaveSRAM(S9xGetFilename(".srm"));
}

extern "C"
void emu_save(int type) {
	if (type == -1) {
		// Yoshi's Island somehow modifies SRAM without activating this flag
		if (1 /* CPU.SRAMModified */) {
			CPU.SRAMModified = FALSE;
			CPU.AutoSaveTimer = 0;
			S9xAutoSaveSRAM();
		}
	}
}

extern "C"
void emu_exit() {
	if (Settings.SPC7110) (*CleanUp7110)();

	S9xSetSoundMute(TRUE);
	emu_save(-1);
	Memory.Deinit();
	S9xDeinitAPU();
}

extern "C"
void emu_reset(void) {
	S9xReset();
}

const char *S9xGetFilename(const char *ex) {
	static char	buf[PATH_MAX + 1];
	const char *s1, *s2, *s = Memory.ROMFilename;
	int a;
slash:
	s1 = s; s2 = NULL;
	do {
		a = *s++;
		if (a == '/') goto slash;
		if (a == '.') s2 = s - 1;
	} while (a);
	if (!s2) s2 = s;
	snprintf(buf, sizeof(buf), "%.*s%s", (int)(s2 - s1), s1, ex);
	return buf;
}

extern "C"
const char *S9xGetFilenameInc(const char *ex) {
	return S9xGetFilename(ex);
}

extern "C" {
extern void (*app_pal_update)(uint16_t *s, uint16_t *d);
}

void S9xSetPalette() {
#if !USE_16BIT
	app_pal_update(PPU.CGDATA, framebuf);
#endif
}

bool8 S9xInitUpdate() { return TRUE; }

bool8 S9xDeinitUpdate(int Width, int Height, bool8 sixteen_bit) {
	//printf("S9xDeinitUpdate %d %d\n", Width, Height);
	emu_refresh(GFX.Screen, Height);
	return TRUE;
}

void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext) {
	const char *p = strrchr(path, SLASH_CHAR);
	*drive = 0;
	if (p) {
		strncpy(dir, path, p - path);
		dir += p - path;
		path = p + 1;
	}
	*dir = 0;
	strcpy(fname, path);
	p = strrchr(path, '.');
	*ext = 0;
	if (p) {
		fname[p - path] = 0;
		strcpy(ext, p + 1);
	}
}

void _makepath(char *path, const char *drive, const char *dir, const char *fname, const char *ext) {
	(void)drive;

	if (dir && *dir) {
		strcpy(path, dir);
		strcat(path, SLASH_STR);
	} else *path = 0;

	strcat(path, fname);

	if (ext && *ext) {
		strcat(path, ".");
		strcat(path, ext);
	}
}

#define WAIT_FRAME_END(c1, c2) \
	int t2; \
	t0 = sys_timer_ms(); \
	t2 = t1 - t0; \
	if (t2 < 0) { \
		if (t2 < -c2) t1 = t0 - c2; \
		timerlast = t1; \
		return t2 >= -c1; \
	} \
	timerlast = t1; /* time to next frame */ \
	if (t2 > c1) sys_wait_ms(t2); \
	return 1;

static int wait_frame50(void) {
	unsigned t0, t1 = timerlast + 20;
	WAIT_FRAME_END(10, 20)
}

static int wait_frame60(void) {
	unsigned t0 = timertick, t1;
	t1 = timerlast + 17 - (t0 >> 1);
	timertick = (t0 + 1 + (t0 >> 1)) & 3;
	WAIT_FRAME_END(8, 17)
}

void S9xSyncSpeed() {
#if PERF_LOG && EMBEDDED == 2 && LIBC_SDIO < 3
	static int cnt = 0;
	int render = 0;
	if (++cnt >= Memory.ROMFramesPerSecond) {
		static uint32_t t0;
		uint32_t t1 = sys_timer_ms(), t2 = t1 - t0;
		cnt = 0; t0 = t1; printf("%u\n", t2);
	}
#else
	int frames = Memory.ROMFramesPerSecond;
	int render = frames == 60 ? wait_frame60() : wait_frame50();
#endif
	int skipped = IPPU.SkippedFrames;
	if (!render && skipped < 1) skipped++;
	else render = 1, skipped = 0;
	IPPU.SkippedFrames = skipped;
	IPPU.RenderThisFrame = render;
}

bool8 S9xOpenSoundDevice(int mode, bool8 stereo, int buffer_size) { return 0; }
void S9xGenerateSound() {}
void *S9xProcessSound(void *) { return NULL; }

extern "C" unsigned emu_getpad0();

uint32 S9xReadJoypad(int which1) {
	return which1 ? 0 : emu_getpad0();
}

void S9xLoadSDD1Data() {}

extern "C"
void S9xMovieUpdate() {}

#if NO_FLOAT
unsigned sqrt_u32(unsigned a) {
	unsigned n = 0, b = 0, c;
	do n += 2; while (a >> n);
	n -= 2;
	do {
		b <<= 1; c = b + 1;
		if (c * c <= a >> n) b = c;
	} while ((int)(n -= 2) >= 0);
	return b;
}
#endif

void emu_warn_disabled(int i, const char *name) {
	static int warned = 0;
	if (warned >> i & 1) return;
	warned |= 1 << i;
	printf("!!! %s is disabled in this build\n", name);
}

#if NO_SA1
extern "C" {
void S9xSA1Init() {}
void S9xSA1MainLoop() {}
void S9xSA1ExecuteDuringSleep() {}
uint8 S9xGetSA1(uint32 Address) { return 0; }
void S9xSetSA1(uint8 byte, uint32 Address) { emu_warn_disabled(5, "SA1"); }
}
#endif

#if NO_C4
void S9xInitC4() {}
uint8 S9xGetC4(uint16 Address) { return 0; }
void S9xSetC4(uint8 byte, uint16 Address) { emu_warn_disabled(0, "C4"); }
#endif

#if NO_DSP1
uint8 DSP1GetByte(uint16 address) { return 0; }
void DSP1SetByte(uint8 byte, uint16 Address) { emu_warn_disabled(1, "DSP1"); }
#endif

#if NO_DSP4
uint8 DSP4GetByte(uint16 address) { return 0; }
void DSP4SetByte(uint8 byte, uint16 Address) { emu_warn_disabled(4, "DSP4"); }
#endif

