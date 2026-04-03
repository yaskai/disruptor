#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "player_gun.h"
#include "geo.h"
#include "ent.h"
#include "v_effect.h"
#include "config.h"

#define USE_MWHEEL (true)

Vector3 gun_pos = {0};
float gun_rot = 0;
float p, y, r;
Matrix mat = {0};

#define LOCAL_UP (Vector3) { 0, 1, 0 }

#define REVOLVER_REST (Vector3) { -0.65f, -2.05f, 5.0f }
//#define REVOLVER_REST (Vector3) { -0.75f, 6.25f, -2.35f }
#define REVOLVER_ANGLE_REST 1.5f

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

typedef struct {
	EntityHandler *handler;	
	MapSection *sect;
	Entity *player;
	vEffect_Manager *effect_manager;
	Config *conf;
	Camera3D *world_cam;

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
		.damage = 3,

		.clip_size = 6,
		.in_clip = 6,
		.ammo = 24,

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

enum CROSSHAIR_IDS : short {
	CROSSHAIR_DEFAULT 		= 0,
	CROSSHAIR_PROJECTILE	= 1,
};

Texture2D crosshair_textures[4];
void LoadCrosshairTextures() {
	crosshair_textures[0] = LoadTexture("resources/ui/crosshair/default.png");
	crosshair_textures[1] = LoadTexture("resources/ui/crosshair/arrow.png");
}

void PlayerGunInit(
	PlayerGun *player_gun,
	Entity *player,
	EntityHandler *handler,
	MapSection *sect,
	vEffect_Manager *effect_manager,
	Config *conf) 
{
	player_gun->cam = (Camera3D) {
		.position = (Vector3) { 0, 0, -1 },
		.target = (Vector3) { 0, 0, 1 },
		.up = LOCAL_UP,
		.fovy = 54,
		.projection = CAMERA_PERSPECTIVE
	};

	models[WEAP_PISTOL] 	= LoadModel("resources/models/weapons/pistol_00.glb");
	models[WEAP_SHOTGUN] 	= LoadModel("resources/models/weapons/pistol_00.glb");
	models[WEAP_REVOLVER] 	= LoadModel("resources/models/weapons/rev_00.glb");
	models[WEAP_DISRUPTOR] 	= LoadModel("resources/models/weapons/bug_00.glb");

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
}

void PlayerGunUpdate(PlayerGun *player_gun, float dt) {
	int scroll = 0;

	if(recoil <= 1.0f) {
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

	if(IsKeyPressed(KEY_R)) 
		PlayerGunReload(player_gun, 1);
}

void PlayerGunUpdatePistol(PlayerGun *player_gun, float dt) {
	float recoil_angle = Clamp(recoil + gun_rot, -30, 90.0f);
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
	/*
	float recoil_angle = Clamp(recoil + gun_rot, -30, 90.0f);
	mat = MatrixRotateX(-recoil_angle * DEG2RAD);
	mat = MatrixMultiply(mat, MatrixRotateY(REVOLVER_ANGLE_REST * DEG2RAD));

	float lerp_t = (recoil_angle > 80) ? 10 : 30; 

	cam_recoil = Lerp(cam_recoil, recoil*0.00033f, dt*lerp_t);
	cam_recoil = Clamp(cam_recoil, 0.0f, 0.33f);
	PlayerSetRecoilInput(gun_refs.player, cam_recoil);

	friction = (recoil > 40) ? 4.9f : 10.5f;
	//if(recoil_angle >= 80.0f) friction *= 0.5f;

	recoil -= (recoil * friction) * dt; 
	if(recoil <= -EPSILON) recoil = 0;

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && recoil <= 1.0f) {
		PlayerShootRevolver(player_gun, gun_refs.handler, gun_refs.sect);

		recoil_add = false;
		recoil += 130 + (GetRandomValue(10, 30) * 0.1f);
	}

	gun_pos = REVOLVER_REST;
	gun_pos.z = REVOLVER_REST.z - (recoil * 0.05f);

	models[WEAP_REVOLVER].transform = mat;
	*/

	/*
	friction = (recoil_force < 40) ? 4.9f : 10.5f;

	if(recoil_force <= 1.0f) {
		recoil_force = 0;
		//recoil = Lerp(recoil, recoil-friction, dt*30);
		recoil -= friction * dt * 50;

	} else {
		//recoil_force = Lerp(recoil_force, 0.0f, dt*10);
		recoil += recoil_force * dt * 10;
		recoil_force -= friction * dt * 50;
	}

	//recoil += (recoil_force) * dt*50;

	float recoil_angle = Clamp(recoil + gun_rot, 0.0f, 60.0f);
	mat = MatrixRotateX(-recoil_angle * DEG2RAD);
	mat = MatrixMultiply(mat, MatrixRotateY(REVOLVER_ANGLE_REST * DEG2RAD));

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && recoil <= 1.0f) {
		PlayerShootRevolver(player_gun, gun_refs.handler, gun_refs.sect);
		recoil = 0;
		recoil_force += 130 + (GetRandomValue(10, 30) * 0.1f);
	}

	gun_pos = REVOLVER_REST;

	float rest_ofs = (recoil * 0.05f);
	rest_ofs = Clamp(rest_ofs, 0.0f, 0.01f); 	
	gun_pos.z = REVOLVER_REST.z - (rest_ofs);

	models[WEAP_REVOLVER].transform = mat;
	*/

	float recoil_angle = Clamp(recoil + gun_rot, -30, 55.0f);
	mat = MatrixRotateX(-recoil_angle * DEG2RAD);
	mat = MatrixMultiply(mat, MatrixRotateY(REVOLVER_ANGLE_REST * DEG2RAD));

	friction = (recoil_angle > 50) ? 4.9f : 10.5f;

	recoil -= (recoil * friction) * dt; 
	if(recoil <= -EPSILON) recoil = 0;

	if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && recoil <= 1.0f) {
		PlayerShootRevolver(player_gun, gun_refs.handler, gun_refs.sect);

		recoil_add = false;
		recoil += 130 + (GetRandomValue(10, 30) * 0.1f);
	}

	gun_pos = REVOLVER_REST;
	float rest_ofs = (recoil * 0.04f);
	rest_ofs = Clamp(rest_ofs, 0.0f, 10.0f); 	
	gun_pos.z = REVOLVER_REST.z - (rest_ofs);

	models[WEAP_REVOLVER].transform = mat;

	float lerp_t = (recoil_angle < 50) ? 30 : 30; 
	cam_recoil = Lerp(cam_recoil, recoil*0.0003f, dt*lerp_t);
	//cam_recoil = Clamp(cam_recoil, 0.0f, 0.33f);
	PlayerSetRecoilInput(gun_refs.player, cam_recoil);
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
	if(gun_refs.player->comp_ai.state == STATE_DEAD)
		return;

	Entity *bug_ent = &gun_refs.handler->ents[gun_refs.handler->bug_id];
	bool skip_draw = false;

	if(player_gun->current_gun == WEAP_DISRUPTOR) {
		if(bug_ent->comp_ai.state > 0) {
			skip_draw = true;
		}
	}

	if(!skip_draw) {
		float scale = 1.0f;
		if(player_gun->current_gun == WEAP_DISRUPTOR)
			scale = 0.8f;

		BeginMode3D(player_gun->cam);
		DrawModel(models[player_gun->current_gun], gun_pos, scale, WHITE);
		EndMode3D();
	}

	DrawText(TextFormat("_H_%d", gun_refs.player->comp_health.amount), 64, 980, 80, ColorAlpha(SKYBLUE, 0.95f));	
	DrawText(
		TextFormat("%d | %d", curr_gun->in_clip, curr_gun->ammo),
		1640,
		980,
		80,
		ColorAlpha(SKYBLUE, 0.95f)
	);
	/*
	DrawText(
		TextFormat("%d | %d", 6, 24),
		1640,
		980,
		80,
		ColorAlpha(SKYBLUE, 0.95f)
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
	comp_Transform *ct = &gun_refs.player->comp_transform;

	Vector3 trace_start = ct->position;
	trace_start.z += 12;

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
	if(curr_gun->in_clip <= 0) {
		curr_gun->in_clip = 0;
		PlayerGunReload(player_gun, 1);
	}
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
	ct->position.z += 10;

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
}

void PlayerGunReload(PlayerGun *player_gun, float dt) {
	// Already reloading, do nothing
	/*
	if(curr_gun->reload_timer > 0) {
		return;
	}
	*/

	// No more ammo available, do nothing
	if(curr_gun->ammo <= 0) {
		curr_gun->ammo = 0;
		return;
	}

	// Clip is already full, do nothing
	if(curr_gun->in_clip == curr_gun->clip_size) {
		return;
	}

	// Fill clip
	int clip_refill = curr_gun->clip_size - curr_gun->in_clip;

	if(curr_gun->ammo + curr_gun->in_clip < curr_gun->clip_size)
		clip_refill = curr_gun->ammo;

	curr_gun->ammo -= clip_refill;
	curr_gun->in_clip += clip_refill;

	// Set timer
	curr_gun->reload_timer = curr_gun->reload_time_amnt;
}

