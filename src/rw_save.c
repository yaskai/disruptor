#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "raylib.h"
#include "../include/num_redefs.h"
#include "../include/log_message.h"
#include "rw_save.h"
#include "config.h"

char recent_write[64];
char *rw_GetRecentWritePath() { return recent_write; }

void rw_WriteSaveNew(EntityHandler *ent_handler, char *dir_path, rw_GlobalData global_data) {
	int iter = 0;
	while(DirectoryExists(TextFormat("data/%s_%d", dir_path, iter))) {
		iter++;
	}

	char path[255] = {'\0'};
	snprintf(path, sizeof(path), "data/%s_%d", dir_path, iter);

	rw_WriteSave(ent_handler, path, global_data);
}

u8 rw_LoadMostRecent(EntityHandler *ent_handler, rw_GlobalData *global_data) {
	// Open metadata file
	char *meta_path = "data/svd_meta";
	FILE *pF = fopen(meta_path, "rb"); 
	if(!pF) {
		MessageError("ERROR: Save data meta file does not exist ", meta_path);
		return 0;
	}

	// Calculate number of save entries
	int num_entries = GetFileLength(meta_path) / sizeof(rw_Meta);

	// Read contents
	rw_Meta meta[num_entries];
	fread(&meta, sizeof(rw_Meta) * num_entries, 1, pF);

	// Find latest save with matching map file
	int latest_id = 0;
	bool map_match = false;
	for(int i = num_entries-1; i >= 0; i--) {
		if(!streq(meta[i].map, global_data->map))
			continue;

		map_match = true;
		latest_id = i;	
		break;
	}

	fclose(pF);

	if(!map_match)
		return 0;

	// Read save data
	rw_ReadSave(ent_handler, meta[latest_id].name, global_data);

	return 1;
}

u8 rw_WriteSave(EntityHandler *ent_handler, char *dir_path, rw_GlobalData global_data) {
	u8 result = 0, needed = 0;

	MakeDirectory(dir_path);

	char file_prefix[64] = {'\0'};
	memcpy(file_prefix, dir_path, strlen(dir_path));
	file_prefix[strlen(file_prefix)] = '/';
	memcpy(file_prefix + strlen(file_prefix), "svd_", strlen("svd_"));

	++needed;
	char glob_path[64] = {'\0'}; 
	memcpy(glob_path, file_prefix, strlen(file_prefix));
	glob_path[strlen(glob_path)] = 'g';
	result += rw_WriteGlobalData(global_data, glob_path);

	++needed;
	char ent_path[64] = {'\0'};
	memcpy(ent_path, file_prefix, strlen(file_prefix));
	ent_path[strlen(ent_path)] = 'e';
	result += rw_WriteEntData(ent_handler, ent_path);

	++needed;
	char eff_path[64] = {'\0'};
	memcpy(eff_path, file_prefix, strlen(file_prefix));
	eff_path[strlen(eff_path)] = 'v';
	result += rw_WriteEffectData(ent_handler->effect_manager, eff_path);

	rw_Meta meta = (rw_Meta) {
		.name = {'\0'},
		.map = {'\0'},
		.time_stamp = 0,
		.flags = 0
	};
	time(&meta.time_stamp);
	
	char *sep = strrchr(dir_path, '/');
	*sep = '\0';
	dir_path = sep+1;

	memcpy(meta.map, global_data.map, sizeof(meta.map));
	memcpy(meta.name, dir_path, sizeof(meta.name));
	memcpy(recent_write, meta.name, sizeof(meta.name));

	char *met_path = "data/svd_meta";	
	FILE *pF = fopen(met_path, "ab"); 
	fwrite(&meta, sizeof(rw_Meta), 1, pF);
	fclose(pF);
	
	return (result == needed);
}

