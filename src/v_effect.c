#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "v_effect.h"
#include "../include/num_redefs.h"
#include "geo.h"
#include "rlgl.h"

Texture2D decal_textures[8];
Model decal_mesh;

void vEffectsRunTrails(vEffect_Manager *manager, float dt) {
	for(u8 i = 0; i < V_EFFECT_MAX_TRAILS; i++) {
		vEffect_Trail *trail = &manager->trails[i];

		if(!trail->active) 
			continue;

		trail->timer -= dt*1.75f;
		if(trail->timer <= 0) {
			trail->active = false;
			manager->trail_count--;
			continue;
		}

		float alpha = trail->timer;
		alpha = Clamp(alpha, 0.0f, 0.5f);

		Vector3 axis = Vector3Zero();
		float angle = 0;
		QuaternionToAxisAngle(trail->q, &axis, &angle);
	
		Vector3 scale = (Vector3) { 0.7f, trail->length, 0.7f };
		DrawModelEx(manager->trail_model, trail->point_A, axis, angle*RAD2DEG, scale, ColorAlpha(RAYWHITE, alpha));
	}
}

void vEffectsRunDecals(vEffect_Manager *manager, float dt) {
	rlDisableBackfaceCulling();

	for(u8 i = 0; i < V_EFFECT_MAX_IMPACT_DECALS; i++) {
		vEffect_ImpactDecal *dec = &manager->impact_decals[i];
		
		if(!dec->active)
			continue;

		Vector3 axis = Vector3Zero();
		float angle = 0;
		QuaternionToAxisAngle(dec->q, &axis, &angle);
		DrawModelEx(decal_mesh, dec->position, axis, angle*RAD2DEG, Vector3One(), ColorAlpha(WHITE, 0.5f));
	}
	
	rlEnableBackfaceCulling();
}

void vEffectsRunTracers(vEffect_Manager *manager, float dt) {
	for(u8 i = 0; i < V_EFFECT_MAX_TRACERS; i++) {
		vEffectTracer *tracer = &manager->tracers[i];

		if(!tracer->active) 
			continue;

		tracer->timer -= dt*1.75f;
		if(tracer->timer <= 0) {
			tracer->active = false;
			manager->tracer_count--;
			continue;
		}

		Vector3 axis = Vector3Zero();
		float angle = 0;
		QuaternionToAxisAngle(tracer->q, &axis, &angle);

		Vector3 d = Vector3Subtract(tracer->point_B, tracer->point_A);
		if(Vector3LengthSqr(d) <= 128.0f*128.0f) {
			tracer->active = false;
			continue;
		}

		Vector3 dir = Vector3Normalize(d);
		Vector3 stale = tracer->point_A;
		tracer->point_A = Vector3Add(tracer->point_A, Vector3Scale(dir, 8000.0f*dt));
		tracer->length -= Vector3Length(Vector3Scale(dir, 8000.0f*dt));
	
		Vector3 scale = (Vector3) { 0.25f, tracer->length * 0.33f, 0.25f };
		DrawModelEx(manager->trail_model, tracer->point_A, axis, angle*RAD2DEG, scale, ColorBrightness(ORANGE, 0.25f));

		scale = (Vector3) { 0.25f, tracer->length * 0.33f, 0.25f };
		DrawModelEx(manager->trail_model, stale, axis, angle*RAD2DEG, scale, ColorAlpha(ORANGE, 0.75f));
	}
}

void vEffectsInit(vEffect_Manager *manager) {
	*manager = (vEffect_Manager) {0};

	manager->trail_model = LoadModelFromMesh(GenMeshCylinder(1.0f, 1.0f, 5.0f));
	manager->trail_model.transform = MatrixIdentity();

	manager->trail_material = LoadMaterialDefault();

	decal_textures[0] = LoadTexture("tools/Disruptor/textures/custom/bul_dec00.png");
	decal_mesh = LoadModelFromMesh(GenMeshPlane(2, 2, 1, 1));
	decal_mesh.materials[0].maps[0].texture = decal_textures[0];
}

void vEffectsRun(vEffect_Manager *manager, float dt) {
	vEffectsRunTrails(manager, dt);
	vEffectsRunDecals(manager, dt);
	vEffectsRunTracers(manager, dt);
}

void vEffectsAddTrail(vEffect_Manager *manager, Vector3 start, Vector3 end) {
	u8 id = 0;
	for(u8 i = 0; i < V_EFFECT_MAX_TRAILS; i++) {
		vEffect_Trail *trail = &manager->trails[i];

		if(!trail->active) { 
			id = i;
			break;
		}
	}

	vEffect_Trail new_trail = (vEffect_Trail) {
		.point_A = start,
		.point_B = end,
		.timer = 1,
		.active = true
	}; 	

	Vector3 d = Vector3Subtract(end, start);
	float length = Vector3Length(d);	
	new_trail.length = length;
	Vector3 dir = Vector3Normalize(d);
	new_trail.q = QuaternionFromVector3ToVector3( (Vector3) { 0, 1, 0 }, dir);

	manager->trails[id] = new_trail;
	manager->trail_count++;
}

void vEffectsAddImpactDecal(vEffect_Manager *manager, Vector3 position, Vector3 normal) {
	u8 id = 0;
	for(u8 i = 0; i < V_EFFECT_MAX_IMPACT_DECALS; i++) {
		vEffect_ImpactDecal *dec = &manager->impact_decals[i];

		if(!dec->active) { 
			id = i;
			break;
		}
	}

	vEffect_ImpactDecal new_dec = (vEffect_ImpactDecal) {
		.q = QuaternionFromVector3ToVector3( (Vector3) { 0, 1, 0 }, normal),
		.position = position,
		.normal = normal,
		.texture_id = 0,
		.active = true,
	};

	manager->impact_decals[id] = new_dec;
}

void vEffectsAddTracer(vEffect_Manager *manager, Vector3 start, Vector3 end) {
	u8 id = 0;
	for(u8 i = 0; i < V_EFFECT_MAX_TRAILS; i++) {
		vEffect_Trail *trail = &manager->trails[i];

		if(!trail->active) { 
			id = i;
			break;
		}
	}

	vEffectTracer new_tracer = (vEffectTracer) {
		.point_A = start,
		.point_B = end,
		.timer = 1,
		.active = true
	}; 	

	Vector3 d = Vector3Subtract(end, start);
	float length = Vector3Length(d);	
	new_tracer.length = length;
	Vector3 dir = Vector3Normalize(d);
	new_tracer.q = QuaternionFromVector3ToVector3( (Vector3) { 0, 1, 0 }, dir);

	manager->tracers[id] = new_tracer;
	manager->tracer_count++;
}
