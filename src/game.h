#include "raylib.h"
#include "config.h"
#include "input_handler.h"
#include "geo.h"
#include "ent.h"
#include "player_gun.h"
#include "v_effect.h"
#include "../include/num_redefs.h"
#include "audioplayer.h"
#include "rw_save.h"
#include "lit.h"
#include "hud.h"
#include "ui.h"

#ifndef GAME_H_
#define GAME_H_

#define FLAG_EXIT_REQUEST	0x01
#define FLAG_LOAD_COMPLETE 	0x02
#define FLAG_GAME_STARTED 	0x04

typedef struct {
	AudioPlayer audio_player;

	MapSection test_section;

	EntityHandler ent_handler;

	PlayerGun player_gun;

	vEffect_Manager effect_manager;

	RenderTexture2D render_target3D;
	RenderTexture2D render_target2D;
	RenderTexture2D render_target_debug;

	InputHandler input_handler;

	Hud hud;
	UiHandler ui;

	Camera3D camera;
	Camera3D camera_debug;

	Config *conf;

	rw_GlobalData _gsave_state;

	LightHandler lh;

	u8 flags;

} Game;

void GameInit(Game *game, Config *conf);
void GameClose(Game *game);

void GameRenderSetup(Game *game);
void GameAudioSetup(Game *game);

void GameLoadScene(Game *game, char *path, u8 flags);

void GameUpdate(Game *game, float dt);
void GameDraw(Game *game, float dt);

void StartNewGame(Game *game);
void GameTitleScreen(Game *game);

#endif
