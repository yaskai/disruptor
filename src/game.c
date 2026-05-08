#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "game.h"
#include "input_handler.h"
#include "geo.h"
#include "../include/log_message.h"
#include "map.h"
#include "rw_save.h"

Game *game_self_ptr;

Shader blur_shader;

Vector2 mouse_pause_position;

void VirtCameraControls(Camera3D *cam, float dt, Vector3 target_point);
void EndScreen(Game *game, float dt);

RenderTexture2D save_rt;
bool screen_cap = false;

pthread_t cap_thread;
bool cap_thread_started = false;
typedef struct {
	const char *export_path;
	Image img;

} CapData;

void *ExportCap(void *arg) {
	CapData *cap_data = (CapData*)arg;
	//cap_thread_started = true;
	//cap_thread_done = (bool*)false;
	ExportImage(cap_data->img, cap_data->export_path);
	UnloadImage(cap_data->img);
	free((void*)cap_data->export_path);
	free(cap_data);
	return NULL;
} 

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

u16 tri_count = 0;
Tri *tris;

Model sphere_model;
float delta_time = 0;

void GameInit(Game *game, Config *conf) {
	game->conf = conf;	

	InputInit(&game->input_handler);
	game->input_handler.mouse_sensitivity = game->conf->mouse_sensitivity * 0.0001f;

	game->_gsave_state = (rw_GlobalData) {0};

	SetLogState(1);

	game->flags = 0;

	game_self_ptr = game;
}

void GameClose(Game *game) {
	if(cap_thread_started)
		pthread_join(cap_thread, NULL);

	// Unload render textures
	if(IsTextureValid(game->render_target3D.texture))
		UnloadRenderTexture(game->render_target3D);

	if(IsTextureValid(game->render_target2D.texture))
		UnloadRenderTexture(game->render_target2D);

	MapSectionClose(&game->test_section);
	EntHandlerClose(&game->ent_handler);
}

void GameRenderSetup(Game *game) {
	sphere_model = LoadModelFromMesh(GenMeshSphere(1, 12, 12));

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
	EntHandlerInit(&game->ent_handler, &game->effect_manager, &game->audio_player);

	mat_default = LoadMaterialDefault();
	mat_default.maps[MATERIAL_MAP_DIFFUSE].color = ColorAlpha(BLUE, 0.25f);

	game->lh = (LightHandler) {0};
	InitPointLights(&game->lh);
	//lh_SetShaderLocs(&game->test_section.bsp_data);
	lh_SetBspPtr(&game->test_section.bsp_data);
	lh_SetShaderLocs(&game->test_section.bsp_data);

	game->hud = (Hud) {0};
	HudInit(&game->hud, game->conf, &game->ent_handler, &game->player_gun);

	SetShaderValue(
			game->hud.text_shader,
			GetShaderLocation(game->hud.text_shader, "texture0"),
			&game->render_target2D.texture, 
			SHADER_UNIFORM_SAMPLER2D
			);

	Vector2 res = (Vector2) { VIRT_W, VIRT_H };
	SetShaderValue(
			game->hud.text_shader,
			GetShaderLocation(game->hud.text_shader, "resolution"),
			&res,
			SHADER_UNIFORM_VEC2
			);

	game->ui = (UiHandler) {0};
	ui_Init(&game->ui, game->conf);

	blur_shader = LoadShader("resources/shaders/base_v.glsl", "resources/shaders/blur_f.glsl");

	//save_rt = LoadRenderTexture(540/2, 360/2);
	save_rt = LoadRenderTexture(640, 360);
}

void GameAudioSetup(Game *game) {
	AP_Init(&game->audio_player, &game->camera);
	AP_SetGlobalVolume(&game->audio_player, game->conf->volume);
}

