#include <stdlib.h>
#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "geo.h"

typedef void(*DamageFunc)(Entity *ent, short amount);
DamageFunc damage_fn[4] = {
	&PlayerDamage,
	NULL,
	NULL,
	NULL
};

typedef void(*DieFunc)(Entity *ent);
DieFunc die_fn[4] = {
	&PlayerDie,
	NULL,
	NULL,
	NULL
};

typedef void(*UpdateFunc)(Entity *ent, float dt);
UpdateFunc update_fn[4] = {
	&PlayerUpdate,
	NULL,
	NULL,
	NULL
};

typedef void(*DrawFunc)(Entity *ent);
DrawFunc draw_fn[4] = {
	&PlayerDraw,
	NULL,
	NULL,
	NULL
};

void ApplyMovement(comp_Transform *comp_transform, Vector3 wish_point, MapSection *sect, float dt) {
	Vector3 destination = wish_point;

	Vector3 direction = Vector3Subtract(wish_point, comp_transform->position);
	float full_travel_dist = Vector3Length(direction);

	Ray ray = (Ray) { .position = comp_transform->position, .direction = Vector3Normalize(direction) };
	
	float d = FLT_MAX;
	BvhTracePoint(ray, sect, 0, &d, &destination, false);

	float partial_dist = Vector3Distance(comp_transform->position, destination);
	float delta = full_travel_dist - partial_dist;

	if(delta <= EPSILON) {
		comp_transform->position = wish_point;
		return;
	}

	Vector3 extent = BoxExtent(comp_transform->bounds);
	float radius = (extent.x + extent.z) * 0.25f;

	float new_travel_dist = partial_dist - radius; 	
	new_travel_dist = fmaxf(new_travel_dist, 0.0f);

	comp_transform->position = Vector3Add(comp_transform->position, Vector3Scale(direction, new_travel_dist));
}

void EntHandlerInit(EntityHandler *handler) {
	handler->count = 0;
	handler->capacity = 128;
	handler->ents = calloc(handler->capacity, sizeof(Entity));
}

void EntHandlerClose(EntityHandler *handler) {
	if(handler->ents) free(handler->ents);
}

void UpdateEntities(EntityHandler *handler, float dt) {
	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];

		if(update_fn[ent->behavior_id])	
			update_fn[ent->behavior_id](ent, dt);
	}
}

void RenderEntities(EntityHandler *handler) {
	for(u16 i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];

		if(draw_fn[ent->behavior_id])	
			draw_fn[ent->behavior_id](ent);
	}

}