u8 rw_ReadSave(EntityHandler *ent_handler, char *dir_path, rw_GlobalData *global_data) {
	u8 result = 0, needed = 0;

	char file_prefix[64] = "data/";
	memcpy(file_prefix + strlen(file_prefix), dir_path, strlen(dir_path));
	file_prefix[strlen(file_prefix)] = '/';
	memcpy(file_prefix + strlen(file_prefix), "svd_", strlen("svd_"));

	++needed;
	char glob_path[64] = {'\0'}; 
	memcpy(glob_path, file_prefix, strlen(file_prefix));
	glob_path[strlen(glob_path)] = 'g';
	result += rw_ReadGlobalData(global_data, glob_path);

	++needed;
	char ent_path[64] = {'\0'};
	memcpy(ent_path, file_prefix, strlen(file_prefix));
	ent_path[strlen(ent_path)] = 'e';
	result += rw_ReadEntData(ent_handler, ent_path);

	++needed;
	char eff_path[64] = {'\0'};
	memcpy(eff_path, file_prefix, strlen(file_prefix));
	eff_path[strlen(eff_path)] = 'v';
	result += rw_ReadEffectData(ent_handler->effect_manager, eff_path);

	ent_handler->ai_tick = global_data->ai_tick;
	
	return (result == needed);
}

u8 rw_WriteGlobalData(rw_GlobalData global_data, char *file_path) {
	FILE *pF = fopen(file_path, "wb");
	fwrite(&global_data, sizeof(rw_GlobalData), 1, pF);
	fclose(pF);
	return 1;
}

u8 rw_ReadGlobalData(rw_GlobalData *global_data, char *file_path) {
	FILE *pF = fopen(file_path, "rb");
	if(!pF) {
		MessageError("ERROR: File does not exist ", file_path);
		return 0;
	}

	fread(global_data, sizeof(rw_GlobalData), 1, pF);

	fclose(pF);

	return 1;
}

// Dump entity data to file
u8 rw_WriteEntData(EntityHandler *ent_handler, char *file_path) {
	Message("rw_WriteEntData()", ANSI_BLUE);

	// Create new file for save data 
	FILE *pF = fopen(file_path, "wb");

	// Iterate entities in scene
	for(u16 i = 0; i < ent_handler->count; i++) {
		Entity *ent = &ent_handler->ents[i];
		// * NOTE:
		// Entity's attached graphics data is ignored and managed elsewhere...
		// Writing models and animations to save file is wasteful and probably unsafe
		// *
		// Write component data to file:
		// AI
		fwrite(&ent->comp_ai, sizeof(comp_Ai), 1, pF);		
		// Transform
		fwrite(&ent->comp_transform, sizeof(comp_Transform), 1, pF);
		// Health
		fwrite(&ent->comp_health, sizeof(comp_Health), 1, pF);
		// Weapon
		fwrite(&ent->comp_weapon, sizeof(comp_Weapon), 1, pF);
		// *
		// * Base data:
		// Spatial info:
		fwrite(&ent->bsp_model, sizeof(int), 1, pF);
		fwrite(&ent->cell_id, sizeof(i16), 1, pF);
		// Trigger/switch info:
		fwrite(&ent->trigger_id, sizeof(u16), 1, pF);
		fwrite(&ent->on_trigger, sizeof(u8), 1, pF);
		fwrite(&ent->trigger_condition, sizeof(u8), 1, pF);
		fwrite(&ent->trigger_state, sizeof(u8), 1, pF);
		// Flags & type:
		fwrite(&ent->type, sizeof(i8), 1, pF);
		fwrite(&ent->flags, sizeof(u8), 1, pF);
		// Anim state:
		fwrite(&ent->anim_state.curr_frame, sizeof(int), 1, pF);
		fwrite(&ent->anim_state.anim_id, sizeof(int), 1, pF);
		fwrite(&ent->anim_state.acc, sizeof(float), 1, pF);
		fwrite(&ent->anim_state.loop_count, sizeof(u8), 1, pF);
	}

	// End file write
	fclose(pF);

	return 1;
}

