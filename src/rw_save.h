#include <raylib.h>
#include <time.h>
#include "../include/num_redefs.h"
#include "ent.h"
#include "ai.h"
#include "map.h"

#ifndef RW_SAVE_H_
#define RW_SAVE_H_

typedef struct {
	char map[64];
	time_t curr_time;

} rw_GlobalData;

u8 rw_WriteSave(EntityHandler *ent_handler, char *dir_path, rw_GlobalData global_data);
u8 rw_ReadSave(EntityHandler *ent_handler, char *dir_path);

u8 rw_WriteGlobalData(rw_GlobalData global_data, char *file_path);
u8 rw_ReadGlobalData(rw_GlobalData global_data, char *file_path);

u8 rw_WriteEntData(EntityHandler *ent_handler, char *file_path);
u8 rw_ReadEntData(EntityHandler *ent_handler, char *file_path);

#endif
