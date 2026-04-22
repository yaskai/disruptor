#include "../include/num_redefs.h"
#include "config.h"
#include "ent.h"
#include "player_gun.h"

#ifndef HUD_H_
#define HUD_H_

typedef struct {
	Config *conf;
	EntityHandler *handler;
	PlayerGun *player_gun;

	u8 flags;
	
} Hud;

#endif
