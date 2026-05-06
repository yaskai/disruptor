#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include "ent.h"
#include "../include/log_message.h"
#include <stdio.h>

#define TRIGGERED 0x01

// -------------------------------------------------
// Trigger functions:
void OnTriggerToggle(Entity *ent) {
	Message("Toggle()", ANSI_BLUE);
	ent->flags ^= ENT_ACTIVE;
}

void OnTriggerTurnOn(Entity *ent) {
	Message("TurnOn()", ANSI_BLUE);
	ent->flags |= ENT_ACTIVE;
}

void OnTriggerTurnOff(Entity *ent) {
	Message("TurnOff()", ANSI_BLUE);
	ent->flags &= ~ENT_ACTIVE;
}

void OnTriggerOpen(Entity *ent) {
	Message("Open()", ANSI_BLUE);
	
	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	u8 do_trigger = 0;
	if(Vector3Distance(ct->position, ai->targ_data.position) <= EPSILON) {
		++do_trigger;
	}

	if(Vector3Distance(ct->position, Vector3Zero()) <= EPSILON) {
		++do_trigger;
	}

	if(!do_trigger)
		return;

	ent->flags ^= DOOR_OPENING;
}

void OnTriggerClose(Entity *ent) {
	Message("Close()", ANSI_BLUE);
	ent->flags &= ~DOOR_OPENING;
}

typedef void (*OnTriggerFunc)(Entity *ent);
OnTriggerFunc on_trigger_funcs[] = {
	NULL,
	&OnTriggerToggle,
	&OnTriggerTurnOn,
	&OnTriggerTurnOff,
	&OnTriggerOpen,
	&OnTriggerClose,
};
// -------------------------------------------------

EntityHandler *ptr_handler_switch;

void SwitchSetup(EntityHandler *handler) {
	ptr_handler_switch = handler;

	for(int i = 0; i < handler->count; i++) {
		Entity *ent = &handler->ents[i];

		if(ent->type != ENT_SWITCH)
			continue;

		ent->comp_health.on_hit = -1;
		ent->trigger_state = 0;

		if(ent->trigger_condition == TRIGGER_COND_HIT) {
			ent->comp_health.on_hit = 5;
			ent->comp_health.hit_box = ent->comp_transform.bounds;
			ent->comp_health.hit_box.min = Vector3Subtract(ent->comp_transform.bounds.min, BODY_VOLUME_SMALL);
			ent->comp_health.hit_box.max = Vector3Add(ent->comp_transform.bounds.max, BODY_VOLUME_SMALL);
		}

		if(ent->trigger_condition == TRIGGER_COND_COLL_BUG) {
			ent->model = LoadModel("resources/models/bug_plat_00.glb");

			ent->comp_transform.bounds = (BoundingBox) {
				Vector3Subtract(ent->comp_transform.bounds.min, Vector3Scale(BODY_VOLUME_SMALL, 0.5f)),
				Vector3Add(ent->comp_transform.bounds.max, Vector3Scale(BODY_VOLUME_SMALL, 0.5f)),
			};
		}

		if(ent->trigger_condition == TRIGGER_COND_COLL_PLR) {
			ent->comp_transform.bounds = (BoundingBox) {
				Vector3Subtract(ent->comp_transform.bounds.min, Vector3Scale(BODY_VOLUME_MEDIUM, 0.5f)),
				Vector3Add(ent->comp_transform.bounds.max, Vector3Scale(BODY_VOLUME_MEDIUM, 0.5f)),
			};

			ent->flags &= ~ENT_COLLIDERS;
		}

	}
}

void SwitchUpdate(EntityHandler *handler, Entity *switch_ent, float dt) {
	Coords coords = Vec3ToCoords(switch_ent->comp_transform.position, &handler->grid);
	i16 cell_id = CellCoordsToId(coords, &handler->grid);	
	EntGridCell *cell = &handler->grid.cells[cell_id];

	for(short i = 0; i < cell->ent_count; i++) {
		Entity *ent = &handler->ents[cell->ents[i]];

		bool collide = CheckCollisionBoxes(ent->comp_transform.bounds, switch_ent->comp_transform.bounds);

		switch(switch_ent->trigger_condition) {
			case TRIGGER_COND_COLL_ENT: {
				
				if(!(switch_ent->trigger_state & TRIGGERED) && collide)
					DoTrigger(handler, switch_ent);

				if(!collide)
					switch_ent->trigger_state &= ~TRIGGERED;

			} break;

			case TRIGGER_COND_COLL_BUG: {
				if(ent->type == ENT_DISRUPTOR) {
					ent->flags &= ~BUG_ON_SWITCH;

					if(!(ent->flags & ENT_COLLIDERS))
						continue;

					if(ent->flags & BUG_RECALL)
						collide = false;

					if(collide) {
						ent->model.transform = switch_ent->model.transform;
						ent->flags |= BUG_ON_SWITCH;
					}

					if(!(switch_ent->trigger_state & TRIGGERED) && collide)	
						DoTrigger(handler, switch_ent);

					if(!collide)
						switch_ent->trigger_state &= ~TRIGGERED;
				}

			} break;

			case TRIGGER_COND_COLL_PLR: {
				if(ent->type == ENT_PLAYER) {
					if(!(switch_ent->trigger_state & TRIGGERED) && collide)	
						DoTrigger(handler, switch_ent);

					if(!collide)
						switch_ent->trigger_state &= ~TRIGGERED;
				}

			} break;
		}
	}
}

void SwitchDraw(Entity *ent, EntityHandler *handler, float dt) {
	switch(ent->trigger_condition) {
		case TRIGGER_COND_HIT:
			DrawBoundingBox(ent->comp_health.hit_box, RED);
			break;

		case TRIGGER_COND_COLL_BUG:
			//DrawBoundingBox(ent->comp_transform.bounds, GREEN);
			
			ent->comp_transform.yaw += dt;
			if(ent->comp_transform.yaw > 360.0f)
				ent->comp_transform.yaw = 0.0f;

			ent->model.transform = MatrixRotateX(90*DEG2RAD);
			ent->model.transform = MatrixMultiply(MatrixRotateX(90*DEG2RAD), MatrixRotateZ(ent->comp_transform.yaw));
			ent->model.transform = MatrixMultiply(ent->model.transform, MatrixTranslate(0, 0, sinf(GetTime() * 2) * 0.25f));

			DrawModel(ent->model, ent->comp_transform.position, 3, WHITE);

			break;
	}
}

void OnHitSwitch(Entity *ent, short damage, Vector3 bullet_pos) {
	Message("OnHitSwitch()", ANSI_BLUE);

	EntityHandler *handler = ptr_handler_switch;
	DoTrigger(handler, ent);
}

void DoTrigger(EntityHandler *handler, Entity *switch_ent) {
	for(int i = 0; i < handler->count; i++) {
		Entity *obj = &handler->ents[i];

		if(obj->trigger_id != switch_ent->trigger_id)	
			continue;

		if(!obj->on_trigger)
			continue;

		if(!on_trigger_funcs[obj->on_trigger])
			continue;

		on_trigger_funcs[obj->on_trigger](obj);
	}

	switch_ent->trigger_state |= TRIGGERED;
}

