#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "ai.h"

void MaintainerUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	/*
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
	*/

	//EntMove(ent, sect, handler, dt);
	//ent->anim_frame = (ent->anim_frame + 1) % ent->animations[ent->curr_anim].frameCount;

	EntMove(ent, sect, handler, dt);

	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	if(ai->task_state.task_id == TASK_FACE_DIR || ai->task_state.task_id == TASK_GOTO_POINT) {
		ct->forward = Vector3Lerp(ct->forward, ct->targ_look, 10*dt);
		ct->forward.z = 0;
		ct->forward = Vector3Normalize(ct->forward);

		if(Vector3DotProduct(ct->forward, ct->targ_look) < 0.8f) {
			ai->speed = 0;
		} else {
			ai->speed = 100;
		}
	}

}

void MaintainerDraw(Entity *ent, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	if(ai->state == STATE_DEAD) {
		Vector3 pos = ent->comp_transform.position;		
		pos.z -= 20;

		DrawModelEx(
			ent->model,
			pos,
			Vector3CrossProduct(ct->forward, UP),
			90,
			Vector3Scale(Vector3One(), 0.1f),
			LIGHTGRAY 
		);
		return;
	}
	
	Vector3 pos = ent->comp_transform.position;
	float yaw = atan2f(-ct->forward.x, ct->forward.y);
	ent->model.transform = MatrixMultiply(MatrixRotateX(90*DEG2RAD), MatrixRotateZ(yaw+(90*DEG2RAD)*-1));
	DrawModel(ent->model, pos, 0.1f, LIGHTGRAY);
	/*
	DrawModelEx(
		ent->model,
		pos,
		UP,
		yaw*RAD2DEG,
		Vector3Scale(Vector3One(), 0.1f),
		LIGHTGRAY 
	);
	*/

	DrawLine3D(ct->position, Vector3Add(ct->position, Vector3Scale(ct->forward, 10)), GREEN);
	DrawLine3D(ct->position, Vector3Add(ct->position, Vector3Scale(ct->targ_look, 10)), RED);
}

