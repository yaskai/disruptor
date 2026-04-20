#include <time.h>
#include "raylib.h"
#include "../include/num_redefs.h"
#include "ent.h"
#include "ai.h"
#include "map.h"

#ifndef RW_SAVE_H_
#define RW_SAVE_H_

#define AUTOSAVE_TICKRATE (300.0f*dt)
#define IS_AUTOSAVE	0x01
typedef struct {
	time_t time_stamp;
	char map[64];
	char name[64];
	u8 flags;

} rw_Meta;

typedef struct {
	comp_Weapon weapons[4];

	Vector2 sway;

	float recoil;
	float gun_angle;

	int curr_id;

} rw_PlayerWeaponData;

typedef struct {
	char map[64];
	time_t curr_time;

	rw_PlayerWeaponData player_weap_data;

	float ai_tick;

} rw_GlobalData;

void rw_WriteSaveNew(EntityHandler *ent_handler, char *dir_path, rw_GlobalData global_data);
u8 rw_LoadMostRecent(EntityHandler *ent_handler, rw_GlobalData *global_data);

u8 rw_WriteSave(EntityHandler *ent_handler, char *dir_path, rw_GlobalData global_data);
u8 rw_ReadSave(EntityHandler *ent_handler, char *dir_path, rw_GlobalData *global_data);

u8 rw_WriteGlobalData(rw_GlobalData global_data, char *file_path);
u8 rw_ReadGlobalData(rw_GlobalData *global_data, char *file_path);

u8 rw_WriteEntData(EntityHandler *ent_handler, char *file_path);
u8 rw_ReadEntData(EntityHandler *ent_handler, char *file_path);

void EntHandlerPassRwState(rw_GlobalData *data);

#endif
