#include "i_joystick.h"

int use_analog = 0;
int joystick_turn_sensitivity = 10;
int joystick_move_sensitivity = 10;
int joystick_look_sensitivity = 10;
 
void I_InitJoystick(void) {}
void I_UpdateJoystick(void) {}
void I_BindJoystickVariables(void) {}

#if NO_DEH
#include "deh_main.h"

void DEH_ParseCommandLine(void) {}
void DEH_AutoLoadPatches(const char *path) {}
#endif

#if NO_NET
#include "net_client.h"

boolean net_client_connected = false;
boolean drone = false;

void NET_Init(void) {}
void NET_CL_Run(void) {}
void NET_SV_Run(void) {}
void NET_BindVariables(void) {}
#endif

#if NO_SOUND
#include "i_sound.h"

int snd_pitchshift = -1;
int snd_musicdevice = SNDDEVICE_SB;

void I_SetOPLDriverVer(opl_driver_ver_t ver) {}
void I_InitSound(GameMission_t mission) {}
void I_ShutdownSound(void) {}
int I_GetSfxLumpNum(sfxinfo_t *sfxinfo) { return 0; }
void I_UpdateSound(void) {}
void I_UpdateSoundParams(int channel, int vol, int sep) {}
int I_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep, int pitch) { return 0; }
void I_StopSound(int channel) {}
boolean I_SoundIsPlaying(int channel) { return false; }
void I_PrecacheSounds(sfxinfo_t *sounds, int num_sounds) {}
void I_InitMusic(void) {}
void I_ShutdownMusic(void) {}
void I_SetMusicVolume(int volume) {}
void I_PauseSong(void) {}
void I_ResumeSong(void) {}
void *I_RegisterSong(void *data, int len) { return NULL; }
void I_UnRegisterSong(void *handle) {}
void I_PlaySong(void *handle, boolean looping) {}
void I_StopSong(void) {}
boolean I_MusicIsPlaying(void) { return false; }
void I_BindSoundVariables(void) {}
#endif

void I_Endoom(byte *endoom_data) {}

