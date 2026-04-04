#include <stdio.h>
#include "ai.h"
#include "ent.h"
#include "sched_defs.c"
#include "../include/log_message.h"

u8 ExecWaitTime(comp_Ai *ai, float dt) {
	ai->task_state.timer -= dt;	
	//ai->task_state.timer--;
	if(ai->task_state.timer <= 0) {
		ai->task_state.timer = 0;
		return 1;
	}

	return 0;
}

u8 ExecFireWeapon(Entity *ent, comp_Ai *ai) {
	//Message("ExecFireWeapon()", ANSI_BLUE);
	//printf("ammo: %d\n", ent->comp_weapon.ammo);

	if(ent->comp_weapon.ammo <= 0)
		return 1;

	return 0;
}

u8 ExecReloadWeapon(Entity *ent, comp_Ai *ai, float dt) {
	if(ai->task_state.timer <= 0 && !ai->task_state.is_init) {
		ai->task_state.timer = ent->comp_weapon.reload_time_amnt;
		ai->task_state.is_init = true;
	}

	ai->task_state.timer -= dt;
	//ai->task_state.timer--;
	if(ai->task_state.timer <= 0) {
		ai->task_state.timer = 0;
		ent->comp_weapon.ammo = ent->comp_weapon.clip_size;
		return 1;
	}
	
	return 0;
}

u8 ExecLookAtEntity(Entity *ent, EntityHandler *handler, comp_Ai *ai) {
	// Actual rotation happens in per-frame update
	Entity *targ = &handler->ents[ai->targ_data.ent_id];
	comp_Transform *ct = &ent->comp_transform;

	Vector3 to_targ = Vector3Normalize(Vector3Subtract(targ->comp_transform.position, ct->position));
	if(Vector3DotProduct(to_targ, ct->forward) >= 0.95f)
		return 1;

	return 0;
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
		// Don't update invalid components
		if(!ent->comp_ai.component_valid)
			continue;

		comp_Ai *ai = &ent->comp_ai;
		AiComponentUpdate(ent, handler, ai, sect, dt);
	}
}

// Update an entity's ai component
void AiComponentUpdate(Entity *ent, EntityHandler *handler, comp_Ai *ai, MapSection *sect, float dt) {
	if(ent->comp_ai.state == STATE_DEAD)
		return;

	// Update input mask
	AiCheckInputs(ent, handler, sect);

	// Execute current ai scehdule
	AiDoSchedule(ent, handler, sect, ai, dt);

	// Tick disrupt timer down (if disrupted)
	if(ai->input_mask & AI_INPUT_SELF_GLITCHED) {
		//ai->disrupt_timer -= dt;
		ai->disrupt_timer--;
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

	Entity *player_ent = &handler->ents[handler->player_id];

	Vector3 eye_pos = Vector3Add(ct->position, Vector3Scale(UP, 0.0f));
	Vector3 to_player = Vector3Normalize(Vector3Subtract(player_ent->comp_transform.position, eye_pos));
	float d_to_player = Vector3LengthSqr(to_player);

	// Player is in ai's sight cone
	if(Vector3DotProduct(ct->forward, to_player) >= ai->sight_cone && d_to_player <= 1000.0f) { 
		// Check for obstructions
		Ray ray = (Ray) { .position = eye_pos, .direction = to_player };

		EntTraceData ent_tr = EntTraceDataEmpty();
		TraceEntities(ray, handler, 2000.0f, ent->id, &ent_tr);

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

	if(ai->targ_data.ent_id == handler->player_id && (ai->input_mask & AI_INPUT_SEE_PLAYER)) {
		ai->targ_data.known_position = player_ent->comp_transform.position;
		// * NOTE: 
		// Remove later, might want to prioritize other entities as targets
		ai->targ_data.ent_id = handler->player_id;
	}
	// ***

	ai->input_mask &= ~AI_INPUT_HEAR_PLAYER;
	bool in_hearing_range = (d_to_player < ai->hear_distance*ai->hear_distance);
	if(in_hearing_range && Vector3LengthSqr(player_ent->comp_transform.velocity) >= 0.1f) {
		ai->input_mask |= AI_INPUT_HEAR_PLAYER;

		if(!(ai->input_mask & AI_INPUT_LOST_PLAYER)) {
			ai->targ_data.known_position = player_ent->comp_transform.position;
		}
	}

	if(!prev_seen_player && (ai->input_mask & AI_INPUT_LOST_PLAYER))
		ai->input_mask &= ~AI_INPUT_LOST_PLAYER;
}

// Execute ai schedule
void AiDoSchedule(Entity *ent, EntityHandler *handler, MapSection *sect, comp_Ai *ai, float dt) {
	Ai_SchedState *sched_state = &ai->sched_state;
	Ai_SchedDef *sched_def = &sched_defs[sched_state->sched_id];

	// Interrupt
	if(ai->input_mask & sched_def->interrupt_mask) {
		AiSetSchedule(ai, sched_def->interrupt_sched);
		return;
	}

	// Run task
	u8 task_id = sched_def->tasks[sched_state->curr_task];
	ai->task_state.task_id = task_id;

	u8 complete = AiDoTask(ent, handler, sect, ai, task_id, dt);
	if(complete) {
		ai->sched_state.curr_task++;
	}

	if(sched_state->curr_task >= sched_def->num_tasks) {
		AiSetSchedule(ai, sched_def->next_sched);
	}
}

u8 AiDoTask(Entity *ent, EntityHandler *handler, MapSection *sect, comp_Ai *ai, u8 task_id, float dt) {
	u8 done = 1;

	switch(task_id) {
		case TASK_GOTO_POINT:
			break;

		case TASK_FIRE_WEAPON:
			done = ExecFireWeapon(ent, ai);
			break;

		case TASK_RELOAD_WEAPON:
			done = ExecReloadWeapon(ent, ai, dt);
			break;

		case TASK_WAIT_TIME:
			done = ExecWaitTime(ai, dt);
			break;

		case TASK_FACE_DIR:
			break;

		case TASK_FIND_POINT:
			break;

		case TASK_MAKE_PATROL_PATH:
			break;

		case TASK_LOOK_AT_ENTITY:
			done = ExecLookAtEntity(ent, handler, ai);
			break;

		case TASK_LOOK_AROUND:
			break;

		case TASK_DO_FIX:
			break;

		case TASK_THROW_PROJECTILE:
			break;
	}

	return done;
}

void AiSetSchedule(comp_Ai *ai, u8 sched_id) {
	ai->sched_state.sched_id = sched_id;
	ai->sched_state.curr_task = 0;
	ai->task_state = (Ai_TaskState) {0};
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


