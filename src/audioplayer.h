#include "../include/num_redefs.h"
#include "hash.h"
#include "raylib.h"

#ifndef AUDIOPLAYER_H_
#define AUDIOPLAYER_H_

typedef struct {
	Camera3D *camera;

	HashMap sound_hashmap;
	HashMap track_hashmap;

	Sound *sounds;
	Music *tracks;

	int sound_count;
	int track_count;

} AudioPlayer;

void AudioPlayerInit(AudioPlayer *ap, Camera3D *camera);
void AudioPlayerClose(AudioPlayer *ap);

void AudioPlayerLoadNeeded(AudioPlayer *ap, char *directory);

#define MODE_SOUND	0
#define MODE_TRACK	1
void SetSoundPosition(AudioPlayer *ap, char *name, Vector3 pos, float max_dist, short mode);

#endif
