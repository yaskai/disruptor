#include <stdlib.h>
#include "ent.h"
#include "../include/log_message.h"

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

void OnHitSwitch(Entity *ent, short damage) {
	Message("OnHitSwitch()", ANSI_BLUE);

	EntityHandler *handler = ptr_handler_switch;

	for(int i = 0; i < handler->count; i++) {
		Entity *obj = &handler->ents[i];

		if(obj->trigger_id != ent->trigger_id)	
			continue;

		if(obj->on_trigger)
			on_trigger_funcs[obj->on_trigger](obj);
	}
}

