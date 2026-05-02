#include <math.h>
#include <stdio.h>
#include <float.h>
#include "raylib.h"
#include "raymath.h"
#include "ent.h"
#include "pm.h"
#include "kbsp.h"
#include "../include/log_message.h"
#include "audioplayer.h"

#define BUG_MAX_BOUNCES 			16
#define BUG_MAX_RECALL_BOUNCES		16
#define BUG_MAX_VEL 				450.0f
#define BUG_GRAV					975.0f

u8 bug_bounce = 0;
float launch_timer = 0;
bool big_bounce_used = false;
bool disrupt_used = false;

Model model_dead;

float bug_cooldown = 0;

bool bug_target_picked = false;

float bug_z_vel_prev = 0;

Vector3 plr_ent_pos;

bool bug_on_plat = false;

// This function handles setting Bug's target as well as moving towards it. 
void BugBounce(Entity *bug_ent, comp_Transform *ct, MapSection *sect, EntityHandler *handler, u8 *bounce, float dt) {
	EntGrid *grid = &handler->grid;
	Coords coords = Vec3ToCoords(ct->position, grid);

	// Find target if not set already 
	if(!bug_target_picked) {
		// Search in nearby grid cells
		Coords cell_coords[] = {
			coords,
			(Coords) { coords.c - 1, coords.r - 1, coords.t, },
			(Coords) { coords.c + 0, coords.r - 1, coords.t, },
			(Coords) { coords.c + 1, coords.r - 1, coords.t, },
			(Coords) { coords.c - 1, coords.r + 0, coords.t, },
			(Coords) { coords.c + 1, coords.r + 0, coords.t, },
			(Coords) { coords.c - 1, coords.r + 1, coords.t, },
			(Coords) { coords.c + 0, coords.r + 1, coords.t, },
			(Coords) { coords.c + 1, coords.r + 1, coords.t, },
		};
		short adj_count = sizeof(cell_coords) / sizeof(cell_coords[0]);
		
		float closest = FLT_MAX;
		i16 enemy_id = -1;

		for(u8 j = 0; j < adj_count; j++) {
			if(!CoordsInBounds(cell_coords[j], grid))
				continue;

			i16 cell_id = CellCoordsToId(cell_coords[j], grid);
			EntGridCell *cell = &grid->cells[cell_id];

			for(u8 i = 0; i < cell->ent_count; i++) {
				Entity *enemy_ent = &handler->ents[cell->ents[i]];
				comp_Ai *enemy_ai = &enemy_ent->comp_ai;

				// **
				// Skip things that are not valid targets
				// (dead entities, player, self, etc.)
				if(enemy_ai->state == STATE_DEAD)
					continue;

				if(!(enemy_ent->flags & ENT_ACTIVE))
					continue;
				
				if(!enemy_ai->component_valid)
					continue;

				if(enemy_ai->input_mask & AI_INPUT_SELF_GLITCHED)
					continue;

				if(disrupt_used)
					continue;

				Vector3 to_enemy = Vector3Subtract(
					Vector3Add(
						Vector3Add(
							Vector3Scale(enemy_ent->comp_transform.velocity, dt),
							enemy_ent->comp_transform.position), enemy_ent->comp_health.bug_point),
					ct->position
				);	

				float dist = Vector3Length(to_enemy);

				// Too far
				if(dist > 250.0f)
					continue;

				if(to_enemy.z <= -60.0f)
					continue;

				BvhTraceData tr = TraceDataEmpty();
				Ray ray = (Ray) { .position = ct->position, .direction = Vector3Normalize(to_enemy) };
				/*
				BvhTracePointEx(ray, sect, &sect->bvh[0], 0, &tr, dist);
				if(tr.hit) {
					continue;
				}
				*/
				
				bool vis = true;
				for(short k = 0; k < sect->bvh_hullgroup_count; k++) {
					Bvh_HullGroup *hull = &sect->bvh_hullgroups[k];

					if(!(hull->flags & HULLGROUP_ACTIVE))
						continue;

					BvhTraceData temp_tr = TraceDataEmpty();
					BvhTracePointEx(ray, sect, &hull->bvh[2], 0, &temp_tr, Vector3Length(to_enemy));

					if(temp_tr.hit)
						vis = false;
				}

				if(!vis)
					continue;

				// Set target to closest candidate
				if(dist < closest) {
					closest = dist;
					enemy_id = enemy_ent->id;
					bug_target_picked = true;
				}
			}
		}

		bug_ent->comp_ai.targ_data.ent_id = enemy_id;
	} 

	// Increment bounce count
	(*bounce)++;

	if(ct->velocity.z < 0)
		ct->velocity.z *= -0.6f;		


	// Set forward direction, only really used for model's rotation
	Vector3 hdir = (Vector3) { ct->velocity.x, ct->velocity.y, 0 };
	hdir = Vector3Normalize(hdir);
	ct->forward = hdir;	

	float angle = GetRandomValue(-70, 70);
	ct->forward = Vector3RotateByAxisAngle(ct->forward, UP, angle*DEG2RAD);

	// **
	// Update velocity
	if(!bug_target_picked) 
		return;

	Entity *enemy_ent = &handler->ents[bug_ent->comp_ai.targ_data.ent_id];
	if(enemy_ent->comp_ai.state == STATE_DEAD) {
		bug_target_picked = false;
		bug_ent->comp_ai.targ_data.ent_id = -1;
		*bounce = 0;
	}

	/*
	Vector3 h_fwd = Vector3Normalize( (Vector3) { enemy_ent->comp_transform.forward.x, enemy_ent->comp_transform.forward.y, 0 } );
	float h_add = (ct->position.z - enemy_ent->id > 64.0f) ? 16 : 8;

	Vector3 targ_point = (enemy_ent->id == handler->player_id) 
		? Vector3Add(enemy_ent->comp_transform.position, Vector3Scale(h_fwd, h_add))
		: enemy_ent->comp_transform.position;
	*/

	Vector3 targ_point =  enemy_ent->comp_transform.position;

	Vector3 to_enemy = Vector3Subtract( targ_point, ct->position );	
	float d = Vector3Length(to_enemy);
	to_enemy.z = 0;
	to_enemy = Vector3Normalize(to_enemy);

	ct->velocity.x *= 0.5f;
	ct->velocity.y *= 0.5f;

	if(Vector3DotProduct(to_enemy, enemy_ent->comp_transform.velocity) >= 0.99f) {
		to_enemy = Vector3Subtract( Vector3Add(enemy_ent->comp_transform.position, enemy_ent->comp_transform.velocity), ct->position );	
		d = Vector3Length(to_enemy);
		to_enemy.z = 0;
		to_enemy = Vector3Normalize(to_enemy);
	}

	if(d > 100 && (fabsf(enemy_ent->comp_transform.position.z - ct->position.z) <= 64) && *bounce > 0) {
		//ct->velocity.x = to_enemy.x * d * (1.2f + (GetRandomValue(0, 5) * 0.1f));	
		//ct->velocity.y = to_enemy.y * d * (1.2f + (GetRandomValue(0, 5) * 0.1f));	
		ct->velocity.x += to_enemy.x * (d*1.01f);	
		ct->velocity.y += to_enemy.y * (d*1.01f);	
		//ct->velocity.z += fabsf(to_enemy.x + to_enemy.y) * Vector3Distance(ct->position, targ_point) * 0.033f;
	} else {
		ct->velocity.x += to_enemy.x * (d*1.01f);	
		ct->velocity.y += to_enemy.y * (d*1.01f);	
		//ct->velocity.z += fabsf((to_enemy.x + to_enemy.y)) * Vector3Distance(ct->position, targ_point) * 0.033f;
	}

	//ct->velocity.z += (d*0.05f);

	if(d <= 265.0f) {
		if(enemy_ent->id != handler->player_id) {
			//ct->velocity.z += 300 + (1.5f*(*bounce));
			if(*bounce >= BUG_MAX_BOUNCES && !big_bounce_used) {
				(*bounce)--;
				big_bounce_used = true;
			}
		} else {
			ct->velocity.z += 60.0f + (1.15f*(*bounce));
			if(enemy_ent->comp_transform.position.z > ct->position.z + 128.0f) {
				//ct->velocity.z += 500.0f;

			} else if(enemy_ent->comp_transform.position.z < ct->position.z) {
				ct->velocity.x *= 0.95f;
				ct->velocity.y *= 0.95f;
			}
		}
	}

	// * NOTE: 
	// Forgiveness,
	// feels very bad when bug doesn't hit and lands super close to enemy
	if(d <= 128 && *bounce >= BUG_MAX_BOUNCES) {
		bug_ent->comp_ai.state = BUG_LAUNCHED;
		ct->velocity.z += 100.0f;
		(*bounce)--;
	}

	ct->velocity.x = Clamp(ct->velocity.x, -700.0f, 700.0f);
	ct->velocity.y = Clamp(ct->velocity.y, -700.0f, 700.0f);

	if(ct->velocity.z < 130.0f)
		ct->velocity.z = 130.0f;

	/*
	float ceil_z = ct->velocity.z * 1.5f;
	Ray ceil_ray = (Ray) { .position = ct->position, .direction = UP };
	BvhTraceData ceil_tr = TraceDataEmpty();
	BvhTracePointEx(ceil_ray, sect, &sect->bvh[BVH_BOX_SMALL], 0, &ceil_tr, ceil_z);
	if(ceil_tr.distance < ct->position.z + ct->velocity.z) {
		float over = (ct->position.z + ct->velocity.z) - ceil_tr.distance;
		float len = Vector3Length(ct->velocity);
		ct->velocity.z -= over * 0.5f;
		ct->velocity.x += copysignf(fmaxf(1.0f, over*0.007f), ct->velocity.x); 
		ct->velocity.y += copysignf(fmaxf(1.0f, over*0.007f), ct->velocity.y); 
	} 
	*/

	ct->forward = Vector3Normalize( (Vector3) { ct->velocity.x, ct->velocity.y, 0 } );

}

