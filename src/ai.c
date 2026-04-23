#include <float.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "ai.h"
#include "ent.h"
#include "sched_defs.c"
#include "../include/log_message.h"

#define AI_TICKRATE 6.0f
float ai_tick = 0.0f;

#define MEELEE_RANGE (48.0f*48.0f)

const bool AI_LOG = false;

short alert_sphere_count = 0;
AlertSphere alert_spheres[MAX_ALERT_SPHERES] = {0};
float alert_clear_tick = 0;

void AddAlertSphere(Entity *ent, float radius) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	if(ai->input_mask & AI_INPUT_SELF_GLITCHED)
		return;

	short id = alert_sphere_count++;
	short iters = 0;
	while(alert_spheres[id].flags & ALERT_SPHERE_ACTIVE) {
		id = (id + 1) % MAX_ALERT_SPHERES;
		iters++;

		if(iters >= MAX_ALERT_SPHERES)
			break;
	}

	alert_spheres[id] = (AlertSphere) {
		.position = ct->position,
		.radius = radius,
		.graph_id = ai->navgraph_id,
		.flags = ( ALERT_SPHERE_ACTIVE )
	};
};

u8 ExecWaitTime(comp_Ai *ai, float dt) {
	if(ai->task_state.timer <= 0) {
		ai->task_state.timer = 0;
		return 1;
	}

	return 0;
}

u8 ExecFireWeapon(Entity *ent, comp_Ai *ai) {
	//if(AI_LOG) Message("ExecFireWeapon()", ANSI_BLUE);

	if(ent->comp_weapon.ammo <= 0)
		return 1;

	return 0;
}

u8 ExecReloadWeapon(Entity *ent, comp_Ai *ai, float dt) {
	//if(AI_LOG) Message("ExecReloadWeapon()", ANSI_BLUE);

	if(ai->task_state.timer <= 0 && !ai->task_state.is_init) {
		ai->task_state.timer = ent->comp_weapon.reload_time_amnt;
		ai->task_state.is_init = true;
	}

	if(ai->task_state.timer <= 0) {
		ai->task_state.timer = 0;
		ent->comp_weapon.ammo = ent->comp_weapon.clip_size;
		return 1;
	}
	
	return 0;
}

u8 ExecLookAtEntity(Entity *ent, EntityHandler *handler, comp_Ai *ai) {
	// Actual rotation happens in per-frame update
	if(ai->targ_data.ent_id < 0)
		return 1;

	Entity *targ = &handler->ents[ai->targ_data.ent_id];
	comp_Transform *ct = &ent->comp_transform;

	Vector3 to_targ = Vector3Normalize(Vector3Subtract(targ->comp_transform.position, ct->position));
	if(Vector3DotProduct(to_targ, ct->forward) >= 0.95f)
		return 1;

	return 0;
}

u8 ExecFaceDir(Entity *ent, Vector3 dir) {
	//dir.z = 0;
	//dir = Vector3Normalize(dir);

	comp_Transform *ct = &ent->comp_transform;
	if(Vector3DotProduct(ct->forward, dir) >= 0.99f)
		return 1;
	
	return 0;
}

#define NODE_REACH_RADIUS (40.0f*40.0f)
u8 ExecGotoPoint(Entity *ent, MapSection *sect) {		
	if(AI_LOG) Message("ExecGotoPoint()", ANSI_BLUE);

	comp_Ai *ai = &ent->comp_ai;
	comp_Transform *ct = &ent->comp_transform;
	NavPath *path = &ai->task_state.path;
	NavGraph *graph = &sect->navgraphs[ai->navgraph_id];

	if(path->curr >= path->count) {
		return 1;
	}

	Vector3 node_pos = graph->nodes[path->nodes[path->curr]].position;
	Vector3 to_node = Vector3Subtract(node_pos, ct->position);
	if(Vector3LengthSqr(to_node) <= MEELEE_RANGE) {
		if(path->curr++ >= path->count)
			return 1;
	}

	//Vector3 to_targ = Vector3Subtract(ai->targ_data.known_position, ent->comp_transform.position);
	//Vector3 to_targ = Vector3Subtract(ai->task_state.move_dest, ent->comp_transform.position);
	//if(Vector3LengthSqr(to_targ) <= NODE_REACH_RADIUS) {
		//path->curr++;
	//}

	if(!AiMoveToNode(ent, graph, path->curr)) {
		return 1;
	}

	return 0;
}

