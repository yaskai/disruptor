#include "cam.h"
#include "raymath.h"

#define CAM_Z_OFFS 		16.0f
#define Z_OFFS_CROUCH 	0.0f
#define CAM_TILT_MAX	0.015f

Cam_Controller cc_Init(Camera3D *camera, u16 entity) {
	Cam_Controller cc = (Cam_Controller) {0};

	cc.camera = camera;
	cc.entity = entity;
	cc.z_speed = 20.0f;

	return cc;
}

void cc_Resolve(Cam_Controller *cc, EntityHandler *ent_handler, float alpha) {
	float t = GetTime(), dt = GetFrameTime();

	Entity *ent = &ent_handler->ents[cc->entity];

	Vector3 delta = Vector3Subtract(ent->comp_transform.position, ent->comp_transform.prev_pos);
	Vector3 h_vel = (Vector3) { delta.x, delta.y, 0.0f };
	Vector3 h_fwd = Vector3Normalize( (Vector3) { ent->comp_transform.forward.x, ent->comp_transform.forward.y, 0.0f } );

	float z_offs = (ent->comp_transform.flags & CT_CROUCH) ? Z_OFFS_CROUCH : CAM_Z_OFFS;
	cc->z_targ = ent->comp_transform.position.z + z_offs; 
	cc->z_curr = Lerp(cc->z_curr, cc->z_targ, dt * cc->z_speed);

	// Camera walk bobbing:
	if(Vector3Length(h_vel) >= 1.0f && ent->comp_transform.on_ground)
		cc->bob += (sinf(t * 20) * 0.5f) * dt*50;
	else
		cc->bob = Lerp(cc->bob, cc->bob * 0.5f, dt*50);

	//Camera tilt when moving sideways:
	Vector3 right = Vector3Normalize(Vector3CrossProduct(h_fwd, UP));
	float denom = Vector3DotProduct(Vector3Normalize(h_vel), right);

	if(fabsf(denom) >= 0.65f && Vector3Length(h_vel) >= 1.0f) {
		cc->tilt += denom * dt;
		cc->tilt = Clamp(cc->tilt, -CAM_TILT_MAX, CAM_TILT_MAX);

		Vector3 tilt_targ = Vector3RotateByAxisAngle(UP, ent->comp_transform.forward, cc->tilt);
		cc->camera->up = Vector3Lerp(cc->camera->up, tilt_targ, dt*20.0f);

	} else {
		cc->tilt = 0.0f;
		cc->camera->up = Vector3Lerp(cc->camera->up, UP, dt*20.0f);
	}
	
	Vector3 origin = ent->comp_transform.prev_pos;
	Vector3 target = ent->comp_transform.position;

	Vector3 pos = (Vector3) {
		.x = Lerp(origin.x, target.x, alpha),
		.y = Lerp(origin.y, target.y, alpha),
		.z = cc->z_curr + cc->bob
	};

	cc->camera->position = pos;
	cc->camera->target = Vector3Add(cc->camera->position, ent->comp_transform.forward);
}