void GameLoadScene(Game *game, char *path, u8 flags) {
	PlayerGunClose(&game->player_gun);

	AP_StopAll(&game->audio_player);

	ClosePointLights(&game->lh);

	DisableCursor();

	game->flags &= ~FLAG_LOAD_COMPLETE;

	HandlerSetPtrGun(&game->player_gun);

	for(int i = 0; i < 64; i++)
		game->effect_manager.impact_decals[i].active = false;

	char *path_dup = calloc(strlen(path), 1);
	memcpy(path_dup, path, strlen(path));
	char *sep = strrchr(path_dup, '/');
	*sep = '\0';
	memcpy(game->_gsave_state.map, sep+1, strlen(path_dup));
	free(path_dup);

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
		&game->camera,
		&game->audio_player,
		&game->input_handler
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

	Vector3 player_pos = (flags & AT_LEVEL_BACK) ? game->ent_handler.player_ret : game->ent_handler.player_start;
	Vector3 player_fwd = (flags & AT_LEVEL_BACK) ? game->ent_handler.player_ret_fwd : game->ent_handler.player_start_fwd;
	SpawnPlayer(&game->ent_handler.ents[game->ent_handler.player_id], player_pos, player_fwd);

	game->ent_handler.spawn_list.count = spawn_list.count;
	game->ent_handler.spawn_list.arr = calloc(spawn_list.count, sizeof(EntSpawn));
	memcpy(game->ent_handler.spawn_list.arr, spawn_list.arr, sizeof(EntSpawn) * spawn_list.count);

	// Setup checkpoints	
	for(u16 i = 0; i < game->ent_handler.checkpoint_list.count; i++) {
		game->ent_handler.checkpoint_list.cells[i] = CellCoordsToId(
			Vec3ToCoords(game->ent_handler.checkpoint_list.points[i], &game->ent_handler.grid), &game->ent_handler.grid);	
	}

	DSP_AudioSetup(&game->test_section.bsp_data, &game->audio_player, &spawn_list);

	lh_SetSectPointer(&game->test_section);
	lh_SetEntHandlerPtr(&game->ent_handler);
	lh_SetBspPtr(&game->test_section.bsp_data);
	lh_SetShaderLocs(&game->test_section.bsp_data);

	for(int i = 0; i < game->test_section.bvh_hullgroup_count; i++) {
		game->test_section.bvh_hullgroups[i].ent_id = game->test_section.bsp_data.hull_groups[i].ent_id;
	}

	game->flags |= FLAG_LOAD_COMPLETE;
	game->flags |= FLAG_GAME_STARTED;

	GetMouseDelta();	// Clears existing delta, needed to make camera not move on start
	game->input_handler.mouse_delta = Vector2Zero();

	/*
	printf("bvh hg count: %d\n", game->test_section.bvh_hullgroup_count);
	printf("bsp hg count: %d\n", game->test_section.bsp_data.num_models);

	for(int i = 0; i < game->test_section.bsp_data.num_models; i++) {
		printf("bvh hg[%d] ent_id: %d\n", i, game->test_section.bvh_hullgroups[i].ent_id);
		printf("bsp hg[%d] ent_id: %d\n", i, game->test_section.bsp_data.hull_groups[i].ent_id);
	}

	printf("player_id: %d\n", game->ent_handler.player_id);
	printf("bug_id: %d\n", game->ent_handler.bug_id);
	printf("sect ptr (src) %p\n", &game->test_section);
	*/
}