u8 ExecStopMove(Entity *ent) {
	if(AI_LOG) Message("ExecStopmove()", ANSI_BLUE);

	comp_Ai *ai = &ent->comp_ai;
	comp_Transform *ct = &ent->comp_transform;

	if(Vector3Length(ct->velocity) < 1.0f) {
		ai->wish_dir = Vector3Zero();
		ct->velocity.x = 0;
		ct->velocity.y = 0;
		return 1;
	}

	return 0;
}

u8 ExecMakePatrolPath(Entity *ent, MapSection *sect) {
	if(AI_LOG) Message("ExecMakePatrolPath()", ANSI_BLUE);

	comp_Ai *ai = &ent->comp_ai;

	if(ai->navgraph_id < 0) {
		AiSetSchedule(ai, sched_defs[ai->sched_state.sched_id].fail_sched);
		return 0;
	}

	NavGraph *graph = &sect->navgraphs[ai->navgraph_id];

	//printf("curr_navnode_id: %d\n", ai->curr_navnode_id);

	if(MakeNavPath(ent, graph, GetRandomValue(0, graph->node_count-1))) {
		Vector3 point = graph->nodes[ai->task_state.path.nodes[ai->task_state.path.count-1]].position;
		ai->task_state.move_dest = point;
		ai->targ_data.known_position = point;
		ai->targ_data.ent_id = -1;
		//Message("success", ANSI_GREEN);
		return 1;
	}

	return 0;
}

u8 ExecFindPoint(Entity *ent, MapSection *sect) {
	comp_Ai *ai = &ent->comp_ai;

	if(ai->navgraph_id < 0) {
		return 1;
	}

	NavGraph *graph = &sect->navgraphs[ai->navgraph_id];
	i16 node = FindClosestNavNodeInGraph(ai->targ_data.known_position, graph);

	if(node < 0) {
		return 1;
	}

	MakeNavPath(ent, graph, node);

	return 1;
}

u8 ExecGotoPos(Entity *ent, MapSection *sect) {
	if(AI_LOG) Message("ExecGotoPos()", ANSI_BLUE);

	comp_Ai *ai = &ent->comp_ai;
	comp_Transform *ct = &ent->comp_transform;

	if(ai->task_state.use_path)
		return ExecGotoPoint(ent, sect);

	Vector3 to_dest = Vector3Subtract(ai->task_state.move_dest, ct->position);
	if(Vector3LengthSqr(to_dest) <= MEELEE_RANGE) {
		return 1;
	}

	Bsp_Hull *hull = &sect->bsp_data.hull_groups[0].hulls[1];
	Bsp_TraceData tr = Bsp_TraceDataEmpty();
	Bsp_RecursiveTraceEx(hull, hull->first_node, 0, 1, ct->position, ai->targ_data.position, &tr);
	
	if(tr.fraction < 1)
		return 1;

	ai->wish_dir = to_dest;
	ai->wish_dir.z = 0;
	ai->wish_dir = Vector3Normalize(ai->wish_dir);
	ct->targ_look = ai->wish_dir; 

	return 0;
}

u8 ExecFindPos(Entity *ent, MapSection *sect) {
	comp_Ai *ai = &ent->comp_ai;
	comp_Transform *ct = &ent->comp_transform;

	Bsp_Data *bsp = &sect->bsp_data;
	Bsp_Hull *bsp_hull = &bsp->hull_groups[0].hulls[1];

	Bsp_TraceData tr = Bsp_TraceDataEmpty();

	Vector3 tr_start = ct->position;
	//tr_start.z += 24;
	tr_start.z += 8;

	Vector3 tr_dest = ai->targ_data.known_position;
	//tr_dest.z += 24;
	tr_dest.z += 8;

	Bsp_RecursiveTraceEx(bsp_hull, bsp_hull->first_node, 0, 1, tr_start, tr_dest, &tr);
	if(tr.fraction >= 1.0f) {
		ai->task_state.move_dest = ai->targ_data.known_position;
		ai->task_state.use_path = false;
		return 1;
	} 
		
	if(ai->navgraph_id < 0) {
		return 1;
	}

	NavGraph *graph = &sect->navgraphs[ai->navgraph_id];
	i16 node = FindClosestNavNodeInGraph(ai->targ_data.known_position, graph);

	if(node < 0)
		return 1;

	MakeNavPath(ent, graph, node);
	ai->task_state.move_dest = graph->nodes[ai->task_state.path.nodes[ai->task_state.path.curr]].position;

	return 1;
}

