#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "game.h"
#include "input_handler.h"
#include "geo.h"
#include "../include/log_message.h"
#include "map.h"

void VirtCameraControls(Camera3D *cam, float dt, Vector3 target_point);
void EndScreen(Game *game, float dt);

#define VIRT_W (1920)
#define VIRT_H (1080)

float *plr_accel;

Color colors[] = {
	PINK,
	BLUE,
	GREEN,
	SKYBLUE,
	PURPLE,
	ORANGE,
	RED
};

PlayerDebugData player_data = {0};

Material mat_default;

BrushPool brush_pool = (BrushPool) {0};
BrushPool brush_pool_exp = (BrushPool) {0};

u16 tri_count = 0;
Tri *tris;

Model sphere_model;
float delta_time = 0;

void GameInit(Game *game, Config *conf) {
	game->conf = conf;	

	InputInit(&game->input_handler);
	game->input_handler.mouse_sensitivity = game->conf->mouse_sensitivity * 0.0001f;

	//SetLogState(true);
	SetLogState(false);

	game->flags = 0;
}

void GameClose(Game *game) {
	// Unload render textures
	if(IsTextureValid(game->render_target3D.texture))
		UnloadRenderTexture(game->render_target3D);

	if(IsTextureValid(game->render_target2D.texture))
		UnloadRenderTexture(game->render_target2D);

	MapSectionClose(&game->test_section);
	EntHandlerClose(&game->ent_handler);
}

void GameRenderSetup(Game *game) {
	// Initalize 3D camera
	game->camera = (Camera3D) {
		.position = (Vector3) { 30, 30, 30 },
		.target = (Vector3) { 0, 0, 0 },
		.up = UP,
		.fovy = 90,
		.projection = CAMERA_PERSPECTIVE
	};

	game->camera_debug = (Camera3D) {
		.position = (Vector3) { -1500, -1500, 1000 },
		.target = (Vector3) { 0, 0, 0 },
		.up = UP,
		.fovy = 90,
		.projection = CAMERA_PERSPECTIVE
	};

	// Load render textures
	//game->render_target3D = LoadRenderTexture(game->conf->window_width, game->conf->window_height);
	//game->render_target2D = LoadRenderTexture(game->conf->window_width, game->conf->window_height);
	//game->render_target_debug = LoadRenderTexture(game->conf->window_width * 0.5f, game->conf->window_height * 0.5f);

	int vw = game->conf->window_width, vh = game->conf->window_height;

	game->render_target3D = LoadRenderTexture(vw, vh);
	game->render_target2D = LoadRenderTexture(vw, vh);
	//game->render_target_debug = LoadRenderTexture(game->conf->window_width * 0.5f, game->conf->window_height * 0.5f);
	game->render_target_debug = LoadRenderTexture(vw, vh);

	SetTextureFilter(game->render_target3D.texture, TEXTURE_FILTER_TRILINEAR);
	SetTextureFilter(game->render_target2D.texture, TEXTURE_FILTER_TRILINEAR);
	SetTextureFilter(game->render_target_debug.texture, TEXTURE_FILTER_TRILINEAR);

	vEffectsInit(&game->effect_manager);
	EntHandlerInit(&game->ent_handler, &game->effect_manager);

	mat_default = LoadMaterialDefault();
	mat_default.maps[MATERIAL_MAP_DIFFUSE].color = ColorAlpha(BLUE, 0.25f);
}

