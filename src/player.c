#include <math.h>
#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "geo.h"

#define PLAYER_MAX_PITCH (89.0f * DEG2RAD)
#define PLAYER_SPEED 100.0f

Camera3D *ptr_cam;
InputHandler *ptr_input;
MapSection *ptr_sect;

void PlayerInput(Entity *player, InputHandler *input, float dt);

typedef struct {
	Vector3 view_dir, view_dest;
	float view_length;

	Vector3 move_dir, move_dest;
	float move_length;

} PlayerDebugData;
PlayerDebugData player_debug_data = { 0 };

void PlayerInit(Camera3D *camera, InputHandler *input, MapSection *test_section) {
	ptr_cam = camera;
	ptr_input = input;
	ptr_sect = test_section;
}

void PlayerUpdate(Entity *player, float dt) {
	player->comp_transform.bounds = BoxTranslate(player->comp_transform.bounds, player->comp_transform.position);

	PlayerInput(player, ptr_input, dt);	
}

void PlayerDraw(Entity *player) {
}

void PlayerInput(Entity *player, InputHandler *input, float dt) {
	// Adjust pitch and yaw using mouse delta
	player->comp_transform.pitch = Clamp(
		player->comp_transform.pitch - input->mouse_delta.y * dt * input->mouse_sensitivity, -PLAYER_MAX_PITCH, PLAYER_MAX_PITCH);

	player->comp_transform.yaw += input->mouse_delta.x * dt * input->mouse_sensitivity;
	
	// Update player's forward vector
	player->comp_transform.forward = (Vector3) {
		.x = cosf(player->comp_transform.yaw) * cosf(player->comp_transform.pitch),
		.y = sinf(player->comp_transform.pitch),
		.z = sinf(player->comp_transform.yaw) * cosf(player->comp_transform.pitch)
	};

	player->comp_transform.forward = Vector3Normalize(player->comp_transform.forward);

	// Update camera target
	ptr_cam->target = Vector3Add(player->comp_transform.position, player->comp_transform.forward);

	Vector3 right = Vector3CrossProduct(player->comp_transform.forward, UP);

	Vector3 movement = Vector3Zero();

	if(input->actions[ACTION_MOVE_UP].state == INPUT_ACTION_DOWN)
		movement = Vector3Add(movement, player->comp_transform.forward);

	if(input->actions[ACTION_MOVE_DOWN].state == INPUT_ACTION_DOWN)	
		movement = Vector3Subtract(movement, player->comp_transform.forward);

	if(input->actions[ACTION_MOVE_RIGHT].state == INPUT_ACTION_DOWN)
		movement = Vector3Add(movement, right);

	if(input->actions[ACTION_MOVE_LEFT].state == INPUT_ACTION_DOWN)	
		movement = Vector3Subtract(movement, right);
	
	movement.y = 0;
	movement = Vector3Normalize(movement);
	movement = Vector3Scale(movement, PLAYER_SPEED * dt);

	Vector3 wish_point = Vector3Add(player->comp_transform.position, movement);	
	ApplyMovement(&player->comp_transform, wish_point, ptr_sect, dt);

	ptr_cam->position = player->comp_transform.position;
	ptr_cam->target = Vector3Add(ptr_cam->position, player->comp_transform.forward);
}

void PlayerDamage(Entity *player, short amount) {
}

void PlayerDie(Entity *player) {
}

void PlayerDisplayDebugInfo(Entity *player) {
	DrawBoundingBox(player->comp_transform.bounds, RED);

	Ray view_ray = (Ray) { .position = player->comp_transform.position, .direction = player->comp_transform.forward };	
	player_debug_data.view_dest = Vector3Add(view_ray.position, Vector3Scale(view_ray.direction, FLT_MAX * 0.25f));	

	player_debug_data.view_length = FLT_MAX;
	BvhTracePoint(view_ray, ptr_sect, 0, &player_debug_data.view_length, &player_debug_data.view_dest, false);	

	DrawLine3D(player->comp_transform.position, player_debug_data.view_dest, GREEN);
	DrawSphere(player_debug_data.view_dest, 2, GREEN);
}

