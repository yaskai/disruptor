#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "geo.h"
#include "ai.h"
#include "../include/log_message.h"
#include "../include/sort.h"
#include "pm.h"

#define STEP_SIZE 8.0f

float grid_tick = 0.0f;

Vector3 debug_bullet_dest;
Vector3 debug_bullet_norm;

MapSection *ptr_handler_sect = NULL;
EntityHandler *ptr_handler_self = NULL;

typedef void (*OnHitFunc)(Entity *ent, short damage);
OnHitFunc on_hit_funcs[] = {
	&OnHitPlayer,
	&OnHitTurret,
	&OnHitMaintainer,
	&OnHitRegulator,
	&OnHitBug,
};

void LoadEntityBaseModels(EntityHandler *handler) {
	char *prefix = "resources/models";
	handler->base_ent_models[ENT_TURRET] = LoadModel(TextFormat("%s/enemies/turret.glb", prefix));	 
	handler->base_ent_models[ENT_MAINTAINER] = LoadModel(TextFormat("%s/enemies/maintainer.glb", prefix));	 
}

int base_ent_anims_count[16] = {0};
ModelAnimation *base_ent_anims[16] = {0};
void LoadEntityBaseAnims() {
	char *prefix = "resources/models";
	base_ent_anims[ENT_MAINTAINER] = LoadModelAnimations(TextFormat("%s/enemies/maintainer.glb", prefix), &base_ent_anims_count[ENT_MAINTAINER]);	 
}

Model projectile_models[4];

void EntHandlerInit(EntityHandler *handler, vEffect_Manager *effect_manager) {

	handler->count = 0;
	handler->capacity = 128;
	handler->ents = calloc(handler->capacity, sizeof(Entity));
	handler->player_id = 0;

	LoadEntityBaseModels(handler);
	LoadEntityBaseAnims();

	handler->ai_tick = 0;

	EntGridInit(handler);
	handler->checkpoint_list = (CheckPointList) {0}; 
	handler->checkpoint_list.active = -1;

	handler->effect_manager = effect_manager;

	handler->projectile_capacity = 128;
	handler->projectiles = calloc(handler->projectile_capacity, sizeof(Projectile));
}

void EntHandlerClose(EntityHandler *handler) {
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

	prev_pos_tick -= dt;
	if(prev_pos_tick < 0.0f) {
		for(u16 i = 0; i < handler->count; i++) {
			Entity *ent = &handler->ents[i];
			ent->comp_transform.prev_pos = ent->comp_transform.position;
		}

		prev_pos_tick = 4*dt;
	}

	Entity *player_ent = &handler->ents[handler->player_id];
	PlayerUpdate(player_ent, dt);

	if(player_ent->comp_ai.state == STATE_DEAD && player_ent->comp_ai.task_data.timer >= 2) {
		ReloadEntities(handler, sect, 1);
		return;
	}

	render_list.count = 0;
	Vector3 view_dir = player_ent->comp_transform.forward;

	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];

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
		}

		ent->comp_health.bug_box = BoxTranslate(
			ent->comp_health.bug_box,
			Vector3Add(ent->comp_transform.position, ent->comp_health.bug_point)	
		);
		ent->comp_health.damage_cooldown -= dt;

		comp_Health *health = &ent->comp_health;
		if(health->component_valid) {
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

	handler->ai_tick -= dt;
	if(handler->ai_tick < 0.0f) {
		// Do next ai update in ~11 frames
		handler->ai_tick = (AI_TICK_RATE*dt);

		AiSystemUpdate(handler, sect, dt);
	}

	ManageProjectiles(handler, sect, dt);

	grid_tick -= dt;
	if(grid_tick < 0.0f) {
		// Do next grid update in ~1 frames
		grid_tick = (1*dt);

		UpdateGrid(handler);
	}
}

// Entity draw loop, every frame, draws all entities.
void RenderEntities(EntityHandler *handler, float dt) {
	EntGrid *grid = &handler->grid;

	//for(u16 i = 0; i < render_list.count; i++) {
	for(u16 i = 0; i < handler->count; i++) {;
		Entity *ent = &handler->ents[i];

		if(!(ent->flags & ENT_ACTIVE)) 
			continue;

		switch(ent->type) {
			case ENT_TURRET:
				TurretDraw(ent);
				break;

			case ENT_MAINTAINER:
				MaintainerDraw(ent, dt);
				break;

			case ENT_DISRUPTOR:
				BugDraw(ent);
				break;
		}
	}

	RenderProjectiles(handler);
}