void GameUpdate(Game *game, float dt) {
	//if(IsKeyPressed(KEY_ESCAPE))
		//game->flags |= FLAG_EXIT_REQUEST;

	if(game->ui.flags & UI_EXIT_GAME_REQ)
		game->flags |= FLAG_EXIT_REQUEST;

	if(game->ui.flags & UI_START_GAME_REQ) {
		GetMouseDelta();	// Clears existing delta, needed to make camera not move on start
		StartNewGame(game);
		GetMouseDelta();	// Clears existing delta, needed to make camera not move on start
		SetMousePosition(0, 0);
		GetMouseDelta();	// Clears existing delta, needed to make camera not move on start
		game->input_handler.mouse_delta = (Vector2) { 0, 0 };
		game->ui.flags &= ~UI_START_GAME_REQ;
		game->ui.flags = 0;

		return;
	}

	if(game->ui.flags & UI_LOAD_SAVE_REQ) {
		if(strcmp(game->ui.map_name, game->_gsave_state.map)) {
			BeginDrawing();
			ClearBackground(BLACK);
			DisplayLoadIndicator(&game->hud);
			EndDrawing();

			char *path_pref = "resources/maps/";
			char path[255] = {'\0'};
			memcpy(path, path_pref, strlen(path_pref));
			memcpy(path + strlen(path), game->ui.map_name, strlen(game->ui.map_name));

			EntHandlerClose(&game->ent_handler);

			MapSectionClose(&game->test_section);
			SectFreeBrushData(&game->test_section);

			EntHandlerInit(&game->ent_handler, &game->effect_manager, &game->audio_player);

			GameLoadScene(game, path, game->ent_handler.flags);
			PlayerGunOnLoad(&game->_gsave_state, &game->player_gun);
		}
		
		if(rw_ReadSave(&game->ent_handler, game->ui.save_name, &game->_gsave_state)) {
			memcpy(game->_gsave_state.map, game->ui.map_name, 64);
			PlayerGunOnLoad(&game->_gsave_state, &game->player_gun);
			game->ui.flags &= ~UI_LOAD_SAVE_REQ;
			game->ui.flags |= UI_TOGGLE_REQ;
		}
	}

	if(IsKeyPressed(KEY_ESCAPE))
		game->ui.flags |= UI_TOGGLE_REQ;

	if(game->ui.flags & UI_TOGGLE_REQ) {
		GetMouseDelta();
		ui_Toggle();

		if(game->ui.flags & UI_ACTIVE)
			mouse_pause_position = GetMousePosition();
		else 
			SetMousePosition(mouse_pause_position.x, mouse_pause_position.y);

		game->ui.flags &= ~UI_TOGGLE_REQ;
	}


	if(game->ent_handler.flags & AT_LEVEL_END) {
		screen_cap = true;

		PlayerGunOnSave(&game->_gsave_state, &game->player_gun);
		rw_WriteSaveNew(&game->ent_handler, game->_gsave_state.map, game->_gsave_state);

		char *path_pref = "resources/maps/";
		char path[255] = {'\0'};
		memcpy(path, path_pref, strlen(path_pref));
		memcpy(path + strlen(path), game->ent_handler.grid.sect_next, strlen(game->ent_handler.grid.sect_next));

		Entity player = game->ent_handler.ents[game->ent_handler.player_id];

		EntHandlerClose(&game->ent_handler);
		MapSectionClose(&game->test_section);
		SectFreeBrushData(&game->test_section);

		EntHandlerInit(&game->ent_handler, &game->effect_manager, &game->audio_player);

		GameLoadScene(game, path, game->ent_handler.flags);
		PlayerGunOnLoad(&game->_gsave_state, &game->player_gun);

		game->ent_handler.ents[game->ent_handler.player_id].comp_health = player.comp_health;

		game->ent_handler.flags &= ~AT_LEVEL_END;
	}

	if(game->ent_handler.flags & AT_LEVEL_BACK) {
		screen_cap = true;

		PlayerGunOnSave(&game->_gsave_state, &game->player_gun);
		rw_WriteSaveNew(&game->ent_handler, game->_gsave_state.map, game->_gsave_state);

		char *path_pref = "resources/maps/";
		char path[255] = {'\0'};
		memcpy(path, path_pref, strlen(path_pref));
		memcpy(path + strlen(path), game->ent_handler.grid.sect_prev, strlen(game->ent_handler.grid.sect_prev));

		EntHandlerClose(&game->ent_handler);
		MapSectionClose(&game->test_section);
		SectFreeBrushData(&game->test_section);

		EntHandlerInit(&game->ent_handler, &game->effect_manager, &game->audio_player);

		GameLoadScene(game, path, (AT_LEVEL_BACK));

		Entity player = game->ent_handler.ents[game->ent_handler.player_id];

		rw_LoadMostRecent(&game->ent_handler, &game->_gsave_state);
		PlayerGunOnLoad(&game->_gsave_state, &game->player_gun);
		game->ent_handler.ents[game->ent_handler.player_id] = player;

		game->ent_handler.flags &= ~AT_LEVEL_BACK;
	}

	EntHandlerPassRwState(&game->_gsave_state);

	VirtCameraControls(&game->camera_debug, dt, game->ent_handler.ents[game->ent_handler.player_id].comp_transform.position);

	if(game->ui.flags & UI_ACTIVE) {
		/*
		BeginTextureMode(game->render_target2D);
		ui_PauseUpdate(&game->ui);
		EndTextureMode();
		*/
		game->input_handler.mouse_delta = (Vector2) { 0, 0 };

	} else if (game->flags & FLAG_GAME_STARTED && !(game->ui.flags & UI_START_GAME_REQ)) {
		PollInput(&game->input_handler);

		MapUpdateBvhOffsets(&game->test_section);
		UpdateEntities(&game->ent_handler, &game->test_section, dt);
		AP_Update(&game->audio_player, dt);
		DSP_UpdateBlend(&game->test_section, &game->audio_player, game->camera.position, dt);
		PlayerGunUpdate(&game->player_gun, dt);
	}

	if((game->ent_handler.flags & AUTOSAVE_REQUEST) ^ (game->ui.flags & UI_SAVE_GAME_REQ)) {
		PlayerGunOnSave(&game->_gsave_state, &game->player_gun);
		rw_WriteSaveNew(&game->ent_handler, game->_gsave_state.map, game->_gsave_state);

		game->ent_handler.flags &= ~AUTOSAVE_REQUEST;
		game->ui.flags &= ~UI_SAVE_GAME_REQ;

		screen_cap = true;
	}
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

	ManagePointLights(&game->test_section.bsp_data, &game->ent_handler, dt);
	// Render level geometry
	DrawMap(&game->test_section, game->camera.position);
	// Render entities
	RenderEntities(&game->ent_handler, GetFrameTime());
	// Run/draw visual effects
	vEffectsRun(&game->effect_manager, dt);
	// Render dynamic brush entities
	RenderBrushEntities(&game->ent_handler);
	// Render transparent level geometry
	DrawMapTranslucent(&game->test_section, game->camera.position);
	// Draw player (for debug only)
	//PlayerDraw(&game->ent_handler.ents[game->ent_handler.player_id]);

	EndMode3D();

	// Fade to black effect on player death
	if(player_ent->comp_ai.state == STATE_DEAD) {
		//float deathscreen_alpha = player_ent->comp_ai.task_state.timer*0.5f;
		float deathscreen_alpha = game->ent_handler.player_death_timer*0.5f;
		if(deathscreen_alpha > 1) deathscreen_alpha = 1;
		DrawRectangleRec((Rectangle) { 0, 0, VIRT_W, VIRT_H } , ColorAlpha(BLACK, deathscreen_alpha));
	}

	EndTextureMode();
}

