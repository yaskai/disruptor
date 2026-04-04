#include "ent.h"
#include "ai.h"
#include "raylib.h"
#include "raymath.h"
#include "../include/log_message.h"

// When turret is hit
// * NOTE:
// Nothing right now, 
// unsure design-wise what to do here...
void OnHitTurret(Entity *ent, short damage) {

}

void OnFixTurret(Entity *ent) {
	/*
	comp_Ai *ai = &ent->comp_ai;

	ai->curr_schedule = SCHED_SENTRY;
	ai->task_data.schedule_id = SCHED_SENTRY;
	ai->task_data.task_id = TASK_WAIT_TIME;
	ai->task_data.timer = 1;
	*/
}

// Update logic for turret
void TurretUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Ai *ai = &ent->comp_ai;
	comp_Transform *ct = &ent->comp_transform;

	if(ai->task_state.task_id == TASK_FIRE_WEAPON) {
		TurretShoot(ent, handler, sect, dt);
	}

	if(ai->input_mask & AI_INPUT_SELF_GLITCHED) {
		float angle_max = 70, angle_min = -angle_max;

		ai->task_state.timer = 0;		
		ai->task_state.task_id = TASK_FIRE_WEAPON;

		float angle = sinf(GetTime() * 1.5f);
		angle = Clamp(angle, angle_min, angle_max);

		if(ent->comp_weapon.ammo > 0) {
			ct->forward = Vector3RotateByAxisAngle(ent->comp_transform.targ_look, UP, angle);		
		} else {
			AiSetSchedule(ai, SCHED_IDLE);
			ct->forward.z = Lerp(ct->forward.z, ct->start_forward.z - 0.2f, dt);
			ct->forward = Vector3Normalize(ct->forward);

			if(fabsf(ct->start_forward.z - ct->forward.z) >= 0.19f)	
				ai->state = STATE_DEAD;
		}

		return;
	}

	Vector3 targ_look = ct->forward;

	if(ai->task_state.task_id == TASK_FIRE_WEAPON || ai->task_state.task_id == TASK_LOOK_AT_ENTITY) {
		if(ai->input_mask & AI_INPUT_SEE_PLAYER) {
			Entity *player = &handler->ents[handler->player_id];
			targ_look = Vector3Normalize(Vector3Subtract(player->comp_transform.position, ct->position));
			if(Vector3DotProduct(targ_look, ct->start_forward) >= 0.1f)
				ct->targ_look = Vector3Lerp(ct->targ_look, targ_look, 80*dt);

			ai->targ_data.known_position = player->comp_transform.position;

		} else if(ai->input_mask & AI_INPUT_LOST_PLAYER) {
			targ_look = Vector3Normalize(Vector3Subtract(ai->targ_data.known_position, ct->position));
			if(Vector3DotProduct(targ_look, ct->start_forward) >= 0.1f)
				ct->targ_look = Vector3Lerp(ct->targ_look, targ_look, 80*dt);
		}
	}

	if(ai->task_state.task_id != TASK_FIRE_WEAPON) {

		if(ai->input_mask & AI_INPUT_LOST_PLAYER) {
			targ_look = Vector3Normalize(Vector3Subtract(ai->targ_data.known_position, ct->position));
			if(Vector3DotProduct(targ_look, ct->start_forward) >= 0.1f)
				ct->targ_look = Vector3Lerp(ct->targ_look, targ_look, 80*dt);
		}
	}

	ct->forward = Vector3Normalize(Vector3Lerp(ct->forward, ct->targ_look, 10*dt));
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
	/*
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
				ct->targ_look = Vector3Lerp(ct->targ_look, targ, 80*dt);
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
	float offset = GetRandomValue(-6, 6) * 0.01f;	

	Vector3 right = Vector3CrossProduct(ct->forward, UP);
	dir = Vector3Add(dir, Vector3Scale(right, offset));

	offset = GetRandomValue(-6, 6) * 0.01f;
	dir = Vector3Add(dir, Vector3Scale(UP, offset));

	dir = Vector3Normalize(dir);

	bool hit = false;
	// * NOTE:
	// Purpose of the dummy value is to cause no dammage on the first few shots, gives the player a warning for fairness.
	bool dummy = (ent->comp_weapon.ammo > ent->comp_weapon.clip_size - 1);
	Vector3 bullet_dest = TraceBullet(handler, sect, trace_start, dir, ent->id, &hit, dummy);

	Vector3 trail_end = Vector3Add(trace_start, Vector3Scale(dir, Vector3Distance(trace_start, bullet_dest)));
	if(!hit) {
		trail_end = Vector3Add(trace_start, Vector3Scale(ct->forward, 2000.0f));
	}
	
	// Add bullet trail effect
	float dist = Vector3Distance(trace_start, trail_end);
	vEffectsAddTrail(handler->effect_manager, trace_start, trail_end);

	weap->cooldown = 0.065f;
	weap->ammo--;
	*/

	comp_Weapon *weap = &ent->comp_weapon;
	comp_Transform *ct = &ent->comp_transform;

	weap->cooldown -= dt;
	if(weap->cooldown > 0)
		return; 

	if(weap->ammo <= 0) 
		return;

	Vector3 trace_start = ct->position;
	trace_start.z += 12;
	trace_start = Vector3Add(trace_start, Vector3Scale(ct->forward, 38));

	Vector3 dir = ct->forward;
	float offset = GetRandomValue(-6, 6) * 0.01f;	

	Vector3 right = Vector3CrossProduct(ct->forward, UP);
	dir = Vector3Add(dir, Vector3Scale(right, offset));

	offset = GetRandomValue(-6, 6) * 0.01f;
	dir = Vector3Add(dir, Vector3Scale(UP, offset));

	dir = Vector3Normalize(dir);

	bool hit = false;
	// * NOTE:
	// Purpose of the dummy value is to cause no dammage on the first few shots, gives the player a warning for fairness.
	bool dummy = (ent->comp_weapon.ammo > ent->comp_weapon.clip_size - 1);
	Vector3 bullet_dest = TraceBullet(handler, sect, trace_start, dir, ent->id, &hit, dummy);

	Vector3 trail_end = Vector3Add(trace_start, Vector3Scale(dir, Vector3Distance(trace_start, bullet_dest)));
	if(!hit) {
		trail_end = Vector3Add(trace_start, Vector3Scale(ct->forward, 2000.0f));
	}
	
	// Add bullet trail effect
	float dist = Vector3Distance(trace_start, trail_end);
	vEffectsAddTrail(handler->effect_manager, trace_start, trail_end);

	weap->cooldown = 0.065f;
	weap->ammo--;
}