// Update logic for turret
void TurretUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	float angle_min = -70;
	float angle_max =  70;

	if((ent->comp_ai.input_mask & AI_INPUT_SELF_GLITCHED)) {
		ent->comp_ai.task_data.timer = 0;
		ent->comp_ai.task_data.task_id = TASK_FIRE_WEAPON;
		
		float angle = sinf(GetTime() * 1.5f);
		angle = Clamp(angle, angle_min, angle_max);

		if(ent->comp_weapon.ammo > 0) {
			ent->comp_transform.forward = Vector3RotateByAxisAngle(ent->comp_transform.targ_look, UP, angle);		
		} else {
			ent->comp_ai.task_data.task_id = TASK_WAIT_TIME;
		}

	} else {
		ent->comp_transform.forward = Vector3Lerp(ent->comp_transform.forward, ent->comp_transform.targ_look, 10*dt);
	}

	if(ent->comp_ai.task_data.task_id == TASK_FIRE_WEAPON) {
		TurretShoot(ent, handler, sect, dt);
	}
}

// Draw logic for turret
void TurretDraw(Entity *ent) {
	comp_Transform *ct = &ent->comp_transform;

	float yaw = atan2f(ct->forward.x, -ct->forward.y);

	float xz_len = Vector2Length( (Vector2) { ct->forward.x, ct->forward.y } );
	float pitch = atan2f(-ct->forward.z, xz_len);

	Matrix mat_base = MatrixMultiply(ent->model.transform, MatrixTranslate(ct->position.x, ct->position.y, ct->position.z));

	Matrix mat_gun = MatrixMultiply(MatrixRotateX(pitch), MatrixRotateY(yaw));
	mat_gun = MatrixMultiply(mat_gun, MatrixRotateX(90*DEG2RAD));
	mat_gun = MatrixMultiply(mat_gun, MatrixTranslate(ct->position.x, ct->position.y, ct->position.z));

	DrawMesh(ent->model.meshes[1], ent->model.materials[1], mat_gun);
	DrawMesh(ent->model.meshes[0], ent->model.materials[1], mat_base);
}

void TurretShoot(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Weapon *weap = &ent->comp_weapon;
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	weap->cooldown -= dt;
	if(weap->cooldown > 0)
		return; 

	if(!(ai->input_mask & AI_INPUT_SELF_GLITCHED)) {
		// Not disrupted and see's player
		if(ai->input_mask & AI_INPUT_SEE_PLAYER) {
			Entity *targ_ent = &handler->ents[ai->task_data.target_entity];

			Vector3 look_point = targ_ent->comp_transform.position;
			look_point = Vector3Add(look_point, Vector3Scale(targ_ent->comp_transform.velocity, 5*dt));

			Vector3 targ = Vector3Normalize(Vector3Subtract(look_point, ct->position));
			if(Vector3DotProduct(targ, ct->start_forward) >= -0.1f)
				ct->targ_look = Vector3Lerp(ct->targ_look, targ, 100*dt);
				//ct->targ_look = targ;

		} else {
			// Disrupted
			Entity *targ_ent = &handler->ents[ai->task_data.target_entity];

			Vector3 look_point = ai->task_data.known_target_position;
			look_point = Vector3Add(look_point, Vector3Scale(targ_ent->comp_transform.velocity, 10*dt));

			Vector3 targ = Vector3Normalize(Vector3Subtract(look_point, ct->position));

			if(Vector3DotProduct(targ, ct->start_forward) >= -0.1f)
				ct->targ_look = targ;
			else 
				ct->targ_look = ct->start_forward;
		}
	} else {
		ct->targ_look.z = Lerp(ct->targ_look.z, 0, dt * 5);

		if(ent->comp_ai.disrupt_timer >= 99.9f)
			weap->ammo = weap->clip_size;
	}

	if(weap->ammo <= 0) 
		return;

	Vector3 trace_start = ct->position;
	trace_start.z += 12;
	trace_start = Vector3Add(trace_start, Vector3Scale(ct->forward, 38));

	Vector3 dir = ct->forward;
	float offset = GetRandomValue(-4, 4) * 0.01f;	

	Vector3 right = Vector3CrossProduct(ct->forward, UP);
	dir = Vector3Add(dir, Vector3Scale(right, offset));

	offset = GetRandomValue(-4, 4) * 0.01f;
	dir = Vector3Add(dir, Vector3Scale(UP, offset));

	dir = Vector3Normalize(dir);

	bool hit = false;
	// * NOTE:
	// Purpose of the dummy value is to cause no dammage on the first few shots, gives the player a warning for fairness.
	bool dummy = (ent->comp_weapon.ammo > ent->comp_weapon.clip_size - 3);
	Vector3 bullet_dest = TraceBullet(handler, sect, trace_start, dir, ent->id, &hit, dummy);

	Vector3 trail_end = Vector3Add(trace_start, Vector3Scale(dir, Vector3Distance(trace_start, bullet_dest)));
	if(!hit) {
		trail_end = Vector3Add(trace_start, Vector3Scale(ct->forward, 2000.0f));
	}
	
	// Add bullet trail effect
	float dist = Vector3Distance(trace_start, trail_end);
	vEffectsAddTrail(handler->effect_manager, trace_start, trail_end);

	weap->cooldown = 0.075f;
	weap->ammo--;
}

void MaintainerUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Ai *ai = &ent->comp_ai;
	comp_Transform *ct = &ent->comp_transform;

	switch(ai->state) {
		case STATE_IDLE:
			ent->curr_anim = 0;
			break;

		case STATE_MOVE:
			ent->curr_anim = 1;
			break;
	}

	if((ai->input_mask & AI_INPUT_SELF_GLITCHED) && ai->state != STATE_DEAD) {
		ai->curr_schedule = SCHED_IDLE;
		float angle = sinf(GetTime()*20) * PI;
		ent->comp_transform.forward = Vector3RotateByAxisAngle(ent->comp_transform.forward, UP, angle);
		ent->model.transform = MatrixMultiply(MatrixRotateX(90*DEG2RAD), MatrixRotateZ(angle)); 
	}

	if(ai->input_mask & AI_INPUT_SEE_GLITCHED)
		ai->curr_schedule = SCHED_FIX_FRIEND;

	ent->comp_health.hit_box = BoxTranslate(ent->comp_health.hit_box, ent->comp_transform.position);

	if(ai->curr_schedule == SCHED_MAINTAINER_ATTACK) {
		if(ai->input_mask & AI_INPUT_SEE_PLAYER) {
			ct->forward =  Vector3Lerp(ct->forward, Vector3Subtract(ai->task_data.known_target_position, ct->position), 30*dt);
			ct->forward.z = 0;
			ct->forward = Vector3Normalize(ct->forward);

			float angle = atan2f(-ct->forward.x, ct->forward.y);
			ent->model.transform = MatrixMultiply(MatrixRotateX(90*DEG2RAD), MatrixRotateZ(angle+(90*DEG2RAD)*-1));
		}
	}

	EntMove(ent, sect, handler, dt);
	//ent->anim_frame = (ent->anim_frame + 1) % ent->animations[ent->curr_anim].frameCount;
}

void MaintainerDraw(Entity *ent, float dt) {
	comp_Ai *ai = &ent->comp_ai;

	if(ai->state == STATE_DEAD) {
		Vector3 pos = ent->comp_transform.position;		
		pos.z -= 20;

		DrawModelEx(
			ent->model,
			pos,
			Vector3CrossProduct(ent->comp_transform.forward, UP),
			90,
			Vector3Scale(Vector3One(), 0.1f),
			LIGHTGRAY 
		);
		return;
	}
	
	Vector3 pos = ent->comp_transform.position;
	DrawModel(ent->model, pos, 0.1f, LIGHTGRAY);
}

// Update an entitie's ai component
void AiComponentUpdate(Entity *ent, EntityHandler *handler, comp_Ai *ai, Ai_TaskData *task_data, MapSection *sect, float dt) {
	if(ent->comp_ai.state == STATE_DEAD)
		return;

	// Handle interrupts
	// * NOTE: 
	// Complete this later
	for(u32 i = 0; i < 32; i++) {
		u32 mask = (1 << i);
		if(task_data->interrupt_mask & mask) {

		}
	}

	// Update input mask
	AiCheckInputs(ent, handler, sect);

	// Execute current ai scehdule
	AiDoSchedule(ent, handler, sect, ai, task_data, dt);

	// Tick timer down
	ai->task_data.timer--;

	// Tick disrupt timer down (if disrupted)
	if(ai->input_mask & AI_INPUT_SELF_GLITCHED) {
		ai->disrupt_timer--;
	}
}

// Ai tick, not every frame,
// has it's own tick timer (every 11 frames)
void AiSystemUpdate(EntityHandler *handler, MapSection *sect, float dt) {
	Entity *player = &handler->ents[handler->player_id];
	player->comp_ai.navgraph_id = -1;
	for(u16 j = 0; j < sect->navgraph_count; j++) {
		NavGraph *graph = &sect->navgraphs[j];

		int closest_node = FindClosestNavNodeInGraph(player->comp_transform.position, graph);
		if(closest_node > -1) {
			player->comp_ai.navgraph_id = j;
			player->comp_ai.curr_navnode_id = closest_node;
			break;
		}
	}

	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];
		// Don't update player ai (does not make sense)
		if(handler->player_id == i)
			continue;

		// Don't update invalid components
		if(!ent->comp_ai.component_valid)
			continue;

		comp_Ai *ai = &ent->comp_ai;
		AiComponentUpdate(ent, handler, ai, &ai->task_data, sect, dt);
	}
}