void GameLoadScene(Game *game, char *path) {
	game->flags &= ~FLAG_LOAD_COMPLETE;

	SpawnList sl = (SpawnList) {0}; 
	game->test_section = BuildMapSect(path, &sl);
	game->test_section.navgraphs = malloc(sizeof(NavGraph) * 32);
	SpawnList spawn_list = ParseBspEnts(&game->ent_handler, &game->test_section.bsp_data);

	// ----------------------------------------------------------------------------------------
	Entity player = (Entity) {0};
	player.type = ENT_PLAYER;
	player.flags |= ENT_ACTIVE;

	Entity bug = (Entity) {0};
	bug.type = ENT_DISRUPTOR;
	bug.flags |= ENT_ACTIVE;
	// ----------------------------------------------------------------------------------------

	game->test_section.base_navgraph = (NavGraph) {
		.nodes = calloc(128, sizeof(NavNode)),
		.edges = calloc(128, sizeof(NavEdge)),
		.node_cap = 128, .edge_cap = 128,
		.node_count = 0, .edge_count = 0
	};

	game->ent_handler.count = 0;
	for(int i = 0; i < spawn_list.count; i++) 
		ProcessEntity(&spawn_list.arr[i], &game->ent_handler, &game->test_section.base_navgraph, &game->test_section.bsp_data);

	SwitchSetup(&game->ent_handler);
	
	player.id = game->ent_handler.player_id;
	game->ent_handler.ents[game->ent_handler.player_id] = player;
	bug.id = game->ent_handler.bug_id;
	game->ent_handler.ents[game->ent_handler.bug_id] = bug;

	//printf("player_id: %d\n", game->ent_handler.player_id);
	//printf("bug_id: %d\n", game->ent_handler.bug_id);

	PlayerInit(&game->camera, &game->input_handler, &game->test_section, &player_data, &game->ent_handler);

	game->player_gun = (PlayerGun) {0};
	PlayerGunInit(
		&game->player_gun,
		&game->ent_handler.ents[game->ent_handler.player_id],
		&game->ent_handler,
		&game->test_section,
		&game->effect_manager,
		game->conf,
		&game->camera
	);

	// -----------------------------------------------------------------------------------------------------------------
	// *
	BuildNavEdges(&game->test_section.base_navgraph, &game->test_section);

	game->test_section.navgraphs[0] = (NavGraph) {
		.node_count = game->test_section.base_navgraph.node_count,
		.edge_count = game->test_section.base_navgraph.edge_count,
		.node_cap = game->test_section.base_navgraph.node_cap,
		.edge_cap = game->test_section.base_navgraph.edge_cap
	};
	game->test_section.navgraphs[0].nodes = calloc(game->test_section.navgraphs[0].node_count, sizeof(NavNode));
	game->test_section.navgraphs[0].edges = calloc(game->test_section.navgraphs[0].edge_count, sizeof(NavEdge));

	SubdivideNavGraph(&game->test_section, &game->test_section.base_navgraph);
	AiNavSetup(&game->ent_handler, &game->test_section);
	// *
	// -----------------------------------------------------------------------------------------------------------------

	BugInit(&game->ent_handler.ents[game->ent_handler.bug_id], &game->ent_handler, &game->test_section);
	SpawnPlayer(&game->ent_handler.ents[game->ent_handler.player_id], game->ent_handler.player_start, Vector3Zero());

	game->ent_handler.spawn_list.count = spawn_list.count;
	game->ent_handler.spawn_list.arr = calloc(spawn_list.count, sizeof(EntSpawn));
	memcpy(game->ent_handler.spawn_list.arr, spawn_list.arr, sizeof(EntSpawn) * spawn_list.count);

	ReloadEntities(&game->ent_handler, &game->test_section, 0);

	// Setup checkpoints	
	for(u16 i = 0; i < game->ent_handler.checkpoint_list.count; i++) {
		game->ent_handler.checkpoint_list.cells[i] = CellCoordsToId(
			Vec3ToCoords(game->ent_handler.checkpoint_list.points[i], &game->ent_handler.grid), &game->ent_handler.grid);	
	}

	game->flags |= FLAG_LOAD_COMPLETE;
}

void GameUpdate(Game *game, float dt) {
	if(IsKeyPressed(KEY_ESCAPE))
		game->flags |= FLAG_EXIT_REQUEST;

	if(game->ent_handler.flags & AT_LEVEL_END) {
		EndScreen(game, dt);
		return;
	}

	VirtCameraControls(&game->camera_debug, dt, game->ent_handler.ents[game->ent_handler.player_id].comp_transform.position);

	PollInput(&game->input_handler);
	PlayerGunUpdate(&game->player_gun, dt);

	UpdateEntities(&game->ent_handler, &game->test_section, dt);
}

#define DEBUG_ENABLE			0x01
#define DEBUG_DRAW_HULLS 		0x02
#define DEBUG_DRAW_BIG	 		0x04
#define DEBUG_DRAW_FULL_MODEL	0x08
#define DEBUG_DRAW_BVH			0x10
u8 debug_draw_flags = (0);

