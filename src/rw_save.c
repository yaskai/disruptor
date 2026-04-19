#include <stdint.h>
#include <stdio.h>
#include "../include/num_redefs.h"
#include "rw_save.h"

void rw_WriteEntData(EntityHandler *ent_handler, char *file_path) {
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
}

