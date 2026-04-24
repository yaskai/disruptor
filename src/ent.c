#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "ent.h"
#include "geo.h"
#include "ai.h"
#include "../include/log_message.h"
#include "../include/sort.h"
#include "pm.h"
#include "rw_save.h"
#include "player_gun.h"

Vector3 debug_bullet_dest;
Vector3 debug_bullet_norm;

MapSection *ptr_handler_sect = NULL;
EntityHandler *ptr_handler_self = NULL;

rw_GlobalData *_hgd;
void EntHandlerPassRwState(rw_GlobalData *data) { _hgd = data; }

PlayerGun *handler_ptr_player_gun;
void HandlerSetPtrGun(PlayerGun *player_gun) { handler_ptr_player_gun = player_gun; }

void ent_TraceMoveEx(Entity *ent, Vector3 start, Vector3 wish_vel, pmTraceData *pm, float dt, EntityHandler *handler) {
	MapSection *sect = ptr_handler_sect;

	comp_Transform *ct = &ent->comp_transform;

	Bsp_Data *bsp = &sect->bsp_data;
	Bsp_Hull *hull = &bsp->hull_groups[0].hulls[1];

	EntGrid *grid = &handler->grid;

	*pm = (pmTraceData) { .start_in_solid = -1, .end_in_solid = -1, .origin = start, .block = 0, .clip_count = 0 };

	Vector3 dest = start;
	Vector3 vel = wish_vel;

	pm->start_vel = wish_vel;

	float t_remain = dt;

	Vector3 clips[MAX_CLIPS] = {0};
	u8 num_clips = 0;

	for(short i = 0; i < MAX_BUMPS; i++) {
		// End slide trace if velocity too low
		if(Vector3LengthSqr(vel) <= STOP_EPS*STOP_EPS)
			break;
		
		// Scale slide movement by time remaining
		Vector3 move = Vector3Scale(vel, t_remain);

		// Upate ray
		Ray ray = (Ray) { .position = dest, .direction = Vector3Normalize(move) };

		// Trace geometry 
		Bsp_TraceData tr = Bsp_TraceDataEmpty();
		float fraction = 1.0f;

		Bsp_RecursiveTraceEx(hull, hull->first_node, 0, 1, dest, Vector3Add(dest, move), &tr);
		fraction = tr.fraction;	

		/*
		for(int j = 0; j < 1; j++) {
			if(!(bsp->hull_groups[j].flags & HULLGROUP_ACTIVE))
				continue;

			Bsp_Hull *hull = &bsp->hull_groups[j].hulls[1];

			Bsp_TraceData temp_tr = Bsp_TraceDataEmpty();
			Bsp_RecursiveTraceEx(hull, hull->first_node, 0, 1, dest, Vector3Add(dest, move), &temp_tr);

			// Determine how much of movement was obstructed
			float hull_frac = temp_tr.fraction;
			hull_frac = Clamp(hull_frac, 0.0f, 1.0f);

			if(hull_frac < fraction) {
				tr = temp_tr;
				fraction = hull_frac;
			}
		}		
		*/

		// Repeat trace steps for entity collisions,
		// if entity hit is closer, use that collision for clipping plane 
		EntTraceData ent_tr = { .dist = Vector3Length(move), .hit_ent = -1, .point = dest, .normal = Vector3Zero() };
		Vector3 ent_point = TraceEntities(ray, handler, Vector3Length(move) * 1.5f, ent->id, &ent_tr);

		float ent_frac = 1.0f;
		bool use_ent = (ent_tr.hit_ent != -1);

		use_ent = false;
		
		// Prevent entity trace from using Bug,
		if(ent_tr.hit_ent == handler->bug_id)
			use_ent = false;

		if(handler->ents[ent_tr.hit_ent].type == ENT_FORCEFIELD)
			use_ent = false;

		if(use_ent) {
			ent_frac = (ent_tr.dist / Vector3Length(move)) - 0.05f;
			ent_frac = Clamp(ent_frac, 0.0f, 1.0f);
		}

		if(num_clips < MAX_CLIPS && tr.fraction < 1.0f) {
			clips[num_clips++] = *(Vector3 *) tr.plane.normal; 
		} 

		if(num_clips < MAX_CLIPS && ent_frac < 1.0f && use_ent) {
			clips[num_clips++] = ent_tr.normal;
		}

		if(use_ent)
			fraction = Clamp(fraction, 0.0f, ent_frac);

		// Update destination, move to point
		dest = Vector3Add(dest, Vector3Scale(move, fraction));

		// No obstruction, do full movement 
		if(fraction >= 1.0f) 
			break;

		// Update velocity by each clip plane
		for(short j = 0; j < num_clips; j++) {
			float into = Vector3DotProduct(vel, clips[j]);

			if(into < 0) {
				u8 block = pm_ClipVelocity(vel, clips[j], &vel, 1.005f, pm->block);
				pm->block |= block;
			}
		}

		// Update remaining time
		t_remain *= (1 - fraction);
	}

	// Set movement data values
	pm->move_dist = Vector3Distance(start, dest);
	pm->fraction = (pm->move_dist / Vector3Length(wish_vel));

	pm->end_vel = vel;
	pm->end_pos = dest;

	/*
	if(ent->comp_ai.state == STATE_MOVE) {
		ct->targ_look = Vector3Normalize( (Vector3) { vel.x, vel.y, 0 } );
	}
	*/

	pm->clip_count = num_clips;
	memcpy(pm->clips, clips, sizeof(Vector3) * num_clips);
}

u8 ent_CheckGround(comp_Transform *ct, Vector3 position, MapSection *sect) {
	Ray ray = (Ray) { .position = ct->position, .direction = DOWN };	

	Vector3 dest = Vector3Add(ct->position, Vector3Scale(DOWN, 0.1f));

	Bsp_Data *bsp = &sect->bsp_data;

	Bsp_TraceData tr = Bsp_TraceDataEmpty();
	float fraction = 1.0f;

	for(int j = 0; j < 1; j++) {
		if(!(bsp->hull_groups[j].flags & HULLGROUP_ACTIVE))
			continue;

		Bsp_Hull *hull = &bsp->hull_groups[j].hulls[1];

		Bsp_TraceData temp_tr = Bsp_TraceDataEmpty();
		Bsp_RecursiveTraceEx(hull, hull->first_node, 0, 1, ct->position, dest, &temp_tr);

		// Determine how much of movement was obstructed
		float hull_frac = temp_tr.fraction;
		hull_frac = Clamp(hull_frac, 0.0f, 1.0f);

		if(hull_frac < fraction) {
			tr = temp_tr;
			fraction = hull_frac;
		}
	}		

	if(tr.fraction >= 1) {
		ct->ground_normal = Vector3Zero();
		return 0;
	}

	// Set stored ground normal
	ct->ground_normal = *(Vector3 *) tr.plane.normal;

	// Clip velocity going into ground
	pm_ClipVelocity(ct->velocity, ct->ground_normal, &ct->velocity, 1.001f, 0);

	if(fabsf(ct->velocity.z) < STOP_EPS)
		ct->velocity.z = 0;

	return 1;
}