void RenderGunLayer(Game *game) {
	BeginTextureMode(game->render_target2D);
	ClearBackground(BLANK);

	PlayerGunDraw(&game->player_gun);
	HudUpdate(&game->hud, GetFrameTime());

	EndTextureMode();
}

void RenderDebugLayer(Game *game) {
	if(IsKeyPressed(KEY_V)) debug_draw_flags ^= DEBUG_ENABLE;
	if((debug_draw_flags & DEBUG_ENABLE) == 0)
		return;

	// 3D Rendering, debug
	BeginTextureMode(game->render_target_debug);

	float clear_alpha = (debug_draw_flags & DEBUG_DRAW_BIG) ? 1.0f : 0.95f;
	ClearBackground(ColorAlpha(BLACK, clear_alpha));

	BeginMode3D(game->camera_debug);

	float dt = GetFrameTime();

	// Render level geometry
	DrawMap(&game->test_section, game->camera.position);
	// Render entities
	RenderEntities(&game->ent_handler, GetFrameTime());
	// Render transparent level geometry
	DrawMapTranslucent(&game->test_section, game->camera.position);
	// Render dynamic brush entities
	RenderBrushEntities(&game->ent_handler);
	// Draw player (for debug only)
	PlayerDraw(&game->ent_handler.ents[game->ent_handler.player_id]);
	//PlayerDisplayDebugInfo(&game->ent_handler.ents[game->ent_handler.player_id]);

	if(IsKeyPressed(KEY_H)) debug_draw_flags ^= DEBUG_DRAW_HULLS;
	if(debug_draw_flags & DEBUG_DRAW_HULLS) { 
		/*
		for(u16 j = 0; j < game->test_section.bvh[1].tris.count; j++) {
			Tri *tri = &game->test_section.bvh[1].tris.arr[j];
			Color color = colors[tri->hull_id % 7];
			DrawTriangle3D(tri->vertices[0], tri->vertices[1], tri->vertices[2], ColorTint(color, BROWN));
		}
		for(u16 j = 0; j < game->test_section._hulls[1].count; j++) {
			Hull *hull = &game->test_section._hulls[1].arr[j];
			DrawBoundingBox(hull->aabb, colors[j % 7]);
		}
		*/
		for(u16 j = 0; j < game->test_section.bvh_hullgroups->bvh[2].tris.count; j++) {
			Tri *tri = &game->test_section.bvh_hullgroups->bvh[2].tris.arr[j];
			Color color = colors[tri->hull_id % 7];
			DrawTriangle3D(tri->vertices[0], tri->vertices[1], tri->vertices[2], ColorAlpha(color, 0.5f));
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
	PlayerDebugText(&game->ent_handler.ents[game->ent_handler.player_id]);
	PlayerDraw(&game->ent_handler.ents[game->ent_handler.player_id]);
	EndTextureMode();
}

void GameDraw(Game *game, float dt) {
	bool pause = false;

	if(game->flags & FLAG_GAME_STARTED) {
		if((game->ent_handler.flags & AT_LEVEL_END) ^ (game->ent_handler.flags & AT_LEVEL_BACK)) {
			BeginTextureMode(game->render_target2D);
			HudUpdate(&game->hud, dt);
			EndTextureMode();

		} else {
			RenderMainLayer(game, dt);
			RenderGunLayer(game);
			RenderDebugLayer(game);
		}

		if(game->ui.flags & UI_ACTIVE)
			pause = true;

	} else {
		GameTitleScreen(game);
	}

	// 2D Rendering
	// 2D
	//DrawText(TextFormat("accel: %.02f", player_data.accel), 0, 40, 32, RAYWHITE);
	//DrawEntsDebugInfo();

	// Draw to buffers:
	// Main
	ClearBackground(BLACK);
	BeginBlendMode(BLEND_ALPHA);

	Color tint = (pause) ? DARKGRAY : WHITE;

	Rectangle rt_src = (Rectangle) { 0, 0, game->render_target3D.texture.width, -game->render_target3D.texture.height };
	Rectangle rt_dst = (Rectangle) { 0, 0, game->conf->window_width, game->conf->window_height };
	DrawTexturePro(game->render_target3D.texture, rt_src, rt_dst, Vector2Zero(), 0, tint);

	rt_src = (Rectangle) { 0, 0, game->render_target2D.texture.width, -game->render_target2D.texture.height };
	rt_dst = (Rectangle) { 0, 0, game->conf->window_width, game->conf->window_height };
	DrawTexturePro(game->render_target2D.texture, rt_src, rt_dst, Vector2Zero(), 0, tint);

	if(screen_cap) {
		BeginTextureMode(save_rt);
		Rectangle rt_src = (Rectangle) { 0, 0, game->render_target3D.texture.width, game->render_target3D.texture.height };
		Rectangle rt_dst = (Rectangle) { 0, 0, save_rt.texture.width, save_rt.texture.height };
		DrawTexturePro(game->render_target3D.texture, rt_src, rt_dst, Vector2Zero(), 0, WHITE);

		rt_src = (Rectangle) { 0, 0, game->render_target2D.texture.width, game->render_target2D.texture.height };
		rt_dst = (Rectangle) { 0, 0, save_rt.texture.width, save_rt.texture.height };
		DrawTexturePro(game->render_target2D.texture, rt_src, rt_dst, Vector2Zero(), 0, WHITE);
		EndTextureMode();

		const char *export_path = TextFormat("data/%s/cap.png", rw_GetRecentWritePath());
		Image img = LoadImageFromTexture(save_rt.texture);
		
		CapData *cap_data = malloc(sizeof(CapData)); 
		char *path_copy = malloc(strlen(export_path)+1);
		strcpy(path_copy, export_path);

		cap_data->export_path = path_copy;
		cap_data->img = img;

		if(pthread_create(&cap_thread, NULL, ExportCap, cap_data) == 0) {
			cap_thread_started = true; 
		} else {
			UnloadImage(img);
			free(path_copy);
			free(cap_data);
		}

		screen_cap = false;
	}

	EndBlendMode();

	if(pause) {
		ui_PauseUpdate(&game->ui);
	}

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
	DrawText(TextFormat("fps: %d", fps), 4, 4, 32, RAYWHITE);
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
		return;
	}

	if(IsKeyDown(KEY_P)) {
		movement = Vector3Scale(right,  300 * dt);
		cam->position = Vector3Add(cam->position, movement);
		return;
	}

	cam->position = Vector3Add(cam->position, movement);
	cam->target = Vector3Add(cam->target, movement);
	
	if(IsKeyDown(KEY_M))
		cam->target = target_point;
}

void StartNewGame(Game *game) {
	if(cap_thread_started)
		pthread_join(cap_thread, NULL);

	EntHandlerClose(&game->ent_handler);
	MapSectionClose(&game->test_section);
	SectFreeBrushData(&game->test_section);

	EntHandlerInit(&game->ent_handler, &game->effect_manager, &game->audio_player);
	game->flags &= ~FLAG_GAME_STARTED;
	ui_Toggle();
	game->ui.flags = 0;
	//GameLoadScene(game, "resources/maps/10_a", 0);
	GameLoadScene(game, "resources/maps/10_a_1", 0);
	//GameLoadScene(game, "resources/maps/10_a_3", 0);
	//GameLoadScene(game, "resources/maps/10_b", 0);
	//GameLoadScene(game, "resources/maps/10", 0);
	game->flags |= FLAG_GAME_STARTED;
	ResetPlayerWeapons();
}

void GameTitleScreen(Game *game) {
	//game->input_handler.skip_frames = 5;
	game->input_handler.mouse_delta = (Vector2) { 0, 0 };
	ui_TitleUpdate(&game->ui);

	if(game->ui.flags & UI_START_GAME_REQ) {
		game->ui.flags &= ~UI_START_GAME_REQ;		
		GetMouseDelta();	// Clears existing delta, needed to make camera not move on start
		StartNewGame(game);
		GetMouseDelta();	// Clears existing delta, needed to make camera not move on start
		SetMousePosition(0, 0);
		GetMouseDelta();	// Clears existing delta, needed to make camera not move on start
		game->input_handler.mouse_delta = (Vector2) { 0, 0 };
	} else {
		mouse_pause_position = GetMousePosition();
	}

	if(game->ui.flags & UI_EXIT_GAME_REQ)
		game->flags |= FLAG_EXIT_REQUEST;
}

