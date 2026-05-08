#include "../include/num_redefs.h"
#include "hash.h"
#include "raylib.h"

#ifndef AUDIOPLAYER_H_
#define AUDIOPLAYER_H_

#define MAX_ACTIVE_SFX	16

typedef struct { 
	float room_size;
	float damping;	
	float width;
	float wet;
	float dry;

} dsp_preset_vals;

enum DSP_PRESETS : u8 {
	DSP_DEFAULT			= 0,
	DSP_SMALL_ROOM		= 1,
	DSP_OPEN			= 2,
}; 

typedef struct {
	Camera3D *camera;

	HashMap sound_hashmap;
	HashMap track_hashmap;

	int sound_stack[MAX_ACTIVE_SFX];

	int sound_count;
	int track_count;

	u8 *ref_presets;
	int num_ref_presets;

} AudioPlayer;

void AP_Init(AudioPlayer *ap, Camera3D *camera);
void AP_Close(AudioPlayer *ap);

void AP_Update(AudioPlayer *ap, float dt);

void AP_LoadNeeded(AudioPlayer *ap, char *dir);

#define MODE_SOUND	0
#define MODE_TRACK	1

void AP_SetSoundPosition(AudioPlayer *ap, char *name, Vector3 pos, short mode);
void AP_SetSoundDir(AudioPlayer *ap, char *name, Vector3 dir, short mode);
void AP_SetSoundPitch(AudioPlayer *ap, char *name, float pitch);

void AP_RequestSound(AudioPlayer *ap, char *name);

void AP_ReqNearBulletSound(AudioPlayer *ap, Vector3 pos, Vector3 dir);
void AP_ReqSoundRandPitch(AudioPlayer *ap, char *name, float min, float max);

void AP_ReqMaintainerWalkSound(AudioPlayer *ap, Vector3 pos);

void AP_SetDsp(AudioPlayer *ap, u8 preset_id);
void AP_BlendDsp(AudioPlayer *ap, float dt, float speed, u8 preset_id);

void AP_SetGlobalVolume(AudioPlayer *ap, int volume);

void AP_Pause(AudioPlayer *ap);
void AP_Resume(AudioPlayer *ap);

void AP_StopAll(AudioPlayer *ap);

#endif