void ent_GroundMove(Entity *ent, comp_Transform *ct, Vector3 start, pmTraceData *pm, float dt, Vector3 wish_vel, EntityHandler *handler) {
	MapSection *sect = ptr_handler_sect;
	Bsp_Hull *hull = &sect->bsp_data.hull_groups[0].hulls[1];

	// First try moving to destination
	pmTraceData base_pm = (pmTraceData) { .end_in_solid = -1, .start_in_solid = -1, .origin = start, .clip_count = 0 };
	ent_TraceMoveEx(ent, start, wish_vel, &base_pm, dt, handler);
	//pm_TraceMoveEx(ent, start, wish_vel, &base_pm, dt, handler);
	//pm_TraceMove(ct, start, wish_vel, pm, dt);

	*pm = base_pm;

	if(Vector3LengthSqr(base_pm.start_vel) <= STOP_EPS)
		return;

	if(base_pm.fraction >= 1.0f) {
		return;
	}

	if(!(base_pm.block & BLOCK_STEP))
		return;

	// Move up stair height
	Vector3 step_start = start; 
	step_start.z += PM_STEP_Z;

	pmTraceData step_pm = (pmTraceData) { .end_in_solid = -1, .start_in_solid = -1, .origin = step_start, .clip_count = 0 };	
	ent_TraceMoveEx(ent, step_start, (Vector3) { wish_vel.x, wish_vel.y, 0 }, &step_pm, dt, handler);
	//pm_TraceMove(ct, step_start, (Vector3) { wish_vel.x, wish_vel.y, 0 }, &step_pm, dt);

	float dist_base = Vector2Distance( (Vector2) { base_pm.origin.x, base_pm.origin.y }, (Vector2) { base_pm.end_pos.x, base_pm.end_pos.y } );
	float dist_step = Vector2Distance( (Vector2) { base_pm.origin.x, base_pm.origin.y }, (Vector2) { step_pm.end_pos.x, step_pm.end_pos.y } );

	Bsp_TraceData tr = Bsp_TraceDataEmpty();
	Bsp_RecursiveTraceEx(
		hull,
		hull->first_node,
		0,
		1,
		step_pm.end_pos,
		Vector3Add(base_pm.end_pos, Vector3Scale(DOWN, 1.01f)), 
		&tr
	);

	bool use_step = (dist_step > dist_base) && tr.plane.normal[2] >= 1.0f;

	if(tr.start_solid || tr.all_solid)
		use_step = false;

	/*
	if(!use_step) {
		return;
	}
	*/

	// Press step down
	Vector3 down_vel = Vector3Scale(DOWN, PM_STEP_Z);
	
	pmTraceData down_pm = (pmTraceData) { .end_in_solid = -1, .start_in_solid = -1, .origin = step_pm.end_pos, .clip_count = 0 };	
	ent_TraceMoveEx(ent, down_pm.origin, down_vel, &down_pm, 1, handler);
	//pm_TraceMove(ct, down_pm.origin, down_vel, &down_pm, dt);

	float down_dist = Vector2Distance( (Vector2) { base_pm.origin.x, base_pm.origin.y }, (Vector2) { down_pm.end_pos.x, down_pm.end_pos.y } );

	tr = Bsp_TraceDataEmpty();
	Bsp_RecursiveTraceEx(
		hull,
		hull->first_node,
		0,
		1,
		step_pm.end_pos,
		Vector3Add(down_pm.end_pos, Vector3Scale(DOWN, 1)), 
		&tr
	);

	bool use_down = false;
	if((down_pm.block & BLOCK_GROUND) && (down_dist > dist_base + 0.001f) && (down_pm.end_pos.z > base_pm.end_pos.z + 0.1f))
		use_down = true;

	//if(tr.start_solid && tr.all_solid)
		//use_down = false;

	if(tr.plane.normal[2] < 1.0f)
		use_down = false;

	if(use_down && use_step) {
		step_pm.end_pos.z = down_pm.end_pos.z;
		*pm = step_pm;

	} else if(use_down) { 
		//step_pm.end_vel.z += down_vel.z;
		base_pm.end_pos.z = down_pm.end_pos.z;
		*pm = base_pm;

	} else {
		*pm = base_pm;
	}
}

// -------------------------------------------------
// Hit functions:
typedef void (*OnHitFunc)(Entity *ent, short damage, Vector3 bullet_pos);
OnHitFunc on_hit_funcs[] = {
	&OnHitPlayer,
	&OnHitTurret,
	&OnHitMaintainer,
	&OnHitRegulator,
	&OnHitBug,
	&OnHitSwitch,
};

int shader_holo_t_loc;
void LoadEntityShaders(EntityHandler *handler) {
	char *prefix = "resources/shaders";

	handler->ent_shader = LoadShader(TextFormat("%s/lit_ent_v.glsl", prefix), TextFormat("%s/lit_ent_f.glsl", prefix));
	handler->ent_shader_locs.locs[LC_LIGHT_POS] = GetShaderLocation(handler->ent_shader, "light_pos");
	handler->ent_shader_locs.locs[LC_LIGHT_CLR] = GetShaderLocation(handler->ent_shader, "light_clr");
	handler->ent_shader_locs.locs[LC_MODEL_MAT] = GetShaderLocation(handler->ent_shader, "matModel");
	handler->ent_shader_locs.locs[LC_MODEL_TEX] = GetShaderLocation(handler->ent_shader, "texture0");
	handler->ent_shader_locs.locs[LC_VIEW_POS]  = GetShaderLocation(handler->ent_shader, "view_pos");
}

void UpdateEntityShaders(EntityHandler *handler) {
	float t = GetTime();
}

void LoadEntityBaseModels(EntityHandler *handler) {
	char *prefix = "resources/models";
	handler->base_ent_models[ENT_TURRET] = LoadModel(TextFormat("%s/enemies/turret.glb", prefix));	 
	handler->base_ent_models[ENT_MAINTAINER] = LoadModel(TextFormat("%s/enemies/maintainer.glb", prefix));	 
	//handler->base_ent_models[ENT_MAINTAINER] = LoadModel(TextFormat("%s/enemies/maintainer02.glb", prefix));	 
}

int base_ent_anims_count[16] = {0};
ModelAnimation *base_ent_anims[16] = {0};
void LoadEntityBaseAnims() {
	char *prefix = "resources/models";
	base_ent_anims[ENT_MAINTAINER] = LoadModelAnimations(TextFormat("%s/enemies/maintainer02.glb", prefix), &base_ent_anims_count[ENT_MAINTAINER]);	 
}

Model projectile_models[4];

void EntHandlerInit(EntityHandler *handler, vEffect_Manager *effect_manager, AudioPlayer *ap) {
	handler->ap = ap;

	handler->count = 0;
	handler->capacity = 128;
	handler->ents = calloc(handler->capacity, sizeof(Entity));
	handler->player_id = 0;

	LoadEntityShaders(handler);
	LoadEntityBaseModels(handler);
	LoadEntityBaseAnims();

	handler->ai_tick = 0;
	handler->autosave_tick = 0;

	EntGridInit(handler);
	handler->checkpoint_list = (CheckPointList) {0}; 
	handler->checkpoint_list.active = -1;

	handler->effect_manager = effect_manager;

	handler->projectile_capacity = 128;
	handler->projectiles = calloc(handler->projectile_capacity, sizeof(Projectile));
}

