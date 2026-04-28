#include "raylib.h"
#include "ent.h"

void RegulatorUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	ent->comp_health.crit_box.min = (Vector3) { -9, -9, -9 };
	ent->comp_health.crit_box.max = (Vector3) {  9,  9,  9 };
	ent->comp_health.crit_box = BoxTranslate(ent->comp_health.crit_box, Vector3Add(ct->position, Vector3Scale(UP, 22)));

	if(ai->task_state.task_id == TASK_FACE_DIR) {
		if(ai->targ_data.ent_id == handler->player_id) {
			Vector3 to_player = Vector3Subtract(handler->ents[handler->player_id].comp_transform.position, ct->position);
			to_player.z = 0;
			to_player = Vector3Normalize(to_player);

			ct->targ_look = to_player; 
		}

		ct->forward = Vector3Lerp(ct->forward, ct->targ_look, 10*dt);
	}

	EntMove(ent, sect, handler, dt);
}

void RegulatorDraw(Entity *ent, EntityHandler *handler, float dt) {
	comp_Transform *ct = &ent->comp_transform;	
	comp_Ai *ai = &ent->comp_ai;

	if(ai->state == STATE_DEAD) {
		ent->anim_state.curr_frame = 0;
		ent->anim_state.anim_id = 0;
		anim_Apply(&ent->anim_state, &ent->model, ent->animations);

		Vector3 pos = ent->comp_transform.position;		
		pos.z -= 20;

		/*
		DrawModelEx(
			ent->model,
			pos,
			Vector3CrossProduct(ct->forward, UP),
			90,
			Vector3Scale(Vector3One(), 1.0f),
			LIGHTGRAY 
		);
		*/

		EntDrawLitModelEx(handler, ent, pos, 1.0f, Vector3CrossProduct(ct->forward, UP), 90, 100);

		return;
	}

	anim_Switch(&ent->anim_state, 0);
	//anim_Update(&ent->anim_state, ent->animations, dt);
	anim_Apply(&ent->anim_state, &ent->model, ent->animations);
	float yaw = atan2f(-ct->targ_look.x, ct->targ_look.y);
	ent->model.transform = MatrixMultiply(MatrixRotateX(90*DEG2RAD), MatrixRotateZ(yaw+(90*DEG2RAD)));
	EntDrawLitModelEx(handler, ent, ct->position, 1.0f, Vector3CrossProduct(ct->forward, UP), 0, 100);

	DrawBoundingBox(ent->comp_health.hit_box, GREEN);
}