u8 ExecMeeleeAttack(Entity *ent, EntityHandler *handler) {
	if(AI_LOG) Message("ExecMeeleeAttack()", ANSI_BLUE);

	comp_Ai *ai = &ent->comp_ai;	
	comp_Transform *ct = &ent->comp_transform;

	if(ai->task_state.timer > 0)
		return 0;

	Entity *victim = &handler->ents[ai->targ_data.ent_id];

	if(Vector3DistanceSqr(ct->position, victim->comp_transform.position) <= MEELEE_RANGE)
		OnHitEnt(victim, 10, ct->position);

	return 1;
}

u8 ExecMakeChasePath(Entity *ent, EntityHandler *handler, MapSection *sect) {
	if(AI_LOG) Message("ExecMakeChasePath()", ANSI_BLUE);

	comp_Ai *ai = &ent->comp_ai;
	comp_Transform *ct = &ent->comp_transform;

	Bsp_Data *bsp = &sect->bsp_data;
	Bsp_Hull *bsp_hull = &bsp->hull_groups[0].hulls[1];

	Bsp_TraceData tr = Bsp_TraceDataEmpty();

	Vector3 tr_start = ct->position;
	//tr_start.z += 24;
	tr_start.z += 8;

	Vector3 tr_dest = ai->targ_data.known_position;
	//tr_dest.z += 24;
	tr_dest.z += 8;

	u8 block = 0;

	Bsp_RecursiveTraceEx(bsp_hull, bsp_hull->first_node, 0, 1, tr_start, tr_dest, &tr);
	/*
	if(tr.fraction >= 1.0f) {
		//ai->task_state.move_dest = ai->targ_data.known_position;
		//ai->task_state.use_path = false;
		//return 1;
	}
	*/
	if(tr.fraction < 1.0f)
		block = 1;

	EntTraceData ent_tr = EntTraceDataEmpty();
	Vector3 dir = Vector3Subtract(ai->targ_data.known_position, ct->position);
	float dist = Vector3Length(dir);
	dir.z = 0;
	dir = Vector3Normalize(dir);

	Ray ray = (Ray) { .position = tr_start, .direction = dir };

	TraceEntities(ray, handler, MEELEE_RANGE, ent->id, &ent_tr);
	if(ent_tr.hit_ent != ai->targ_data.ent_id && ent_tr.hit_ent != -1)
		block |= 0x01;

	if(!block) {
		ai->task_state.move_dest = ai->targ_data.known_position;
		ai->task_state.use_path = false;
		return 1;
	}

	if(block & 0x02) {
		//AiSetSchedule(ai, ai->sched_state.sched_id);
		return ExecMakePatrolPath(ent, sect);
		ai->state = STATE_STUNNED;
	}

	if(ai->navgraph_id < 0) {
		AiSetSchedule(ai, sched_defs[ai->sched_state.sched_id].fail_sched);
		return 0;
	}

	NavGraph *graph = &sect->navgraphs[ai->navgraph_id];

	int node = FindClosestNavNodeInGraph(ai->targ_data.known_position, graph);
	/*
	if(node < 0) {
		AiSetSchedule(ai, sched_defs[ai->sched_state.sched_id].fail_sched);
		return 0;
	} 
	*/

	//printf("curr_navnode_id: %d\n", ai->curr_navnode_id);

	if(MakeNavPath(ent, graph, node)) {
		Vector3 point = graph->nodes[ai->task_state.path.nodes[ai->task_state.path.count-1]].position;
		ai->task_state.move_dest = point;
		//Message("success", ANSI_GREEN);
		return 1;
	}

	return 0;
}

