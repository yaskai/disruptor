#include <stdlib.h>
#include <string.h>
#include "../include/num_redefs.h"
#include "raylib.h"
#include "raymath.h"
#include "audioplayer.h"
#include "../include/miniaudio.h" 
#include "../include/log_message.h"
#include "../include/ma_reverb_node.h"

#define DEVICE_FORMAT ma_format_f32

static dsp_preset_vals presets[] = {
	[DSP_DEFAULT] = {
		.room_size = 0.8f,
		.damping = 0.9f,
		.width = 1.0f,
		.wet = 0.02f,
		.dry = 1.0f
	},	

	[DSP_SMALL_ROOM] = {
		.room_size = 0.55,
		.damping = 0.4f,
		.width = 0.8f,
		.wet = 0.65f,
		.dry = 1.0f
	},

	[DSP_OPEN] = {
		.room_size = 0.9f,
		.damping = 0.95f,
		.width = 1.0f,
		.wet = 0.04f,
		.dry = 0.9f
	}
};

ma_node_graph _ma_graph;

ma_audio_buffer_ref supply_data;
ma_data_source_node supply_node; 

ma_engine _ma_engine;
ma_sound *sounds;

ma_reverb_node rev_node;
ma_reverb_node_config rev_conf;

char *bullet_near_sounds[5] = {
	"bullet_near_00",
	"bullet_near_01",
	"bullet_near_02",
	"bullet_near_04",
	"bullet_near_05",
};

void AP_ReqNearBulletSound(AudioPlayer *ap, Vector3 pos, Vector3 dir) {
	int sfx_id = GetRandomValue(0, 4); 

	if(ma_sound_is_playing(&sounds[HashFetch(&ap->sound_hashmap, bullet_near_sounds[sfx_id])]))
		return;

	AP_SetSoundPosition(ap, bullet_near_sounds[sfx_id], pos, 0);
	AP_SetSoundDir(ap, bullet_near_sounds[sfx_id], (dir), 0);
	ma_sound_set_velocity(&sounds[HashFetch(&ap->sound_hashmap, bullet_near_sounds[sfx_id])], dir.x*0.1f, dir.y*0.1f, dir.z*0.1f);
	AP_ReqSoundRandPitch(ap, bullet_near_sounds[sfx_id], 90, 100);
}