u8 bug_CheckGround(Entity *ent, comp_Transform *ct, Vector3 position, MapSection *sect, u8 *bounce, EntityHandler *handler, float dt) {
	Ray ray = (Ray) { .position = ct->position, .direction = DOWN };	

	BvhTraceData tr = TraceDataEmpty();	
	BvhTracePointEx(ray, sect, &sect->bvh[2], 0, &tr, 1 + EPSILON);
	//BvhBoxSweep(ray, sect, &sect->bvh[0], 0, ent->comp_transform.bounds, &tr, 8 + 1 + EPSILON);

	i16 ent_id = -1;
	for(int j = 0; j < sect->bvh_hullgroup_count; j++) {
		if(!(sect->bvh_hullgroups[j].flags & HULLGROUP_ACTIVE))	
			continue;
		
		BvhTraceData temp_tr = TraceDataEmpty();
		BvhTracePointEx(ray, sect, &sect->bvh_hullgroups[j].bvh[2], 0, &temp_tr, 8 + 1 + EPSILON);
		//BvhBoxSweep(ray, sect, &sect->bvh_hullgroups[j].bvh[0], 0, ct->bounds, &temp_tr, 8 + 1 + EPSILON);

		if(temp_tr.distance < tr.distance) {
			tr = temp_tr;

			ent_id = sect->bvh_hullgroups[j].ent_id;
			if(ent_id > 0) {
				if(handler->ents[ent_id].type == ENT_DOOR) {
					Entity *lift_ent = &handler->ents[ent_id];
					lift_ent->flags |= BUG_ON_PLATFORM;		
					bug_on_plat = true;
				}
			}
		}

		if(temp_tr.hit)
			continue;

		BvhTracePointEx(ray, sect, &sect->bvh_hullgroups[j].bvh[0], 0, &temp_tr, 8 + 1 + EPSILON);
		if(temp_tr.distance < tr.distance) {
			tr = temp_tr;

			ent_id = sect->bvh_hullgroups[j].ent_id;
			if(ent_id > 0) {
				if(handler->ents[ent_id].type == ENT_DOOR) {
					Entity *lift_ent = &handler->ents[ent_id];
					lift_ent->flags |= BUG_ON_PLATFORM;		
					bug_on_plat = true;
				}
			}
		}
	}
	
	if(!tr.hit) {
		ct->ground_normal = Vector3Zero();
		return 0;
	}

	if(tr.fraction >= 1.0f) {
		ct->ground_normal = Vector3Zero();
		return 0;
	}

	if(handler->ents[handler->player_id].comp_transform.position.z - ct->position.z > 400.0f && launch_timer <= 0) {
		ent->comp_health.amount = 0;
		ent->comp_ai.state = STATE_DEAD;
		bug_cooldown = 5;
		return 0;
	}

	ct->ground_normal = tr.normal;


	short max_bounces = (ent->flags & BUG_RECALL) ? BUG_MAX_RECALL_BOUNCES : BUG_MAX_BOUNCES;
	if(*bounce >= max_bounces) {
		//ct->velocity.z = 0;
		return 1;
	}

	BugBounce(ent, ct, sect, handler, bounce, dt);
	if(ct->velocity.z >= 40.0f) {
		AP_SetSoundPosition(handler->ap, "click", ct->position, 0);
		AP_SetSoundPitch(handler->ap, "click", GetRandomValue(90, 110) * 0.01f);
		AP_RequestSound(handler->ap, "click");
	}

	return 0;
}