u8 ExecDoFix(Entity *ent, EntityHandler *handler) {
	//if(AI_LOG) Message("ExecMakeChasePath()", ANSI_BLUE);
	//Message("ExecDoFix()", ANSI_BLUE);

	comp_Ai *ai = &ent->comp_ai;
	comp_Transform *ct = &ent->comp_transform;

	Entity *other = &handler->ents[ai->targ_data.ent_id];
	DoFix(other);

	//ai->targ_data.ent_id = -1;
	ai->targ_data.ent_id = handler->player_id;
	ai->input_mask &= ~AI_INPUT_SEE_GLITCHED;

	handler->ents[handler->bug_id].flags &= ~BUG_DISRUPTED_ENEMY;
	handler->ents[handler->bug_id].comp_ai.state = BUG_LAUNCHED;
	handler->ents[handler->bug_id].comp_transform.on_ground = false;
	handler->ents[handler->bug_id].comp_ai.targ_data.ent_id = handler->player_id;

	return 1;
}

u8 ExecGotoEnt(Entity *ent, EntityHandler *handler) {
	comp_Ai *ai = &ent->comp_ai;	
	comp_Transform *ct = &ent->comp_transform;

	Entity *targ = &handler->ents[ai->targ_data.ent_id];
	if(Vector3DistanceSqr(ct->position, targ->comp_transform.position) > MEELEE_RANGE)
		return 0;

	return 1;
}

u8 ExecRestoreSched(Entity *ent) {
	AiSetSchedule(&ent->comp_ai, ent->comp_ai.sched_state.prev_sched);
	return 1;
}

void CheckForBrokenAlly(Entity *ent, EntityHandler *handler) {
	comp_Ai *ai = &ent->comp_ai;	
	comp_Transform *ct = &ent->comp_transform;

	if(ai->navgraph_id < 0)
		return;

	if(ai->input_mask & AI_INPUT_SELF_GLITCHED)
		return;

	if(handler->ents[handler->bug_id].comp_ai.targ_data.ent_id == ent->id)
		return;

	if(ai->sched_state.sched_id == SCHED_FIX_FRIEND_A || ai->sched_state.sched_id == SCHED_FIX_FRIEND_B) {
		ai->input_mask |= AI_INPUT_SEE_GLITCHED;
		return;
	}

	for(u16 i = 0; i < handler->count; i++) {
		Entity *other = &handler->ents[i];

		if(i == ent->id)
			continue;

		if(!other->comp_ai.component_valid)
			continue;

		if(!(other->comp_ai.input_mask & AI_INPUT_SELF_GLITCHED))
			continue;

		if(other->comp_ai.navgraph_id != ai->navgraph_id || other->comp_ai.navgraph_id == -1)
			continue;

		if(other->comp_ai.task_state.task_id == TASK_FIRE_WEAPON)
			continue;

		if(Vector3DistanceSqr(other->comp_transform.position, ct->position) > (1024*1024))
			continue;

		if(other->comp_ai.state == STATE_DEAD)
			continue;

		ai->input_mask |= AI_INPUT_SEE_GLITCHED;
		ai->targ_data.ent_id = i;
		ai->targ_data.known_position = other->comp_transform.position;
		ai->targ_data.position = other->comp_transform.position;

		if(ai->sched_state.sched_id != SCHED_FIX_FRIEND_A && ai->sched_state.sched_id != SCHED_FIX_FRIEND_B) {
			AiSetSchedule(ai, SCHED_FIX_FRIEND_A);
			return;
		}

		break;
	}

	ai->input_mask &= ~AI_INPUT_SEE_GLITCHED;
}

