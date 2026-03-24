#include "../include/num_redefs.h"
#include "raylib.h"
#include "raymath.h"
#include "audioplayer.h"

void AudioPlayerInit(AudioPlayer *ap, Camera3D *camera) {

}

void AudioPlayerClose(AudioPlayer *ap) {

}

void AudioPlayerLoadNeeded(AudioPlayer *ap, char *directory) {

}

void SetSoundPosition(AudioPlayer *ap, char *name, Vector3 sound_pos, float max_dist, short mode) {
	Vector3 dir = Vector3Subtract(sound_pos, ap->camera->position);
	float dist = Vector3Length(dir);

	float attenuation = 1.0f / (1.0f + (dist / max_dist));
	attenuation = Clamp(attenuation, 0.0f, 1.0f);

	dir = Vector3Normalize(dir);
	Vector3 forward = Vector3Normalize(Vector3Subtract(ap->camera->target, ap->camera->position));
	Vector3 right = Vector3CrossProduct(ap->camera->up, forward);

	float dot = Vector3DotProduct(forward, dir);
	if(dot < 0.0f) attenuation *= (1.0f + dot * 0.5f); 

	float pan = 0.5f + 0.5f * Vector3DotProduct(dir, right);

	if(mode == MODE_SOUND) {

	} else if(mode == MODE_TRACK) {

	}
}