// Update senses inputs for an entitie's AI component,
// executed once per frame for every entity with a valid component.
void AiCheckInputs(Entity *ent, EntityHandler *handler, MapSection *sect) {
	comp_Ai *ai = &ent->comp_ai;

	comp_Transform *ct = &ent->comp_transform;
	BvhTree *bvh = &sect->bvh[0];

	// ** Check if player is visible **	
	//
	// Clear 'see player' flag
	bool prev_seen_player = (ai->input_mask & AI_INPUT_SEE_PLAYER);
	ai->input_mask &= ~AI_INPUT_SEE_PLAYER;

	if(prev_seen_player && !(ai->input_mask & AI_INPUT_SEE_PLAYER))
		ai->input_mask |= AI_INPUT_LOST_PLAYER;

	Entity *player_ent = &handler->ents[handler->player_id];

	Vector3 eye_pos = Vector3Add(ct->position, Vector3Scale(UP, 0.0f));
	Vector3 to_player = Vector3Normalize(Vector3Subtract(player_ent->comp_transform.position, eye_pos));
	float d_to_player = Vector3LengthSqr(to_player);

	// Player is in ai's sight cone
	if(Vector3DotProduct(ct->forward, to_player) >= ai->sight_cone && d_to_player <= 1000.0f) { 
		// Check for obstructions
		Ray ray = (Ray) { .position = eye_pos, .direction = to_player };

		EntTraceData ent_tr = EntTraceDataEmpty();
		TraceEntities(ray, handler, 1000.0f, ent->id, &ent_tr);

		// Trace map geometry
		// Small affordance to account for spatial partition structure (+32)
		BvhTraceData tr = TraceDataEmpty();
		BvhTracePointEx(ray, sect, bvh, 0, &tr, ent_tr.dist + BoundsToRadius(player_ent->comp_transform.bounds));

		// Player hitbox collision closer than possible surface collision.
		// No obstruction, player is visible 
		if(!tr.hit && ent_tr.hit_ent == handler->player_id) {
			ai->input_mask |= AI_INPUT_SEE_PLAYER;
			ai->input_mask &= ~AI_INPUT_LOST_PLAYER;
		}
	}

	if(ai->task_data.target_entity == handler->player_id && (ai->input_mask & AI_INPUT_SEE_PLAYER)) {
		ai->task_data.known_target_position = player_ent->comp_transform.position;
	}
	// ***

	ai->input_mask &= ~AI_INPUT_HEAR_PLAYER;
	bool in_hearing_range = (d_to_player < ai->hear_distance*ai->hear_distance);
	if(in_hearing_range && Vector3LengthSqr(player_ent->comp_transform.velocity) >= 0.1f) {
		ai->input_mask |= AI_INPUT_HEAR_PLAYER;

		if(!(ai->input_mask & AI_INPUT_LOST_PLAYER)) {
			ai->task_data.known_target_position = player_ent->comp_transform.position;
		}
	}
}

// Execute ai schedule
void AiDoSchedule(Entity *ent, EntityHandler *handler, MapSection *sect, comp_Ai *ai, Ai_TaskData *task_data, float dt) {
	// Wait time special case:
	task_data->timer -= dt;
	if(task_data->task_id == TASK_WAIT_TIME) {
		if(task_data->timer > 0) {
			return;
		}
	}

	// Schedule state machine:
	switch(ai->curr_schedule) {
		case SCHED_IDLE:
			break;

		case SCHED_PATROL:
			AiPatrol(ent, sect, dt);
			break;

		case SCHED_WAIT:
			break;

		case SCHED_FIX_FRIEND:
			AiFixFriendSchedule(ent, handler, sect, dt);
			break;

		case SCHED_SENTRY:
			AiSentrySchedule(ent, handler, sect, dt);
			break;

		case SCHED_CHASE_PLAYER:
			AiChasePlayerSchedule(ent, handler, sect, dt);
			break;

		case SCHED_MAINTAINER_ATTACK:
			AiMaintainerAttackSchedule(ent, handler, sect, dt);
			break;

		case SCHED_MAINTAINER_MAKE_NEW:
			AiMaintainerMakeNewSchedule(ent, handler, sect, dt);
			break;
	}
}

