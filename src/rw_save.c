#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "../include/num_redefs.h"
#include "../include/log_message.h"
#include "rw_save.h"

u8 rw_WriteSave(EntityHandler *ent_handler, char *dir_path, rw_GlobalData global_data) {
	u8 result = 0, needed = 0;

	MakeDirectory(TextFormat("data/%s", dir_path));

	char file_prefix[64] = "data/";
	memcpy(file_prefix + strlen(file_prefix), dir_path, strlen(dir_path));
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
	}

	// End file read
	fclose(pF);
	return 1;
}

