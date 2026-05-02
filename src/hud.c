#include "raylib.h"
#include "hud.h"

#define TEXT_GREEN (Color) { 98, 234, 201, 255 }

void HudInit(Hud *hud, Config *conf, EntityHandler *handler, PlayerGun *player_gun) {
	hud->conf = conf;
	hud->handler = handler;
	hud->player_gun = player_gun;

	hud->font = LoadFontEx("resources/fonts/shuretech.ttf", 64, NULL, 0);
	SetTextureFilter(hud->font.texture, TEXTURE_FILTER_TRILINEAR);

	hud->font_size = 60.0f;
	hud->font_spacing = 1.0f;
}

void HudUpdate(Hud *hud, float dt) {
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

	const char *text = TextFormat("%d | %d", weap->in_clip, weap->ammo);

	Rectangle rec = (Rectangle) {
		.x = 1600,
		.y = hud->conf->window_height - 120,
		.width = 300,
		.height = 90

	};
	DisplayBackgroundRec(hud, rec);

	Vector2 text_pos = HudTextCenter(hud, rec, text);

	DrawTextEx(
		hud->font,
		text,
		text_pos,
		hud->font_size,
		hud->font_spacing, 
		ColorAlpha(TEXT_GREEN, 0.75f)
	);
}

void DisplayHealthCounter(Hud *hud) {
	Entity *player = &hud->handler->ents[hud->handler->player_id];
	comp_Health *health = &player->comp_health;

	const char *text = TextFormat("+%d", health->amount);

	Rectangle rec = (Rectangle) {
		.x = 32,
		.y = hud->conf->window_height - 120,
		.width = 300,
		.height = 90

	};
	DisplayBackgroundRec(hud, rec);

	Vector2 text_pos = HudTextCenter(hud, rec, text);

	DrawTextEx(
		hud->font,
		text,
		text_pos,
		hud->font_size,
		hud->font_spacing, 
		ColorAlpha(TEXT_GREEN, 0.75f)
	);
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

	DrawTextEx(
		hud->font,
		text,
		text_pos,
		hud->font_size * 0.5f,
		hud->font_spacing, 
		ColorAlpha(TEXT_GREEN, 0.75f)
	);
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
	const char *text = TextFormat("%s", text_obj->text);

	Rectangle rec = (Rectangle) {
		.x = 32,
		.y = hud->conf->window_height - 220,
		.width = 300,
		.height = 90

	};
	DisplayBackgroundRec(hud, rec);

	Vector2 text_pos = HudTextCenterEx(hud, rec, text, hud->font_size * 0.5f, hud->font_spacing); 

	DrawTextEx(
		hud->font,
		text,
		text_pos,
		hud->font_size * 0.5f,
		hud->font_spacing, 
		ColorAlpha(TEXT_GREEN, 0.75f)
	);
}