void EntHandlerClose(EntityHandler *handler) {
	for(int i = 0; i < handler->count; i++) {
		anim_Close(&handler->ents[i].anim_state);
	}

	if(handler->ents) 
		free(handler->ents);

	if(handler->spawn_list.arr)
		free(handler->spawn_list.arr);

	if(handler->projectiles)
		free(handler->projectiles);

	if(handler->grid.cells)
		free(handler->grid.cells);

	if(handler->checkpoint_list.points)
		free(handler->checkpoint_list.points);
	
	if(handler->checkpoint_list.angles)
		free(handler->checkpoint_list.angles);

	if(handler->checkpoint_list.cells)
		free(handler->checkpoint_list.cells);
}

// **
// This struct stores IDs of entities to draw
#define MAX_RENDERED_ENTS	128
#define MIN_VIEW_RADIUS		(64.0*64.0)
#define MAX_VIEW_DOT		(-0.107f * DEG2RAD)
typedef struct {
	u16 ids[MAX_RENDERED_ENTS];
	u16 count;	

} RenderList;
RenderList render_list = {0};

// * NOTE: 
// Render list will be swapped out for different system.
// Current implementation is more expensive than just drawing the entities.
// Could be swapped out for some sort of trace + hashing thing...

void UpdateRenderList(EntityHandler *handler, MapSection *sect) {
	render_list.count = 0;
}

float prev_pos_tick = 0.0f;

// Entity update loop, called once per frame, every frame
void UpdateEntities(EntityHandler *handler, MapSection *sect, float dt) {
	if(!ptr_handler_self)
		ptr_handler_self = handler;

	if(!ptr_handler_sect)
		ptr_handler_sect = sect;

	//if(handler->flags & AT_LEVEL_END)
		//return;

	prev_pos_tick -= dt;
	if(prev_pos_tick < 0.0f) {
		for(u16 i = 0; i < handler->count; i++) {
			Entity *ent = &handler->ents[i];
			ent->comp_transform.prev_pos = ent->comp_transform.position;
		}

		prev_pos_tick = 4*dt;
	}

	UpdateEntityShaders(handler);

	Entity *player_ent = &handler->ents[handler->player_id];
	PlayerUpdate(player_ent, dt);

	if(player_ent->comp_ai.state == STATE_DEAD && handler->player_death_timer >= 2) {
		ReloadEntities(handler, sect, 1);
		return;
	}

	if(player_ent->comp_ai.state != STATE_DEAD) {
		handler->autosave_tick -= dt;
		if(handler->autosave_tick <= 0) {
			//handler->flags |= AUTOSAVE_REQUEST;
			//handler->autosave_tick = AUTOSAVE_TICKRATE;
		}
	}

	render_list.count = 0;
	Vector3 view_dir = player_ent->comp_transform.forward;

	float ff_closest_dist = FLT_MAX;
	AP_SetSoundPosition(handler->ap, "ff_loop", Vector3Scale(Vector3One(), FLT_MAX), 1);

	// Manage brush entity flags
	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];

		if(!ent->bsp_model)
			continue;

		if(!(ent->flags & ENT_ACTIVE)) {
			sect->bsp_data.hull_groups[ent->bsp_model].flags &= ~HULLGROUP_ACTIVE;
			sect->bvh_hullgroups[ent->bsp_model].flags &= ~HULLGROUP_ACTIVE;
		} else {
			sect->bsp_data.hull_groups[ent->bsp_model].flags |=  HULLGROUP_ACTIVE;
			sect->bvh_hullgroups[ent->bsp_model].flags |= HULLGROUP_ACTIVE;

			if(ent->type == ENT_FORCEFIELD) {
				float dist = Vector3Distance(player_ent->comp_transform.position, BoxCenter(GetModelBoundingBox(ent->model)));
				if(dist < ff_closest_dist) {
					AP_SetSoundPosition(handler->ap, "ff_loop", BoxCenter(GetModelBoundingBox(ent->model)), 1);
					ff_closest_dist = dist;
				}
			}
		}

		if(ent->type == ENT_DOOR)
			DoorUpdate(ent, handler, sect, dt);
	}

	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];

		if(ent->bsp_model)
			continue;

		if(ent->type <= 0)
			continue;

		if(!(ent->flags & ENT_ACTIVE))
			continue;

		if(ent->type == ENT_PLAYER)
			continue;

		switch(ent->type) {
			case ENT_TURRET: 
				TurretUpdate(ent, handler, sect, dt);
				break;

			case ENT_MAINTAINER:
				MaintainerUpdate(ent, handler, sect, dt);
				break;

			case ENT_DISRUPTOR:
				BugUpdate(ent, handler, sect, dt);
				break;
			
			case ENT_SWITCH:
				SwitchUpdate(handler, ent, dt);
				break;
		}

		ent->comp_health.bug_box = BoxTranslate(
			ent->comp_health.bug_box,
			Vector3Add(ent->comp_transform.position, ent->comp_health.bug_point)	
		);
		ent->comp_health.damage_cooldown -= dt;

		comp_Health *health = &ent->comp_health;
		if(health->component_valid) {
			health->hit_box = BoxTranslate(health->hit_box, ent->comp_transform.position);

			health->damage_cooldown -= dt;

			if(health->damage_cooldown < 0)
				health->damage_cooldown = 0;
		}

		// *** Render visibility checking ***

		/*
		Vector3 view_pos = player_ent->comp_transform.position;
		Vector3 to_player = Vector3Subtract(view_pos, ent->comp_transform.position);

		float dist = Vector3LengthSqr(to_player);
		to_player = Vector3Normalize(to_player);

		if(dist > 2000.0f*2000.0f)
			continue;

		// Entities that are very close will always be rendered
		if(dist <= MIN_VIEW_RADIUS) {
			render_list.ids[render_list.count++] = i;
			continue;
		}

		// Cull behind camera
		float vis_dot = Vector3DotProduct(to_player, view_dir);
		if(vis_dot > MAX_VIEW_DOT) 
			continue;

		Vector3 right = Vector3CrossProduct(view_dir, UP);
		short visible = 3;
		*/

		/*
		Ray ray = (Ray) { .position = view_pos, .direction = Vector3Negate(to_player) };

		EntTraceData ent_tr = EntTraceDataEmpty();
		TraceEntities(ray, handler, 2000.0f, -1, &ent_tr);

		BvhTraceData tr = TraceDataEmpty();
		BvhTracePointEx(ray, sect, &sect->bvh[0], 0, &tr, dist);

		if(Vector3DistanceSqr(ray.position, tr.point) < (dist + (MIN_VIEW_RADIUS*0.25f)))
			visible = 0;
		*/

		/*
		for(short j = 0; j < 3; j++) {
			short offset = (j & 1) ? -1 : 1;
			if(j == 0) offset = 0;

			Vector3 test_point = Vector3Subtract(ent->comp_transform.position, Vector3Scale(right, 72 * offset));
			if(view_pos.y > ent->comp_transform.position.y) test_point.y = ent->comp_transform.bounds.max.y;

			to_player = Vector3Normalize(Vector3Subtract(view_pos, test_point));
				
			Ray ray = (Ray) { .position = view_pos, .direction = Vector3Negate(to_player) };

			BvhTraceData tr = TraceDataEmpty();
			BvhTracePointEx(ray, sect, &sect->bvh[0], 0, &tr, dist);

			if(Vector3DistanceSqr(ray.position, tr.point) < (dist + (MIN_VIEW_RADIUS*0.25f)))
				visible--;
		}
		*/

		// * NOTE:
		// Special case for bug 
		// fix visibility for when player is below entity,
		// won't be needed after
		/*
		if(i == handler->bug_id)
			visible = 3;
		*/

		// ******* remove later!!! ********
		//visible = 3;

		/*
		if(visible <= 0)
			continue;

		render_list.ids[render_list.count++] = i;
		*/
	}

	/*
	handler->ai_tick -= dt;
	if(handler->ai_tick < 0.0f) {
		// Do next ai update in ~11 frames
		handler->ai_tick = (AI_TICK_RATE*dt);

		AiSystemUpdate(handler, sect, dt);
	}
	*/

	AiSystemUpdate(handler, sect, dt);
	ManageProjectiles(handler, sect, dt);

	UpdateGrid(handler);

	/*
	if(IsKeyPressed(KEY_F)) {
		for(int i = 0; i < handler->count; i++) {
			if(handler->ents[i].bsp_model == 2)
				handler->ents[i].flags ^= ENT_ACTIVE;
		}
	}
	*/
}

