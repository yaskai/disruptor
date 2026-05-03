#include <stdlib.h>
#include "raylib.h"
#include "ui.h"

UiHandler *ui_self_ptr = NULL;

void ui_Init(UiHandler *ui) {
	// Bind ui self pointer
	ui_self_ptr = ui;

	ui->font_size = 30.0f;
	ui->font_spacing = 1.0f;
	ui->font = LoadFontEx("resources/fonts/shuretech.ttf", 64, NULL, 0);
	
	ui->style = (UiStyle) {
		.background = { BLACK, GRAY, GRAY },
		.text = { GRAY, WHITE, WHITE },
		.outline = { GRAY, WHITE, WHITE }
	};
}

void ui_Update(UiHandler *ui, float dt) {
	// Do nothing if ui is inactive
	if(!(ui->flags & UI_ACTIVE))	
		return;

	ui->cursor_pos = GetMousePosition();
	ui_DisplayCursor(ui);
}

void ui_Toggle() {
	// Toggle active flag
	ui_self_ptr->flags ^= UI_ACTIVE;

	if(ui_self_ptr->flags & UI_ACTIVE) {
		// Enable and hide OS cursor on activation
		EnableCursor();
		//HideCursor();
		GetMouseDelta();
	} else {
		// Disable OS cursor on deactivation
		DisableCursor();
		GetMouseDelta();
	}
}

void ui_DisplayCursor(UiHandler *ui) {
	
}

u8 ui_Button(UiHandler *ui, Rectangle rec, const char *text) {
	u8 state = 0;
	bool hover = CheckCollisionPointRec(ui->cursor_pos, rec);

	if(hover) {
		state = WG_HOVERED;
		
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			state = WG_PRESSED;
	}

	DrawRectangleRec(rec, ui->style.background[state]);
	DrawRectangleLinesEx(rec, 2, ui->style.outline[state]);
	
	Vector2 text_pos = ui_TextCenter(ui, rec, text);
	DrawTextEx(ui->font, text, text_pos, ui->font_size, ui->font_spacing, ui->style.text[state]);

	return (state == WG_PRESSED) ? 1 : 0;
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

void ui_TitleUpdate(UiHandler *ui) {
	if(!(ui->flags & UI_ACTIVE))
		ui_Toggle();

	ui->cursor_pos = GetMousePosition();

	if(IsKeyPressed(KEY_SPACE))	
		ui->flags |= UI_START_GAME_REQ;

	if(ui_Button(ui, (Rectangle) { 128, 1080 * 0.25f, 300, 100 }, "New Game"))
		ui->flags |= UI_START_GAME_REQ;
}

void ui_PauseUpdate(UiHandler *ui) {
}


