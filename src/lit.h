#include "../include/num_redefs.h"
#include "raylib.h"

#ifndef LIT_H_
#define LIT_H_

#define PL_ACTIVE	0x01
typedef struct {
	Color color;

	Vector3 position;	

	float radius;
	float timer;

	u8 flags;
	u8 id;

} PointLight;

#define MAX_POINT_LIGHTS 16
typedef struct { 
	PointLight point_lights[MAX_POINT_LIGHTS];	
	u8 num_point_lights;

} LightHandler;

void InitPointLights(LightHandler *lh);
void ManagePointLights(LightHandler *lh, float dt);

#endif
