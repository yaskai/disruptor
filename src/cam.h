#include "raylib.h"
#include "../include/num_redefs.h"
#include "ent.h"

#ifndef CAM_H_
#define CAM_H_

typedef struct {
	// Pointer to raylib camera struct
	Camera3D *camera;

	// Vertical movement,
	// when character is walking/running
	float bob;

	// Horizontal rotation, 
	// when character is moving sideways 
	// or for damage visual effect
	float tilt;

	float z_curr; 
	float z_targ;
	float z_speed;

	u16 entity;

} Cam_Controller;

Cam_Controller cc_Init(Camera3D *camera, u16 entity);
void cc_Resolve(Cam_Controller *cc, EntityHandler *ent_handler, float alpha);

#endif
