#include "raylib.h"
#include "../include/num_redefs.h"

#ifndef V_EFFECT_H_
#define V_EFFECT_H_

#define V_EFFECT_MAX_TRAILS	32
typedef struct {
	Quaternion q;

	Vector3 point_A;
	Vector3 point_B;

	float length;
	float timer;

	bool active;

} vEffect_Trail;

#define V_EFFECT_MAX_IMPACT_DECALS 64
typedef struct {
	Quaternion q;

	Vector3 position;
	Vector3 normal;

	u8 texture_id;

	bool active;

} vEffect_ImpactDecal;

#define V_EFFECT_MAX_TRACERS 32
typedef struct {
	Quaternion q;

	Vector3 point_A;
	Vector3 point_B;

	float length;
	float timer;

	bool active;

} vEffectTracer;

#define V_EFFECT_MAX_PARTICLES 32 
typedef struct {
	Color color;
	
	Vector3 position;
	Vector3 velocity;

	float scale;
	float timer;

	u8 model_id;
	
} vEffectParticle;

typedef struct {
	Model trail_model;
	Material trail_material;

	vEffect_Trail trails[V_EFFECT_MAX_TRAILS];
	vEffect_ImpactDecal impact_decals[V_EFFECT_MAX_IMPACT_DECALS];
	vEffectTracer tracers[V_EFFECT_MAX_TRACERS];
	vEffectParticle particles[V_EFFECT_MAX_PARTICLES];

	u8 trail_count;
	u8 impact_decal_count;
	u8 tracer_count;
	u8 particle_count;

} vEffect_Manager;

void vEffectsInit(vEffect_Manager *manager);
void vEffectsRun(vEffect_Manager *manager, float dt);

void vEffectsAddTrail(vEffect_Manager *manager, Vector3 start, Vector3 end);
void vEffectsAddImpactDecal(vEffect_Manager *manager, Vector3 position, Vector3 normal);
void vEffectsAddTracer(vEffect_Manager *manager, Vector3 start, Vector3 end);
void vEffectsAddParticle(vEffect_Manager *manager, vEffectParticle particle);

#endif // !V_EFFECT_H_