#define NODE_REACH_RADIUS (32.0f*32.0f)
void AiPatrol(Entity *ent, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	Ai_TaskData *task = &ai->task_data;
	NavPath *path = &task->path;

	NavGraph *graph = &sect->navgraphs[ai->navgraph_id];

	if(task->task_id == TASK_MAKE_PATROL_PATH) {
		//printf("setting path...\n");
		u16 new_targ = GetRandomValue(0, graph->node_count-1);
		if(MakeNavPath(ent, graph, new_targ) == true) {
			task->task_id = TASK_GOTO_POINT;
			ai->state = STATE_MOVE;

		} else { 
			ai->state = STATE_IDLE;

			task->timer = 0.5f;
			task->task_id = TASK_WAIT_TIME;

			ct->velocity = Vector3Zero();
		}

		return;
	}

	if(!task->path_set) {
		//printf("path not set...\n");
		task->task_id = TASK_MAKE_PATROL_PATH;		

		ai->state = STATE_IDLE;
		return;
	}

	if(Vector3Length(ct->velocity) == 0 && task->task_id == TASK_GOTO_POINT && path->curr == 0) {
		AiMoveToNode(ent, graph, path->curr++);
		task->task_id = TASK_GOTO_POINT;

		ai->state = STATE_MOVE;
		return;
	}

	Vector3 to_targ = (Vector3Subtract(task->target_position, ct->position));
	if(Vector3LengthSqr(to_targ) <= NODE_REACH_RADIUS) {
		if(!AiMoveToNode(ent, graph, path->curr++)) {
			ct->velocity = Vector3Zero();

			task->task_id = TASK_WAIT_TIME;
			task->timer = 0.05f;

			task->path_set = false;

			ai->state = STATE_IDLE;

			return;
		}
	}
}

// Maintainer find and fix
void AiFixFriendSchedule(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	Ai_TaskData *task = &ai->task_data;
	NavPath *path = &task->path;

	NavGraph *graph = &sect->navgraphs[ai->navgraph_id];

	Entity *friend = &handler->ents[ai->task_data.target_entity];

	// **
	// Move to target entity
	if(task->task_id == TASK_GOTO_POINT && (friend->comp_ai.input_mask & AI_INPUT_SELF_GLITCHED)) {
		if(friend->comp_ai.navgraph_id != ai->navgraph_id)
			return;

		if(!task->path_set) {
			MakeNavPath(ent, graph, FindClosestNavNodeInGraph(friend->comp_transform.position, graph));	
			task->path_set = true;
			task->task_id = TASK_GOTO_POINT;
			return;
		}

		if(Vector3Length(ct->velocity) == 0 && task->task_id == TASK_GOTO_POINT && path->curr == 0) {
			AiMoveToNode(ent, graph, path->curr++);
			task->task_id = TASK_GOTO_POINT;

			ai->state = STATE_MOVE;
			return;
		}

		Vector3 to_targ = (Vector3Subtract(task->target_position, ct->position));
		if(Vector3LengthSqr(to_targ) <= NODE_REACH_RADIUS || CheckCollisionBoxes(ct->bounds, friend->comp_transform.bounds) ) { 
			if(!AiMoveToNode(ent, graph, path->curr++)) {
				ct->velocity = Vector3Zero();

				task->task_id = TASK_DO_FIX;
				task->timer = 100;
				
				return;
			}
		}

		if(CheckCollisionBoxes(ct->bounds, friend->comp_transform.bounds)) {
			ct->velocity = Vector3Zero();

			task->task_id = TASK_DO_FIX;
			task->timer = 100;
			
			return;
		}
	} else if (task->task_id == TASK_GOTO_POINT && !(friend->comp_ai.input_mask & AI_INPUT_SELF_GLITCHED)) {
		if(Vector3Length(ct->velocity) == 0 && task->task_id == TASK_GOTO_POINT && path->curr == 0) {
			AiMoveToNode(ent, graph, path->curr++);
			task->task_id = TASK_GOTO_POINT;

			ai->state = STATE_MOVE;
			return;
		}

		Vector3 to_targ = (Vector3Subtract(task->target_position, ct->position));
		if(Vector3LengthSqr(to_targ) <= NODE_REACH_RADIUS) { 
			if(!AiMoveToNode(ent, graph, path->curr++)) {
				ct->velocity = Vector3Zero();

				ai->curr_schedule = SCHED_MAINTAINER_ATTACK;
				
				return;
			}
		}
	}

	// **
	// Fix friend
	if(task->task_id == TASK_DO_FIX) {
		if(task->timer < 0) {
			// Perform fix action
			DoFix(&handler->ents[task->target_entity]);

			// End schedule
			ai->curr_schedule = SCHED_IDLE;

			// Backoff
			task->task_id = TASK_GOTO_POINT;
			task->path_set = false;

			ai->input_mask &= ~AI_INPUT_SEE_GLITCHED;

			Vector3 targ_point = Vector3Subtract(ct->position, Vector3Scale(ct->forward, 128)); 
			i16 targ_node = FindClosestNavNodeInGraph(targ_point, graph);
			MakeNavPath(ent, graph, targ_node);
		}
	}
}