void bug_TraceMove(Entity *bug_ent, Vector3 start, Vector3 wish_vel, pmTraceData *pm, float dt, MapSection *sect, EntityHandler *handler) {
	comp_Transform *ct = &bug_ent->comp_transform;

	*pm = (pmTraceData) { .start_in_solid = -1, .end_in_solid = -1, .origin = start, .block = 0 };

	Vector3 dest = start;
	Vector3 vel = wish_vel;

	pm->start_vel = wish_vel;

	float t_remain = dt;

	// Tracked clip planes
	Vector3 clips[MAX_CLIPS] = {0};
	u8 num_clips = 0;

	for(short i = 0; i < MAX_BUMPS; i++) {
		// End slide trace if velocity too low
		if(Vector3LengthSqr(vel) <= STOP_EPS)
			break;
		
		// Scale slide movement by time remaining
		Vector3 move = Vector3Scale(vel, t_remain);

		// Update ray
		Ray ray = (Ray) { .position = dest, .direction = Vector3Normalize(move) };

		// Trace geometry 
		BvhTraceData tr = TraceDataEmpty();
		BvhTracePointEx(ray, sect, &sect->bvh[2], 0, &tr, Vector3Length(move));

		for(int j = 0; j < sect->bvh_hullgroup_count; j++) {
			if(!(sect->bvh_hullgroups[j].flags & HULLGROUP_ACTIVE))	
				continue;
			
			BvhTraceData temp_tr = TraceDataEmpty();
			BvhTracePointEx(ray, sect, &sect->bvh_hullgroups[j].bvh[2], 0, &temp_tr, Vector3Length(move));
			if(temp_tr.distance < tr.distance) {
				tr = temp_tr;
			}

			if(temp_tr.hit)
				continue;

			temp_tr = TraceDataEmpty();
			BvhTracePointEx(ray, sect, &sect->bvh_hullgroups[j].bvh[0], 0, &temp_tr, Vector3Length(move));
			if(temp_tr.distance < tr.distance) {
				tr = temp_tr;
			}
		}

		// Determine how much of movement was obstructed
		//float fraction = (tr.distance / Vector3Length(move));
		float fraction = (tr.contact_dist / Vector3Length(move));
		fraction = Clamp(fraction, 0.0f, 1.0f);

		EntTraceData ent_tr = { .dist = Vector3Length(move), .hit_ent = -1, .point = dest, .normal = Vector3Zero() };
		Vector3 ent_point = TraceEntities(ray, handler, Vector3Length(move), handler->bug_id, &ent_tr);
		float ent_frac = 1.0f;

		bool use_ent = (ent_tr.hit_ent > -1 && ent_tr.hit_ent < handler->count && ent_tr.hit_ent != handler->player_id);

		Entity *other_ent = &handler->ents[ent_tr.hit_ent];
		if((other_ent->comp_ai.state == STATE_DEAD && other_ent->type != ENT_TURRET) || other_ent->type == ENT_PLAYER)
			use_ent = false;

		if(other_ent->type == ENT_SWITCH && other_ent->trigger_condition == TRIGGER_COND_COLL_BUG) {
			use_ent = false;
		}

		if(other_ent->type == ENT_DOOR)
			use_ent = false;

		if(launch_timer >= 0.1f || bug_ent->flags & BUG_RECALL)
			use_ent = false;

		if(use_ent) {
			ent_frac = (ent_tr.dist / Vector3Length(move));
			ent_frac = Clamp(ent_frac, 0.0f, 1.0f);
			//fraction = ent_frac;
			fraction = Clamp(fraction, 0.0f, ent_frac);

			// Add clip plane
			if(num_clips + 1 < MAX_CLIPS) {
				clips[num_clips++] = ent_tr.normal; 

				// Update velocity by each clip plane
				for(short j = 0; j < num_clips; j++) {
					float into = Vector3DotProduct(vel, clips[j]);
					float clip_bounce = (use_ent && j == num_clips - 1) ? 1.8f : 1.5005f;
					clip_bounce *= 0.5f;
					clip_bounce = 1.0f;
					if(clips[j].z < 0) {
						clip_bounce = Clamp(clip_bounce, 1.001f, 1.025f);
					}

					if(into < 0) 
						pm_ClipVelocity(vel, clips[j], &vel, clip_bounce, pm->block);
				}
			}  
		}

		pm->fraction = fraction;

		// Update destination
		dest = Vector3Add(dest, Vector3Scale(move, fraction));

		// No obstruction, do full movement 
		if(fraction >= 1.0f) 
			break;

		if(use_ent) {
			fraction -= 0.01f;
			if(fraction < 0) fraction = 0;
		}

		// Add clip plane
		if(num_clips + 1 < MAX_CLIPS) {
			clips[num_clips++] = tr.normal;

			// Update velocity by each clip plane
			for(short j = 0; j < num_clips; j++) {
				float into = Vector3DotProduct(vel, clips[j]);
				float clip_bounce = (use_ent && j == num_clips - 1) ? 1.8f : 1.5005f;

				if(into < 0) 
					pm_ClipVelocity(vel, clips[j], &vel, clip_bounce, pm->block);
			}
		}  

		// Update remaining time
		t_remain *= (1 - fraction);
	}

	pm->move_dist = Vector3Distance(start, dest);
	pm->end_vel = vel;
	pm->end_pos = dest;
}

