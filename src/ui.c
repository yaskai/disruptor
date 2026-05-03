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
Texture2D save_img;
bool save_img_init = false;
int save_focus = -1;

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

	float width = 250;
	float height = 90;
	float left = 128;
	float top = 1080 * 0.25f;

	ui->cursor_pos = GetMousePosition();

	if(ui_Button(ui, (Rectangle) { left, top, width, height }, "New Game", false)) {
		ui->flags |= UI_START_GAME_REQ;
	}

	if(ui_Button(ui, (Rectangle) { left, top + ((height+20) * 1), width, height }, "Load Game", false)) {
		ui->active_tab = (ui->active_tab == TAB_LOAD) ? TAB_NONE : TAB_LOAD;
		save_scroll = 0.0f;
		save_focus = -1;
		rw_data_init = false;
		save_img_init = false;
	}

	if(ui_Button(ui, (Rectangle) { left, top + ((height+20) * 2), width, height }, "Exit", false)) {
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

void ui_PauseUpdate(UiHandler *ui) {
	ClearBackground(BLANK);

	ui->cursor_pos = GetMousePosition();

	float width = 250;
	float height = 90;
	float top = 1080 * 0.33f;

	if(ui_Button(ui, (Rectangle) { 128, top, width, height }, "Resume", false))
		ui->flags |= UI_TOGGLE_REQ;

	if(ui_Button(ui, (Rectangle) { 128, top + ((height+20) * 1), width, height }, "Load Game", false)) {
		ui->active_tab = (ui->active_tab == TAB_LOAD) ? TAB_NONE : TAB_LOAD;
		save_scroll = 0.0f;
		save_focus = -1;
		rw_data_init = false;
		save_img_init = false;
	}

	if(ui_Button(ui, (Rectangle) { 128, top + ((height+20) * 2), width, height }, "Save Game", false)) {
		ui->flags |= UI_SAVE_GAME_REQ; 
	}

	if(ui_Button(ui, (Rectangle) { 128, top + ((height+20) * 3), width, height }, "Options", false)) {
		ui->active_tab = (ui->active_tab == TAB_OPTIONS) ? TAB_NONE : TAB_OPTIONS;
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
	
	float width = 320;
	float height = 64;
	float left = ui->conf->window_width * 0.5f - width;
	float top = ui->conf->window_height * 0.15f; 

	float bot = ui->conf->window_height - (ui->conf->window_height * 0.15f);

	save_scroll += GetMouseWheelMove() * 2.0f;
	save_scroll = Clamp(save_scroll, -num_save_entries * (height*0.5f), 0.0f);

	for(int i = num_save_entries-1; i > 0; i--) {
		int y = (top + ((height * (num_save_entries - i) ) + 4)) + save_scroll;

		if(y - height < top || y + height > bot)
			continue;

		const char *text = TextFormat("%s", ctime(&meta[i].time_stamp));

		DrawLineV((Vector2) { left, y }, (Vector2) { left + width, y }, RAYWHITE);
		
		if(ui_ButtonEx(ui, (Rectangle) { left, y, width, height }, text, false, ui->font_size * 0.5f)) {
			if(i != save_focus)
				save_img_init = false;

			save_focus = i;
		}
	}

	if(save_focus > -1) {
		if(!save_img_init) {
			save_img = LoadTexture(TextFormat("data/%s/cap.png", meta[save_focus].name));
			SetTextureFilter(save_img, TEXTURE_FILTER_TRILINEAR);
			save_img_init = true;
		}

		Vector2 img_pos = (Vector2) { left + width + (width*0.25f), top + height };
		DrawTextureEx(save_img, img_pos, 0, 1.25f, WHITE);

		Rectangle btn_rec = (Rectangle) { img_pos.x, img_pos.y + (save_img.height * 1.25f), save_img.width * 1.25f, 64 };

		if(ui_ButtonEx(ui, btn_rec, "load", true, ui->font_size)) {
			memcpy(ui->save_name, meta[save_focus].name, 64);
			memcpy(ui->map_name, meta[save_focus].map, 64);
			ui->flags |= UI_LOAD_SAVE_REQ;
		}
	}
}

void ui_OptionsTab(UiHandler *ui) {
	float width = 400;
	float height = 64;
	float left = ui->conf->window_width * 0.475f - width;
	float top = ui->conf->window_height * 0.15f; 
	float bot = ui->conf->window_height - (ui->conf->window_height * 0.25f);

	Rectangle panel_rec = (Rectangle) { left, top, left + width, bot };

	DrawRectangleLinesEx(panel_rec, 2, RAYWHITE);
	
	const char *text = "under construction...";
	Vector2 text_pos = ui_TextCenter(ui, panel_rec, text);
	DrawTextEx(ui->font, text, text_pos, ui->font_size, ui->font_spacing, WHITE);
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

