#include <math.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "player_gun.h"
#include "geo.h"
#include "ent.h"
#include "v_effect.h"
#include "config.h"
#include "audioplayer.h"

#define USE_MWHEEL (true)

Vector3 gun_pos = {0};
float gun_rot = 0;
float p, y, r;
Matrix mat = {0};

float gun_angle = 0;

#define LOCAL_UP (Vector3) { 0, 1, 0 }

#define REVOLVER_REST (Vector3) { -0.85f, -2.1f, 5.0f }
//#define REVOLVER_REST (Vector3) { -0.75f, 6.25f, -2.35f }
#define REVOLVER_ANGLE_REST 2.5f

#define PISTOL_REST (Vector3) { -1.15f, -2.65f, 6.25f }
//#define PISTOL_REST (Vector3) { -1.15f, 6.25f, -2.35f }
#define PISTOL_ANGLE_REST 0.1f

#define DISRUPTOR_REST (Vector3) {  -1.75f, -1.35f, 6.25f  }
//#define DISRUPTOR_REST (Vector3) { -1.75f, 6.25f, -1.35f }
//#define DISRUPTOR_REST (Vector3) { 1.75f, 6.25f, -1.35f }
#define DISRUPTOR_REST_ANGLE_REST 2.5f

#define DISRUPTOR_THROW_FORCE 450.0f

float recoil = 0.0f;
float cam_recoil = 0.0f;
bool recoil_add = false;

float recoil_force = 0.0f;
float friction = 0.0f;

Vector2 sway = { 0, 0 };
float sway_t = 0;

float init_t = 1.0f;

typedef struct {
	EntityHandler *handler;	
	MapSection *sect;
	Entity *player;
	vEffect_Manager *effect_manager;
	Config *conf;
	Camera3D *world_cam;
	AudioPlayer *ap;

} PlayerGunRefs;
PlayerGunRefs gun_refs = {0};

comp_Weapon *curr_gun = NULL;

comp_Weapon weapons[] = {
	// Disruptor
	(comp_Weapon) {
		.id = WEAP_DISRUPTOR,
		.damage = 0,

		.clip_size = 0,
		.in_clip = 0,
		.ammo = 0,

		.reload_time_amnt = 100,
		.reload_timer = 0,
	},
	// Revolver
	(comp_Weapon) {
		.id = WEAP_REVOLVER,
		.damage = 4,

		.clip_size = 6,
		.in_clip = 6,
		//.ammo = 12,
		.ammo = 999,

		.reload_time_amnt = 8,
		.reload_timer = 0,
	},
	// Pistol
	(comp_Weapon) {
		.id = WEAP_PISTOL,
		.damage = 1,

		.clip_size = 12,
		.in_clip = 12,
		.ammo = 24,

		.reload_time_amnt = 2,
		.reload_timer = 0,
	},
	// Shotgun
	(comp_Weapon) {
		.id = WEAP_SHOTGUN,
		.damage = 3,

		.clip_size = 8,
		.in_clip = 8,
		.ammo = 24,

		.reload_time_amnt = 4,
		.reload_timer = 0,
	},
};

Model models[4];
Matrix gun_matrix;

ModelAnimation anims[4];
AnimState anim_states[4];

bool reload_active = false;
bool reload_sound_set = false;

char *gun_shoot_sounds[4][3] = {
	// Disruptor
	{ "" },					
	// Revolver
	{ "pistol", "pistol2", "pistol3" },
	// Pistol
	{ "" },
	// Pistol
	{ "" },
};

enum CROSSHAIR_IDS : short {
	CROSSHAIR_DEFAULT 		= 0,
	CROSSHAIR_PROJECTILE	= 1,
};

Texture2D crosshair_textures[4];
void LoadCrosshairTextures() {
	crosshair_textures[0] = LoadTexture("resources/ui/crosshair/default.png");
	crosshair_textures[1] = LoadTexture("resources/ui/crosshair/arrow.png");
}

Texture2D muz_flash;
Vector2 muz_pos;
float muz_rot[12] = {0};

bool freeze_frame = false;

Font hud_font;

void DrawSectionTransition();

Color weap_tint = WHITE;