void BugInit(Entity *ent, EntityHandler *handler, MapSection *sect) {
	ent->model = LoadModel("resources/models/weapons/bug_01.glb");
	for(int i = 0; i < ent->model.materialCount; i++) 
		ent->model.materials[i].shader = handler->ent_shader;
	model_dead = LoadModel("resources/models/weapons/bug_dead_00.glb");
	model_dead.materials[0].shader = handler->ent_shader;

	ent->comp_transform.bounds = (BoundingBox) {
		.min = (Vector3) { -4, -4, -4 },
		.max = (Vector3) {  4,  4,  4 }
	};

	ent->comp_ai.component_valid = false;
	ent->comp_ai.targ_data.ent_id = -1;
	ent->comp_ai.state = 0;
}

void BugUpdate(Entity *ent, EntityHandler *handler, MapSection *sect, float dt) {
	bug_on_plat = false;

	Entity *player_ent = &handler->ents[handler->player_id];

	if(player_ent->comp_health.amount <= 0 || player_ent->comp_ai.state == STATE_DEAD) {
		ent->comp_ai.state = BUG_DEFAULT;
	}

	// *
	plr_ent_pos = player_ent->comp_transform.position;

	comp_Transform *ct = &ent->comp_transform;
	comp_Ai *ai = &ent->comp_ai;

	AP_SetSoundPosition(handler->ap, "bug_throw", ct->position, 0);

	EntGrid *grid = &handler->grid;
	Coords coords = Vec3ToCoords(ct->position, grid);
	if(!CoordsInBounds(coords, grid)) {
		ai->state = STATE_DEAD;
		return;
	}

	bug_z_vel_prev = ct->velocity.z;

	if(ai->state == BUG_DEFAULT) {
		disrupt_used = false;

		ct->position = player_ent->comp_transform.position;
		ct->velocity = Vector3Zero();

		ent->flags &= ~ENT_COLLIDERS;	
		ent->flags &= ~BUG_DISRUPTED_ENEMY; 
		ent->flags &= ~BUG_RECALL;
		ent->flags &= ~BUG_ON_SWITCH;

		ct->ground_normal = Vector3Zero();
		bug_bounce = 0;
		big_bounce_used = false;
		launch_timer = 0.5f;

		bug_cooldown = 10;

		ent->comp_health.amount = 100;

		bug_target_picked = false;
		ent->comp_ai.targ_data.ent_id = -1;

		ai->state = BUG_DEFAULT;
	}

	ct->bounds = BoxTranslate(ct->bounds, ct->position);

	if(ai->state) {
		// Check if grounded
		ct->on_ground = bug_CheckGround(ent, ct, ct->position, sect, &bug_bounce, handler, dt);

		// Apply gravity
		if(!ct->on_ground && !(ent->flags & BUG_ON_SWITCH)) { 
			float grav = (bug_bounce > 0) ? BUG_GRAV * 1.1f : BUG_GRAV;
			ct->velocity.z -= grav * dt;
		}

		pmTraceData pm = (pmTraceData) {0};

		Vector3 prev_pos = ct->position;
		bug_TraceMove(ent, ct->position, ct->velocity, &pm, dt, sect, handler);
		ct->velocity = pm.end_vel;
		ct->position = pm.end_pos;
	}

	// -------------------------------------------------------------------------------------------------------------
	if(ai->state == BUG_LAUNCHED) {
		launch_timer -= dt;

		/*
		// Check if grounded
		ct->on_ground = bug_CheckGround(ent, ct, ct->position, sect, &bug_bounce, handler, dt);

		// Apply gravity
		if(!ct->on_ground) { 
			float grav = (bug_bounce > 0) ? BUG_GRAV * 1.1f : BUG_GRAV;
			ct->velocity.z -= grav * dt;
		}

		pmTraceData pm = (pmTraceData) {0};
	
		Vector3 prev_pos = ct->position;
		bug_TraceMove(ent, ct->position, ct->velocity, &pm, dt, sect, handler);
		ct->velocity = pm.end_vel;
		ct->position = pm.end_pos;
		*/
		
		if(launch_timer <= 0)
			ct->velocity = Vector3ClampValue(ct->velocity, -BUG_MAX_VEL, BUG_MAX_VEL);

		EntGrid *grid = &handler->grid;
		Coords coords = Vec3ToCoords(ct->position, grid);
		if(!CoordsInBounds(coords, &handler->grid)) {
			ai->state = BUG_DEFAULT;
			ct->position = player_ent->comp_transform.position;
			return;
		}

		i16 cell_id = CellCoordsToId(coords, grid);
		EntGridCell *cell = &grid->cells[cell_id];

		for(u8 i = 0; i < cell->ent_count; i++) {
			Entity *enemy_ent = &handler->ents[cell->ents[i]];

			if(ent->flags & BUG_RECALL)
				continue;

			if(enemy_ent->type == ENT_PLAYER)
				continue;

			if(enemy_ent->type == ENT_DISRUPTOR)
				continue;

			if(enemy_ent->comp_ai.input_mask & AI_INPUT_SELF_GLITCHED)
				continue;

			if(enemy_ent->comp_ai.state == STATE_DEAD)
				continue;

			if(enemy_ent->type == ENT_FORCEFIELD) {
				continue;
			}

			if(enemy_ent->flags & ENT_IS_PICKUP)
				continue;

			if(enemy_ent->type == ENT_SWITCH) {
				if(enemy_ent->trigger_condition != TRIGGER_COND_COLL_BUG)
					continue;

				if(enemy_ent->trigger_state || (launch_timer > 0.0f && (ent->flags & BUG_RECALL)))
					continue;
			}

			bool height_check =
				(ct->position.z >= enemy_ent->comp_transform.position.z - 16 && ct->position.z < enemy_ent->comp_transform.position.z + 48);

			if(bug_bounce == 0) {
				height_check = true;
			}

			if(!(ent->flags & BUG_DISRUPTED_ENEMY) && !disrupt_used) {
				if(CheckCollisionBoxes(ct->bounds, enemy_ent->comp_transform.bounds) && height_check && !(ent->flags & BUG_RECALL)) {
					ct->on_ground = true;
					ct->position = BoxCenter(enemy_ent->comp_health.bug_box);
					ai->targ_data.ent_id = enemy_ent->id;
					ct->forward = enemy_ent->comp_transform.forward;
					//ent->comp_health.damage_cooldown = 10;
					ct->velocity = Vector3Zero();

					/*
					if(enemy_ent->type == ENT_SWITCH && enemy_ent->trigger_condition == TRIGGER_COND_COLL_BUG)
						ent->model.transform = enemy_ent->model.transform;
						*/

					break;
				}
			}

			if(bug_target_picked) {
				// **
				// Purpose of this block is to reduce likelihood of Bug overshooting it's target.
				// Works in most cases
				Vector3 hvel = (Vector3) { ct->velocity.x, ct->velocity.y, 0 };
				Vector3 self_xy = (Vector3) { ct->position.x, ct->position.y, 0 }; 
				Vector3 targ_xy = (Vector3) { enemy_ent->comp_transform.position.x, enemy_ent->comp_transform.position.y, 0 }; 
				if(Vector3Distance(self_xy, targ_xy) <= 16.0f && height_check) {
					Vector3 to_targ = Vector3Subtract(targ_xy, self_xy);

					float into = Vector3DotProduct(to_targ, Vector3Normalize(hvel));
					if(into <= -0.5f) {
						hvel = Vector3Subtract(hvel, Vector3Scale(to_targ, into));
					}

					ct->velocity.x = hvel.x;
					ct->velocity.y = hvel.y;

					// Apply some extra gravity, to fall more into target
					ct->velocity.z -= (BUG_GRAV) * dt;
				}
				// **
			}
		}

		if(ct->on_ground) {
			ai->state = BUG_LANDED;
			ct->velocity = Vector3Zero();
			if(handler->ents[ai->targ_data.ent_id].comp_ai.state == STATE_DEAD) {
				ai->state = BUG_LAUNCHED;
				ct->on_ground = false;
				ent->flags &= ~BUG_DISRUPTED_ENEMY;
				bug_bounce = 0;
				bug_target_picked = true;
				BugBounce(ent, ct, sect, handler, &bug_bounce, dt);
			}
		}

		launch_timer -= dt;
	}
	// -------------------------------------------------------------------------------------------------------------

	// -------------------------------------------------------------------------------------------------------------
	if(ai->state == BUG_LANDED) {
		bool can_recall = true;
		ct->velocity = Vector3Zero();
		
		// Check if there is an enemy to disrupt
		if(!(ent->flags & BUG_DISRUPTED_ENEMY) && !(ent->flags & BUG_RECALL) && !disrupt_used && !(ent->flags & BUG_ON_SWITCH)) {
			i16 cell_id = CellCoordsToId(coords, grid);
			EntGridCell *cell = &grid->cells[cell_id];

			for(u8 i = 0; i < cell->ent_count; i++) {
				Entity *enemy_ent = &handler->ents[cell->ents[i]];

				if(enemy_ent->type == ENT_PLAYER)
					continue;

				if(enemy_ent->type == ENT_DISRUPTOR)
					continue;

				if(!enemy_ent->comp_ai.component_valid)
					continue;

				if(enemy_ent->comp_ai.input_mask & AI_INPUT_SELF_GLITCHED)
					continue;

				if(CheckCollisionBoxes(ct->bounds, enemy_ent->comp_transform.bounds)) {
					DisruptEntity(handler, enemy_ent->id, sect);	
					ent->flags |= BUG_DISRUPTED_ENEMY;
					disrupt_used = true;
					break;
				}

				if(!CheckCollisionSpheres(ct->position, 128, enemy_ent->comp_transform.position, 196))
					continue;

				bug_bounce = (BUG_MAX_BOUNCES >> 1);
				ent->comp_ai.state = BUG_LAUNCHED;
				BugBounce(ent, ct, sect, handler, &bug_bounce, dt);
			}
		}

		if((ent->flags & BUG_DISRUPTED_ENEMY) && ai->targ_data.ent_id > -1 && ai->targ_data.ent_id < handler->count
		   && !(ent->flags & BUG_RECALL) && disrupt_used) {
			Entity *stick_ent = &handler->ents[ai->targ_data.ent_id];			

			bool do_recall = (stick_ent->comp_ai.state == STATE_DEAD || stick_ent->comp_ai.state == STATE_DISABLED);
			bool recall_to_player = false;

			if(stick_ent->comp_ai.component_valid && !(stick_ent->comp_ai.input_mask & AI_INPUT_SELF_GLITCHED)) {
				do_recall = true;
				recall_to_player = true;
			}

			// Bounce off enemy when it dies
			if(do_recall) {
				AP_RequestSound(handler->ap, "recall");

				ai->state = BUG_LAUNCHED;
				ai->targ_data.ent_id = -1;

				bug_bounce = 0;
				bug_target_picked = false;

				if(!recall_to_player) { 
					ent->flags &= ~BUG_DISRUPTED_ENEMY;
					BugBounce(ent, ct, sect, handler, &bug_bounce, dt);
				}

				if(ai->targ_data.ent_id == -1 || recall_to_player) {
					ai->targ_data.ent_id = handler->player_id;
					bug_target_picked = true;
					BugBounce(ent, ct, sect, handler, &bug_bounce, dt);
				}

			}

			if(!recall_to_player)
				ct->position = Vector3Add(stick_ent->comp_transform.position, stick_ent->comp_health.bug_point);
		}

		// * NOTE:
		// Remove later
		// This is here for retrieval "puzzle" in alpha build 
		//if((ent->flags & BUG_DISRUPTED_ENEMY) && fabsf(player_ent->comp_transform.position.z - ct->position.z) >= 175.0f) 
		if((ent->flags) & BUG_DISRUPTED_ENEMY)
			can_recall = false;

		// Recall
		if(can_recall) {
			if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
				//AP_SetSoundPosition(handler->ap, "recall1", ct->position, 0);
				//AP_RequestSound(handler->ap, "recall1");
				AP_SetSoundPosition(handler->ap, "click", ct->position, 0);
				AP_RequestSound(handler->ap, "click");

				bug_bounce = 0;
				bug_target_picked = true;

				ent->flags &= ~BUG_DISRUPTED_ENEMY;

				ent->comp_ai.targ_data.ent_id = handler->player_id;
				
				float dist_add = 80.0f + (Vector3Distance(player_ent->comp_transform.position, ct->position) * 0.1f);
				dist_add = Clamp(dist_add, 0, 300);
				ct->velocity.z += dist_add;

				if(ct->position.z < player_ent->comp_transform.position.z - 32)
					ct->velocity.z += 150.0f;

				ent->comp_ai.state = BUG_LAUNCHED;
				ent->flags |= BUG_RECALL;

				BugBounce(ent, ct, sect, handler, &bug_bounce, dt);

				launch_timer = 0.49f;

				//return;
			}
		}

		launch_timer -= dt;
	}
	// -------------------------------------------------------------------------------------------------------------
	
	// -------------------------------------------------------------------------------------------------------------
	// Pickup
	float pickup_radius = 18.0f;
	if(ent->flags & BUG_RECALL)
		pickup_radius *= 1.25f;

	if(ent->flags & BUG_DISRUPTED_ENEMY) {
		pickup_radius = 0.0f;
		if(ai->targ_data.ent_id > -1) {
			if(handler->ents[ai->targ_data.ent_id].comp_ai.state == STATE_DEAD) {
				ai->targ_data.ent_id = -1;
				ent->flags &= ~BUG_DISRUPTED_ENEMY;
			} 

			if(!(handler->ents[ai->targ_data.ent_id].comp_ai.input_mask & AI_INPUT_SELF_GLITCHED)) {
				ai->targ_data.ent_id = -1;
			} 
		}
	}

	if(CheckCollisionSpheres(ct->position, pickup_radius, player_ent->comp_transform.position, 16) &&
		(launch_timer <= EPSILON || ((fabsf(ct->velocity.z) <= 0.5f && ct->on_ground) && (!(ent->flags & BUG_ON_SWITCH) && launch_timer <= EPSILON)))) 
	{
		ai->state = BUG_DEFAULT;
		return;
	}

	// -------------------------------------------------------------------------------------------------------------

	if(ai->state == STATE_DEAD) {
		// Pickup dead
		if(CheckCollisionBoxes(ct->bounds, player_ent->comp_transform.bounds)) {
			ai->state = BUG_DEFAULT;
		}

		bug_cooldown -= dt;
		if(bug_cooldown <= 0) 
			ai->state = BUG_DEFAULT;
	}
}

