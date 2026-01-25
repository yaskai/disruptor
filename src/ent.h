#include "raylib.h"
#include "../include/num_redefs.h"

#ifndef ENT_H_
#define ENT_H_

typedef struct  {
	BoundingBox bounds;

	Vector3 position;
	Vector3 velocity;
	
	Vector3 forward;

} comp_Transform;

typedef struct {
	BoundingBox hit_box;

	float damage_cooldown;
	short amount;

	// Hit function id
	short on_hit;

	// Destroy function id
	short on_destroy;

} comp_Health;

void ApplyDamage(u16 health_component_id, short amount);

#define WEAPON_TRAVEL_HITSCAN		0
#define WEAPON_TRAVEL_PROJECTILE	1	

typedef struct {
	float cooldown;

	u16 model_id;

	short travel_type;

	u8 ammo_type;
	u8 ammo;

} comp_Weapon;

#define ENT_ACTIVE	0x01

typedef struct {
	comp_Transform comp_transform;
	comp_Health comp_health;
	comp_Weapon comp_weapon;

	i8 behavior_id;

	u8 flags;

} Entity;

typedef struct {
	Entity *ents;

	u16 count;
	u16 capacity;

} EntityHandler;

#endif
