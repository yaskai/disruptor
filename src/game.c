#include "raylib.h"
#include "game.h"
#include "input_handler.h"

void GameInit(Game *game, Config *conf) {
	game->conf = conf;	

	InputInit(&game->input_handler);
}

void GameClose(Game *game) {
	if(IsTextureValid(game->render_target.texture))
		UnloadRenderTexture(game->render_target);
}

void GameRenderSetup(Game *game) {
	game->render_target = LoadRenderTexture(game->conf->window_width, game->conf->window_height);
}

void GameUpdate(Game *game, float dt) {
	if(IsKeyPressed(KEY_ESCAPE))
		game->flags |= FLAG_EXIT_REQUEST;

	PollInput(&game->input_handler);
}

void GameDraw(Game *game) {
	BeginDrawing();
	ClearBackground(BLACK);

	// 3D Rendering
	BeginMode3D(game->camera);
	EndMode3D();

	EndDrawing();

	// 2D Rendering
}

