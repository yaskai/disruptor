#include "../include/num_redefs.h"
#include "raylib.h"

#ifndef UI_H_
#define UI_H_

#define UI_WIDGET_STATE_COUNT 3
enum ui_widget_states : u8 {
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
#define UI_EXIT_GAME_REQ	0x02
typedef struct {
	Font font;

	UiStyle style;

	Vector2 scale; 
	Vector2 cursor_pos;

	float font_size;
	float font_spacing;

	u8 flags;

} UiHandler;


void ui_Init(UiHandler *ui);
void ui_Update(UiHandler *ui, float dt);

void ui_Toggle();

void ui_DisplayCursor(UiHandler *ui);

u8 ui_Button(UiHandler *ui, Rectangle rec, const char *text);

Vector2 ui_TextCenter(UiHandler *ui, Rectangle rec, const char *text);
Vector2 ui_TextCenterEx(UiHandler *ui, Rectangle rec, const char *text, float font_size, float font_spacing);

void ui_TitleUpdate(UiHandler *ui);
void ui_PauseUpdate(UiHandler *ui);

#endif
