#include "raylib.h"
#include "../include/num_redefs.h"
#include "config.h"
#include "input_handler.h"
#include "geo.h"

#ifndef GAME_H_
#define GAME_H_

#define FLAG_EXIT_REQUEST	0x01

typedef struct {
	MapSection test_section;

	RenderTexture2D render_target;

	InputHandler input_handler;

	Camera3D camera;

	Config *conf;

	u8 flags;

} Game;

void GameInit(Game *game, Config *conf);
void GameClose(Game *game);

void GameRenderSetup(Game *game);
void GameLoadTestScene(Game *game, char *path);

void GameUpdate(Game *game, float dt);
void GameDraw(Game *game);

#endif