// Entity draw loop, every frame, draws all entities.
void RenderEntities(EntityHandler *handler, float dt) {
	EntGrid *grid = &handler->grid;

	//BugDraw(&handler->ents[handler->bug_id]);

	for(u16 i = 0; i < handler->count; i++) {;
		Entity *ent = &handler->ents[i];

		if(ent->bsp_model)
			continue;

		if(!(ent->flags & ENT_ACTIVE)) 
			continue;

		switch(ent->type) {
			case ENT_TURRET:
				TurretDraw(ent);
				break;

			case ENT_MAINTAINER:
				MaintainerDraw(ent, handler, dt);
				break;

			case ENT_DISRUPTOR:
				BugDraw(ent, handler);
				break;

			// * NOTE:
			// Placeholder, replace later
			case ENT_SWITCH:
				//DrawCubeV(ent->comp_transform.position, (Vector3) { 12, 12, 12 }, DARKGREEN);
				/*
				if(ent->trigger_condition > 1)
					DrawBoundingBox(ent->comp_transform.bounds, GREEN);
				else 
					DrawBoundingBox(ent->comp_health.hit_box, RED);
				*/
				SwitchDraw(ent, handler, dt);
				break;

			/*
			case ENT_BRUSH:
				DrawModel(ent->model, Vector3Zero(), 1, WHITE);
				DrawBoundingBox(ent->comp_transform.bounds, RED);
				break;
			*/

			case ENT_HEALTHPACK:
				//DrawBoundingBox(ent->comp_transform.bounds, PINK);
				//DrawModel(ent->model, ent->comp_transform.position, 1, LIGHTGRAY);
				EntDrawLitModel(handler, ent, 1, 100);
				break;

			case ENT_AMMO_REVOLVER:
				//DrawBoundingBox(ent->comp_transform.bounds, DARKGREEN);
				//DrawModel(ent->model, ent->comp_transform.position, 1, DARKGREEN);
				EntDrawLitModel(handler, ent, 1, 100);
				break;

			case ENT_DOOR:
				continue;
		}
	}

	RenderProjectiles(handler);

	/*
	for(int i = 0; i < handler->checkpoint_list.count; i++) {
		Vector3 pos = handler->checkpoint_list.points[i];
		Vector3 fwd = handler->checkpoint_list.angles[i];

		DrawSphere(pos, 2, YELLOW);
		DrawLine3D(pos, Vector3Add(pos, Vector3Scale(fwd, 10)), YELLOW);
		DrawSphere(Vector3Add(pos, Vector3Scale(fwd, 10)), 1, BLUE);
	} 
	*/
}

void RenderBrushEntities(EntityHandler *handler) {
	u16 ff_ids[128] = {0};
	u16 ff_count = 0;

	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];

		if(!ent->bsp_model)
			continue;

		if(!(ent->flags & ENT_ACTIVE)) 
			continue;

		if(ent->type == ENT_FORCEFIELD) {
			ff_ids[ff_count++] = i;
			continue;
		}

		//if(ent->type == ENT_DOOR) {
			//EntDrawLitModel(handler, ent, 1.0f, 10.0f);
			//continue;
		//}

		if(ent->type == ENT_DOOR) {
			Color light_color = lit_SampleLightGrid(&ptr_handler_sect->bsp_data, ent->comp_transform.position);

			Vector3 view_pos = handler->ents[handler->player_id].comp_transform.position;
			SetShaderValue(handler->ent_shader, handler->ent_shader_locs.locs[LC_VIEW_POS], &view_pos, SHADER_UNIFORM_VEC3);

			SetShaderValueV(handler->ent_shader, handler->ent_shader_locs.locs[LC_LIGHT_POS], &ent->comp_transform.position, SHADER_UNIFORM_VEC3, 3);
			SetShaderValueV(handler->ent_shader, handler->ent_shader_locs.locs[LC_LIGHT_CLR], &light_color, SHADER_UNIFORM_VEC3, 3);
		}

		DrawModel(ent->model, Vector3Zero(), 1, WHITE);
	}

	//rlDisableBackfaceCulling();
	rlDisableDepthMask();
	for(u16 i = 0; i < ff_count; i++) {
		Entity *ent = &handler->ents[ff_ids[i]];
		DrawModel(ent->model, Vector3Zero(), 1, WHITE);
	}
	rlEnableDepthMask();
	//rlEnableBackfaceCulling();
}

// Default entity trace data
EntTraceData EntTraceDataEmpty() {
	return (EntTraceData) {
		.point = Vector3Zero(),
		.dist = FLT_MAX,
		.hit_ent = -1
	};
}

