#include <stdlib.h>
#include <pthread.h>
#include "raylib.h"
#include "config.h"
#include "game.h"

enum PLATFORMS : u8 {
	LINUX	= 0,
	WIN64	= 1,
	MACOS	= 2
}; 
u8 platform = LINUX;

unsigned int plat_flags[3] = {
	// Linux
	(FLAG_VSYNC_HINT | FLAG_FULLSCREEN_MODE),						
	
	// Windows
	(FLAG_BORDERLESS_WINDOWED_MODE | FLAG_WINDOW_MAXIMIZED | FLAG_VSYNC_HINT),		

	// MacOS
	(FLAG_FULLSCREEN_MODE | FLAG_VSYNC_HINT)
};

void StartupScreen(Config *conf, Game *game) {
	BeginDrawing();
	ClearBackground(BLACK);

	// * TODO:
	// Add title, logo, etc. here:

	DrawText("...", 32, 0, 80, WHITE);

	EndDrawing();
}

void GameTick(bool *exit, Game *game);
void OnExit(Config *conf, Game *game);

int main() {
	Config conf = (Config) {0};
	ConfigInit(&conf);
	ConfigRead(&conf, "options.conf");

	Game game = (Game) {0};
	GameInit(&game, &conf);

	// Disable raylib logs
	SetTraceLogLevel(LOG_NONE);

	// Set window flags
	SetConfigFlags(plat_flags[platform]);

	// * NOTE:
	// To be fixed and tested
	int monitor = GetCurrentMonitor();
	if(conf.window_width == atoi("auto"))
		conf.window_width = GetMonitorWidth(monitor);
	if(conf.window_height == atoi("auto"))
		conf.window_height = GetMonitorHeight(monitor);
	
	InitWindow(conf.window_width, conf.window_height, "DISRUPTOR");

	StartupScreen(&conf, &game);

	GameRenderSetup(&game);
	GameAudioSetup(&game);

	// Disable exit key (raylib defaults to escape key)
	// * NOTE: 
	// Overwritten back to ESC until UI/menus are implemented
	SetExitKey(KEY_NULL);

	// Disable cursor,
	// prevents drawing cursor image and aiming issues
	DisableCursor();

	//GameLoadScene(&game, "resources/maps/06");
	//GameLoadScene(&game, "resources/maps/07");
	GameLoadScene(&game, "resources/maps/08");
	//GameLoadScene(&game, "resources/maps/09");

	// Run main game loop
	bool exit = false;
	GameTick(&exit, &game);

	// App shutdown
	OnExit(&conf, &game);
	return 0;
}

// Main loop, called every frame
void GameTick(bool *exit, Game *game) {
	while(!(*exit)) {
		*exit = ( (game->flags & FLAG_EXIT_REQUEST) || WindowShouldClose() );
		float dt = GetFrameTime(); 	

		GameUpdate(game, dt);
		GameDraw(game, dt);
	}
}

// Unload game data, called on application shutdown
void OnExit(Config *conf, Game *game) {
	CloseWindow();

	ConfigClose(conf);
	GameClose(game);
}

