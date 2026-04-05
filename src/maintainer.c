#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "ai.h"

void MaintainerThink(Entity *ent, EntityHandler *handler, float dt) {
	comp_Ai *ai = &ent->comp_ai;	

	//ai->targ_data.ent_id = handler->player_id;

	if(ai->input_mask & AI_INPUT_SEE_PLAYER || ai->input_mask & AI_INPUT_HEAR_PLAYER) {
		if(!(ai->input_mask & AI_INPUT_SEE_GLITCHED)) {
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

	if(ai->task_state.task_id == TASK_STOP_MOVE || (ai->input_mask & AI_INPUT_MEELEE_RANGE)) {
		ct->velocity = Vector3Lerp(ct->velocity, Vector3Zero(), 10*dt);
		ai->speed = 0;
		ai->wish_dir = Vector3Zero();
	} else {
		ai->speed = 150;
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

	//DrawLine3D(ct->position, Vector3Add(ct->position, Vector3Scale(ct->forward, 10)), GREEN);
	//DrawLine3D(ct->position, Vector3Add(ct->position, Vector3Scale(ct->targ_look, 10)), RED);

	DrawSphere(ai->targ_data.position, 2, PURPLE);
	DrawSphere(ai->targ_data.known_position, 2, ColorAlpha(PURPLE, 0.5f));
	DrawSphere(ai->task_state.move_dest, 5, PURPLE);
}