void PlayerGunInit(
	PlayerGun *player_gun,
	Entity *player,
	EntityHandler *handler,
	MapSection *sect,
	vEffect_Manager *effect_manager,
	Config *conf,
	Camera3D *world_cam,
	AudioPlayer *ap
	) 
{
	gun_refs.world_cam = world_cam;
	gun_refs.ap = ap;

	player_gun->cam = (Camera3D) {
		.position = (Vector3) { 0, 0, -1 },
		.target = (Vector3) { 0, 0, 1 },
		.up = LOCAL_UP,
		.fovy = 54,
		.projection = CAMERA_PERSPECTIVE
	};

	models[WEAP_PISTOL] 	= LoadModel("resources/models/weapons/pistol_00.glb");
	models[WEAP_SHOTGUN] 	= LoadModel("resources/models/weapons/pistol_00.glb");
	//models[WEAP_REVOLVER] 	= LoadModel("resources/models/weapons/rev_00.glb");
	models[WEAP_REVOLVER] 	= LoadModel("resources/models/weapons/rev_00_e.glb");
	//models[WEAP_DISRUPTOR] 	= LoadModel("resources/models/weapons/bug_00.glb");
	models[WEAP_DISRUPTOR] 	= LoadModel("resources/models/weapons/bug_01.glb");

	int rev_num_anims = 0;
	anims[WEAP_REVOLVER] = *LoadModelAnimations("resources/models/weapons/rev_00_e.glb", &rev_num_anims);
	anim_states[WEAP_REVOLVER] = anim_Init(models[WEAP_REVOLVER]);
	anim_Switch(&anim_states[WEAP_REVOLVER], 0);

	gun_pos = REVOLVER_REST;
	gun_rot = REVOLVER_ANGLE_REST;

	gun_refs.player = player;
	gun_refs.sect = sect;
	gun_refs.handler = handler;
	gun_refs.effect_manager = effect_manager;
	gun_refs.conf = conf;

	//player->comp_weapon.id = WEAP_DISRUPTOR;
	//player_gun->current_gun = WEAP_DISRUPTOR;
	player_gun->current_gun = WEAP_REVOLVER;
	curr_gun = &weapons[player_gun->current_gun];

	player_gun->model = models[player_gun->current_gun];
	mat = player_gun->model.transform;

	LoadCrosshairTextures();

	muz_flash = LoadTexture("resources/fx/muz.png");
	//hud_font = LoadFont("resources/fonts/shuretech.ttf");
	hud_font = LoadFontEx("resources/fonts/shuretech.ttf", 64, NULL, 0);
	SetTextureFilter(hud_font.texture, TEXTURE_FILTER_POINT);

	//sway_t = GetTime();

	init_t = 0.0f;
}

