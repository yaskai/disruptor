#include <raylib.h>
#include <stdlib.h>
#include "ent.h"
#include "../include/log_message.h"

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

typedef void (*OnTriggerFunc)(Entity *ent);
OnTriggerFunc on_trigger_funcs[] = {
	NULL,
	&OnTriggerToggle,
	&OnTriggerTurnOn,
	&OnTriggerTurnOff,
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

		if(ent->trigger_condition == TRIGGER_COND_HIT)
			ent->comp_health.on_hit = 5;
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

void OnHitSwitch(Entity *ent, short damage) {
	Message("OnHitSwitch()", ANSI_BLUE);

	EntityHandler *handler = ptr_handler_switch;
	DoTrigger(handler, ent);
}

void DoTrigger(EntityHandler *handler, Entity *switch_ent) {
	for(int i = 0; i < handler->count; i++) {
		Entity *obj = &handler->ents[i];

		if(obj->trigger_id != switch_ent->trigger_id)	
			continue;

		if(obj->on_trigger)
			on_trigger_funcs[obj->on_trigger](obj);
	}

	switch_ent->trigger_state |= TRIGGERED;
}

