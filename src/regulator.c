#include "raylib.h"
#include "ent.h"

void RegulatorThink(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;
	comp_Weapon *weap = &ent->comp_weapon;
}

void RegulatorFireWeapon(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;
	comp_Weapon *weap = &ent->comp_weapon;

	if(!(ai->input_mask & AI_INPUT_SEE_PLAYER)) {
		if(!CheckLOS(ent, handler->player_id, handler, sect, COLL_IGNORE_BULLET)) {
			AiSetSchedule(ai, SCHED_REGULATOR_RELOAD);
		}
	}

	weap->cooldown -= dt;
	if(weap->cooldown > 0)
		return; 

	if(weap->ammo <= 0) 
		return;

	float sfx_pitch = GetRandomValue(145, 150) * 0.01f;

	if(weap->ammo % 3 == 0) {
		AP_SetSoundPosition(handler->ap, "turret_gun_00", ct->position, 0);
		AP_SetSoundPitch(handler->ap, "turret_gun_00", sfx_pitch);
		AP_RequestSound(handler->ap, "turret_gun_00");
	} else if(weap->ammo % 2 == 0) {
		AP_SetSoundPosition(handler->ap, "turret_gun_01", ct->position, 0);
		AP_SetSoundPitch(handler->ap, "turret_gun_01", sfx_pitch);
		AP_RequestSound(handler->ap, "turret_gun_01");
	}  else {
		AP_SetSoundPosition(handler->ap, "turret_gun_02", ct->position, 0);
		AP_SetSoundPitch(handler->ap, "turret_gun_02", sfx_pitch);
		AP_RequestSound(handler->ap, "turret_gun_02");
	}

	Vector3 trace_start = ct->position;
	trace_start.z += 12;
	trace_start = Vector3Add(trace_start, Vector3Scale(ct->forward, 38));

	Vector3 dir = ct->targ_look;
	float offset = GetRandomValue(-10, 10) * 0.01f;	

	Vector3 right = Vector3CrossProduct(dir, UP);
	dir = Vector3Add(dir, Vector3Scale(right, offset));

	offset = GetRandomValue(-10, 10) * 0.01f;
	dir = Vector3Add(dir, Vector3Scale(UP, offset));

	dir = Vector3Normalize(dir);

	bool hit = false;
	Vector3 bullet_dest = TraceBullet(handler, sect, trace_start, dir, ent->id, &hit, false);

	Vector3 trail_end = Vector3Add(trace_start, Vector3Scale(dir, Vector3Distance(trace_start, bullet_dest)));
	if(!hit) {
		trail_end = Vector3Add(trace_start, Vector3Scale(ct->forward, 2000.0f));
	}
	
	// Add bullet trail effect
	float dist = Vector3Distance(trace_start, trail_end);
	vEffectsAddTrail(handler->effect_manager, trace_start, trail_end);

	weap->cooldown = 0.085f;
	weap->ammo--;
	weap->in_clip--;

	if(weap->ammo % 2 == 0) {
		Ray ray = (Ray) { .position = trace_start, .direction = dir };
		RayCollision near_coll = GetRayCollisionSphere(ray, handler->ents[handler->player_id].comp_transform.position, 96);
		if(near_coll.hit)
			AP_ReqNearBulletSound(handler->ap, near_coll.point, dir);
	}
}

void RegulatorUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	if(ai->state == STATE_DEAD)
		return;

	ent->comp_health.crit_box.min = (Vector3) { -9, -9, -9 };
	ent->comp_health.crit_box.max = (Vector3) {  9,  9,  9 };
	ent->comp_health.crit_box = BoxTranslate(ent->comp_health.crit_box, Vector3Add(ct->position, Vector3Scale(UP, 22)));

	RegulatorThink(ent, handler, sect, dt);

	if(ai->task_state.task_id == TASK_FACE_DIR) {
		if(ai->targ_data.ent_id == handler->player_id) {
			Vector3 to_player = Vector3Subtract(handler->ents[handler->player_id].comp_transform.position, ct->position);
			to_player.z = 0;
			to_player = Vector3Normalize(to_player);

			ct->targ_look = to_player; 
		}

		ct->forward = Vector3Lerp(ct->forward, ct->targ_look, 5*dt);
	}

	if(ai->state == STATE_MOVE) {
		Vector3 move_dir = Vector3Normalize( (Vector3) { ct->velocity.x, ct->velocity.y, 0.0f } );
		ct->yaw = Lerp(ct->yaw, atan2f(-move_dir.x, move_dir.y), 5*dt);
	}

	if(ai->task_state.task_id == TASK_FIRE_WEAPON) {
		ai->state = STATE_ATTACK;
		RegulatorFireWeapon(ent, handler, sect, dt);

		Vector3 to_player = Vector3Subtract(handler->ents[handler->player_id].comp_transform.position, ct->position);
		//to_player.z = 0;
		to_player = Vector3Normalize(to_player);

		ct->targ_look = to_player; 
		ct->forward = Vector3Lerp(ct->forward, ct->targ_look, 5*dt);
	}

	if(ai->task_state.task_id == TASK_RELOAD_WEAPON)
		ai->state = STATE_RELOAD;

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
		EntDrawLitModelEx(handler, ent, pos, 1.0f, Vector3CrossProduct(ct->forward, UP), 90, 100);

		return;
	}

	anim_Switch(&ent->anim_state, 0);
	//anim_Update(&ent->anim_state, ent->animations, dt);
	anim_Apply(&ent->anim_state, &ent->model, ent->animations);

	float yaw = atan2f(-ct->forward.x, ct->forward.y);

	if(ai->state == STATE_MOVE) {
		yaw = ct->yaw;
	}

	ent->model.transform = MatrixMultiply(MatrixRotateX(90*DEG2RAD), MatrixRotateZ(yaw+(90*DEG2RAD)));
	EntDrawLitModelEx(handler, ent, ct->position, 1.0f, Vector3CrossProduct(ct->forward, UP), 0, 100);

	DrawBoundingBox(ent->comp_health.hit_box, GREEN);
}