void PlayerGunUpdate(PlayerGun *player_gun, float dt) {
	init_t -= dt;
	if(init_t > 0)
		return;

	Vector3 hvel = (Vector3) { gun_refs.player->comp_transform.velocity.x, gun_refs.player->comp_transform.velocity.y, 0 };
	float vel_len = Vector3Length(hvel);

	Bsp_TraceData tr = Bsp_TraceDataEmpty();
	Bsp_Hull *hull = &gun_refs.sect->bsp_data.hull_groups[0].hulls[1];

	Vector3 tr_start = gun_refs.player->comp_transform.position;
	Vector3 tr_dest = Vector3Add(tr_start, Vector3Scale(DOWN, 10));

	Bsp_RecursiveTraceEx(hull, hull->first_node, 0, 1, tr_start, tr_dest, &tr);
	bool ground = (tr.fraction < 1);

	bool plr_crouch = (fabsf(gun_refs.player->comp_transform.position.z - gun_refs.world_cam->position.z) <= 8.0f);

	if(vel_len >= 10.1f && (ground || gun_refs.player->comp_transform.on_ground)) {
		float lt = (plr_crouch) ? 0.35f : 1.0f;
		float y_lt = (plr_crouch) ? 0.35f : 1.0f;

		float tX = sinf(sway_t * 7) * (((0.0125f * lt) + (vel_len * 0.0002f)));
		float tY = sinf(sway_t * 14) * (((0.0125f * y_lt ) + (vel_len * 0.0002f)));

		sway.x = Lerp(sway.x, tX, dt*10);
		sway.y = Lerp(sway.y, tY, dt*10);

		sway_t += dt;

	} else {
		//sway = Vector2Lerp(sway, Vector2Zero(), 4*dt);

		if(gun_refs.player->comp_transform.velocity.z >= 50.0f) {
			sway.y = Lerp(sway.y, -0.5f,  2.5f*dt);
			//sway.x = Lerp(sway.x,  0.0f,  15*dt);
		} else if(gun_refs.player->comp_transform.velocity.z <= -100.0f) {
			sway.y = Lerp(sway.y,  0.2f,  2.5f*dt);
		} else { 
			sway.x = Lerp(sway.x, 0.0f, 2.5f*dt);
			sway.y = Lerp(sway.y, 0.0f, 2.5f*dt);
		}
	}

	Vector2 md = GetMouseDelta();
	if(fabsf(md.x) >= 1.5f)
		sway.x += (md.x * dt * 0.025f);

	if(fabsf(md.y) >= 1.5f)
		sway.y += (md.y * dt * 0.025f);

	curr_gun->cooldown -= dt;

	int scroll = 0;

	if(recoil <= 1.0f && !reload_active) {
		if(USE_MWHEEL)
			scroll = GetMouseWheelMove();

		if(IsKeyPressed(KEY_Q)) 
			scroll = -1;

		if(IsKeyPressed(KEY_E))
			scroll = +1;
	}

	int next_gun = player_gun->current_gun + scroll;
	player_gun->current_gun = (next_gun % 2 == 0) ? WEAP_DISRUPTOR : WEAP_REVOLVER;
	//gun_refs.player->comp_weapon = weapons[player_gun->current_gun];

	//gun_refs.player->comp_weapon.id = (gun_refs.player->comp_weapon.id + scroll) % 2;
	//player_gun->current_gun = gun_refs.player->comp_weapon.id;
	//gun_refs.player->comp_weapon = weapons[gun_refs.player->comp_weapon.id];

	curr_gun = &weapons[player_gun->current_gun];
	gun_refs.player->comp_weapon = *curr_gun;

	if(gun_refs.player->comp_ai.state == STATE_DEAD)
		return;

	switch(player_gun->current_gun) {
		case WEAP_PISTOL:
			PlayerGunUpdatePistol(player_gun, dt);
			break;

		case WEAP_SHOTGUN:
			PlayerGunUpdateShotgun(player_gun, dt);
			break;

		case WEAP_REVOLVER:
			PlayerGunUpdateRevolver(player_gun, dt);
			break;

		case WEAP_DISRUPTOR:
		 	PlayerGunUpdateDisruptor(player_gun, dt);
			break;
	}	

	if(player_gun->current_gun == WEAP_DISRUPTOR)
		return;

	if(IsKeyPressed(KEY_R) && curr_gun->ammo > 0 && !reload_active) { 
		reload_active = true;
		reload_sound_set = false;
	}

	if(reload_active) {
		PlayerGunReload(player_gun, dt);
	}
}

void PlayerGunUpdatePistol(PlayerGun *player_gun, float dt) {
	float recoil_angle = Clamp((recoil*0.75f) + gun_rot, -30, 90.0f);
	mat = MatrixRotateX(-recoil_angle * DEG2RAD);
	mat = MatrixMultiply(mat, MatrixRotateY(PISTOL_ANGLE_REST * DEG2RAD));

	//friction = (recoil > 40) ? 4.9f : 10.5f;
	friction = 17.5f;

	recoil -= (recoil * friction) * dt; 
	if(recoil <= -EPSILON) recoil = 0;

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && recoil <= 0.9f) {
		PlayerShootPistol(player_gun, gun_refs.handler, gun_refs.sect);

		recoil_add = false;
		recoil += 15 + (GetRandomValue(10, 20) * 0.01f);
	}

	gun_pos = PISTOL_REST;
	gun_pos.z = PISTOL_REST.z - (recoil * 0.1f);

	models[WEAP_PISTOL].transform = mat;
}

void PlayerGunUpdateShotgun(PlayerGun *player_gun, float dt) {
}