// Ai tick, not every frame,
// has it's own tick timer (every 11 frames)
void AiSystemUpdate(EntityHandler *handler, MapSection *sect, float dt) {
	AiRunTimers(handler, dt);

	if(handler->ents[handler->player_id].comp_ai.state == STATE_DEAD) {
		alert_sphere_count = 0;
		for(short i = 0; i < MAX_ALERT_SPHERES; i++) {
			alert_spheres[i].flags = 0;
		}

		alert_clear_tick = 2;
	}

	handler->ai_tick -= dt;
	if(handler->ai_tick > 0) {
		return;
	}

	Entity *player = &handler->ents[handler->player_id];
	player->comp_ai.navgraph_id = -1;
	float best = FLT_MAX;
	for(u16 j = 0; j < sect->navgraph_count; j++) {
		NavGraph *graph = &sect->navgraphs[j];

		int closest_node = FindClosestNavNodeInGraph(player->comp_transform.position, graph);
		if(closest_node > -1) {
			float d = Vector3DistanceSqr(graph->nodes[closest_node].position, player->comp_transform.position); 
			if(d < best) {
				player->comp_ai.navgraph_id = j;
				player->comp_ai.curr_navnode_id = closest_node;
				best = d;
			}
		}
	}

	alert_clear_tick--;
	if(alert_clear_tick <= 0) {
		alert_sphere_count = 0;
		for(short i = 0; i < MAX_ALERT_SPHERES; i++) {
			alert_spheres[i].flags = 0;
		}

		alert_clear_tick = 2;
	}

	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];
		// Don't update invalid components
		if(!ent->comp_ai.component_valid)
			continue;

		comp_Ai *ai = &ent->comp_ai;
		AiComponentUpdate(ent, handler, ai, sect, dt);
	}

	handler->ai_tick = (AI_TICKRATE*dt);
}

void AiRunTimers(EntityHandler *handler, float dt) {
	for(u16 i = 0; i < handler->count; i++) {
		comp_Ai *ai = &handler->ents[i].comp_ai;
		if(!ai->component_valid) 
			continue;

		ai->task_state.timer -= dt;
	}
}

