#include <stdlib.h>
#include "raylib.h"
#include "ui.h"

UiHandler *ui_self_ptr = NULL;

void ui_Toggle() {
	// Toggle active flag
	ui_self_ptr->flags ^= UI_ACTIVE;

	if(ui_self_ptr->flags & UI_ACTIVE) {
		// Enable and hide OS cursor on activation
		EnableCursor();
		HideCursor();
	} else {
		// Disable OS cursor on deactivation
		DisableCursor();
	}
}

void ui_Init(UiHandler *ui) {
	// Bind ui self pointer
	ui_self_ptr = ui;

	ui->font_size = 60.0f;
	ui->font_spacing = 1.0f;
	ui->font = LoadFontEx("resources/fonts/shuretech.ttf", 64, NULL, 0);
}

void ui_Update(UiHandler *ui, float dt) {
	// Do nothing if ui is inactive
	if(!(ui->flags & UI_ACTIVE))	
		return;

	ui->cursor_pos = GetMousePosition();
	ui_DisplayCursor(ui);
}

void ui_DisplayCursor(UiHandler *ui) {
	
}

u8 ui_Button(UiHandler *ui, Rectangle rect) {
	bool hover = CheckCollisionPointRec(ui->cursor_pos, rect);

	if(hover) {
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			return 1;
	}	

	return 0;
}

Vector2 ui_TextCenter(UiHandler *ui, Rectangle rec, const char *text) {
	Vector2 rec_mid = { rec.x + rec.width * 0.5f, rec.y + rec.height * 0.5f };
	Vector2 text_bounds = MeasureTextEx(ui->font, text, ui->font_size, ui->font_spacing);

	return (Vector2) { rec_mid.x - text_bounds.x * 0.5f, rec_mid.y - text_bounds.y * 0.5f };
}

Vector2 ui_TextCenterEx(UiHandler *ui, Rectangle rec, const char *text, float font_size, float font_spacing) {
	Vector2 rec_mid = { rec.x + rec.width * 0.5f, rec.y + rec.height * 0.5f };
	Vector2 text_bounds = MeasureTextEx(ui->font, text, font_size, font_spacing);

	return (Vector2) { rec_mid.x - text_bounds.x * 0.5f, rec_mid.y - text_bounds.y * 0.5f };
}