void PlayerGunUpdateRevolver(PlayerGun *player_gun, float dt) {
	//if(reload_active)
		//return;

	//float angle_targ = Clamp((recoil*1.0f) + gun_rot, -30, 60.0f);
	float recoil_angle = Clamp((recoil*10.35f), -30, 40.0f);

	/*
	if(freeze_frame)
		recoil_angle = 0;
		*/

	gun_angle = recoil_angle;
	gun_angle = Clamp(gun_angle, REVOLVER_ANGLE_REST, 30.0f);

	//mat = MatrixRotateX(-recoil_angle * DEG2RAD);
	mat = MatrixRotateX(-gun_angle * DEG2RAD);
	mat = MatrixMultiply(mat, MatrixRotateY(REVOLVER_ANGLE_REST * DEG2RAD));

	//float friction_targ = (recoil_angle >= 25.0f) ? 7.9f : 20.5f;
	float friction_targ = 10.0f;
	if(gun_angle >= 35.0f && recoil >= 80.0f) friction_targ *= 0.85f;
	friction = Lerp(friction, friction_targ, dt);

	recoil -= (recoil * friction) * dt; 
	if(recoil <= -EPSILON) recoil = 0;

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && recoil <= 1.0f && !reload_active) {
		PlayerShootRevolver(player_gun, gun_refs.handler, gun_refs.sect);
	}

	gun_pos = REVOLVER_REST;
	//float rest_ofs = (recoil * 0.04f);
	float rest_ofs = (recoil * 0.065f);

	//float rest_ofs = (recoil * 0.0554f);
	rest_ofs = Clamp(rest_ofs, 0.0f, 10.0f); 	
	gun_pos.z = REVOLVER_REST.z - (rest_ofs*0.55f);
	gun_pos.y = REVOLVER_REST.y + (rest_ofs*0.11f);

	//player_gun->cam.target.y = Lerp(player_gun->cam.target.y, -rest_ofs*0.2f, 5*dt);
	//player_gun->cam.position.y = player_gun->cam.target.y;
	//player_gun->cam.position.y = Lerp(player_gun->cam.target.y, rest_ofs*0.5f, 30*dt);

	models[WEAP_REVOLVER].transform = mat;

	//float lerp_t = (recoil_angle < 50) ? 30 : 30; 
	float lerp_t = (recoil >= 80.0f) ? 15 : 10; 
	//cam_recoil = Lerp(cam_recoil, recoil*0.0016f, dt*lerp_t);
	//cam_recoil = Lerp(cam_recoil, recoil*0.0013f, dt*lerp_t);
	//cam_recoil = Lerp(cam_recoil, recoil*0.0035f, dt*lerp_t);
	//cam_recoil = Lerp(cam_recoil, fmaxf(recoil*0.006f, (gun_angle*0.001f)), dt*lerp_t);
	cam_recoil = Lerp(cam_recoil, fmaxf(recoil*0.019f, (gun_angle*0.001f)), dt*lerp_t);
	//cam_recoil = Clamp(cam_recoil, 0.0f, 0.33f);
	PlayerSetRecoilInput(gun_refs.player, cam_recoil);

	freeze_frame = false;
}

void PlayerGunUpdateDisruptor(PlayerGun *player_gun, float dt) {
	gun_pos = DISRUPTOR_REST;

	mat = MatrixRotateX(-DISRUPTOR_REST_ANGLE_REST * DEG2RAD);
	mat = MatrixMultiply(mat, MatrixRotateY(DISRUPTOR_REST_ANGLE_REST * DEG2RAD));

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		PlayerShoot(player_gun, gun_refs.handler, gun_refs.sect);
	}

	//models[WEAP_DISRUPTOR].transform = MatrixMultiply(mat, MatrixRotateX(90*DEG2RAD));
	models[WEAP_DISRUPTOR].transform = mat;

	cam_recoil = 0;
}