// For turret
void AiSentrySchedule(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	Ai_TaskData *task = &ai->task_data;

	if(ai->input_mask & AI_INPUT_SELF_GLITCHED) {
		AiSentryDisruptionSchedule(ent, handler, sect, dt);
		return;
	}

	if(task->task_id == TASK_FIRE_WEAPON) {
		if(ent->comp_weapon.ammo <= 0) {
			task->task_id = TASK_RELOAD_WEAPON;
			task->timer = 10.01f;
			//printf("reload start\n");
		}
		return;
	}

	if(task->task_id == TASK_RELOAD_WEAPON) {
		if(task->timer <= 0) {
			ent->comp_weapon.ammo = ent->comp_weapon.clip_size;
			ent->comp_weapon.cooldown = 10.45f;
			task->task_id = TASK_WAIT_TIME;
			task->timer = 20.01f;
			//printf("reload done\n");
		}

		return;
	}

	if(ai->input_mask & AI_INPUT_SEE_PLAYER) {
		task->task_id = TASK_LOOK_AT_ENTITY;
		task->target_entity = handler->player_id;

	} else if((ai->task_data.task_id != TASK_FIRE_WEAPON) && ent->comp_weapon.ammo <= 0) {
		Vector3 targ = Vector3Lerp(ct->forward, ct->start_forward, 0.1f);
		/*
		if(ai->input_mask & AI_INPUT_HEAR_PLAYER && ai->input_mask & AI_INPUT_LOST_PLAYER)
			targ = ai->task_data.known_target_position;	
		*/

		ct->targ_look = targ;

		task->task_id = TASK_WAIT_TIME;
		task->timer = 0.1f;
	}

	if(task->task_id == TASK_LOOK_AT_ENTITY) {
		Vector3 look_point = Vector3Add(ct->position, ct->forward);

		if(ai->input_mask & AI_INPUT_SEE_PLAYER) {
			look_point = handler->ents[task->target_entity].comp_transform.position;
			//ai->task_data.known_target_position = look_point;

			Vector3 target_vel = handler->ents[task->target_entity].comp_transform.velocity;
			look_point = Vector3Add(look_point, Vector3Scale(target_vel, 0.25f));
		} else if(ai->input_mask & AI_INPUT_LOST_PLAYER) {
			look_point = ai->task_data.known_target_position;
		}

		Vector3 targ = Vector3Normalize(Vector3Subtract(look_point, ct->position));
		ct->targ_look = targ;

		if(ai->input_mask & AI_INPUT_SEE_PLAYER) {
			task->task_id = TASK_FIRE_WEAPON;
			ent->comp_weapon.ammo = ent->comp_weapon.clip_size;
			ent->comp_weapon.cooldown = 1.0f;

		} else if(ai->input_mask & AI_INPUT_LOST_PLAYER) {
			task->task_id = TASK_WAIT_TIME;
			task->timer = 25.0f;
		}
	}
}

// Turret disrupted
void AiSentryDisruptionSchedule(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;
	comp_Weapon *weap = &ent->comp_weapon;

	Ai_TaskData *task = &ai->task_data;

	if(task->task_id == TASK_FIRE_WEAPON) {
		return;
	}

	if(ai->disrupt_timer <= 0) {
		ai->curr_schedule = SCHED_IDLE;
	}
}

void AiChasePlayerSchedule(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	Ai_TaskData *task = &ai->task_data;
	NavPath *path = &task->path;

	NavGraph *graph = &sect->navgraphs[ai->navgraph_id];

	Entity *player = &handler->ents[handler->player_id];

	if(player->comp_ai.navgraph_id != ai->navgraph_id) {
		//ct->velocity = Vector3Zero();
		return;
	}

	// **
	// Move to target entity
	if(task->task_id == TASK_GOTO_POINT) {
		if(!task->path_set) {
			MakeNavPath(ent, graph, FindClosestNavNodeInGraph(player->comp_transform.position, graph));	
			task->path_set = true;
			task->task_id = TASK_GOTO_POINT;
			return;
		}

		if(Vector3Length(ct->velocity) == 0 && task->task_id == TASK_GOTO_POINT && path->curr == 0) {
			AiMoveToNode(ent, graph, path->curr++);
			task->task_id = TASK_GOTO_POINT;

			ai->state = STATE_MOVE;
			return;
		}

		Vector3 to_targ = (Vector3Subtract(task->target_position, ct->position));
		if(Vector3LengthSqr(to_targ) <= NODE_REACH_RADIUS) {
			if(!AiMoveToNode(ent, graph, path->curr++)) {
				task->path_set = false;
				return;
			}
		}
	}
}

