#include "raylib.h"
#include "raymath.h"
#include "ent.h"

void DoorUpdate(Entity *ent, MapSection *sect, float dt) {
	comp_Transform 	*ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	if(ent->flags & DOOR_OPENING) {
		ct->position = Vector3Lerp(ct->position, ai->targ_data.position, dt*5);
	} else {
		ct->position = Vector3Lerp(ct->position, Vector3Zero(), dt*5);
	}

	//ent->model.transform = MatrixMultiply(ent->model.transform, MatrixTranslate(ct->position.x, ct->position.y, ct->position.z));
	ent->model.transform = MatrixTranslate(ct->position.x, ct->position.y, ct->position.z);

	sect->bsp_data.hull_groups[ent->bsp_model].origin = ct->position;
	sect->bvh_hullgroups[ent->bsp_model].origin = ct->position;
}