void EndScreen(Game *game, float dt) {
	if(IsKeyPressed(KEY_ESCAPE))
		game->flags |= FLAG_EXIT_REQUEST;

	if(IsKeyPressed(KEY_Y)) {
		game->ent_handler.flags &= ~AT_LEVEL_END;
		game->ent_handler.checkpoint_list.active = -1;
		ReloadEntities(&game->ent_handler, &game->test_section, 0);  
		game->ent_handler.ents[game->ent_handler.player_id].comp_transform.forward = 
			game->ent_handler.ents[game->ent_handler.player_id].comp_transform.start_forward;
	}
}

void RenderMainLayer(Game *game, float dt) {
	Entity *player_ent = &game->ent_handler.ents[game->ent_handler.player_id];

	// 3D Rendering, main
	BeginDrawing();
	BeginTextureMode(game->render_target3D);
	
	ClearBackground(BLACK);
	BeginMode3D(game->camera);

	// Render level geometry
	DrawMap(&game->test_section, game->camera.position);
	// Render entities
	RenderEntities(&game->ent_handler, GetFrameTime());
	// Run/draw visual effects
	vEffectsRun(&game->effect_manager, dt);
	// Render transparent level geometry
	DrawMapTranslucent(&game->test_section, game->camera.position);
	// Render dynamic brush entities
	RenderBrushEntities(&game->ent_handler);
	// Draw player (for debug only)
	PlayerDraw(&game->ent_handler.ents[game->ent_handler.player_id]);

	EndMode3D();

	// Fade to black effect on player death
	if(player_ent->comp_ai.state == STATE_DEAD) {
		float deathscreen_alpha = player_ent->comp_ai.task_state.timer*0.5f;
		if(deathscreen_alpha > 1) deathscreen_alpha = 1;
		DrawRectangleRec((Rectangle) { 0, 0, VIRT_W, VIRT_H } , ColorAlpha(BLACK, player_ent->comp_ai.task_state.timer*0.5f));
	}

	EndTextureMode();
}

void RenderGunLayer(Game *game) {
	BeginTextureMode(game->render_target2D);
	ClearBackground(BLANK);

	PlayerGunDraw(&game->player_gun);

	EndTextureMode();
}

void RenderDebugLayer(Game *game) {
	if(IsKeyPressed(KEY_V)) debug_draw_flags ^= DEBUG_ENABLE;
	if((debug_draw_flags & DEBUG_ENABLE) == 0)
		return;

	// 3D Rendering, debug
	BeginTextureMode(game->render_target_debug);

	float clear_alpha = (debug_draw_flags & DEBUG_DRAW_BIG) ? 1 : 0.95f;
	ClearBackground(ColorAlpha(BLACK, clear_alpha));

	BeginMode3D(game->camera_debug);

	DrawMap(&game->test_section, game->camera.position);
	PlayerDisplayDebugInfo(&game->ent_handler.ents[game->ent_handler.player_id]);

	RenderEntities(&game->ent_handler, GetFrameTime());

	if(IsKeyPressed(KEY_H)) debug_draw_flags ^= DEBUG_DRAW_HULLS;
	if(debug_draw_flags & DEBUG_DRAW_HULLS) { 
		for(u16 j = 0; j < game->test_section.bvh[1].tris.count; j++) {
			Tri *tri = &game->test_section.bvh[1].tris.arr[j];
			Color color = colors[tri->hull_id % 7];
			DrawTriangle3D(tri->vertices[0], tri->vertices[1], tri->vertices[2], ColorTint(color, BROWN));
		}
		for(u16 j = 0; j < game->test_section._hulls[1].count; j++) {
			Hull *hull = &game->test_section._hulls[1].arr[j];
			DrawBoundingBox(hull->aabb, colors[j % 7]);
		}
	}

	if(IsKeyPressed(KEY_B)) debug_draw_flags ^= DEBUG_DRAW_BVH;
	if(debug_draw_flags & DEBUG_DRAW_BVH) { 
		for(u16 j = 0; j < game->test_section.bvh[0].count; j++) {
			BvhNode *node = &game->test_section.bvh->nodes[j];

			Color color = colors[j % 7];
			bool leaf = (node->tri_count > 0);
			if(!leaf) color = ColorAlpha(GRAY, 0.5f);

			DrawBoundingBox(node->bounds, color);
		}
	}

	DebugDrawNavGraphs(&game->test_section, sphere_model);

	EndMode3D();

	Vector2 dbg_window_size = (Vector2) { .x = game->render_target_debug.texture.width, .y = game->render_target_debug.texture.height };
	//DebugDrawNavGraphsText(&game->test_section, game->camera_debug, dbg_window_size);
	//DebugDrawEntText(&game->ent_handler, game->camera_debug);
	EndTextureMode();
}

