#include "../include/num_redefs.h"
#include "raylib.h"

#ifndef UI_H_
#define UI_H_

#define UI_ACTIVE	0x01
typedef struct {
	Vector2 scale; 
	Vector2 cursor_pos;

	u8 flags;

} UiHandler;

void ui_Toggle();
void ui_Update(UiHandler *ui, float dt);

void ui_DisplayCursor(UiHandler *ui);

u8 ui_Button(UiHandler *ui, Rectangle rect);

#endif
