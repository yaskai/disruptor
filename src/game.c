#include <stdio.h>
#include <string.h>
#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "game.h"
#include "input_handler.h"
#include "geo.h"
#include "../include/log_message.h"

void VirtCameraControls(Camera3D *cam, float dt);

Color colors[] = {
	PINK,
	BLUE,
	GREEN,
	SKYBLUE,
	GRAY,
	MAGENTA
};

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

	game->camera_debug = (Camera3D) {
		.position = (Vector3) { -300, 200, -300 },
		.target = (Vector3) { 0, 0, 0 },
		.up = (Vector3) {0, 1, 0},
		.fovy = 50,
		.projection = CAMERA_PERSPECTIVE
	};

	// Load render textures
	game->render_target3D = LoadRenderTexture(game->conf->window_width, game->conf->window_height);
	game->render_target2D = LoadRenderTexture(game->conf->window_width, game->conf->window_height);
	game->render_target_debug = LoadRenderTexture(game->conf->window_width * 0.5f, game->conf->window_height * 0.5f);

	EntHandlerInit(&game->ent_handler);
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

	PlayerInit(&game->camera, &game->input_handler, &game->test_section);

	Entity player = (Entity) {
		.comp_transform = (comp_Transform) {0},
		.comp_health = (comp_Health) {0},
		.comp_weapon = (comp_Weapon) {0},
		.behavior_id = 0,
		.flags = (ENT_ACTIVE)
	};

	player.comp_transform.bounds.max = (Vector3) { PLAYER_BOX_LENGTH, PLAYER_BOX_HEIGHT, PLAYER_BOX_LENGTH };
	player.comp_transform.bounds.min = Vector3Scale(player.comp_transform.bounds.max, -1);

	player.comp_transform.position.y = 30;

	game->ent_handler.ents[game->ent_handler.count++] = player;
}

void GameUpdate(Game *game, float dt) {
	if(IsKeyPressed(KEY_ESCAPE))
		game->flags |= FLAG_EXIT_REQUEST;

	VirtCameraControls(&game->camera_debug, dt);

	PollInput(&game->input_handler);

	UpdateEntities(&game->ent_handler, dt);
}

void GameDraw(Game *game) {
	// 3D Rendering, main
	BeginDrawing();
	BeginTextureMode(game->render_target3D);
	ClearBackground(BLACK);
		BeginMode3D(game->camera);

			//DrawModel(game->test_section.model, Vector3Zero(), 1, DARKGRAY);
			//DrawModelWires(game->test_section.model, Vector3Zero(), 1, BLACK);

			//MapSectionDisplayNormals(&game->test_section);

			for(u16 i = 0; i < game->test_section.bvh.count; i++) {
				BvhNode *node = &game->test_section.bvh.nodes[i];

				bool is_leaf = node->tri_count > 0;
				if(!is_leaf) continue;

				//DrawBoundingBox(node->bounds, SKYBLUE);

				Color color = colors[i % 6];
				for(u16 j = 0; j < node->tri_count; j++) {
					u16 tri_id = node->first_tri + j;
					Tri *tri = &game->test_section.tris[tri_id];

					DrawTriangle3D(tri->vertices[0], tri->vertices[1], tri->vertices[2], color);
				}
			}

			RenderEntities(&game->ent_handler);

		EndMode3D();
	EndTextureMode();
	
	// 3D Rendering, debug
	BeginTextureMode(game->render_target_debug);
	ClearBackground(ColorAlpha(BLACK, 0.5f));
		BeginMode3D(game->camera_debug);
			//DrawModel(game->test_section.model, Vector3Zero(), 1, ColorAlpha(DARKGRAY, 1.0f));
			DrawModelWires(game->test_section.model, Vector3Zero(), 1, BLUE);

			PlayerDisplayDebugInfo(&game->ent_handler.ents[0]);

		EndMode3D();
	EndTextureMode();

	// 2D Rendering

	// Draw to buffers:
	// Main
	Rectangle rt_src = (Rectangle) { 0, 0, game->render_target3D.texture.width, -game->render_target3D.texture.height };
	Rectangle rt_dst = (Rectangle) { 0, 0, game->render_target3D.texture.width,  game->render_target3D.texture.height };
	DrawTexturePro(game->render_target3D.texture, rt_src, rt_dst, Vector2Zero(), 0, WHITE);

	// Debug
	rt_src = (Rectangle) { 0, 0, game->render_target_debug.texture.width, -game->render_target_debug.texture.height };
	rt_dst = (Rectangle) { 0, 0, game->render_target_debug.texture.width,  game->render_target_debug.texture.height };
	DrawTexturePro(game->render_target_debug.texture, rt_src, rt_dst, Vector2Zero(), 0, WHITE);

	EndDrawing();
}

void VirtCameraControls(Camera3D *cam, float dt) {
	Vector3 forward = Vector3Normalize(Vector3Subtract(cam->target, cam->position)); 
	Vector3 right = Vector3CrossProduct(forward, cam->up);
	
	Vector3 movement = Vector3Zero();	

	movement = Vector3Add(movement, Vector3Scale(forward, GetMouseWheelMove()));

	if(IsKeyDown(KEY_UP)) 		movement = Vector3Add(movement, cam->up);
	if(IsKeyDown(KEY_RIGHT)) 	movement = Vector3Add(movement, right);
	if(IsKeyDown(KEY_DOWN))		movement = Vector3Subtract(movement, cam->up);
	if(IsKeyDown(KEY_LEFT))		movement = Vector3Subtract(movement, right);

	movement = Vector3Scale(movement, 300 * dt);
	
	cam->position = Vector3Add(cam->position, movement);
	cam->target = Vector3Add(cam->target, movement);
}
