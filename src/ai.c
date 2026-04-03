#include "ai.h"
#include "ent.h"

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

// Update an entity's ai component
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
