#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "game.h"
#include "input_handler.h"
#include "geo.h"
#include "../include/log_message.h"

void GameInit(Game *game, Config *conf) {
	game->conf = conf;	

	InputInit(&game->input_handler);
}

void GameClose(Game *game) {
	// Unload render textures
	if(IsTextureValid(game->render_target3D.texture))
		UnloadRenderTexture(game->render_target3D);

	if(IsTextureValid(game->render_target2D.texture))
		UnloadRenderTexture(game->render_target2D);
}

void GameRenderSetup(Game *game) {
	// Initalize 3D camera
	game->camera = (Camera3D) {
		.position = (Vector3) { 30, 30, 30 },
		.target = (Vector3) { 0, 0, 0 },
		.up = (Vector3) {0, 1, 0},
		.fovy = 90,
		.projection = CAMERA_PERSPECTIVE
	};

	// Load render textures
	game->render_target3D = LoadRenderTexture(game->conf->window_width, game->conf->window_height);
	game->render_target2D = LoadRenderTexture(game->conf->window_width, game->conf->window_height);
}

void GameLoadTestScene(Game *game, char *path) {
	if(!DirectoryExists(path)) {
		MessageError("Missing directory", path);
		return;
	}

	FilePathList path_list = LoadDirectoryFiles(path);

	short model_id = -1;
	for(short i = 0; i < path_list.count; i++) {
		if(strcmp(GetFileExtension(path_list.paths[i]), ".glb") == 0) 
			model_id = i;
	}

	if(model_id == -1) {
		MessageError("Missing model", NULL);
		return;
	}

	Model model = LoadModel(path_list.paths[model_id]);

	game->test_section = (MapSection) {0};
	MapSectionInit(&game->test_section, model);
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

	//DrawModel(game->test_section.model, Vector3Zero(), 1, WHITE);
	DrawModelWires(game->test_section.model, Vector3Zero(), 1, BLUE);

	EndMode3D();

	EndDrawing();

	// 2D Rendering
}

