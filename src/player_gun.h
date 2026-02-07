#include "../include/num_redefs.h"
#include "raylib.h"

#ifndef PLAYER_GUN_H_
#define PLAYER_GUN_H_

typedef struct {
	Model model;

	Camera3D cam;

	u16 current_gun;

} PlayerGun;

void PlayerGunInit(PlayerGun *player_gun);
void PlayerGunUpdate(PlayerGun *player_gun, float dt);
void PlayerGunDraw(PlayerGun *player_gun);

#endif
