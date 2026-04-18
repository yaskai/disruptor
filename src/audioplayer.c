#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/num_redefs.h"
#include "raylib.h"
#include "raymath.h"
#include "audioplayer.h"
#include "../include/miniaudio.h" 
#include "../include/log_message.h"

ma_engine _ma_engine;
ma_sound *sounds;

void AP_Init(AudioPlayer *ap, Camera3D *camera) {
	ap->camera = camera;

	int engine_valid = ma_engine_init(NULL, &_ma_engine);
	if(engine_valid != MA_SUCCESS) {
		MessageError("INIT FAILURE", "ma_engine_init()");
	}

	ma_engine_listener_set_enabled(&_ma_engine, 0, MA_TRUE);
	ma_engine_listener_set_world_up(&_ma_engine, 0, 0, 0, 1);

	AP_LoadNeeded(ap, "resources/audio/sfx");
}

void AP_Close(AudioPlayer *ap) {
}

void AP_LoadNeeded(AudioPlayer *ap, char *directory) {
	FilePathList path_list = LoadDirectoryFiles(directory);
	
	ap->sound_count = path_list.count;
	sounds = malloc(sizeof(ma_sound) * ap->sound_count);

	ap->sound_hashmap = (HashMap) {0};

	for(int i = 0; i < ap->sound_count; i++) {
		ma_sound_init_from_file(&_ma_engine, path_list.paths[i], 0, NULL, NULL, &sounds[i]);
		printf("%s\n", path_list.paths[i]);

		char path[255] = {0};
		memcpy(path, path_list.paths[i], strlen(path_list.paths[i]));

		char *sep_f = strrchr(path, '/');
		char *sep_b = strrchr(path, '\\');
		char *sep = (sep_f > sep_b) ? sep_f : sep_b;

		*sep = '\0';

		char *format = sep + 1;
		char *dot = strrchr(format, '.');
		*dot = '\0';

		HashInsert(&ap->sound_hashmap, format, i);
	}

	ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "pistol")], 		MA_FALSE);
	ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "pistol2")], 		MA_FALSE);
	ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "pistol3")], 		MA_FALSE);
	ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "rev_reload")], 	MA_FALSE);

	for(int i = 0; i < ap->sound_count; i++) {
		ma_sound_set_min_distance(&sounds[i], 200.0f);
		ma_sound_set_max_distance(&sounds[i], 2500.0f);
		ma_sound_set_rolloff(&sounds[i], 0.85f);
		ma_sound_set_directional_attenuation_factor(&sounds[i], 0.5f);
	}
}

void AP_Update(AudioPlayer *ap, float dt) {
	ma_engine_listener_set_position(
			&_ma_engine,
			0,
			ap->camera->position.x,
			ap->camera->position.y,
			ap->camera->position.z
			);

	Vector3 fwd = Vector3Normalize(Vector3Subtract(ap->camera->target, ap->camera->position));
	ma_engine_listener_set_direction(&_ma_engine, 0, fwd.x, fwd.y, fwd.z);
}

void AP_SetSoundPosition(AudioPlayer *ap, char *name, Vector3 pos, short mode) {
	int id = HashFetch(&ap->sound_hashmap, name);
	ma_sound_set_position(&sounds[id], pos.x, pos.y, pos.z);
}

void AP_SetSoundDir(AudioPlayer *ap, char *name, Vector3 dir, short mode) {
	int id = HashFetch(&ap->sound_hashmap, name);
	ma_sound_set_direction(&sounds[id], dir.x, dir.y, dir.z);
}

void AP_RequestSound(AudioPlayer *ap, char *name) {
	int id = HashFetch(&ap->sound_hashmap, name);
	ma_sound_seek_to_second(&sounds[id], 0);
	ma_sound_start(&sounds[id]);
}