void BugDraw(Entity *ent, EntityHandler *handler) {
	//DrawBoundingBox(BoxTranslate(ent->comp_transform.bounds, ent->comp_transform.position), PURPLE);
	//DrawSphere(ent->comp_transform.position, 8, PURPLE);
	//DrawLine3D(ent->comp_transform.position, plr_ent_pos, PURPLE);

	if(ent->comp_ai.state == 0)
		return;

	if(launch_timer >= 0.4725f)
		return;

	comp_Transform *ct = &ent->comp_transform;

	if(!(ent->flags & BUG_ON_SWITCH)) {
		float angle = atan2f(-ent->comp_transform.forward.x, ent->comp_transform.forward.y);
		ent->model.transform = MatrixRotateY(angle);
		ent->model.transform = MatrixMultiply(ent->model.transform, MatrixRotateX(90*DEG2RAD));
	}

	if(ent->comp_ai.state == STATE_DEAD) {
		model_dead.transform = ent->model.transform;
		//DrawModel(model_dead, ent->comp_transform.position, 3, LIGHTGRAY);	
		DrawModel(model_dead, ent->comp_transform.position, 3, DARKGRAY);	
 	} else {
		//DrawModel(ent->model, ent->comp_transform.position, 3, WHITE);	
		//EntDrawLitModel(handler, ent, 3.0f, 100);
		Vector3 pos = Vector3Add(ct->position, Vector3Scale(DOWN, 1.0f));
		if(bug_on_plat) pos = Vector3Add(pos, Vector3Scale(DOWN, 6.0f));
		EntDrawLitModelEx(handler, ent, pos, 3.0f, Vector3Zero(), 0.0f, 60);
	}
}