// Read entity save data and pass values
// * NOTE:
// Entities should be spawned as normal and have data overwritten, 
// maintains ID ordering and graphics data
u8 rw_ReadEntData(EntityHandler *ent_handler, char *file_path) {
	Message("rw_ReadEntData()", ANSI_BLUE);

	// Open save file
	// return early if missing
	FILE *pF = fopen(file_path, "rb");
	if(!pF) {
		MessageError("ERROR: File does not exist ", file_path);
		return 0;
	}

	// Iterate entities in scene
	for(u16 i = 0; i < ent_handler->count; i++) {
		Entity *ent = &ent_handler->ents[i];

		// Read component data
		fread(&ent->comp_ai, sizeof(comp_Ai), 1, pF);
		fread(&ent->comp_transform, sizeof(comp_Transform), 1, pF);
		fread(&ent->comp_health, sizeof(comp_Health), 1, pF);
		fread(&ent->comp_weapon, sizeof(comp_Weapon), 1, pF);
		
		// Read spatial data
		fread(&ent->bsp_model, sizeof(int), 1, pF);
		fread(&ent->cell_id, sizeof(i16), 1, pF);

		// Read trigger/switch info
		fread(&ent->trigger_id, sizeof(u16), 1, pF);
		fread(&ent->on_trigger, sizeof(u8), 1, pF);
		fread(&ent->trigger_condition, sizeof(u8), 1, pF);
		fread(&ent->trigger_state, sizeof(u8), 1, pF);

		// ID and flags
		fread(&ent->type, sizeof(i8), 1, pF);
		fread(&ent->flags, sizeof(u8), 1, pF);

		// Anim state:
		fread(&ent->anim_state.curr_frame, sizeof(int), 1, pF);
		fread(&ent->anim_state.anim_id, sizeof(int), 1, pF);
		fread(&ent->anim_state.acc, sizeof(float), 1, pF);
		fread(&ent->anim_state.loop_count, sizeof(u8), 1, pF);
	}

	// End file read
	fclose(pF);
	return 1;
}

u8 rw_WriteEffectData(vEffect_Manager *eff_manager, char *file_path) {
	Message("rw_WriteEffectData()", ANSI_BLUE);
	FILE *pF = fopen(file_path, "wb");

	for(u8 i = 0; i < V_EFFECT_MAX_TRAILS; i++)			fwrite(&eff_manager->trails[i], sizeof(vEffect_Trail), 1, pF);
	for(u8 i = 0; i < V_EFFECT_MAX_IMPACT_DECALS; i++)	fwrite(&eff_manager->impact_decals[i], sizeof(vEffect_ImpactDecal), 1, pF);
	for(u8 i = 0; i < V_EFFECT_MAX_TRACERS; i++)		fwrite(&eff_manager->tracers[i], sizeof(vEffectTracer), 1, pF);
	for(u8 i = 0; i < V_EFFECT_MAX_PARTICLES; i++)		fwrite(&eff_manager->particles[i], sizeof(vEffectParticle), 1, pF);

	fclose(pF);
	return 1;
}

u8 rw_ReadEffectData(vEffect_Manager *eff_manager, char *file_path) {
	Message("rw_ReadEffectData()", ANSI_BLUE);

	FILE *pF = fopen(file_path, "rb");
	if(!pF) {
		MessageError("ERROR: File does not exist ", file_path);
		return 0;
	}

	for(u8 i = 0; i < V_EFFECT_MAX_TRAILS; i++)			fread(&eff_manager->trails[i], sizeof(vEffect_Trail), 1, pF);
	for(u8 i = 0; i < V_EFFECT_MAX_IMPACT_DECALS; i++)	fread(&eff_manager->impact_decals[i], sizeof(vEffect_ImpactDecal), 1, pF);
	for(u8 i = 0; i < V_EFFECT_MAX_TRACERS; i++)		fread(&eff_manager->tracers[i], sizeof(vEffectTracer), 1, pF);
	for(u8 i = 0; i < V_EFFECT_MAX_PARTICLES; i++)		fread(&eff_manager->particles[i], sizeof(vEffectParticle), 1, pF);

	fclose(pF);
	return 1;
}