Vector3 TraceEntities(Ray ray, EntityHandler *handler, float max_dist, u16 sender, EntTraceData *trace_data) {
	EntGrid *grid = &handler->grid;
	Coords cell = Vec3ToCoords(ray.position, grid); 

	int step_x = (ray.direction.x > 0) ? 1 : -1;
	int step_y = (ray.direction.y > 0) ? 1 : -1;
	int step_z = (ray.direction.z > 0) ? 1 : -1;

	float td_x = fabsf(ENT_GRID_CELL_EXTENTS.x / ray.direction.x); 
	float td_y = fabsf(ENT_GRID_CELL_EXTENTS.y / ray.direction.y); 
	float td_z = fabsf(ENT_GRID_CELL_EXTENTS.z / ray.direction.z); 

	Vector3 cell_min = CoordsToVec3(cell, grid);
	Vector3 cell_max = Vector3Add(cell_min, ENT_GRID_CELL_EXTENTS);

	float tmax_X = (ray.direction.x > 0) ? (cell_max.x - ray.position.x) / ray.direction.x : (cell_min.x - ray.position.x) / ray.direction.x;
	float tmax_Y = (ray.direction.y > 0) ? (cell_max.y - ray.position.y) / ray.direction.y : (cell_min.y - ray.position.y) / ray.direction.y;
	float tmax_Z = (ray.direction.z > 0) ? (cell_max.z - ray.position.z) / ray.direction.z : (cell_min.z - ray.position.z) / ray.direction.z;

	float t = 0.0f;

	float ent_hit_dist = max_dist;
	Vector3 ent_hit_point = ray.position;
	Vector3 ent_hit_norm = Vector3Zero();

	while(CoordsInBounds(cell, grid) && t < max_dist) {
		i16 cell_id = CellCoordsToId(cell, grid);
		EntGridCell *pCell = &grid->cells[cell_id];

		for(short i = 0; i < pCell->ent_count; i++) {
			Entity *ent = &handler->ents[pCell->ents[i]];

			// Skip collision checks with shooting entity  
			if(ent->id == sender)
				continue;

			if(!(ent->flags & ENT_ACTIVE))
				continue;

			if(!(ent->flags & ENT_COLLIDERS))
				continue;

			Vector3 to_ent = Vector3Subtract(ent->comp_transform.position, ray.position);
			if(Vector3DotProduct(to_ent, ray.direction) < 0) 
				continue;
			
			RayCollision coll = GetRayCollisionBox(ray, ent->comp_transform.bounds);
				
			if(coll.hit && coll.distance < ent_hit_dist && coll.distance < max_dist) {
				ent_hit_dist = coll.distance;
				ent_hit_point = coll.point;
				ent_hit_norm = coll.normal;

				trace_data->hit_ent = ent->id;
			}
		}

		if(tmax_X < tmax_Y) {
			if(tmax_X < tmax_Z) {
				cell.c += step_x;
				t = tmax_X;
				tmax_X += td_x;
			} else {
				cell.t += step_z;
				t = tmax_Z;
				tmax_Z += td_z;
			}
		} else {
			if(tmax_Y < tmax_Z) {
				cell.r += step_y;
				t = tmax_Y;
				tmax_Y += td_y;
			} else {
				cell.t += step_z;
				t = tmax_Z;
				tmax_Z += td_z;
			}
		}
	}

	trace_data->dist = ent_hit_dist;
	trace_data->normal = ent_hit_norm;
	trace_data->point = ent_hit_point;

	return ent_hit_point;
}

Vector3 TraceBullet(EntityHandler *handler, MapSection *sect, Vector3 origin, Vector3 dir, u16 sender, bool *hit, bool dummy) {
	// Two steps: 
	// 1. Trace surfaces of the level 
	// 2. Trace Entities
	// Lowest distance between the two traces is the destination of the bullet 
	Vector3 dest = Vector3Add(origin, Vector3Scale(dir, FLT_MAX));

	Ray ray = (Ray) { .position = origin, .direction = dir };

	// 1. 
	// Normal BVH trace for level geometry
	BvhTraceData tr = TraceDataEmpty();
	for(int i = 0; i < sect->bvh_hullgroup_count; i++) {
		Bvh_HullGroup *hg = &sect->bvh_hullgroups[i];

		if(!(hg->flags & HULLGROUP_ACTIVE))	
			continue;

		if(hg->ent_id > -1) {
			if(handler->ents[hg->ent_id].type == ENT_FORCEFIELD) {
				continue;
			}
		}

		BvhTraceData temp_tr = TraceDataEmpty();
		BvhTracePointEx(ray, sect, &sect->bvh_hullgroups[i].bvh[0], 0, &temp_tr, 4000.0f);

		if(temp_tr.distance < tr.distance) {
			tr = temp_tr;
		}
	}
	if(tr.hit) *hit = true;

	// 2. DDA for entities using static grid
	EntGrid *grid = &handler->grid;
	Coords cell = Vec3ToCoords(origin, grid); 

	int step_x = (dir.x > 0) ? 1 : -1;
	int step_y = (dir.y > 0) ? 1 : -1;
	int step_z = (dir.z > 0) ? 1 : -1;

	float td_x = fabsf((ENT_GRID_CELL_EXTENTS.x) / dir.x); 
	float td_y = fabsf((ENT_GRID_CELL_EXTENTS.y) / dir.y); 
	float td_z = fabsf((ENT_GRID_CELL_EXTENTS.z) / dir.z); 

	Vector3 cell_min = CoordsToVec3(cell, grid);
	Vector3 cell_max = Vector3Add(cell_min, ENT_GRID_CELL_EXTENTS);

	float tmax_X = (dir.x > 0) ? (cell_max.x - origin.x) / dir.x : (cell_min.x - origin.x) / dir.x;
	float tmax_Y = (dir.y > 0) ? (cell_max.y - origin.y) / dir.y : (cell_min.y - origin.y) / dir.y;
	float tmax_Z = (dir.z > 0) ? (cell_max.z - origin.z) / dir.z : (cell_min.z - origin.z) / dir.z;

	float t = 0.0f;

	float ent_hit_dist = FLT_MAX;
	Vector3 ent_hit_point = ray.position;

	i16 ent_hit_id = -1;

	while(CoordsInBounds(cell, grid) && t < tr.distance) {
		i16 cell_id = CellCoordsToId(cell, grid);
		EntGridCell *pCell = &grid->cells[cell_id];

		for(short i = 0; i < pCell->ent_count; i++) {
			Entity *ent = &handler->ents[pCell->ents[i]];

			// Skip collision checks with shooting entity  
			if(ent->id == sender)
				continue;

			if(!(ent->flags & ENT_ACTIVE))
				continue;
				
			if(!(ent->flags & ENT_COLLIDERS))
				continue;
			
			// * NOTE:
			// Change from transform bounds to actual damage hit box later 
			//RayCollision coll = GetRayCollisionBox(ray, ent->comp_transform.bounds);
			RayCollision coll = GetRayCollisionBox(ray, ent->comp_health.hit_box);
				
			if(coll.hit && coll.distance <= ent_hit_dist) {
				ent_hit_dist = coll.distance;
				ent_hit_point = coll.point;

				ent_hit_id = ent->id;

				*hit = true;
			}
		}

		if(tmax_X < tmax_Y) {
			if(tmax_X < tmax_Z) {
				cell.c += step_x;
				t = tmax_X;
				tmax_X += td_x;
			} else {
				cell.t += step_z;
				t = tmax_Z;
				tmax_Z += td_z;
			}
		} else {
			if(tmax_Y < tmax_Z) {
				cell.r += step_y;
				t = tmax_Y;
				tmax_Y += td_y;
			} else {
				cell.t += step_z;
				t = tmax_Z;
				tmax_Z += td_z;
			}
		}
	}
	
	bool ent_first = (ent_hit_dist < tr.distance);
	if(ent_first) {
		dest = ent_hit_point;	
	} else {
		dest = tr.point;
		ent_hit_id = -1;
	}

	if(*hit && ent_hit_id > -1 && !dummy) {
		Entity *hit_ent = &handler->ents[ent_hit_id];
		OnHitEnt(hit_ent, handler->ents[sender].comp_weapon.damage, dest);
	}

	debug_bullet_dest = dest;

	return dest;

	/*
	// *NOTE:
	// BVH is still more accurate for point traces so I'll continue using it for now...
	Bsp_TraceData trace = Bsp_TraceDataEmpty();
	Bsp_Data *bsp = &sect->bsp_data;
	
	Bsp_RecursiveTraceEx(&bsp->hull_groups[0].hulls[0], bsp->hull_groups[0].hulls[0].first_node, 0, 1, origin, dest, &trace);
	*hit = trace.fraction < 1;

	if(*hit) dest = trace.point;
	
	debug_bullet_dest = dest;

	trace.normal = (Vector3) { trace.plane.normal[0], trace.plane.normal[1], trace.plane.normal[2] };
	debug_bullet_norm = trace.normal;

	return dest;
	*/
}

