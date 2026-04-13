#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "ai.h"

void MaintainerThink(Entity *ent, EntityHandler *handler, float dt) {
	comp_Ai *ai = &ent->comp_ai;	

	//ai->targ_data.ent_id = handler->player_id;

	if(ai->input_mask & AI_INPUT_SELF_GLITCHED) {
		if(ai->sched_state.sched_id != SCHED_PATROL)
			AiSetSchedule(ai, SCHED_PATROL);
		else {
			if(ai->disrupt_timer <= 0) {
				AiSetSchedule(ai, SCHED_MAINTAINER_IDLE);
				ai->input_mask &= ~AI_INPUT_SELF_GLITCHED;
			}
		}

		return;
	}

	if(     ai->sched_state.sched_id != SCHED_FIX_FRIEND_A &&
			ai->sched_state.sched_id != SCHED_FIX_FRIEND_B &&
			!(ai->input_mask & AI_INPUT_SEE_GLITCHED) &&
			ai->state != STATE_STUNNED) {

		if(
			(ai->input_mask & AI_INPUT_SEE_PLAYER) || (ai->input_mask & AI_INPUT_HEAR_PLAYER) || 
			(ai->sched_state.sched_id == SCHED_CHASE_PLAYER && !ai->task_state.use_path)) 
		{

			ai->targ_data.ent_id = handler->player_id;
			ai->targ_data.position = handler->ents[handler->player_id].comp_transform.position;
			ai->targ_data.known_position = ai->targ_data.position;
		}

	} else {
		if(!ai->task_state.use_path) {
			ai->targ_data.position = handler->ents[ai->targ_data.ent_id].comp_transform.position;
			ai->targ_data.known_position = ai->targ_data.position;
		}
	}
}

void MaintainerUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	EntMove(ent, sect, handler, dt);

	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	ent->comp_health.crit_box.min = (Vector3) { -9, -9, -9 };
	ent->comp_health.crit_box.max = (Vector3) {  9,  9,  9 };
	ent->comp_health.crit_box = BoxTranslate(ent->comp_health.crit_box, Vector3Add(ct->position, Vector3Scale(UP, 22)));

	if(ai->state == STATE_DEAD)
		return;

	MaintainerThink(ent, handler, dt);

	if(ai->state != STATE_MOVE) {
		ct->forward = Vector3Lerp(ct->forward, ct->targ_look, 10*dt);
		ct->forward.z = 0;
		ct->forward = Vector3Normalize(ct->forward);

	} else {
		ct->forward = Vector3Lerp(ct->forward, Vector3Normalize(ct->velocity), 10*dt);
		ct->forward.z = 0;
		ct->forward = Vector3Normalize(ct->forward);
	}

	if(ai->task_state.task_id == TASK_FACE_DIR) {
		if(ai->targ_data.ent_id == handler->player_id) {
			Vector3 to_player = Vector3Subtract(handler->ents[handler->player_id].comp_transform.position, ct->position);
			to_player.z = 0;
			to_player = Vector3Normalize(to_player);

			ct->targ_look = to_player; 
		}

		ct->forward = Vector3Lerp(ct->forward, ct->targ_look, 10*dt);
	}

	bool stop = false;
	if(ai->task_state.task_id == TASK_STOP_MOVE)
		stop = true;

	if(
		(ai->input_mask & AI_INPUT_MEELEE_RANGE) &&
		(ai->sched_state.sched_id == SCHED_CHASE_PLAYER || ai->sched_state.sched_id == SCHED_MAINTAINER_ATTACK)
	) {
		stop = true;
	}

	if(
		(ai->input_mask & AI_INPUT_MEELEE_RANGE) &&
		(ai->sched_state.sched_id == SCHED_FIX_FRIEND_A || ai->sched_state.sched_id == SCHED_FIX_FRIEND_B)
	) {
		stop = true;
	}

	if(ai->state == STATE_STUNNED)
		stop = true;

	if(stop) {
		ct->velocity = Vector3Lerp(ct->velocity, Vector3Zero(), 2*dt);
		ai->speed = Lerp(ai->speed, 0.0f, 2*dt);
		//ai->speed = 0;
		ai->wish_dir = Vector3Zero();
	} else {
		//ai->speed = 175;
		float wish = (ai->input_mask & AI_INPUT_SELF_GLITCHED) ? 200.0f : 200.0f;
		ai->speed = Lerp(ai->speed, wish, 10*dt);

	}

	if(ai->state != STATE_STUNNED)
		anim_Update(&ent->anim_state, ent->animations, dt);
}

void MaintainerDraw(Entity *ent, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	if(ai->state == STATE_DEAD) {
		ent->anim_state.curr_frame = 0;
		ent->anim_state.anim_id = 0;
		anim_Apply(&ent->anim_state, &ent->model, ent->animations);

		Vector3 pos = ent->comp_transform.position;		
		pos.z -= 20;

		DrawModelEx(
			ent->model,
			pos,
			Vector3CrossProduct(ct->forward, UP),
			90,
			Vector3Scale(Vector3One(), 1.0f),
			LIGHTGRAY 
		);

		return;
	}

	//ent->anim_state.anim_id = 0;
	//anim_Switch(&ent->anim_state, 0);
	if(Vector3Length(ai->wish_dir)) { 
		//ent->anim_state.anim_id = 1;
		anim_Switch(&ent->anim_state, 1);
	} else {
		anim_Switch(&ent->anim_state, 0);
	}
	
	//anim_Update(&ent->anim_state, ent->animations, dt);
	anim_Apply(&ent->anim_state, &ent->model, ent->animations);

	Vector3 pos = ent->comp_transform.position;
	float yaw = atan2f(-ct->forward.x, ct->forward.y);
	ent->model.transform = MatrixMultiply(MatrixRotateX(90*DEG2RAD), MatrixRotateZ(yaw+(90*DEG2RAD)*-1));
	DrawModel(ent->model, pos, 1.0f, LIGHTGRAY);

	//DrawLine3D(ct->position, Vector3Add(ct->position, Vector3Scale(ct->forward, 10)), GREEN);
	//DrawLine3D(ct->position, Vector3Add(ct->position, Vector3Scale(ct->targ_look, 10)), RED);

	//DrawSphere(ai->targ_data.position, 2, PURPLE);
	//DrawSphere(ai->targ_data.known_position, 2, ColorAlpha(PURPLE, 0.5f));
	//DrawSphere(ai->task_state.move_dest, 5, PURPLE);

	//DrawSphere(ct->position, ai->hear_distance, ColorAlpha(YELLOW, 0.5f));

	/*
	if(ai->input_mask & AI_INPUT_SELF_GLITCHED)
		DrawBoundingBox(ent->comp_health.hit_box, PURPLE);

	if(ai->input_mask & AI_INPUT_SEE_GLITCHED)
		DrawBoundingBox(ent->comp_health.hit_box, RED);
	*/

	//DrawBoundingBox(ent->comp_health.hit_box, GREEN);
	//DrawBoundingBox(ent->comp_health.crit_box, RED);
}

void OnFixMaintainer(Entity *ent) {
	AiSetSchedule(&ent->comp_ai, SCHED_MAINTAINER_IDLE);
}

