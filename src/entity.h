#include "raylib.h"
#include "../include/num_redefs.h"

#ifndef ENTITY_H_
#define ENTITY_H_

enum ENTITY_TYPES : u8 {
	ENT_TYPE_PLAYER,

	ENT_TYPE_ENEMY_00,
	ENT_TYPE_ENEMY_01,
	ENT_TYPE_ENEMY_02,

};

#define ENT_ACTIVE		0x01
#define ENT_SOLID		0x02
#define ENT_ACTOR		0x04

typedef struct {
	Matrix matrix;

	Vector3 position;
	Vector3 velocity;
	Vector3 forward;

	// Type specific data
	void *data;

	// Entity index
	u16 id;

	// Model index for rendering
	u16 model_id;

	u8 type;
	u8 flags;

} Entity;

void EntityUpdate(Entity *ent, float dt);
void EntityDraw(Entity *ent);

#endif