void AP_Init(AudioPlayer *ap, Camera3D *camera) {
	ap->camera = camera;

	int engine_valid = ma_engine_init(NULL, &_ma_engine);
	if(engine_valid != MA_SUCCESS) {
		MessageError("INIT FAILURE", "ma_engine_init()");
	}

	ma_engine_listener_set_enabled(&_ma_engine, 0, MA_TRUE);
	ma_engine_listener_set_world_up(&_ma_engine, 0, 0, 0, 1);

	rev_conf = ma_reverb_node_config_init(2, ma_engine_get_sample_rate(&_ma_engine));
	ma_reverb_node_init(ma_engine_get_node_graph(&_ma_engine), &rev_conf, NULL, &rev_node);
	ma_node_attach_output_bus(&rev_node, 0, ma_engine_get_endpoint(&_ma_engine), 0);

	AP_SetDsp(ap, DSP_DEFAULT);

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
	ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "disrupt")], 		MA_FALSE);
	ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "recall")], 		MA_FALSE);
	//ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "recall1")], 		MA_FALSE);
	ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "plr_step1")], 	MA_FALSE);
	ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "plr_step2")],	MA_FALSE);
	ma_sound_set_spatialization_enabled(&sounds[HashFetch(&ap->sound_hashmap, "plr_step3")],	MA_FALSE);

	for(int i = 0; i < ap->sound_count; i++) {
		ma_sound_set_min_distance(&sounds[i], 96.0f);
		ma_sound_set_max_distance(&sounds[i], 2500.0f);
		ma_sound_set_rolloff(&sounds[i], 1.0f);
		ma_sound_set_directional_attenuation_factor(&sounds[i], 0.5f);

		//ma_node_detach_output_bus(&sounds[i], 0);
		//ma_node_attach_output_bus(&sounds[i], 0, &rev_node, 0);

		ma_node_attach_output_bus(&sounds[i], 0, &rev_node, 0);
		//ma_node_attach_output_bus(&sounds[i], 0, ma_engine_get_endpoint(&_ma_engine), 0);
	}

	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "recall3")], 16.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "throw_bug")], 16.0f);

	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step1")], 32.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step2")], 32.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step3")], 32.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step4")], 32.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step5")], 32.0f);

	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step1")], 256.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step2")], 256.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step3")], 256.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step4")], 256.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "plr_step5")], 256.0f);

	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_01")], 60.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_02")], 60.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_03")], 60.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_04")], 60.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_05")], 60.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_06")], 60.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_07")], 60.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_08")], 60.0f);

	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_01")], 250.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_02")], 250.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_03")], 250.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_04")], 250.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_05")], 250.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_06")], 250.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_07")], 250.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal_steps_08")], 250.0f);

	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal3")], 100.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal4")], 100.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal5")], 100.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal7")], 100.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal3")], 2000.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal4")], 2000.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal5")], 2000.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "metal7")], 2000.0f);
	ma_sound_set_volume(&sounds[HashFetch(&ap->sound_hashmap, "metal3")], 1.2f);
	ma_sound_set_rolloff(&sounds[HashFetch(&ap->sound_hashmap, "metal3")], 0.5f);
	ma_sound_set_volume(&sounds[HashFetch(&ap->sound_hashmap, "metal4")], 1.2f);
	ma_sound_set_rolloff(&sounds[HashFetch(&ap->sound_hashmap, "metal4")], 0.5f);
	ma_sound_set_volume(&sounds[HashFetch(&ap->sound_hashmap, "metal5")], 1.2f);
	ma_sound_set_rolloff(&sounds[HashFetch(&ap->sound_hashmap, "metal5")], 0.5f);
	ma_sound_set_volume(&sounds[HashFetch(&ap->sound_hashmap, "metal7")], 1.2f);
	ma_sound_set_rolloff(&sounds[HashFetch(&ap->sound_hashmap, "metal7")], 0.5f);

	ma_sound_set_pitch(&sounds[HashFetch(&ap->sound_hashmap, "ground_hit")], 2.0f);

	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "recall")], 16.0f);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "throw_bug")], 16.0f);

	ma_sound_set_looping(&sounds[HashFetch(&ap->sound_hashmap, "ff_loop")], MA_TRUE);
	ma_sound_set_min_distance(&sounds[HashFetch(&ap->sound_hashmap, "ff_loop")], 96.0f);
	ma_sound_set_max_distance(&sounds[HashFetch(&ap->sound_hashmap, "ff_loop")], 300.0f);
	ma_sound_set_volume(&sounds[HashFetch(&ap->sound_hashmap, "ff_loop")], 0.1f);
	ma_sound_set_rolloff(&sounds[HashFetch(&ap->sound_hashmap, "ff_loop")], 3.5f);
	ma_sound_start(&sounds[HashFetch(&ap->sound_hashmap, "ff_loop")]);

	ma_sound_set_volume(&sounds[HashFetch(&ap->sound_hashmap, "click")], 2.0f);

	for(int i = 0; i < 5; i++) {
		ma_sound *sound = &sounds[HashFetch(&ap->sound_hashmap, bullet_near_sounds[i])];

		ma_sound_set_min_distance(sound, 16.0f);
		ma_sound_set_max_distance(sound, 64.0f);
		ma_sound_set_rolloff(sound, 1.0f);
		ma_sound_set_directional_attenuation_factor(sound, 1.5f);
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

void AP_SetSoundPitch(AudioPlayer *ap, char *name, float pitch) {
	int id = HashFetch(&ap->sound_hashmap, name);
	ma_sound_set_pitch(&sounds[id], pitch);
}

void AP_RequestSound(AudioPlayer *ap, char *name) {
	int id = HashFetch(&ap->sound_hashmap, name);
	ma_sound_seek_to_second(&sounds[id], 0);
	ma_sound_start(&sounds[id]);
}

void AP_ReqSoundRandPitch(AudioPlayer *ap, char *name, float min, float max) {
	ma_sound *sound = &sounds[HashFetch(&ap->sound_hashmap, name)];	

	if(ma_sound_is_playing(sound))
		return;

	float pitch = GetRandomValue(min, max) * 0.01f;
	ma_sound_set_pitch(sound, pitch);

	AP_RequestSound(ap, name);
}

void AP_SetDsp(AudioPlayer *ap, u8 preset_id) {
	verblib *verb = &rev_node.reverb;
	dsp_preset_vals *preset = &presets[preset_id];

	verblib_set_room_size(verb, preset->room_size);
	verblib_set_damping(verb, preset->damping);
	verblib_set_width(verb, preset->width);
	verblib_set_wet(verb, preset->wet);
	verblib_set_dry(verb, preset->dry);
}

void AP_BlendDsp(AudioPlayer *ap, float dt, float speed, u8 preset_id) {
	verblib *verb = &rev_node.reverb;
	dsp_preset_vals *preset = &presets[preset_id];

	verblib_set_room_size(verb, Lerp(verblib_get_room_size(verb), preset->room_size, dt * speed));
	verblib_set_damping(verb, Lerp(verblib_get_damping(verb), preset->damping, dt * speed));
	verblib_set_width(verb, Lerp(verblib_get_width(verb), preset->width, dt * speed));
	verblib_set_wet(verb, Lerp(verblib_get_wet(verb), preset->wet, dt * speed));
	verblib_set_dry(verb, Lerp(verblib_get_dry(verb), preset->dry, dt * speed));
}

void AP_SetGlobalVolume(AudioPlayer *ap, int volume) {
	float val = Clamp(volume * 0.01f, 0.0f, 1.0f);
	ma_engine_set_volume(&_ma_engine, val);
}

void AP_Pause(AudioPlayer *ap) {
}

void AP_Resume(AudioPlayer *ap) {
}