void PlayerGunDraw(PlayerGun *player_gun) {
	/*
	if(gun_refs.handler->flags & AT_LEVEL_END) {
		ClearBackground(ColorAlpha(BLACK, 0.85f));

		DrawTextEx(
			hud_font,
			"MISSION STATUS: COMPLETE",
			(Vector2) { 0, 0 },
			80, 
			1, 
			PURPLE
		);

		DrawTextEx(
			hud_font,
			"PRESS [Y] TO PLAY AGAIN",
			(Vector2) { 0, 100 },
			80, 
			1, 
			PURPLE
		);

		DrawTextEx(
			hud_font,
			"PRESS [ESC] TO EXIT",
			(Vector2) { 0, 200 },
			80, 
			1, 
			PURPLE
		);

		return;
	}
	*/

	if(gun_refs.player->comp_ai.state == STATE_DEAD)
		return;

	Entity *bug_ent = &gun_refs.handler->ents[gun_refs.handler->bug_id];
	bool skip_draw = false;

	if(player_gun->current_gun == WEAP_DISRUPTOR) {
		if(bug_ent->comp_ai.state != BUG_DEFAULT) {
			skip_draw = true;
		}
	}

	if(player_gun->current_gun == WEAP_REVOLVER) {
		BeginBlendMode(BLEND_ADDITIVE);
		if(curr_gun->cooldown > 0) {
			//DrawTexture(muz_flash, 1920/2, 1080/2, WHITE);
			Vector3 world_pos = REVOLVER_REST;
			world_pos.x += 0.1f;
			world_pos.y += 1.25f;
			muz_pos = GetWorldToScreen(world_pos, player_gun->cam);
			muz_pos.x -= 16;
			muz_pos.y += 16;
			//muz_pos.y -= muz_flash.height * 0.5f;
			//DrawTextureEx(muz_flash, muz_pos, muz_rot, 1.0f, WHITE);

			for(short i = 0; i < 12; i++) {
				DrawTexturePro(
					muz_flash, 
					(Rectangle) { 0, 0, muz_flash.width, muz_flash.height },
					(Rectangle) { muz_pos.x, muz_pos.y, muz_flash.width, muz_flash.height },
					(Vector2) { muz_flash.width * 0.5f, muz_flash.height * 0.5f }, 
					muz_rot[i],
					ColorBrightness(ColorAlpha(WHITE, 0.25f), 1.5f)
				);
			}
		}
		EndBlendMode();
	}

	if(!skip_draw) {
		float scale = 1.0f;
		if(player_gun->current_gun == WEAP_DISRUPTOR)
			scale = 0.8f;

		Vector3 draw_pos = Vector3Add(gun_pos, Vector3Scale( (Vector3) { 1, 0, 0 }, sway.x));
		draw_pos = Vector3Add(draw_pos, Vector3Scale(LOCAL_UP, sway.y));
		draw_pos.z -= sway.y * 0.01f;

		/*
		Color clr = lit_SampleLightGrid(&gun_refs.sect->bsp_data, gun_refs.player->comp_transform.position);
		clr.r = Clamp(clr.r, 0, 255);
		clr.g = Clamp(clr.g, 0, 255);
		clr.b = Clamp(clr.b, 0, 255);
		weap_tint = ColorLerp(weap_tint, clr, GetFrameTime());
		weap_tint.a = 255;
		*/

		Vector3 sample_pos = Vector3Add(gun_refs.player->comp_transform.position, Vector3Scale(gun_refs.player->comp_transform.forward, 5.0f));
		Color light = lit_SampleLightGrid(&gun_refs.sect->bsp_data, sample_pos);
		light.r = Clamp(light.r, 70, 255);
		light.g = light.r;
		light.b = light.r;
		//light.g = Clamp(light.g, 60, 255);
		//light.b = Clamp(light.b, 60, 255);
		weap_tint = ColorLerp(weap_tint, light, GetFrameTime()*5);
		weap_tint.a = 255;

		BeginMode3D(player_gun->cam);
		DrawModel(models[player_gun->current_gun], draw_pos, scale, weap_tint);
		EndMode3D();
	}

	/*
	DrawText(TextFormat("_H_%d", gun_refs.player->comp_health.amount), 64, 980, 80, ColorAlpha(SKYBLUE, 0.95f));	
	DrawText(
		TextFormat("%d | %d", curr_gun->in_clip, curr_gun->ammo),
		1640,
		980,
		80,
		ColorAlpha(SKYBLUE, 0.95f)
	);
	*/

	DrawTextEx(
		hud_font,
		TextFormat("_H_%d", gun_refs.player->comp_health.amount),
		(Vector2) { 64, gun_refs.conf->window_height - 120 },
		80, 
		1, 
		ColorAlpha(SKYBLUE, 0.75f)
	);

	DrawTextEx(
		hud_font,
		TextFormat("%d | %d", curr_gun->in_clip, curr_gun->ammo),
		(Vector2) { 1640, gun_refs.conf->window_height - 120 },
		80,
		1, 
		ColorAlpha(SKYBLUE, 0.75f)
	);

	/*
	DrawTextEx(
		hud_font,
		TextFormat("%d", bug_ent->comp_ai.state),
		(Vector2) { 0, 64 },
		32,
		1, 
		ColorAlpha(GREEN, 0.95f)
	);

	DrawTextEx(
		hud_font,
		TextFormat("%f %f %f", bug_ent->comp_transform.position.x, bug_ent->comp_transform.position.y, bug_ent->comp_transform.position.z),
		(Vector2) { 0, 96 },
		32,
		1, 
		ColorAlpha(GREEN, 0.95f)
	);

	int bug_active = gun_refs.handler->ents[gun_refs.handler->bug_id].flags & ENT_ACTIVE;
	DrawTextEx(
		hud_font,
		TextFormat("%d", bug_active),
		(Vector2) { 0, 128 },
		32,
		1, 
		ColorAlpha(GREEN, 0.95f)
	);


	DrawTextEx(
		hud_font,
		TextFormat("%d", bug_ent->comp_ai.state),
		(Vector2) { 0, 64 },
		32,
		1, 
		ColorAlpha(GREEN, 0.95f)
	);
	*/

	short crosshair_type = CROSSHAIR_DEFAULT;
	switch(player_gun->current_gun) {
		case WEAP_DISRUPTOR: 
			crosshair_type = CROSSHAIR_PROJECTILE;
			break;		

		default:
			crosshair_type = CROSSHAIR_DEFAULT;
			break;
	}

	Vector2 crosshair_pos = (Vector2) {
		.x = 1920 * 0.5f - (crosshair_textures[0].width  * 0.5f),
		.y = 1080 * 0.5f - (crosshair_textures[0].height * 0.5f)
	}; 

	short draw_crosshair = gun_refs.conf->draw_crosshair;
	float crosshair_alpha = 1.0f;
	Color crosshair_color = WHITE;

	if(player_gun->current_gun == WEAP_DISRUPTOR) {
		if((bug_ent->flags & BUG_DISRUPTED_ENEMY) && bug_ent->comp_ai.state == BUG_LANDED) {
			crosshair_color = PURPLE; 
		}
	}

	if(draw_crosshair)
		DrawTextureV(crosshair_textures[crosshair_type], crosshair_pos, ColorAlpha(crosshair_color, crosshair_alpha));
}

