#include "raylib.h"
#include "../include/num_redefs.h"
#include "config.h"
#include "input_handler.h"

#ifndef GAME_H_
#define GAME_H_

#define FLAG_EXIT_REQUEST	0x01

typedef struct {
	RenderTexture2D render_target;

	InputHandler input_handler;

	Camera3D camera;

	Config *conf;

	u8 flags;

} Game;

void GameInit(Game *game, Config *conf);
void GameClose(Game *game);

void GameRenderSetup(Game *game);

void GameUpdate(Game *game, float dt);
void GameDraw(Game *game);

#endif
