#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"
#include "ui.h"
#include "../include/log_message.h"

UiHandler *ui_self_ptr = NULL;

bool rw_data_init = false;
int num_save_entries = 0;
rw_Meta *meta = NULL;

float save_scroll = 0.0f;

void ui_Init(UiHandler *ui, Config *conf) {
	// Bind ui self pointer
	ui_self_ptr = ui;

	ui->conf =  conf;

	ui->font_size = 60.0f;
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

	ui_self_ptr->active_tab = TAB_NONE;
}

void ui_DisplayCursor(UiHandler *ui) {
	
}

u8 ui_Button(UiHandler *ui, Rectangle rec, const char *text, bool bg_rec) {
	u8 state = 0;
	bool hover = CheckCollisionPointRec(ui->cursor_pos, rec);

	if(hover) {
		state = WG_HOVERED;
		
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			state = WG_PRESSED;
	}

	Vector2 text_pos = (Vector2) { rec.x, rec.y };

	if(bg_rec) {
		DrawRectangleRec(rec, ui->style.background[state]);
		DrawRectangleLinesEx(rec, 2, ui->style.outline[state]);
		text_pos = ui_TextCenter(ui, rec, text);
	}
	
	DrawTextEx(ui->font, text, text_pos, ui->font_size, ui->font_spacing, ui->style.text[state]);

	return (state == WG_PRESSED) ? 1 : 0;
}

u8 ui_ButtonEx(UiHandler *ui, Rectangle rec, const char *text, bool bg_rec, float font_size) {
	u8 state = 0;
	bool hover = CheckCollisionPointRec(ui->cursor_pos, rec);

	if(hover) {
		state = WG_HOVERED;
		
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			state = WG_PRESSED;
	}

	Vector2 text_pos = (Vector2) { rec.x, rec.y };

	if(bg_rec) {
		DrawRectangleRec(rec, ui->style.background[state]);
		DrawRectangleLinesEx(rec, 2, ui->style.outline[state]);
		text_pos = ui_TextCenterEx(ui, rec, text, font_size, ui->font_spacing);
	}
	
	DrawTextEx(ui->font, text, text_pos, font_size, ui->font_spacing, ui->style.text[state]);

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
	ClearBackground(BLACK);

	if(!(ui->flags & UI_ACTIVE))
		ui_Toggle();

	ui->cursor_pos = GetMousePosition();

	if(IsKeyPressed(KEY_SPACE))	
		ui->flags |= UI_START_GAME_REQ;

	if(ui_Button(ui, (Rectangle) { 128, 1080 * 0.25f, 300, 100 }, "New Game", false))
		ui->flags |= UI_START_GAME_REQ;
}

void ui_PauseUpdate(UiHandler *ui) {
	ClearBackground(BLANK);

	ui->cursor_pos = GetMousePosition();

	float width = 250;
	float height = 90;
	float top = 1080 * 0.33f;

	if(ui_Button(ui, (Rectangle) { 128, top, width, height }, "Resume", false))
		ui->flags |= UI_TOGGLE_REQ;

	if(ui_Button(ui, (Rectangle) { 128, top + ((height+20) * 1), width, height }, "Load Game", false)) {
		ui->active_tab = (ui->active_tab == 0) ? TAB_LOAD : TAB_NONE;
		save_scroll = 0.0f;
		rw_data_init = false;
	}

	if(ui_Button(ui, (Rectangle) { 128, top + ((height+20) * 2), width, height }, "Save Game", false)) {
	}

	if(ui_Button(ui, (Rectangle) { 128, top + ((height+20) * 3), width, height }, "Options", false)) {
		ui->active_tab = (ui->active_tab == 0) ? TAB_OPTIONS : TAB_NONE;
	}

	if(ui_Button(ui, (Rectangle) { 128, top + ((height+20) * 4), width, height }, "Exit", false)) {
		ui->flags |= UI_EXIT_GAME_REQ;
	}

	switch(ui->active_tab) {
		case TAB_LOAD:
			ui_LoadTab(ui);
			break;

		case TAB_OPTIONS:
			ui_OptionsTab(ui);
			break;
	}
}

void ui_LoadTab(UiHandler *ui) {
	ui_InitRwData(ui);
	
	float width = 300;
	float height = 100;
	float left = ui->conf->window_width * 0.5f - width;
	float top = ui->conf->window_height * 0.15f; 

	float bot = ui->conf->window_height - (ui->conf->window_height * 0.15f);

	save_scroll += GetMouseWheelMove() * 2.0f;

	for(int i = num_save_entries-1; i > 0; i--) {
		int y = (top + ((height * (num_save_entries - i) ) + 20)) + save_scroll;

		if(y < top || y > bot)
			continue;

		const char *text = TextFormat("%s", ctime(&meta[i].time_stamp));
		//const char *text = TextFormat("%s", meta[i].map);
		
		if(ui_ButtonEx(ui, (Rectangle) { left, y, width, height }, text, false, ui->font_size * 0.5f)) {
			memcpy(ui->save_name, meta[i].name, 64);
			memcpy(ui->map_name, meta[i].map, 64);
			ui->flags |= UI_LOAD_SAVE_REQ;
		}
	}
}

void ui_OptionsTab(UiHandler *ui) {

}

u8 ui_InitRwData(UiHandler *ui) {
	if(rw_data_init)
		return 0;
	
	if(meta)	
		free(meta);

	// Open metadata file
	char *meta_path = "data/svd_meta";
	FILE *pF = fopen(meta_path, "rb"); 
	if(!pF) {
		MessageError("ERROR: Save data meta file does not exist ", meta_path);
		return 0;
	}

	// Calculate number of save entries
	num_save_entries = GetFileLength(meta_path) / sizeof(rw_Meta);
	
	// Read contents
	meta = malloc(sizeof(rw_Meta) * num_save_entries);
	fread(meta, sizeof(rw_Meta) * num_save_entries, 1, pF);

	fclose(pF);
	
	rw_data_init = true;
	return 1;
}