// Maintainer attack
// * NOTE:
// temporary throw projectile behavior,
// to be changed later...
void AiMaintainerAttackSchedule(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;

	comp_Ai *ai = &ent->comp_ai;
	Ai_TaskData *task = &ai->task_data;

	if(ai->input_mask & AI_INPUT_SEE_PLAYER) {
		task->known_target_position = handler->ents[handler->player_id].comp_transform.position;

		if(task->task_id == TASK_THROW_PROJECTILE) {
			Vector3 dir = ct->forward;
			float offset_h = GetRandomValue(-3, 3) * 0.01f;
			float offset_v = GetRandomValue(-3, 3) * 0.01f;

			Vector3 right = Vector3CrossProduct(ct->forward, UP);
			dir = Vector3Add(dir, Vector3Scale(right, offset_h));
			dir.z += offset_v;
			dir = Vector3Normalize(dir);

			ProjectileThrow(ent, ct->position, dir, Vector3Distance(task->known_target_position, ct->position), 0, handler);

			task->task_id = TASK_WAIT_TIME;
			task->timer = 10.0f + GetRandomValue(0, 15);

			return;
		}
	}

	task->task_id = TASK_THROW_PROJECTILE;
	task->timer = 10.0f;
}

void AiMaintainerMakeNewSchedule(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Ai *ai = &ent->comp_ai;

	ai->curr_schedule = ai->prev_schedule;
	ai->prev_schedule = ai->curr_schedule;
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
			
			// * NOTE:
			// Change from transform bounds to actual damage hit box later 
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
			if(tmax_X < tmax_Z) {
				cell.r += step_y;
				t = tmax_Y;
				tmax_Y += td_z;
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
	BvhTree *bvh = &sect->bvh[BVH_POINT];
	BvhTraceData tr = TraceDataEmpty();
	BvhTracePointEx(ray, sect, bvh, 0, &tr, FLT_MAX);
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
			RayCollision coll = GetRayCollisionBox(ray, ent->comp_transform.bounds);
				
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
			if(tmax_X < tmax_Z) {
				cell.r += step_y;
				t = tmax_Y;
				tmax_Y += td_z;
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
		OnHitEnt(hit_ent, handler->ents[sender].comp_weapon.damage);
	}

	debug_bullet_dest = dest;

	return dest;

	// *NOTE:
	// BVH is still more accurate for point traces so I'll continue using it for now...
	/*
	Bsp_TraceData trace = Bsp_TraceDataEmpty();
	Bsp_RecursiveTraceEx(&sect->bsp[0], sect->bsp[0].first_node, 0, 1, origin, dest, &trace);
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

// * NOTE:
// This function is placeholder and mostly for testing,
// in the future I'll probably use actual ai inputs for this...
// Sound, sight, etc.
void AlertMaintainers(EntityHandler *handler, u16 disrupted_id) {
	Entity *disrupted_ent = &handler->ents[disrupted_id];
	comp_Ai *disrupted_ai = &disrupted_ent->comp_ai;

	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];
		comp_Ai *ai = &ent->comp_ai;

		if(!ai->component_valid)	
			continue;

		if(ent->type == ENT_PLAYER)
			continue;

		if(ent->type == ENT_DISRUPTOR)
			continue;

		if(ent->comp_ai.state == STATE_DEAD)
			return;
	
		if(ai->navgraph_id != disrupted_ai->navgraph_id)	
			continue;

		if(ai->navgraph_id == -1)
			continue;

		if(ent->type != ENT_MAINTAINER)
			continue;

		if(ent->id == disrupted_id)
			continue;

		ai->task_data.timer = 0;
		ai->input_mask |= AI_INPUT_SEE_GLITCHED;
		ai->curr_schedule = SCHED_FIX_FRIEND;
		ai->task_data.schedule_id = SCHED_FIX_FRIEND;
		ai->task_data.task_id = TASK_GOTO_POINT;
		ai->task_data.target_entity = disrupted_id;
		ai->task_data.path_set = false;
	}
}

// When entity is hit with bullet or other damaging thing
void OnHitEnt(Entity *ent, short damage) {
	comp_Health *health = &ent->comp_health;
	
	if(health->damage_cooldown > 0)
		return;

	health->amount -= damage;
	health->damage_cooldown = 0.1f;

	comp_Ai *ai = &ent->comp_ai;

	if(health->amount <= 0) {
		ent->comp_ai.state = STATE_DEAD;
		ent->comp_ai.curr_schedule = SCHED_DEAD;

		ent->flags &= ~ENT_COLLIDERS;
	}

	if(health->on_hit > -1) {
		on_hit_funcs[health->on_hit](ent, damage);
	}

	if(ent->type == ENT_PLAYER)
		OnHitPlayer(ent, damage);
}

// When turret is hit
// * NOTE:
// Nothing right now, 
// unsure design-wise what to do here...
void OnHitTurret(Entity *ent, short damage) {

}

// Maintainer hit
void OnHitMaintainer(Entity *ent, short damage) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	if(ai->input_mask & AI_INPUT_SELF_GLITCHED) {
		ent->comp_health.amount = 0;
		ai->state = STATE_DEAD;
	}

	Vector3 to_player = Vector3Subtract(ptr_handler_self->ents[ptr_handler_self->player_id].comp_transform.position, ct->position);
	to_player = Vector3Normalize(to_player);

	Vector3 prev_wish = ai->wish_dir;
	float prev_speed = ai->speed;

	ai->speed = 6000;

	Vector3 knockback = Vector3Negate(to_player);
	if(ai->state != STATE_DEAD)
		knockback.z = 0;
	else {
		short dice = GetRandomValue(0, 6);
		if(dice == 6) {
			knockback.z = 0.99f;
			ai->speed = 2500;
		}
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
void OnHitRegulator(Entity *ent, short damage) {

}

// Fix disrupted entity
void DoFix(Entity *ent) {
	ent->comp_ai.input_mask &= ~AI_INPUT_SELF_GLITCHED;

	switch(ent->type) {
		case ENT_TURRET:
			OnFixTurret(ent);
			break;
			
		case ENT_MAINTAINER:
			OnFixMaintainer(ent);
			break;

		case ENT_REGULATOR:
			OnFixRegulator(ent);
			break;
	}
}

void OnFixTurret(Entity *ent) {
	comp_Ai *ai = &ent->comp_ai;

	ai->curr_schedule = SCHED_SENTRY;
	ai->task_data.schedule_id = SCHED_SENTRY;
	ai->task_data.task_id = TASK_WAIT_TIME;
	ai->task_data.timer = 1;
}

void OnFixMaintainer(Entity *ent) {
}

void OnFixRegulator(Entity *ent) {
}

// Move entity through space
void EntMove(Entity *ent, MapSection *sect, EntityHandler *handler, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	ct->bounds = BoxTranslate(ct->bounds, ct->position);

	ct->on_ground = pm_CheckGround(ct, ct->position);
	pm_ApplyGravity(ct, dt);

	Vector3 wish_dir = ai->wish_dir;
	float wish_speed = ai->speed;
	Vector3 wish_vel = Vector3Scale(wish_dir, wish_speed);

	pm_GroundFriction(ct, dt);
	pm_Accelerate(ct, wish_dir, wish_speed, 20.0f, dt);

	if(Vector3LengthSqr(ct->velocity) <= 1.0f)
		return;

	pmTraceData move_data = (pmTraceData) { .start_in_solid = -1, .end_in_solid = -1, .block = 0 };
	pm_TraceMove(ct, ct->position, ct->velocity, &move_data, dt);

	ct->position = move_data.end_pos;
	ct->velocity = move_data.end_vel;
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

	OnHitEnt(ent, (short)damage);

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
	u8 states[handler->count];
	for(u16 i = 0; i < handler->count; i++) {
		states[i] = handler->ents[i].comp_ai.state;
	}

	// Clear entities
	handler->count = 0;

	for(u16 i = 0; i < handler->spawn_list.count; i++) {
		ProcessEntity(&handler->spawn_list.arr[i], handler, NULL);

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

	// Handle checkpoint logic
	if(handler->checkpoint_list.active > -1)
		handler->player_start = handler->checkpoint_list.points[handler->checkpoint_list.active];

	SpawnPlayer(&handler->ents[handler->player_id], handler->player_start);
	handler->ents[handler->bug_id].comp_ai.state = 0;	

	// Set up ai navigation
	AiNavSetup(handler, sect);

	// Reset ai tick
	handler->ai_tick = 1.0f; 
}

