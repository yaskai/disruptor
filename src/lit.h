#include "../include/num_redefs.h"
#include "raylib.h"
#include "kbsp.h"
#include "ent.h"

#ifndef LIT_H_
#define LIT_H_

typedef struct {
	Color color;

	Vector3 position;	

	float radius;
	float timer;

	int active;

} PointLight;

#define MAX_POINT_LIGHTS 16
typedef struct { 
	PointLight point_lights[MAX_POINT_LIGHTS];	
	u8 num_point_lights;

} LightHandler;

void InitPointLights(LightHandler *lh);
void ManagePointLights(Bsp_Data *bsp, EntityHandler *ent_handler, float dt);

void AddPointlight(PointLight point_light);

Vector3 ColorQuantized(Color color);

#endif
