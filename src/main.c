#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "game.h"

int main() {
	Config conf = (Config) {0};
	ConfigInit(&conf);
	ConfigRead(&conf, "options.conf");

	Game game = (Game) {0};
	GameInit(&game, &conf);

	//SetTraceLogLevel(LOG_ERROR);
	SetTraceLogLevel(LOG_NONE);
	//SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_VSYNC_HINT | FLAG_BORDERLESS_WINDOWED_MODE | FLAG_MSAA_4X_HINT);
	SetConfigFlags(FLAG_FULLSCREEN_MODE | FLAG_VSYNC_HINT);

	if(conf.window_width == atoi("auto"))
		conf.window_width = GetScreenWidth();
	if(conf.window_height == atoi("auto"))
		conf.window_height = GetScreenHeight();
	
	InitWindow(conf.window_width, conf.window_height, "DISRUPTOR");

	GameRenderSetup(&game);

	// Disable exit key
	SetExitKey(KEY_NULL);
	DisableCursor();

	GameLoadScene(&game, "resources/maps/06");

	bool exit = false;
	while(!exit) {
		exit = ((game.flags & FLAG_EXIT_REQUEST) || WindowShouldClose());
		float dt = GetFrameTime(); 	

		GameUpdate(&game, dt);
		GameDraw(&game, dt);
	}

	CloseWindow();

	ConfigClose(&conf);
	GameClose(&game);

	return 0;
}