void PlayerShoot(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect) {
	switch(player_gun->current_gun) {
		case WEAP_PISTOL:
			PlayerShootPistol(player_gun, handler, sect);
			break;

		case WEAP_SHOTGUN:
			PlayerShootShotgun(player_gun, handler, sect);
			break;

		case WEAP_REVOLVER:
			PlayerShootRevolver(player_gun, handler, sect);
			break;

		case WEAP_DISRUPTOR:
			PlayerShootDisruptor(player_gun, handler, sect);
			break;
	}
}

void PlayerShootPistol(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect) {
	comp_Transform *ct = &gun_refs.player->comp_transform;

	Vector3 trace_start = ct->position;
	trace_start.z -= 4;

	Vector3 dir = ct->forward;
	float offset = GetRandomValue(-10, 10) * 0.001f;	

	Vector3 right = Vector3CrossProduct(UP, ct->forward);
	dir = Vector3Add(dir, Vector3Scale(right, offset));

	offset = GetRandomValue(-10, 10) * 0.001f;
	dir = Vector3Add(dir, Vector3Scale(UP, offset));

	dir = Vector3Normalize(dir);

	bool trace_hit = false;
	Vector3 point = TraceBullet(
		handler,
		sect,
		trace_start,
		dir,
		handler->player_id,
		&trace_hit,
		false
	);

	Vector3 trail_start = Vector3Add(trace_start, Vector3Scale(ct->forward, 12));
	//Vector3 right = Vector3CrossProduct(ct->forward, UP);
	trail_start = Vector3Add(trail_start, Vector3Scale(right, 2.5f));

	//Vector3 trail_end = Vector3Add(trail_start, Vector3Scale(ct->forward, Vector3Distance(ct->position, point)));
	Vector3 trail_end = point;
	if(!trace_hit) {
		trail_end = Vector3Add(trail_start, Vector3Scale(ct->forward, 2000.0f));
	}

	float dist = Vector3Distance(trail_start, trail_end);

	//if(dist >= 20)
	vEffectsAddTrail(gun_refs.effect_manager, trail_start, trail_end);
	gun_refs.effect_manager->trails[gun_refs.effect_manager->trail_count-1].timer = 0.75f;

	curr_gun->in_clip--;
}

void PlayerShootShotgun(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect) {
}

