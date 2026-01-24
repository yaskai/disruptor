#include "raylib.h"
#include "../include/num_redefs.h"
#include "config.h"

#ifndef GAME_H_
#define GAME_H_

#define FLAG_EXIT_REQUEST	0x01

typedef struct {
	RenderTexture2D render_target;

	Config *conf;

	u8 flags;	

} Game;

void GameInit(Game *game);
void GameClose(Game *game);

void GameUpdate(Game *game, float dt);
void GameDraw(Game *game);

#endif
