#include "../include/num_redefs.h"
#include "raylib.h"
#include "rw_save.h"
#include "config.h"

#ifndef UI_H_
#define UI_H_

#define UI_WIDGET_STATE_COUNT 3
enum UI_WIDGET_STATES : u8 {
	WG_DEFAULT,
	WG_HOVERED,
	WG_PRESSED
};

typedef struct {
	Color background[UI_WIDGET_STATE_COUNT];
	Color text[UI_WIDGET_STATE_COUNT];
	Color outline[UI_WIDGET_STATE_COUNT];
	
} UiStyle;

#define UI_ACTIVE			0x01
#define UI_START_GAME_REQ	0x02
#define UI_EXIT_GAME_REQ	0x04
#define UI_TOGGLE_REQ		0x08
#define UI_LOAD_SAVE_REQ	0x10
#define UI_SAVE_GAME_REQ	0x20

enum UI_TABS : u8 {
	TAB_NONE	= 0,
	TAB_LOAD	= 1,
	TAB_OPTIONS	= 2,
};

typedef struct {
	Font font;
	UiStyle style;

	Config *conf;
	char save_name[64];
	char map_name[64];

	Vector2 scale; 
	Vector2 cursor_pos;

	float font_size;
	float font_spacing;

	u8 active_tab;
	u8 flags;

} UiHandler;

void ui_Init(UiHandler *ui, Config *conf);
void ui_Update(UiHandler *ui, float dt);

void ui_Toggle();

void ui_DisplayCursor(UiHandler *ui);

u8 ui_Button(UiHandler *ui, Rectangle rec, const char *text, bool bg_rec);
u8 ui_ButtonEx(UiHandler *ui, Rectangle rec, const char *text, bool bg_rec, float font_size);

Vector2 ui_TextCenter(UiHandler *ui, Rectangle rec, const char *text);
Vector2 ui_TextCenterEx(UiHandler *ui, Rectangle rec, const char *text, float font_size, float font_spacing);

void ui_TitleUpdate(UiHandler *ui);
void ui_PauseUpdate(UiHandler *ui);

void ui_LoadTab(UiHandler *ui);
void ui_OptionsTab(UiHandler *ui);

u8 ui_InitRwData(UiHandler *ui);

#endif