void DebugDrawEntText(EntityHandler *handler, Camera3D cam) {
	Vector3 cam_dir = Vector3Normalize(Vector3Subtract(cam.target, cam.position));

	for(u16 i = 0; i < handler->count; i++) {

		Entity *ent = &handler->ents[i];
		comp_Transform *ct = &ent->comp_transform;

		Vector3 to_cam = Vector3Normalize(Vector3Subtract(cam.position, ct->position));
		if(Vector3DotProduct(to_cam, cam_dir) > 0) continue;

		float dist = Vector3Distance(ct->position, cam.position);
		float text_size = (30);

		Vector2 pos = GetWorldToScreen(ct->position, cam);

		DrawText(TextFormat("id: %d", ent->id), pos.x, pos.y, text_size, PURPLE);
	}
}


// When entity is hit with bullet or other damaging thing
void OnHitEnt(Entity *ent, short damage, Vector3 bullet_pos) {
	comp_Health *health = &ent->comp_health;
	
	if(health->damage_cooldown > 0)
		return;

	health->amount -= damage;
	health->damage_cooldown = 0.1f;

	comp_Ai *ai = &ent->comp_ai;

	if(health->amount <= 0) {
		ent->comp_ai.state = STATE_DEAD;
		ent->comp_ai.sched_state.sched_id = SCHED_DEAD;

		ent->flags &= ~ENT_COLLIDERS;
	}

	if(health->on_hit > -1) {
		on_hit_funcs[health->on_hit](ent, damage, bullet_pos);
	}

	if(ent->type == ENT_PLAYER) {
		OnHitPlayer(ent, damage, bullet_pos);
	}

	ai->input_mask |= AI_INPUT_TAKE_DAMAGE;
}

// Maintainer hit
char *maintainer_hit_sounds[4] = { "metal3", "metal4", "metal5", "metal7" };
void OnHitMaintainer(Entity *ent, short damage, Vector3 bullet_pos) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;
	
	int sfx_id = GetRandomValue(0, 3);

	Vector3 to_player = Vector3Subtract(ptr_handler_self->ents[ptr_handler_self->player_id].comp_transform.position, ct->position);
	to_player = Vector3Normalize(to_player);

	AP_SetSoundPosition(ptr_handler_self->ap, maintainer_hit_sounds[sfx_id], ct->position, 0);
	AP_SetSoundDir(ptr_handler_self->ap, maintainer_hit_sounds[sfx_id], to_player, 0);

	AP_RequestSound(ptr_handler_self->ap, maintainer_hit_sounds[sfx_id]);

	if(ai->input_mask & AI_INPUT_SELF_GLITCHED)
		ent->comp_health.amount = 0;

	if(CheckCollisionBoxSphere(ent->comp_health.crit_box, bullet_pos, 3.5f)) {
		ent->comp_health.amount -= damage;
		ent->comp_ai.state = STATE_STUNNED;
		//AiSetSchedule(ai, SCHED_STUN);
		ent->comp_health.damage_cooldown = 0.25f;
	}

	if(ent->comp_health.amount <= 0)
		ai->state = STATE_DEAD;

	Vector3 prev_wish = ai->wish_dir;
	float prev_speed = ai->speed;

	ai->speed = 1000;

	Vector3 knockback = Vector3Negate(to_player);
	if(ai->state != STATE_DEAD)
		knockback = Vector3Zero();
	else {
		short dice = GetRandomValue(0, 6);
		/*
		if(dice == 6) {
			//knockback.z = 0.99f;
			knockback.z = 0.1f;
			ai->speed = 3000;
		}
		*/
	}
	knockback = Vector3Normalize(knockback);

	ai->wish_dir = knockback;
	EntMove(ent, ptr_handler_sect, ptr_handler_self, GetFrameTime());

	ai->wish_dir = prev_wish;
	ai->speed = prev_speed;

	if(ai->state == STATE_DEAD) {
		//ct->velocity.x = 0;
		//ct->velocity.y = 0;	
		ai->wish_dir = Vector3Zero();
		ent->flags &= ~ENT_COLLIDERS;
	}
}

// Regulator hit
// * NOTE:
// Nothing right now as that enemy isn't implemented yet...
void OnHitRegulator(Entity *ent, short damage, Vector3 bullet_pos) {
}

void OnFixRegulator(Entity *ent) {
}

// Move entity through space
void EntMove(Entity *ent, MapSection *sect, EntityHandler *handler, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	ct->bounds = BoxTranslate(ct->bounds, ct->position);

	//ct->on_ground = pm_CheckGround(ct, ct->position);
	ct->on_ground = ent_CheckGround(ct, ct->position, sect); 
	pm_ApplyGravity(ct, dt);
	if(ct->on_ground)
		ct->velocity.z = 0;

	Vector3 wish_dir = ai->wish_dir;
	float wish_speed = ai->speed;
	Vector3 wish_vel = Vector3Scale(wish_dir, wish_speed);

	pm_GroundFriction(ct, dt);
	pm_Accelerate(ct, wish_dir, wish_speed, 20.0f, dt);

	if(Vector3LengthSqr(ct->velocity) <= 1.0f)
		return;

	pmTraceData move_data = (pmTraceData) { .start_in_solid = -1, .end_in_solid = -1, .block = 0 };
	//pm_TraceMove(ct, ct->position, ct->velocity, &move_data, dt);
	if(ct->on_ground) 
		ent_GroundMove(ent, ct, ct->position, &move_data, dt, wish_vel, handler);
	else 
		ent_TraceMoveEx(ent, ct->position, ct->velocity, &move_data, dt, handler);
		//pm_TraceMove(ct, ct->position, ct->velocity, &move_data, dt);

	ct->position = move_data.end_pos;
	ct->velocity = move_data.end_vel;

	ct->on_ground = ent_CheckGround(ct, ct->position, sect); 
}