void GameDraw(Game *game, float dt) {
	RenderMainLayer(game, dt);
	RenderGunLayer(game);
	RenderDebugLayer(game);

	// 2D Rendering
	// 2D
	//DrawText(TextFormat("accel: %.02f", player_data.accel), 0, 40, 32, RAYWHITE);
	//DrawEntsDebugInfo();

	// Draw to buffers:
	// Main
	Rectangle rt_src = (Rectangle) { 0, 0, game->render_target3D.texture.width, -game->render_target3D.texture.height };
	Rectangle rt_dst = (Rectangle) { 0, 0, game->conf->window_width, game->conf->window_height };
	DrawTexturePro(game->render_target3D.texture, rt_src, rt_dst, Vector2Zero(), 0, WHITE);

	rt_src = (Rectangle) { 0, 0, game->render_target2D.texture.width, -game->render_target2D.texture.height };
	rt_dst = (Rectangle) { 0, 0, game->conf->window_width, game->conf->window_height };
	DrawTexturePro(game->render_target2D.texture, rt_src, rt_dst, Vector2Zero(), 0, WHITE);

	//PlayerDebugText(&game->ent_handler.ents[game->ent_handler.player_id]);

	if(IsKeyPressed(KEY_T))
		debug_draw_flags ^= DEBUG_DRAW_BIG;

	Vector2 debug_wh = (debug_draw_flags & DEBUG_DRAW_BIG) 
		? (Vector2) { VIRT_W, VIRT_H } 
		: (Vector2) { VIRT_W * 0.5f, VIRT_H * 0.5f };

	// Debug
	if(debug_draw_flags & DEBUG_ENABLE) {
		rt_src = (Rectangle) { 0, 0, game->render_target_debug.texture.width, -game->render_target_debug.texture.height };
		rt_dst = (Rectangle) { 0, 0, debug_wh.x, debug_wh.y };
		DrawTexturePro(game->render_target_debug.texture, rt_src, rt_dst, Vector2Zero(), 0, WHITE);
	}

	int fps = GetFPS();
	//DrawText(TextFormat("fps: %d", fps), 4, 4, 32, RAYWHITE);
	//EntDebugText();

	EndDrawing();
}

void VirtCameraControls(Camera3D *cam, float dt, Vector3 target_point) {
	Vector3 forward = Vector3Normalize(Vector3Subtract(cam->target, cam->position)); 
	Vector3 right = Vector3CrossProduct(forward, cam->up);
	
	Vector3 movement = Vector3Zero();	

	movement = Vector3Add(movement, Vector3Scale(forward, GetMouseWheelMove() * 10));

	if(IsKeyDown(KEY_UP)) 		movement = Vector3Add(movement, cam->up);
	if(IsKeyDown(KEY_RIGHT)) 	movement = Vector3Add(movement, right);
	if(IsKeyDown(KEY_DOWN))		movement = Vector3Subtract(movement, cam->up);
	if(IsKeyDown(KEY_LEFT))		movement = Vector3Subtract(movement, right);

	movement = Vector3Scale(movement, 300 * dt);
	
	if(IsKeyDown(KEY_O)) {
		movement = Vector3Scale(right, -300 * dt);
		cam->position = Vector3Add(cam->position, movement);
		//cam->target = Vector3Subtract(cam->target, Vector3Scale(movement, 1.0f));
		return;
	}

	if(IsKeyDown(KEY_P)) {
		movement = Vector3Scale(right,  300 * dt);
		cam->position = Vector3Add(cam->position, movement);
		//cam->target = Vector3Subtract(cam->target, Vector3Scale(movement, 1.0f));
		return;
	}

	cam->position = Vector3Add(cam->position, movement);
	cam->target = Vector3Add(cam->target, movement);
	
	if(IsKeyDown(KEY_M))
		cam->target = target_point;
}

