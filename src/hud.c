#include <stdlib.h>
#include "raylib.h"
#include "hud.h"

#define TEXT_GREEN (Color) { 98, 234, 201, 255 }
#define TEXT_ORANGE (Color) { 255, 165, 80, 255 }

int *hud_cp = NULL;

void HudInit(Hud *hud, Config *conf, EntityHandler *handler, PlayerGun *player_gun) {
	hud->conf = conf;
	hud->handler = handler;
	hud->player_gun = player_gun;

	hud_cp = malloc((96 + 32) * sizeof(int));
	for(int i = 0; i < 96; i++)
		hud_cp[i] = 32 + i;

	hud_cp[96] = 0xf21e;
	hud_cp[97] = 0xf05f6;
	hud_cp[98] = 0xf0b7a;
	hud->font = LoadFontEx("resources/fonts/shuretech.ttf", 64, hud_cp, 99);
	SetTextureFilter(hud->font.texture, TEXTURE_FILTER_TRILINEAR);

	hud->font_size = 80.0f;
	hud->font_spacing = 1.0f;

	hud->text_shader = LoadShader("resources/shaders/ui_text_v.glsl", "resources/shaders/ui_text_f.glsl");
}

void HudUpdate(Hud *hud, float dt) {
	float t = GetTime();
	SetShaderValue(hud->text_shader, GetShaderLocation(hud->text_shader, "time"), &t, SHADER_UNIFORM_FLOAT);

	if((hud->handler->flags & AT_LEVEL_END) || (hud->handler->flags & AT_LEVEL_BACK)) {
		DisplayLoadIndicator(hud);
		return;
	}

	DisplayAmmoCounter(hud);
	DisplayHealthCounter(hud);

	Vector3 player_pos = hud->handler->ents[hud->handler->player_id].comp_transform.position;

	i16 text_obj_id = -1;
	for(i16 i = 0; i < hud->handler->text_obj_count; i++) {
		TextObject *text_obj = &hud->handler->text_objs[i];

		if(CheckCollisionSpheres(player_pos, 64, text_obj->position, 64)) {
			text_obj_id = i;
		}
	}

	if(text_obj_id > -1)
		DisplayTextObject(hud, text_obj_id);
}

void DisplayAmmoCounter(Hud *hud) {
	Entity *player = &hud->handler->ents[hud->handler->player_id];
	comp_Weapon *weap = &player->comp_weapon;

	const char *text = TextFormat("%.02d / %.03d", weap->in_clip, weap->ammo);

	Rectangle rec = (Rectangle) {
		.x = 1600,
		.y = hud->conf->window_height - 120,
		.width = 300,
		.height = 90

	};
	DisplayBackgroundRec(hud, rec);

	Vector2 text_pos = HudTextCenter(hud, rec, text);

	BeginShaderMode(hud->text_shader);
	DrawTextEx(
		hud->font,
		text,
		text_pos,
		hud->font_size,
		hud->font_spacing, 
		ColorAlpha(TEXT_GREEN, 0.85f)
	);
	EndShaderMode();
}

void DisplayHealthCounter(Hud *hud) {
	Entity *player = &hud->handler->ents[hud->handler->player_id];
	comp_Health *health = &player->comp_health;

	const char *text = TextFormat("󰗶%.03d", health->amount);

	Rectangle rec = (Rectangle) {
		.x = 32,
		.y = hud->conf->window_height - 120,
		.width = 300,
		.height = 90

	};
	DisplayBackgroundRec(hud, rec);

	Vector2 text_pos = HudTextCenter(hud, rec, text);

	BeginShaderMode(hud->text_shader);
	DrawTextEx(
		hud->font,
		text,
		text_pos,
		hud->font_size,
		hud->font_spacing, 
		ColorAlpha(TEXT_GREEN, 0.75f)
	);
	EndShaderMode();
};

void DisplayLoadIndicator(Hud *hud) {
	const char *text = TextFormat("loading...");

	Rectangle rec = (Rectangle) {
		.x = hud->conf->window_width * 0.5f - 125,
		.y = hud->conf->window_height * 0.5f,
		.width = 250,
		.height = 70

	};
	DisplayBackgroundRec(hud, rec);

	Vector2 text_pos = HudTextCenterEx(hud, rec, text, hud->font_size * 0.5f, hud->font_spacing); 

	BeginShaderMode(hud->text_shader);
	DrawTextEx(
		hud->font,
		text,
		text_pos,
		hud->font_size * 0.5f,
		hud->font_spacing, 
		ColorAlpha(TEXT_GREEN, 0.85f)
	);
	EndShaderMode();
}

void DisplayBackgroundRec(Hud *hud, Rectangle rec) {
	Color color = ColorAlpha(BLACK, 0.5f);
	DrawRectangleRounded(rec, 0.5f, 4, color);
}

Vector2 HudTextCenter(Hud *hud, Rectangle rec, const char *text) {
	Vector2 rec_mid = { rec.x + rec.width * 0.5f, rec.y + rec.height * 0.5f };
	Vector2 text_bounds = MeasureTextEx(hud->font, text, hud->font_size, hud->font_spacing);

	return (Vector2) { rec_mid.x - text_bounds.x * 0.5f, rec_mid.y - text_bounds.y * 0.5f };
}

Vector2 HudTextCenterEx(Hud *hud, Rectangle rec, const char *text, float font_size, float font_spacing) {
	Vector2 rec_mid = { rec.x + rec.width * 0.5f, rec.y + rec.height * 0.5f };
	Vector2 text_bounds = MeasureTextEx(hud->font, text, font_size, font_spacing);

	return (Vector2) { rec_mid.x - text_bounds.x * 0.5f, rec_mid.y - text_bounds.y * 0.5f };
}

void DisplayTextObject(Hud *hud, i16 id) {
	TextObject *text_obj = &hud->handler->text_objs[id];
	const char *text = TextFormat("󰭺%s", text_obj->text);

	Rectangle rec = (Rectangle) {
		.x = 32,
		.y = hud->conf->window_height - 220,
		.width = 300,
		.height = 90

	};
	DisplayBackgroundRec(hud, rec);

	Vector2 text_pos = HudTextCenterEx(hud, rec, text, hud->font_size * 0.5f, hud->font_spacing); 

	BeginShaderMode(hud->text_shader);
	DrawTextEx(
		hud->font,
		text,
		text_pos,
		hud->font_size * 0.5f,
		hud->font_spacing, 
		ColorAlpha(TEXT_ORANGE, 0.85f)
	);
	EndShaderMode();
}