// Projectile movement tracing
void proj_TraceMove(Projectile *proj, Vector3 start, Vector3 wish_vel, pmTraceData *pm, float dt, MapSection *sect, short bvh_id) {
	comp_Transform *ct = &proj->ct;
	comp_Health *health = &proj->health;

	*pm = (pmTraceData) { .start_in_solid = -1, .end_in_solid = -1, .origin = start, .block = 0, .fraction = 1.0f };

	// Check if inside solid before starting trace
	Ray start_ray = (Ray) { .position = start, .direction = Vector3Normalize(wish_vel) };
	BvhTraceData start_tr = TraceDataEmpty();
	
	float trace_max_dist = Vector3LengthSqr(wish_vel);
	trace_max_dist = Clamp(trace_max_dist, 0.33f, 2000.0f);

	BvhTracePointEx(start_ray, sect, &sect->bvh[bvh_id], 0, &start_tr, trace_max_dist);
	if(start_tr.hit) {
		pm->start_in_solid = start_tr.hull_id;
	}

	Vector3 dest = start;
	Vector3 vel = wish_vel;

	pm->start_vel = wish_vel;

	float t_remain = dt;

	// Tracked clip planes
	Vector3 clips[MAX_CLIPS] = {0};
	u8 num_clips = 0;

	for(short i = 0; i < MAX_BUMPS; i++) {
		// End slide trace if velocity too low
		if(Vector3LengthSqr(vel) <= STOP_EPS)
			break;
		
		// Scale slide movement by time remaining
		Vector3 move = Vector3Scale(vel, t_remain);

		// Upate ray
		Ray ray = (Ray) { .position = dest, .direction = Vector3Normalize(move) };

		// Trace geometry 
		BvhTraceData tr = TraceDataEmpty();
		BvhTracePointEx(ray, sect, &sect->bvh[bvh_id], 0, &tr, FLT_MAX);

		// Determine how much of movement was obstructed
		float fraction = (tr.distance / Vector3Length(move));
		fraction = Clamp(fraction, 0.0f, 1.0f);
		pm->fraction = fraction;

		// Update destination
		dest = Vector3Add(dest, Vector3Scale(move, fraction));

		if(fraction < 1.0f) {
			pm->end_in_solid = (tr.hit);

			//health->amount -= Vector3Length(vel) * 0.5f;
			health->amount -= Vector3Length(vel) * 0.1f;
		}

		// No obstruction, do full movement 
		if(fraction >= 1.0f) 
			break;

		// Add clip plane
		if(num_clips + 1 < MAX_CLIPS) {
			clips[num_clips++] = tr.normal;

			// Update velocity by each clip plane
			for(short j = 0; j < num_clips; j++) {
				float into = Vector3DotProduct(vel, clips[j]);

				if(into < 0) 
					pm_ClipVelocity(vel, clips[j], &vel, 1.5005f, pm->block);
			}

		} else 
			break;

		// Add small offset to prevent tunneling through surfaces
		dest = Vector3Add(dest, Vector3Scale(tr.normal, 0.01f));

		// Update remaining time
		t_remain *= (1 - fraction);
	}

	pm->move_dist = Vector3Distance(start, dest);
	pm->end_vel = vel;
	pm->end_pos = dest;
}

// Projectile check ground
u8 proj_CheckGround(comp_Transform *ct, Vector3 position, MapSection *sect, short bvh_id) {
	Ray ray = (Ray) { .position = ct->position, .direction = DOWN };	

	BvhTraceData tr = TraceDataEmpty();	
	BvhTracePointEx(ray, sect, &sect->bvh[bvh_id], 0, &tr, 1 + 0.001f);
	
	if(!tr.hit) {
		ct->ground_normal = Vector3Zero();
		return 0;
	}

	ct->ground_normal = tr.normal;
	pm_ClipVelocity(ct->velocity, ct->ground_normal, &ct->velocity, 1.00001f, 0);
	if(fabsf(ct->velocity.z) < STOP_EPS) ct->velocity.z = 0;

	return 1;
}

// Projectile update loop
#define PROJ_GRAVITY 800.0f
void ProjectileUpdate(Projectile *projectile, EntityHandler *handler, MapSection *sect, float dt) {
	if(projectile->health.amount <= 0) {
		projectile->active = false;
		return;
	}

	comp_Transform *ct = &projectile->ct;

	ct->bounds = BoxTranslate(ct->bounds, ct->position);

	EntGrid *grid = &handler->grid;
	Coords coords = Vec3ToCoords(ct->position, grid);

	// Check nearby cells for collisions
	Coords cell_coords[] = {
		coords,
		(Coords) { coords.c - 1, coords.r, coords.t - 1 },
		(Coords) { coords.c + 0, coords.r, coords.t - 1 },
		(Coords) { coords.c + 1, coords.r, coords.t - 1 },
		(Coords) { coords.c - 1, coords.r, coords.t + 0 },
		(Coords) { coords.c + 1, coords.r, coords.t + 0 },
		(Coords) { coords.c - 1, coords.r, coords.t + 1 },
		(Coords) { coords.c + 0, coords.r, coords.t + 1 },
		(Coords) { coords.c + 1, coords.r, coords.t + 1 },
	};
	short adj_count = sizeof(cell_coords) / sizeof(cell_coords[0]);

	for(short i = 0; i < adj_count; i++) {
		if(!CoordsInBounds(cell_coords[i], grid))
			continue;

		EntGridCell *cell = &grid->cells[CellCoordsToId(cell_coords[i], grid)];

		for(short j = 0; j < cell->ent_count; j++) {
			i16 ent_id = cell->ents[j]; 
			Entity *ent = &handler->ents[ent_id];

			// Don't collide between projectile and projectile sender entity
			if(ent->id == projectile->sender)
				continue;

			// Impact entity
			if(CheckCollisionBoxes(ct->bounds, ent->comp_transform.bounds)) {
				ProjectileImpact(projectile, handler, ent_id);
			}
		}
	}

	// Apply gravity
	ct->velocity.z -= PROJ_GRAVITY * dt;

	// Trace movement
	pmTraceData pm = (pmTraceData) {0};
	proj_TraceMove(projectile, ct->position, ct->velocity, &pm, dt, sect, BVH_BOX_SMALL);

	ct->position = pm.end_pos;
	ct->velocity = pm.end_vel;
}

void ProjectileDraw(Projectile *projectile) {
	DrawCubeV(projectile->ct.position, (Vector3) { 8, 8, 8 }, RED);	
	//DrawBoundingBox(projectile->ct.bounds, RED);
}

// Make an entity throw a projectile
void ProjectileThrow(Entity *ent, Vector3 pos, Vector3 dir, float force, u8 type, EntityHandler *handler) {
	Projectile projectile = (Projectile) {0};

	projectile.type = type;
	projectile.sender = ent->id;
	
	comp_Transform *ct = &projectile.ct;
	ct->position = pos;

	ct->bounds = (BoundingBox) { .min = Vector3Scale(Vector3One(), -8), .max = Vector3Scale(Vector3One(), 8) };
	ct->bounds = BoxTranslate(ct->bounds, ct->position);
	
	Vector3 vel = Vector3Scale(dir, force + GetRandomValue(0, 60));
	vel.z += 300 + GetRandomValue(-50, 50);
	ct->velocity = vel;

	projectile.health.amount = 100;

	projectile.active = true;

	u16 slot = 0;
	for(u16 i = 0; i < handler->projectile_capacity; i++) {
		Projectile *p = &handler->projectiles[i];
		if(!p->active) {
			slot = i;
			break;
		}
	}

	handler->projectiles[slot] = projectile;
}

// Projectile hit entity
void ProjectileImpact(Projectile *projectile, EntityHandler *handler, i16 ent_id) {
	if(ent_id == -1) {
		*projectile = (Projectile) {0};
		return;
	}

	if(ent_id == handler->bug_id) {
		*projectile = (Projectile) {0};
		return;
	}

	Entity *ent = &handler->ents[ent_id];
	
	float damage = Vector3Length(projectile->ct.velocity) * 0.01f;
	damage = Clamp(damage, 0, 100);

	OnHitEnt(ent, (short)damage, projectile->ct.position);

	Vector3 knockback = (Vector3) { projectile->ct.velocity.x, projectile->ct.velocity.z, 0 };
	knockback = Vector3Scale(knockback, 0.33f);

	ent->comp_transform.velocity = Vector3Add(ent->comp_transform.velocity, knockback);

	*projectile = (Projectile) {0};
}

