#include "../include/num_redefs.h"
#include "raylib.h"

#ifndef UI_H_
#define UI_H_

#define UI_ACTIVE			0x01
#define UI_START_GAME_REQ	0x02
#define UI_EXIT_GAME_REQ	0x02
typedef struct {
	Font font;

	Vector2 scale; 
	Vector2 cursor_pos;

	float font_size;
	float font_spacing;

	u8 flags;

} UiHandler;

void ui_Toggle();

void ui_Init(UiHandler *ui);
void ui_Update(UiHandler *ui, float dt);

void ui_DisplayCursor(UiHandler *ui);

u8 ui_Button(UiHandler *ui, Rectangle rect);

Vector2 ui_TextCenter(UiHandler *ui, Rectangle rec, const char *text);
Vector2 ui_TextCenterEx(UiHandler *ui, Rectangle rec, const char *text, float font_size, float font_spacing);

#endif
