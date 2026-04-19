#include "ent.h"
#include "ai.h"
#include "map.h"

#ifndef RW_SAVE_H_
#define RW_SAVE_H_

typedef struct {
	char map[64];

} rw_GlobalData;

void rw_WriteSave();
void rw_WriteEntData(EntityHandler *ent_handler, char *file_path);

#endif