// Update all projectiles (every frame)
void ManageProjectiles(EntityHandler *handler, MapSection *sect, float dt) {
	for(u16 i = 0; i < handler->projectile_capacity; i++) {
		Projectile *projectile = &handler->projectiles[i];

		if(!projectile->active) 
			continue;

		ProjectileUpdate(projectile, handler, sect, dt);
	}
}

// Draw all projectiles (every frame)
void RenderProjectiles(EntityHandler *handler) {
	for(u16 i = 0; i < handler->projectile_capacity; i++) {
		Projectile *projectile = &handler->projectiles[i];

		if(!projectile->active)
			continue;

		ProjectileDraw(projectile);
	}
}

void ReloadEntities(EntityHandler *handler, MapSection *sect, short with_states) {
	// Get entity states
	if(with_states) {
		rw_LoadMostRecent(handler, _hgd);
		PlayerGunOnLoad(_hgd, handler_ptr_player_gun);
		return;
	}

	u8 states[handler->count];
	for(u16 i = 0; i < handler->count; i++) {
		states[i] = handler->ents[i].comp_ai.state;
	}

	// Clear entities
	handler->count = 0;

	for(u16 i = 0; i < handler->spawn_list.count; i++) {
		ProcessEntity(&handler->spawn_list.arr[i], handler, NULL, &sect->bsp_data);

		if(with_states) {
			// Spawn new entity, retain it's old state before reload
			// * NOTE:
			// Only really works with dead state at the moment.
			// This whole block will probably disappear when serialization is added
			if(states[handler->count-1] == STATE_DEAD) {
				handler->ents[handler->count-1].comp_ai.state = STATE_DEAD;
			}
		}
	}

	Vector3 player_fwd = handler->player_start_fwd;

	// Handle checkpoint logic
	if(handler->checkpoint_list.active > -1) {
		handler->player_start = handler->checkpoint_list.points[handler->checkpoint_list.active];
		player_fwd = handler->checkpoint_list.angles[handler->checkpoint_list.active];
	}

	SpawnPlayer(&handler->ents[handler->player_id], handler->player_start, player_fwd);
	handler->ents[handler->bug_id].comp_ai.state = 0;	

	// Set up ai navigation
	AiNavSetup(handler, sect);
	SwitchSetup(handler);

	// Reset ai tick
	// handler->ai_tick = 0.1f; 
}

void EntDrawLitModel(EntityHandler *handler, Entity *ent, float scale, short min_light) {
	Bsp_Data *bsp = &ptr_handler_sect->bsp_data;
	comp_Transform *ct = &ent->comp_transform;

	float h = BoxExtent(GetModelBoundingBox(ent->model)).z * scale;
	Vector3 points[3] = {
		Vector3Add(ct->position, Vector3Scale(UP, h*0.5f)),
		ct->position,
		Vector3Add(ct->position, Vector3Scale(DOWN, h*0.5f))
	};

	if(ent->bsp_model > 0) {
		points[1] = Vector3Zero();
	}

	Color light_colors[3] = {0};
	for(short i = 0; i < 3; i++)
		light_colors[i] = lit_SampleLightGrid(bsp, points[i]);

	float light_pos[9];
	for(short i = 0; i < 3; i++) {
		light_pos[i*3+0] = points[i].x;
		light_pos[i*3+1] = points[i].y;
		light_pos[i*3+2] = points[i].z;
	}

	float light_clr[9];
	for(short i = 0; i < 3; i++) {
		light_clr[i*3+0] = light_colors[i].r/255.0f;
		light_clr[i*3+1] = light_colors[i].g/255.0f;
		light_clr[i*3+2] = light_colors[i].b/255.0f;
	}

	Vector3 view_pos = handler->ents[handler->player_id].comp_transform.position;
	SetShaderValue(handler->ent_shader, handler->ent_shader_locs.locs[LC_VIEW_POS], &view_pos, SHADER_UNIFORM_VEC3);

	SetShaderValueV(handler->ent_shader, handler->ent_shader_locs.locs[LC_LIGHT_POS], light_pos, SHADER_UNIFORM_VEC3, 3);
	SetShaderValueV(handler->ent_shader, handler->ent_shader_locs.locs[LC_LIGHT_CLR], light_clr, SHADER_UNIFORM_VEC3, 3);

	Matrix mat = ent->model.transform;
	//mat = MatrixMultiply(mat, MatrixRotateX(-90.0f*DEG2RAD));
	SetShaderValueMatrix(handler->ent_shader, handler->ent_shader_locs.locs[LC_MODEL_MAT], mat);
	//SetShaderValueTexture(handler->ent_shader, handler->ent_shader_locs.locs[LC_MODEL_TEX], ent->model.materials[0].maps[0].texture);

	for(int m = 0; m < ent->model.materialCount; m++) {
		ent->model.materials[m].shader = handler->ent_shader;
	}
	DrawModel(ent->model, ct->position, scale, WHITE);
}

void EntDrawLitModelEx(EntityHandler *handler, Entity *ent, Vector3 pos, float scale, Vector3 axis, float angle, short min_light) {
	Bsp_Data *bsp = &ptr_handler_sect->bsp_data;
	comp_Transform *ct = &ent->comp_transform;

	float h = BoxExtent(GetModelBoundingBox(ent->model)).z * scale;
	Vector3 points[3] = {
		Vector3Add(ct->position, Vector3Scale(UP, h)),
		ct->position,
		Vector3Add(ct->position, Vector3Scale(DOWN, h*0.5f))
	};

	Color light_colors[3] = {0};
	for(short i = 0; i < 3; i++)
		light_colors[i] = lit_SampleLightGrid(bsp, points[i]);

	float light_pos[9];
	for(short i = 0; i < 3; i++) {
		light_pos[i*3+0] = points[i].x;
		light_pos[i*3+1] = points[i].y;
		light_pos[i*3+2] = points[i].z;
	}

	float light_clr[9];
	for(short i = 0; i < 3; i++) {
		light_clr[i*3+0] = light_colors[i].r/255.0f;
		light_clr[i*3+1] = light_colors[i].g/255.0f;
		light_clr[i*3+2] = light_colors[i].b/255.0f;
	}

	Vector3 view_pos = handler->ents[handler->player_id].comp_transform.position;
	SetShaderValue(handler->ent_shader, handler->ent_shader_locs.locs[LC_VIEW_POS], &view_pos, SHADER_UNIFORM_VEC3);

	SetShaderValueV(handler->ent_shader, handler->ent_shader_locs.locs[LC_LIGHT_POS], light_pos, SHADER_UNIFORM_VEC3, 3);
	SetShaderValueV(handler->ent_shader, handler->ent_shader_locs.locs[LC_LIGHT_CLR], light_clr, SHADER_UNIFORM_VEC3, 3);

	Matrix mat = ent->model.transform;
	//mat = MatrixMultiply(mat, MatrixRotateX(-90.0f*DEG2RAD));
	SetShaderValueMatrix(handler->ent_shader, handler->ent_shader_locs.locs[LC_MODEL_MAT], mat);
	//SetShaderValueTexture(handler->ent_shader, handler->ent_shader_locs.locs[LC_MODEL_TEX], ent->model.materials[0].maps[0].texture);

	for(int m = 0; m < ent->model.materialCount; m++) {
		ent->model.materials[m].shader = handler->ent_shader;
	}

	DrawModelEx(ent->model, pos, axis, angle, Vector3Scale(Vector3One(), scale), WHITE);
}

