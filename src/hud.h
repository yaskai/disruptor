#include "../include/num_redefs.h"
#include "raylib.h"
#include "config.h"
#include "ent.h"
#include "player_gun.h"

#ifndef HUD_H_
#define HUD_H_

typedef struct {
	Font font;
	Shader text_shader;

	Config *conf;
	EntityHandler *handler;
	PlayerGun *player_gun;

	float font_size;
	float font_spacing;

	u8 flags;
	
} Hud;

void HudInit(Hud *hud, Config *conf, EntityHandler *handler, PlayerGun *player_gun);
void HudUpdate(Hud *hud, float dt);

void DisplayAmmoCounter(Hud *hud);
void DisplayHealthCounter(Hud *hud);

void DisplayLoadIndicator(Hud *hud);

void DisplayBackgroundRec(Hud *hud, Rectangle rec);

Vector2 HudTextCenter(Hud *hud, Rectangle rec, const char *text);
Vector2 HudTextCenterEx(Hud *hud, Rectangle rec, const char *text, float font_size, float font_spacing);

void DisplayTextObject(Hud *hud, i16 id);

#endif
