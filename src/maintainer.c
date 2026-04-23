#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "ai.h"
#include "audioplayer.h"

char *maintainer_step_sounds[8] = {
	"metal_steps_01",
	"metal_steps_02",
	"metal_steps_03",
	"metal_steps_04",
	"metal_steps_05",
	"metal_steps_06",
	"metal_steps_07",
	"metal_steps_08",
};

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

		Coords coords = Vec3ToCoords(ct->position, &handler->grid);
		i16 cell_id = CellCoordsToId(coords, &handler->grid);
		EntGridCell *cell = &handler->grid.cells[cell_id];

		for(int i = 0; i < cell->ent_count; i++) {
			Entity *other = &handler->ents[cell->ents[i]];

			if(other->id == ent->id)
				continue;

			if(!(other->flags & ENT_ACTIVE))
				continue;

			if(!(other->flags & ENT_COLLIDERS))
				continue;

			if(other->comp_ai.state == STATE_DEAD)
				continue;

			if(other->type == ENT_PLAYER || other->type == ENT_DISRUPTOR || other->type == ENT_SWITCH)
				continue;

			//if(!CheckCollisionBoxes(ct->bounds, other->comp_transform.bounds))
				//continue;

			if(!CheckCollisionSpheres(ct->position, 32, other->comp_transform.position, 96))
				continue;

			Vector3 to_other = Vector3Normalize(Vector3Subtract(other->comp_transform.position, ct->position));
			float into = Vector3DotProduct(to_other, ai->wish_dir);
			if(into > 0) {
				float prev_speed = ai->speed;
				Vector3 prev_wish = ai->wish_dir;
				ai->wish_dir = Vector3Subtract(ai->wish_dir, Vector3Scale(to_other, into));
				ai->speed = Vector3Length(to_other) * 1.1f;
				EntMove(ent, sect, handler, dt);
				ai->speed = prev_speed;
				ai->wish_dir = prev_wish;
			}

			break;
		}
	}

	if(ent->anim_state.anim_id == 1) {
		if(ent->anim_state.curr_frame % (32 + ent->id) == 0) {
			int sfx_id = GetRandomValue(0, 7); 
			AP_SetSoundPosition(handler->ap, maintainer_step_sounds[sfx_id], ct->position, 0);

			//Vector3 sound_dir = Vector3Scale(Vector3Add(ct->forward, DOWN), 0.5f);
			//sound_dir = Vector3Normalize(sound_dir);

			Vector3 hpos = (Vector3) { ct->position.x, ct->position.y, 0 };
			Vector3 player_hpos = handler->ents[handler->player_id].comp_transform.position;
			player_hpos.z = 0;

			if(Vector3Distance(hpos, player_hpos) <= 500.0f)
				AP_RequestSound(handler->ap, maintainer_step_sounds[sfx_id]);
		}
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
		//stop = true;
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
		float wish = (ai->input_mask & AI_INPUT_SELF_GLITCHED) ? 100.0f : 220.0f;

		if(ai->task_state.task_id == TASK_MEELEE_ATTACK)
			wish = 220.0f;

		if(ai->sched_state.sched_id == SCHED_FIX_FRIEND_A || ai->sched_state.sched_id == SCHED_FIX_FRIEND_B)
			wish = 250.0f;

		ai->speed = Lerp(ai->speed, wish, 10*dt);
	}

	if(ai->state != STATE_STUNNED)
		anim_Update(&ent->anim_state, ent->animations, dt);
}

void MaintainerDraw(Entity *ent, EntityHandler *handler, float dt) {
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
	EntDrawLitModelEx(handler, ent, pos, 1.0f, Vector3CrossProduct(ct->forward, UP), 0, 100);
	//EntDrawLitModelEx(handler, ent, pos, 1.0f, Vector3CrossProduct(ct->forward, UP), 90);
	//DrawModel(ent->model, pos, 1.0f, LIGHTGRAY);

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
	ent->comp_ai.disrupt_timer = 0;
	AiSetSchedule(&ent->comp_ai, SCHED_MAINTAINER_IDLE);
}