void PlayerShootRevolver(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect) {
	if(reload_active)
		return;

	if(curr_gun->in_clip <= 0) {
		AP_RequestSound(gun_refs.ap, "outofammo");
		return;
	} 

	if(curr_gun->cooldown > 0)
		return;

	if(gun_angle > REVOLVER_ANGLE_REST + 1)
		return;

	int sfx_id = GetRandomValue(0, 1);
	AP_SetSoundPitch(gun_refs.ap, gun_shoot_sounds[WEAP_REVOLVER][sfx_id], GetRandomValue(80, 90) * 0.01f);
	AP_RequestSound(gun_refs.ap, gun_shoot_sounds[WEAP_REVOLVER][sfx_id]);

	recoil_add = false;
	recoil = 90 + (GetRandomValue(1, 5) * 0.1f);

	freeze_frame = true;

	curr_gun->cooldown = 0.05f;
	muz_rot[0] = -30;
	for(short i = 1; i < 12; i++) muz_rot[i] = muz_rot[0] + GetRandomValue(-360, 360);

	comp_Transform *ct = &gun_refs.player->comp_transform;

	Vector3 trace_start = ct->position;
	//Vector3 trace_start = gun_refs.world_cam->position; 
	//trace_start.z += 12;
	trace_start.z = gun_refs.world_cam->position.z - 2;

	bool trace_hit = false;
	Vector3 point = TraceBullet(
		handler,
		sect,
		trace_start,
		ct->forward,
		handler->player_id,
		&trace_hit,
		false
	);

	Vector3 trail_start = Vector3Add(trace_start, Vector3Scale(ct->forward, 12));
	//Vector3 trail_start = trace_start;
	Vector3 right = Vector3CrossProduct(ct->forward, UP);
	trail_start = Vector3Add(trail_start, Vector3Scale(right, 3.5f));

	//Vector3 trail_end = Vector3Add(trail_start, Vector3Scale(ct->forward, Vector3Distance(ct->position, point)));
	Vector3 trail_end = point;
	if(!trace_hit) {
		trail_end = Vector3Add(trail_start, Vector3Scale(ct->forward, 2000.0f));
	}

	float dist = Vector3Distance(trail_start, trail_end);

	/*
	if(dist >= 20)
		vEffectsAddTrail(gun_refs.effect_manager, trail_start, trail_end);
	*/

	curr_gun->in_clip--;
	if(curr_gun->in_clip <= 0 && curr_gun->ammo > 0) {
		curr_gun->in_clip = 0;
		//PlayerGunReload(player_gun, 1);
		if(!reload_active) 
			AP_RequestSound(gun_refs.ap, "outofammo");

		reload_active = true;
	}

	/*
	if(!reload_active) {
		anim_states[curr_gun->id].loop_count = 0;
		anim_states[curr_gun->id].curr_frame = 0;
		anim_Apply(&anim_states[curr_gun->id], &models[curr_gun->id], &anims[curr_gun->id]);
	}
	*/
}

void PlayerShootDisruptor(PlayerGun *player_gun, EntityHandler *handler, MapSection *sect) {
	Entity *bug_ent = &handler->ents[handler->bug_id];
	Entity *player_ent = &handler->ents[handler->player_id];

	comp_Ai *ai = &bug_ent->comp_ai;
	comp_Transform *ct = &bug_ent->comp_transform;

	if(ai->state > 0) 
		return;

	ai->state = BUG_LAUNCHED;
	bug_ent->flags = ENT_ACTIVE;
	bug_ent->flags |= ENT_COLLIDERS;

	ct->position = player_ent->comp_transform.position;
	ct->position.z = gun_refs.world_cam->position.z;

	ct->forward = player_ent->comp_transform.forward;
	
	ct->position = Vector3Add(ct->position, Vector3Scale(ct->forward, 10));

	float updot = Vector3DotProduct(UP, ct->forward);

	Vector3 throw_dir = (Vector3) { ct->forward.x, ct->forward.y, 0 };
	throw_dir = Vector3Normalize(throw_dir);

	if(ct->forward.z < 1.0f && ct->forward.z > 0) {
		ct->velocity = Vector3Scale(throw_dir, DISRUPTOR_THROW_FORCE);
		ct->velocity.z += 250 + (((ct->forward.z) * (DISRUPTOR_THROW_FORCE)) * updot);
	} else {
		Vector3 throw_dir = (Vector3) { ct->forward.x, ct->forward.y, ct->forward.z };
		throw_dir = Vector3Normalize(throw_dir);

		ct->velocity = Vector3Scale(throw_dir, DISRUPTOR_THROW_FORCE);
		ct->velocity.z += 250;
	}

	if(Vector3DotProduct(player_ent->comp_transform.velocity, ct->forward) > 0)
		ct->velocity = Vector3Add(ct->velocity, Vector3Scale(ct->forward, Vector3Length(player_ent->comp_transform.velocity) * 0.5f));

	float angle = atan2f(-ct->forward.x, -ct->forward.y);
	bug_ent->model.transform = MatrixRotateY(angle);
	bug_ent->model.transform = MatrixMultiply(bug_ent->model.transform, MatrixRotateX(90*DEG2RAD));

	AP_SetSoundDir(gun_refs.ap, "throw_bug", Vector3Negate(ct->forward), 0);
	AP_RequestSound(gun_refs.ap, "throw_bug");
}

