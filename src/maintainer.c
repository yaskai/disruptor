#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "ai.h"

void MaintainerThink(Entity *ent, EntityHandler *handler, float dt) {
	comp_Ai *ai = &ent->comp_ai;	

	//ai->targ_data.ent_id = handler->player_id;

	if(ai->sched_state.sched_id != SCHED_FIX_FRIEND) {
		if(
			(ai->input_mask & AI_INPUT_SEE_PLAYER) || (ai->input_mask & AI_INPUT_HEAR_PLAYER) || 
			(ai->sched_state.sched_id == SCHED_CHASE_PLAYER && !ai->task_state.use_path)) 
		{

			ai->targ_data.ent_id = handler->player_id;
			ai->targ_data.position = handler->ents[handler->player_id].comp_transform.position;
			ai->targ_data.known_position = ai->targ_data.position;
		}
	}
}

void MaintainerUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	EntMove(ent, sect, handler, dt);

	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

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

	if(stop) {
		ct->velocity = Vector3Lerp(ct->velocity, Vector3Zero(), 2*dt);
		ai->speed = Lerp(ai->speed, 0.0f, 2*dt);
		//ai->speed = 0;
		ai->wish_dir = Vector3Zero();
	} else {
		//ai->speed = 175;
		ai->speed = Lerp(ai->speed, 200.0f, 10*dt);
	}
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
	
	anim_Update(&ent->anim_state, ent->animations, dt);
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
}