// Update an entity's ai component
void AiComponentUpdate(Entity *ent, EntityHandler *handler, comp_Ai *ai, MapSection *sect, float dt) {
	if(ent->comp_ai.state == STATE_DEAD)
		return;

	// Tempororarily do nothing if stunned
	if(ent->comp_ai.state == STATE_STUNNED) {
		if(ent->comp_health.damage_cooldown <= 0) ent->comp_ai.state = STATE_IDLE;
		return;
	}

	float best = FLT_MAX;
	for(int i = 0; i < sect->navgraph_count; i++) {
		NavGraph *graph = &sect->navgraphs[i];

		if(!CheckCollisionBoxes(ent->comp_transform.bounds, graph->bounds))
			continue;

		int closest_node = FindClosestNavNodeInGraph(ent->comp_transform.position, graph);
		if(closest_node > -1) {
			float d = Vector3DistanceSqr(graph->nodes[closest_node].position, ent->comp_transform.position); 
			if(d < best) {
				ent->comp_ai.navgraph_id = i;
				ent->comp_ai.curr_navnode_id = closest_node;
				best = d;
			}
		}
	}

	// Update input mask
	AiCheckInputs(ent, handler, sect);

	// Execute current ai scehdule
	AiDoSchedule(ent, handler, sect, ai, dt);

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

	if(ai->state == STATE_DEAD) {
		ai->input_mask = 0;
		return;
	}

	if(ent->type == ENT_MAINTAINER && ai->navgraph_id >= 0)
		CheckForBrokenAlly(ent, handler);

	// ** Check if player is visible **	
	//
	// Clear 'see player' flag
	bool prev_seen_player = (ai->input_mask & AI_INPUT_SEE_PLAYER);
	ai->input_mask &= ~AI_INPUT_SEE_PLAYER;

	Entity *player_ent = &handler->ents[handler->player_id];

	Vector3 eye_pos = Vector3Add(ct->position, Vector3Scale(UP, 10.0f));
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
	}

	if((ai->input_mask & AI_INPUT_SEE_PLAYER) || (ai->input_mask & AI_INPUT_HEAR_PLAYER)) {
		float r = (ai->input_mask & AI_INPUT_LOST_PLAYER) ? 400.0f : 255.0f;
		AddAlertSphere(ent, r);
	}

	bool alert = false;
	for(short i = 0; i < MAX_ALERT_SPHERES; i++) {
		AlertSphere *sphere = &alert_spheres[i];
		if(!(sphere->flags & ALERT_SPHERE_ACTIVE))
			continue;

		if(sphere->graph_id != ai->navgraph_id)
			continue;

		if(CheckCollisionSpheres(sphere->position, sphere->radius*sphere->radius, ct->position, ai->hear_distance)) {
			alert = true;
			break;
		}
	}

	// ***
	ai->input_mask &= ~AI_INPUT_HEAR_PLAYER;
	bool in_hearing_range = (Vector3Distance(player_ent->comp_transform.position, ct->position) <= ai->hear_distance);
	if((in_hearing_range && Vector3LengthSqr(player_ent->comp_transform.velocity) >= 0.1f) || alert) {
		ai->input_mask |= AI_INPUT_HEAR_PLAYER;

		prev_seen_player = true;
		ai->input_mask &= ~AI_INPUT_LOST_PLAYER;

		if(ai->sched_state.sched_id != SCHED_FIX_FRIEND_A && ai->sched_state.sched_id != SCHED_FIX_FRIEND_B)
			ai->targ_data.ent_id = handler->player_id;
	}

	if(
		prev_seen_player &&
		!(ai->input_mask & AI_INPUT_LOST_PLAYER) && !(ai->input_mask & AI_INPUT_HEAR_PLAYER || ai->input_mask & AI_INPUT_SEE_PLAYER)
	) {
		ai->input_mask |= AI_INPUT_LOST_PLAYER;
	}

	if(Vector3DistanceSqr(ai->task_state.move_dest, ent->comp_transform.position) <= 16.0f) {	
		ai->input_mask |= AI_INPUT_DEST_REACHED;
	}
	
	if(ai->targ_data.ent_id < 0)
		return;

	ai->input_mask &= ~AI_INPUT_MEELEE_RANGE;
	Entity *targ_ent = &handler->ents[ai->targ_data.ent_id];
	
	if(Vector3DistanceSqr(targ_ent->comp_transform.position, ent->comp_transform.position) <= MEELEE_RANGE) {	
		ai->input_mask |= AI_INPUT_MEELEE_RANGE;
	}

	ai->input_mask &= ~AI_INPUT_TAKE_DAMAGE;

	ai->input_mask &= ~AI_INPUT_TARG_DEAD;
	if(targ_ent->comp_ai.state == STATE_DEAD)
		ai->input_mask |= AI_INPUT_TARG_DEAD;
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

	// Failure
	if(ai->input_mask & sched_def->fail_mask) {
		AiSetSchedule(ai, sched_def->fail_sched);
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
	comp_Transform *ct = &ent->comp_transform;

	u8 done = 1;

	switch(task_id) {
		case TASK_GOTO_POINT:
			done = ExecGotoPoint(ent, sect);
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
			done = ExecFaceDir(ent, ct->targ_look);
			break;

		case TASK_FIND_POINT:
			done = ExecFindPoint(ent, sect);
			break;

		case TASK_MAKE_PATROL_PATH:
			done = ExecMakePatrolPath(ent, sect);
			break;

		case TASK_LOOK_AT_ENTITY:
			done = ExecLookAtEntity(ent, handler, ai);
			break;

		case TASK_LOOK_AROUND:
			break;

		case TASK_DO_FIX:
			done = ExecDoFix(ent, handler);
			break;

		case TASK_THROW_PROJECTILE:
			break;

		case TASK_STOP_MOVE:
			done = ExecStopMove(ent);
			break;

		case TASK_GOTO_POS:
			done = ExecGotoPos(ent, sect);
			break;

		case TASK_FIND_POS:
			done = ExecFindPos(ent, sect);
			break;

		case TASK_MEELEE_ATTACK:
			done = ExecMeeleeAttack(ent, handler);
			break;

		case TASK_MAKE_CHASE_PATH:
			done = ExecMakeChasePath(ent, handler, sect);
			break;

		case TASK_GOTO_ENT:
			done = ExecGotoEnt(ent, handler);
			break;

		case TASK_RESTORE_SCHED:
			done = ExecRestoreSched(ent);
			break;
	}

	return done;
}

void AiSetSchedule(comp_Ai *ai, u8 sched_id) {
	ai->sched_state.prev_sched = ai->sched_state.sched_id;
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