void PlayerGunReload(PlayerGun *player_gun, float dt) {
	if(recoil > 1.0f)
		return;

	// Clip is already full, do nothing
	if(curr_gun->in_clip == curr_gun->clip_size) {
		reload_active = false;
		return;
	}

	// Already reloading, do nothing
	if(!anim_states[curr_gun->id].loop_count) {
		anim_Update(&anim_states[curr_gun->id], &anims[curr_gun->id], dt);
		anim_Apply(&anim_states[curr_gun->id], &models[curr_gun->id], &anims[curr_gun->id]); 

		curr_gun->reload_timer = curr_gun->reload_time_amnt;

		reload_active = true;

		if(!reload_sound_set) {
			AP_SetSoundPosition(gun_refs.ap, "rev_reload", gun_refs.world_cam->position, 0);
			AP_RequestSound(gun_refs.ap, "rev_reload");
			reload_sound_set = true;
		}

		return;

	} else {
		anim_states[curr_gun->id].loop_count = 0;
		anim_states[curr_gun->id].curr_frame = 0;
		anim_Apply(&anim_states[curr_gun->id], &models[curr_gun->id], &anims[curr_gun->id]);
	}

	// No more ammo available, do nothing
	if(curr_gun->ammo <= 0) {
		curr_gun->ammo = 0;
		reload_active = false;
		curr_gun->reload_timer = 0;
		return;
	}


	// Fill clip
	int clip_refill = curr_gun->clip_size - curr_gun->in_clip;

	if(curr_gun->ammo + curr_gun->in_clip < curr_gun->clip_size)
		clip_refill = curr_gun->ammo;

	curr_gun->ammo -= clip_refill;
	curr_gun->in_clip += clip_refill;

	// Set timer
	//curr_gun->reload_timer = curr_gun->reload_time_amnt;

	curr_gun->reload_timer = 0;

	reload_active = false;
	reload_sound_set = false;
}

void SendAmmoPickupEvent(int pickup_type) {
	switch(pickup_type) {
		case ENT_AMMO_PISTOL:
			weapons[WEAP_PISTOL].ammo += 20;
			break;

		case ENT_AMMO_SHOTGUN:
			weapons[WEAP_SHOTGUN].ammo += 16;
			break;

		case ENT_AMMO_REVOLVER:
			weapons[WEAP_REVOLVER].ammo += 6;
			break;
	}	
}

void DrawSectionTransition() {
	DrawTextEx(
		hud_font,
		"MISSION STATUS: COMPLETE",
		(Vector2) { 0, 0 },
		80, 
		1, 
		PURPLE
	);

	DrawTextEx(
		hud_font,
		"PRESS [Y] TO PLAY AGAIN",
		(Vector2) { 0, 100 },
		80, 
		1, 
		PURPLE
	);

	DrawTextEx(
		hud_font,
		"PRESS [ESC] TO EXIT",
		(Vector2) { 0, 200 },
		80, 
		1, 
		PURPLE
	);
}

void PlayerGunOnSave(rw_GlobalData *data, PlayerGun *player_gun) {
	rw_PlayerWeaponData *weap_data = &data->player_weap_data;

	weap_data->curr_id = player_gun->current_gun;
	weap_data->gun_angle = gun_angle;
	weap_data->recoil = recoil;
	weap_data->sway = sway;

	for(short i = 0; i < 4; i++)
		weap_data->weapons[i] = weapons[i];

}

void PlayerGunOnLoad(rw_GlobalData *data, PlayerGun *player_gun) {
	rw_PlayerWeaponData *weap_data = &data->player_weap_data;

	player_gun->current_gun = weap_data->curr_id;
	gun_angle = weap_data->gun_angle;
	recoil = weap_data->recoil;
	sway = weap_data->sway;

	for(short i = 0; i < 4; i++) {
		weapons[i] = weap_data->weapons[i];
		weapons[i].reload_timer = 0.0f;		
		anim_states[i].curr_frame = anims[i].frameCount;
		anim_Apply(&anim_states[i], &models[i], &anims[i]);
	}
}

