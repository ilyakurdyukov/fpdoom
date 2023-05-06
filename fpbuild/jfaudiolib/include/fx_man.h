#ifndef __FX_MAN_H
#define __FX_MAN_H

enum FX_ERRORS { FX_Ok = 0 };

static inline void FX_SetVolume(int a) {}
static inline void FX_SetReverseStereo(int a) {}
static inline void FX_SetReverb(int a) {}
static inline void FX_SetReverbDelay(int a) {}
static inline int FX_VoiceAvailable(int a) { return 0; }
static inline int FX_EndLooping(int a) { return 0; }
static inline int FX_SetPan(int a, int b, int c, int d) { return 0; }
static inline int FX_SetFrequency(int a, int b ) { return 0; }
static inline int FX_PlayAuto3D(char* a, unsigned b, int c, int d, int e, int f, unsigned g) { return 0; }
static inline int FX_SoundActive(int handle) { return 0; }
static inline int FX_StopSound(int a) { return 0; }
static inline int FX_StopAllSounds(void) { return 0; }

#endif