void DisruptEntity(EntityHandler *handler, u16 ent_id, MapSection *sect) {
	//printf("dirsrupted entity [%d]\n", ent_id);
	Entity *ent = &handler->ents[ent_id];
	comp_Ai *ai = &ent->comp_ai;	
	comp_Transform *ct = &ent->comp_transform;

	if(ai->input_mask & AI_INPUT_SELF_GLITCHED)
		return;

	ai->input_mask |= AI_INPUT_SELF_GLITCHED;
	ai->state = STATE_STUNNED;

	disrupt_used = true;

	AP_RequestSound(handler->ap, "disrupt");

	// * NOTE: 
	// Magic number, change later based on entity type maybe??
	switch(ent->type) {
		case ENT_TURRET: {
			ai->disrupt_timer = 100;
			ai->task_state.timer = 0;
			ent->comp_weapon.ammo = 60;
			ent->comp_weapon.cooldown = 0;
			ai->task_state.task_id = TASK_FIRE_WEAPON;

			ct->forward = ct->start_forward;
			ai->targ_data.known_position = Vector3Add(ct->position, ct->forward);
			ai->targ_data.position = Vector3Add(ct->position, ct->forward);

		} break;

		case ENT_MAINTAINER: {
			ai->disrupt_timer = 500;

		} break;

		case ENT_REGULATOR: {

		} break;
	}

	//handler->ents[handler->bug_id].flags |= BUG_DISRUPTED_ENEMY;
}

// *TODO:
void OnHitBug(Entity *ent, short damage, Vector3 bullet_pos) {
	// ...
}

