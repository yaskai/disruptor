#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "pm.h"

void DoorUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	comp_Transform 	*ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	Vector3 pos_prev = ct->position;

	ct->velocity = Vector3Zero();
	Vector3 dest = Vector3Zero();

	if(ent->flags & DOOR_OPENING) {
		Vector3 dir = Vector3Normalize(Vector3Subtract(ai->targ_data.position, ct->position));
		ct->velocity = Vector3Lerp(ct->velocity, Vector3Scale(dir, ai->speed), dt*20);
		ct->position = Vector3Add(ct->position, Vector3Scale(ct->velocity, dt));
		dest = ai->targ_data.position;
	} else {
		Vector3 dir = Vector3Normalize(Vector3Subtract(Vector3Zero(), ct->position));
		ct->velocity = Vector3Lerp(ct->velocity, Vector3Scale(dir, ai->speed), dt*20);
		ct->position = Vector3Add(ct->position, Vector3Scale(ct->velocity, dt));
		dest = Vector3Zero();
	}

	//Vector3 vel = Vector3Subtract(ct->position, pos_prev);
	Vector3 vel = Vector3Scale(Vector3Subtract(ct->position, pos_prev), 1.0f / dt);
	ct->position = Vector3Clamp(ct->position, Vector3Min(Vector3Zero(), ai->targ_data.position), Vector3Max(Vector3Zero(), ai->targ_data.position));
	ct->velocity = vel;

	ent->model.transform = MatrixTranslate(ct->position.x, ct->position.y, ct->position.z);

	sect->bsp_data.hull_groups[ent->bsp_model].origin = ct->position;
	sect->bvh_hullgroups[ent->bsp_model].origin = ct->position;

	if(ent->flags & PLAYER_ON_PLATFORM) {
		Entity *player_ent = &handler->ents[handler->player_id];
		comp_Transform *player_ct = &player_ent->comp_transform;

		player_ct->position.z += (vel.z * dt);	
		if(player_ct->velocity.z < EPSILON) player_ct->velocity.z = EPSILON;
		player_ct->position.x += (vel.x * dt);
		player_ct->position.y += (vel.y * dt);
	}

	if(ent->flags & BUG_ON_PLATFORM) {
		Entity *bug_ent = &handler->ents[handler->bug_id];
		comp_Transform *bug_ct = &bug_ent->comp_transform;

		bug_ct->position.z += (vel.z * dt);	
		if(bug_ct->velocity.z < EPSILON) bug_ct->velocity.z = EPSILON;
		bug_ct->position.x += (vel.x * dt);
		bug_ct->position.y += (vel.y * dt);
	}

	ent->flags &= ~PLAYER_ON_PLATFORM;
	ent->flags &= ~BUG_ON_PLATFORM;
}

