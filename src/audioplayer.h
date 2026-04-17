#include "../include/num_redefs.h"
#include "hash.h"
#include "raylib.h"

#ifndef AUDIOPLAYER_H_
#define AUDIOPLAYER_H_

typedef struct {
	Camera3D *camera;

	HashMap sound_hashmap;
	HashMap track_hashmap;

	int sound_count;
	int track_count;

} AudioPlayer;

void AP_Init(AudioPlayer *ap, Camera3D *camera);
void AP_Close(AudioPlayer *ap);

void AP_Update(AudioPlayer *ap, float dt);

void AP_LoadNeeded(AudioPlayer *ap, char *dir);

#define MODE_SOUND	0
#define MODE_TRACK	1
void AP_SetSoundPosition(AudioPlayer *ap, char *name, Vector3 pos, float max_dist, short mode);

#endif
