#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "map.h"
#include "ent.h"
#include "kbsp.h"
#include "config.h"
#include "../include/log_message.h"

void ProcessEntity(EntSpawn *spawn_point, EntityHandler *handler, NavGraph *nav_graph, Bsp_Data *bsp) {
	if(streq(spawn_point->classname, "worldspawn")) {
		return;
	}

	if(streq(spawn_point->classname, "text_object")) {
		TextObject text_obj = (TextObject) {0};
		text_obj.position = spawn_point->position;
		// Copy the 64 'extra' bytes from spawn point to text object string
		memcpy(text_obj.text, spawn_point->extra, 64);
		handler->text_objs[handler->text_obj_count++] = text_obj;
	}

	if(streq(spawn_point->classname, "level_end")) {
		EntGrid *grid = &handler->grid;

		Coords coords = Vec3ToCoords(spawn_point->position, grid);
		i16 id = CellCoordsToId(coords, grid);

		grid->level_end = id;
		memcpy(grid->sect_next, spawn_point->extra, strlen(spawn_point->extra));
	}

	if(streq(spawn_point->classname, "level_back")) {
		EntGrid *grid = &handler->grid;

		Coords coords = Vec3ToCoords(spawn_point->position, grid);
		i16 id = CellCoordsToId(coords, grid);

		grid->level_back = id;
		memcpy(grid->sect_prev, spawn_point->extra, strlen(spawn_point->extra));
	}

	if(nav_graph) {
		if(!strcmp(spawn_point->classname, "nav_node")) {
			if(nav_graph->node_count + 1 >= nav_graph->node_cap) {
				nav_graph->node_cap = (nav_graph->node_cap << 1);
				nav_graph->nodes = realloc(nav_graph->nodes, sizeof(NavNode) * nav_graph->node_cap);
			}

			NavNode node = (NavNode) {
				.position = spawn_point->position,
				.id = nav_graph->node_count,
				.edge_count = 0,
				.flags = spawn_point->flags
			};
			memset(node.edges, 0, sizeof(u16) * MAX_EDGES_PER_NODE);
			nav_graph->nodes[nav_graph->node_count] = node;
			nav_graph->node_count++;

			return;
		}
	}

	if(streq(spawn_point->classname, "dsp_node")) {
		return;
	}

	if(streq(spawn_point->classname, "checkpoint")) {
		if(handler->checkpoint_list.count + 1 >= handler->checkpoint_list.capacity) {

			if(handler->checkpoint_list.capacity <= 0) {
				handler->checkpoint_list.capacity = 2;
				handler->checkpoint_list.points = malloc(sizeof(Vector3) * 2);
				handler->checkpoint_list.angles = malloc(sizeof(Vector3) * 2);
				handler->checkpoint_list.cells = malloc(sizeof(u16) * 2);

			} else {
				handler->checkpoint_list.capacity = (handler->checkpoint_list.capacity << 1);
				handler->checkpoint_list.points = realloc(handler->checkpoint_list.points, sizeof(Vector3) * handler->checkpoint_list.capacity);	
				handler->checkpoint_list.angles = realloc(handler->checkpoint_list.angles, sizeof(Vector3) * handler->checkpoint_list.capacity);	
				handler->checkpoint_list.cells = realloc(handler->checkpoint_list.cells, sizeof(u16) * handler->checkpoint_list.capacity);	
			}
		}

		handler->checkpoint_list.points[handler->checkpoint_list.count++] = spawn_point->position;

		//float rad = (-spawn_point->angle) * DEG2RAD;
		Vector3 fwd = Vector3Zero();
		fwd.x = sinf(spawn_point->angle*DEG2RAD);
		fwd.y = cosf(spawn_point->angle*DEG2RAD);
		fwd = Vector3Normalize(fwd);
		handler->checkpoint_list.angles[handler->checkpoint_list.count-1] = fwd;
	}

	if(streq(spawn_point->classname, "info_player_start")) {
		//puts("player_start");

		handler->player_start = spawn_point->position;
		handler->player_start.z += BODY_VOLUME_MEDIUM.z * 0.5f;

		Vector3 fwd = Vector3Zero();
		fwd.x = sinf(spawn_point->angle*DEG2RAD);
		fwd.y = cosf(spawn_point->angle*DEG2RAD);
		fwd = Vector3Normalize(fwd);
		handler->player_start_fwd = fwd;

		u16 player_id = handler->count++;
		handler->player_id = player_id;

		u16 bug_id = handler->count++;
		handler->bug_id = bug_id;

		return;
	}

	if(streq(spawn_point->classname, "info_player_return")) {
		handler->player_ret = spawn_point->position;
		handler->player_ret.z += BODY_VOLUME_MEDIUM.z * 0.5f;

		Vector3 fwd = Vector3Zero();
		fwd.x = sinf(spawn_point->angle*DEG2RAD);
		fwd.y = cosf(spawn_point->angle*DEG2RAD);
		fwd = Vector3Normalize(fwd);
		handler->player_ret_fwd = fwd;

		return;
	}

	if(streq(spawn_point->classname, "func_group")) {
		//puts("skip func_group");
		return;
	}

	if(streq(spawn_point->classname, "func_forcefield")) {
		spawn_point->ent_type = ENT_FORCEFIELD;
	}

	if(spawn_point->ent_type <= 0)
		return;
	
	handler->ents[handler->count] = SpawnEntity(spawn_point, handler, bsp);
}

Entity SpawnEntity(EntSpawn *spawn_point, EntityHandler *handler, Bsp_Data *bsp) {
	Entity ent = (Entity) {0};
	ent.id = handler->count;
	ent.cell_id = -1;

	ent.bsp_model = spawn_point->bsp_model;

	ent.on_trigger = spawn_point->on_trigger;
	ent.trigger_id = spawn_point->trigger_group;
	ent.trigger_condition = spawn_point->trigger_condition;
	ent.trigger_state = 0;

	ent.comp_transform.position = spawn_point->position;

	ent.comp_transform.start_angle = spawn_point->angle;
	float rad = (-spawn_point->angle) * DEG2RAD;

	ent.comp_transform.forward.x = sinf(rad);
	ent.comp_transform.forward.y = cosf(rad);
	ent.comp_transform.forward.z = 0;
	ent.comp_transform.forward = Vector3Normalize(ent.comp_transform.forward);

	ent.comp_ai = (comp_Ai) {0};
	ent.comp_ai.component_valid = false;
	ent.comp_ai.speed = 50;

	ent.comp_health = (comp_Health) {0};
	ent.comp_health.amount = 100;

	// * TODO:
	// Entity type specific stuff
	ent.type = spawn_point->ent_type;
	switch(ent.type) {
		case ENT_TURRET: {
			ent.flags |= ( ENT_COLLIDERS ); 

			ent.model = handler->base_ent_models[ENT_TURRET];

			//ent.comp_transform.position.y -= 20;
			ent.comp_transform.position.z -= 18;

			ent.comp_transform.bounds.max = Vector3Scale(BODY_VOLUME_MEDIUM,  0.5f);
			ent.comp_transform.bounds.min = Vector3Scale(BODY_VOLUME_MEDIUM, -0.5f);
					
			//ent.comp_transform.bounds.min.z *= 0.5f;
			//ent.comp_transform.bounds.max.z *= 0.5f;

			ent.comp_transform.bounds = BoxTranslate(ent.comp_transform.bounds, ent.comp_transform.position);

			// * NOTE:
			// Modify later as needed
			ent.comp_health.hit_box = ent.comp_transform.bounds;

			float angle = atan2f(ent.comp_transform.forward.z, ent.comp_transform.forward.x);
			ent.model.transform = MatrixRotateX(90*DEG2RAD);
			ent.model.transform = MatrixMultiply(ent.model.transform, MatrixRotateZ(-spawn_point->angle*DEG2RAD));

			ent.comp_ai.component_valid = true;

			ent.comp_ai.sight_cone = 0.5f;
			ent.comp_ai.hear_distance = 5.0f;

			AiSetSchedule(&ent.comp_ai, SCHED_SENTRY_IDLE);
			//ent.comp_ai.task_data.task_id = TASK_LOOK_AT_ENTITY;
			//ent.comp_ai.task_state.task_id = TASK_LOOK_AT_ENTITY;

			ent.comp_transform.targ_look = ent.comp_transform.forward;
			
			ent.comp_weapon = (comp_Weapon) {
				.travel_type = WEAPON_TRAVEL_HITSCAN,
				.id = WEAP_TURRET,
				.cooldown = 1,
				.damage = 7,
				.clip_size = 100,
				.ammo = 100,
				.reload_time_amnt = 4.5f,
			};

			ent.comp_health.amount = 100;
			ent.comp_health.on_hit = 1;

			ent.comp_health.bug_point = BUG_POINT_TURRET;

		} break;

		case ENT_MAINTAINER: {
			ent.flags |= (ENT_COLLIDERS);

			ent.model = handler->base_ent_models[ENT_MAINTAINER];
			ent.model.materials[0].shader = handler->ent_shader;

			ent.comp_transform.position.z += 20;

			ent.comp_transform.bounds.max = Vector3Scale(BODY_VOLUME_MEDIUM,  0.5f);
			ent.comp_transform.bounds.min = Vector3Scale(BODY_VOLUME_MEDIUM, -0.5f);
			ent.comp_transform.bounds = BoxTranslate(ent.comp_transform.bounds, ent.comp_transform.position);
			
			float angle = (spawn_point->angle-90) * DEG2RAD;
			ent.model.transform = MatrixRotateX(90*DEG2RAD);
			ent.model.transform = MatrixMultiply(ent.model.transform, MatrixRotateZ(angle));

			ent.comp_ai.component_valid = true;
			ent.comp_ai.sight_cone = 0.05f;
			ent.comp_ai.hear_distance = 64.0f;

			//AiSetSchedule(&ent.comp_ai, SCHED_PATROL);
			AiSetSchedule(&ent.comp_ai, SCHED_MAINTAINER_IDLE);

			ent.comp_health.amount = 20;
			ent.comp_health.on_hit = 2;

			ent.comp_health.bug_point = BUG_POINT_MAINTAINER;
		
			// * NOTE:
			// Modify later as needed
			ent.comp_health.hit_box = ent.comp_transform.bounds;
			ent.comp_health.hit_box.min = Vector3Add(ent.comp_transform.bounds.min,  Vector3Scale(Vector3One(), 3));
			ent.comp_health.hit_box.max = Vector3Add(ent.comp_transform.bounds.max, Vector3Scale(Vector3One(), -3));
			
			ent.comp_ai.speed = 150;

			ent.comp_health.component_valid = true;

			ent.anim_state = anim_Init(ent.model);
			ent.animations = LoadModelAnimations("resources/models/enemies/maintainer.glb", &ent.num_anims);
			//ent.animations = LoadModelAnimations("resources/models/enemies/maintainer02.glb", &ent.num_anims);
			ent.anim_state.speed = (1.0f / 100);

		} break;

		case ENT_REGULATOR: {
			ent.comp_health.amount = 20;
			ent.comp_health.on_hit = 3;
			ent.comp_health.component_valid = true;

			ent.model = handler->base_ent_models[ENT_REGULATOR];
			ent.model.materials[0].shader = handler->ent_shader;

			float angle = (spawn_point->angle-270) * DEG2RAD;
			ent.model.transform = MatrixRotateX(90*DEG2RAD);
			ent.model.transform = MatrixMultiply(ent.model.transform, MatrixRotateZ(angle));

			ent.anim_state = anim_Init(ent.model);
			//ent.animations = LoadModelAnimations("resources/models/enemies/reg_00.glb", &ent.num_anims);
			//ent.animations = LoadModelAnimations("resources/models/enemies/reg_01.glb", &ent.num_anims);
			ent.animations = LoadModelAnimations("resources/models/enemies/reg_02.glb", &ent.num_anims);
			ent.anim_state.speed = (1.0f / 100);

			ent.comp_ai.speed = 210;
			ent.comp_health.bug_point = BUG_POINT_MAINTAINER;

			ent.comp_transform.bounds.max = Vector3Scale(BODY_VOLUME_MEDIUM,  0.5f);
			ent.comp_transform.bounds.min = Vector3Scale(BODY_VOLUME_MEDIUM, -0.5f);
			ent.comp_transform.bounds = BoxTranslate(ent.comp_transform.bounds, ent.comp_transform.position);

			ent.comp_ai.hear_distance = 100.0f;
			ent.comp_ai.sight_cone = 0.25f;
			ent.comp_ai.component_valid = true;

			ent.comp_health.hit_box = ent.comp_transform.bounds;
			ent.comp_health.hit_box.min = Vector3Add(ent.comp_transform.bounds.min,  Vector3Scale(Vector3One(), 3));
			ent.comp_health.hit_box.max = Vector3Add(ent.comp_transform.bounds.max, Vector3Scale(Vector3One(), -3));

			ent.flags |= ENT_COLLIDERS;

			ent.comp_weapon = (comp_Weapon) {
				.ammo_type = WEAPON_TRAVEL_HITSCAN,
				.clip_size = 24,
				.in_clip = 24,
				.cooldown = 0.5f,
				.reload_timer = 0.0f,
				.reload_time_amnt = 2.0f,
				.damage = 5
			};

			AiSetSchedule(&ent.comp_ai, SCHED_REGULATOR_IDLE);

		} break;

		case ENT_SWITCH: {
			ent.flags |= ( ENT_COLLIDERS ); 
			ent.trigger_state = 0;

			ent.comp_transform.bounds = (BoundingBox) {
				.min = Vector3Scale(BODY_VOLUME_SMALL, -0.5f),
				.max = Vector3Scale(BODY_VOLUME_SMALL,  0.5f)
			};

			ent.comp_transform.bounds = BoxTranslate(ent.comp_transform.bounds, ent.comp_transform.position);

			ent.comp_health.on_hit = -1;

		} break;

		case ENT_FORCEFIELD: {
			ent.model = BspModelToRenderModel(bsp, ent.bsp_model);			
			//ent.comp_transform.bounds = GetModelBoundingBox(ent.model);
			//ent.comp_transform.bounds.min = Vector3Subtract(ent.comp_transform.bounds.min, BODY_VOLUME_SMALL);
			//ent.comp_transform.bounds.max = Vector3Add(ent.comp_transform.bounds.max, BODY_VOLUME_SMALL);
				
			//ent.comp_transform.position = BoxCenter(ent.comp_transform.bounds);

			//printf("made ff, %d\n", ent.bsp_model);

			ent.flags &= ~ENT_COLLIDERS;

		} break;

		case ENT_HEALTHPACK: {
			ent.flags |= (ENT_IS_PICKUP);

			ent.comp_transform.bounds = (BoundingBox) {
				.min = Vector3Scale(BODY_VOLUME_SMALL, -0.5f),
				.max = Vector3Scale(BODY_VOLUME_SMALL,  0.5f)
			};

			ent.comp_transform.bounds = BoxTranslate(ent.comp_transform.bounds, ent.comp_transform.position);

			ent.model = LoadModel("resources/models/pickups/hp_00.glb");

			float angle = (spawn_point->angle-90) * DEG2RAD;
			ent.model.transform = MatrixRotateX(90*DEG2RAD);
			ent.model.transform = MatrixMultiply(ent.model.transform, MatrixRotateZ(angle));

		} break;

		case ENT_AMMO_REVOLVER: {
			ent.flags |= (ENT_IS_PICKUP);

			ent.comp_transform.bounds = (BoundingBox) {
				.min = Vector3Scale(BODY_VOLUME_SMALL, -0.5f),
				.max = Vector3Scale(BODY_VOLUME_SMALL,  0.5f)
			};

			ent.comp_transform.bounds = BoxTranslate(ent.comp_transform.bounds, ent.comp_transform.position);

			ent.model = LoadModel("resources/models/pickups/ammo_box.glb");
			float angle = (spawn_point->angle-90) * DEG2RAD;
			ent.model.transform = MatrixRotateX(90*DEG2RAD);
			ent.model.transform = MatrixMultiply(ent.model.transform, MatrixRotateZ(angle));

		} break;

		case ENT_DOOR: { 
			ent.model = BspModelToRenderModel(bsp, ent.bsp_model);
			ent.model.materials[0].shader = handler->ent_shader;

			//ent.model.materials[0].shader = bsp->lm_shader;
			//ent.model.materials[0].maps[1].texture = bsp->lightmaps->lightmap

			ent.comp_ai.component_valid = false;
			ent.comp_ai.targ_data.position = spawn_point->targ_offset;
			ent.comp_ai.speed = 700;

			bsp->hull_groups[ent.bsp_model].ent_id = ent.id;

			ent.flags &= ~ENT_COLLIDERS;

		} break;

		case ENT_LADDER: {
			ent.model = BspModelToRenderModel(bsp, ent.bsp_model);
			ent.model.materials[0].shader = handler->ent_shader;
			ent.comp_transform.bounds = GetModelBoundingBox(ent.model);

			ent.comp_ai.targ_data.position = spawn_point->targ_offset;
			ent.comp_ai.speed = 700;

			bsp->hull_groups[ent.bsp_model].ent_id = ent.id;

			ent.flags &= ~ENT_COLLIDERS;

		} break;

		case ENT_GLASS: {
			ent.model = BspModelToRenderModel(bsp, ent.bsp_model);
			//ent.model.materials[0].shader = handler->ent_shader;
			ent.comp_transform.bounds = GetModelBoundingBox(ent.model);

			ent.comp_ai.targ_data.position = spawn_point->targ_offset;

			bsp->hull_groups[ent.bsp_model].ent_id = ent.id;

			ent.flags &= ~ENT_COLLIDERS;

			ent.comp_health.component_valid = true;
			ent.comp_health.hit_box = ent.comp_transform.bounds;

		} break;

		case ENT_UNLOCK_BUG: {
			ent.flags |= ENT_IS_PICKUP;

			ent.model = LoadModel("resources/models/weapons/bug_01.glb");
			for(int i = 0; i < ent.model.materialCount; i++) 
				ent.model.materials[i].shader = handler->ent_shader;

			ent.model.transform = MatrixRotateX(90*DEG2RAD);

			ent.comp_transform.bounds = (BoundingBox) {
				.min = (Vector3) { -4, -4, -4 },
				.max = (Vector3) {  4,  4,  4 }
			};

			ent.comp_transform.bounds = BoxTranslate(ent.comp_transform.bounds, ent.comp_transform.position);
			ent.comp_ai.component_valid = false;

		} break;

		case ENT_UNLOCK_REVOLVER: {
			ent.flags |= ENT_IS_PICKUP;

			//ent.model = LoadModel("resources/models/weapons/bug_01.glb");
			ent.model = LoadModel("resources/models/weapons/rev_00_e.glb");
			for(int i = 0; i < ent.model.materialCount; i++) 
				ent.model.materials[i].shader = handler->ent_shader;

			ent.model.transform = MatrixMultiply(MatrixRotateX(90*DEG2RAD), MatrixRotateY(90*DEG2RAD));

			ent.comp_transform.bounds = (BoundingBox) {
				.min = (Vector3) { -4, -4, -4 },
				.max = (Vector3) {  4,  4,  4 }
			};

			ent.comp_transform.bounds = BoxTranslate(ent.comp_transform.bounds, ent.comp_transform.position);
			ent.comp_ai.component_valid = false;

		} break;
	}

	ent.comp_health.bug_box = (BoundingBox) {
		.min = Vector3Scale(BODY_VOLUME_SMALL, -0.75f),	
		.max = Vector3Scale(BODY_VOLUME_SMALL,  0.75f)
	};

	ent.comp_transform.start_forward = ent.comp_transform.forward;
	ent.comp_transform.targ_look = ent.comp_transform.start_forward;
	if(spawn_point->start_active) ent.flags |= (ENT_ACTIVE);

	ent.comp_ai.navgraph_id = -1;
	ent.comp_ai.wish_dir = Vector3Zero();
	ent.comp_ai.targ_data.ent_id = -1;

	ent.cell_id = -1;

	if(spawn_point->speed)
		ent.comp_ai.speed = spawn_point->speed;

	handler->count++;

	return ent;
}

SpawnList ParseBspEnts(EntityHandler *handler, Bsp_Data *bsp) {
	Message("ParseBspEnts()", ANSI_BLUE);

	char *cursor = bsp->ent_str;	

	Bsp_Ent *ent_data = calloc(2048, sizeof(Bsp_Ent));
	int count = 0;
		
	while(*cursor) {
		if(*cursor == '{') {
			Bsp_Ent *ent = &ent_data[count++];
			cursor++;
			
			while(*cursor && *cursor != '}') {
				if(*cursor != '"') {
					cursor++;
					continue;
				}

				Bsp_EntProp *prop = &ent->properties[ent->prop_count++];

				// Skip opening quote
				cursor++;
				char *dest = prop->key;
				while(*cursor && *cursor != '"') *dest++ = *cursor++;
				// Skip closing quote
				cursor++;

				// Skip whitespace
				while(*cursor && *cursor != '"') *dest++ = *cursor++;

				// Skip opening quote
				cursor++;
				dest = prop->val;
				while(*cursor && *cursor != '"') *dest++ = *cursor++;
				// Skip closing quote
				cursor++;
			}
			cursor++;
		}
		cursor++;
	}

	SpawnList spawn_list = (SpawnList) {0};
	spawn_list.capacity = count; 
	spawn_list.count = count;
	spawn_list.arr = calloc(spawn_list.capacity, sizeof(EntSpawn));

	for(int i = 0; i < spawn_list.count; i++) {
		Bsp_Ent *bsp_ent = &ent_data[i];
		EntSpawn spawn = (EntSpawn) {0};

		spawn.start_active = 1;

		Message("---------------", ANSI_GREEN);

		int submodel = 0;

		for(int j = 0; j < bsp_ent->prop_count; j++) {
			Bsp_EntProp *prop = &bsp_ent->properties[j];
			prop->key[strlen(prop->key)-1] = 0;

			MessageKeyValPair(prop->key, prop->val);

			if(streq(prop->key, "classname")) {
				memcpy(spawn.classname, prop->val, strlen(prop->val));
			}
	
			if(streq(prop->key, "origin")) {
				int x, y, z;
				sscanf(prop->val, "%d %d %d", &x, &y, &z);
				spawn.position = (Vector3) { x, y, z};
			}

			if(streq(prop->key, "angle")) {
				sscanf(prop->val, "%d", &spawn.angle);
			}

			if(streq(prop->key, "enum_id")) {
				sscanf(prop->val, "%d", &spawn.ent_type);
			}

			if(streq(prop->key, "trigger_group")) {
				sscanf(prop->val, "%d", &spawn.trigger_group);
			}

			if(streq(prop->key, "on_trigger")) {
				sscanf(prop->val, "%d", &spawn.on_trigger);
			}

			if(streq(prop->key, "trigger_condition")) {
				sscanf(prop->val, "%d", &spawn.trigger_condition);
			}

			if(streq(prop->key, "model") || prop->val[0] == '*') {
				sscanf(prop->val, "*%d", &submodel);
			}

			if(streq(prop->key, "start_active")) {
				sscanf(prop->val, "%d", &spawn.start_active);
			}

			if(streq(prop->key, "radius")) {
				sscanf(prop->val, "%d", &spawn.radius);
			}

			if(streq(prop->key, "goto")) {
				memcpy(spawn.extra, prop->val, strlen(prop->val));
			}

			if(streq(prop->key, "targ_x")) {
				int x = 0;
				sscanf(prop->val, "%d", &x);
				spawn.targ_offset.x = x;
			}

			if(streq(prop->key, "targ_y")) {
				int y = 0;
				sscanf(prop->val, "%d", &y);
				spawn.targ_offset.y = y;
			}

			if(streq(prop->key, "targ_z")) {
				int z = 0;
				sscanf(prop->val, "%d", &z);
				spawn.targ_offset.z = z;
			}

			if(streq(prop->key, "speed")) {
				int s = 0;
				sscanf(prop->val, "%d", &s);
				spawn.speed = s;
			}

			if(streq(prop->key, "flags")) {
				int f = 0;
				sscanf(prop->val, "%d", &f);
				spawn.flags = (u8)f;
			}

			if(streq(prop->key, "text")) {
				memcpy(spawn.extra, prop->val, strlen(prop->val));
			}
		}

		spawn.bsp_model = submodel;
		spawn_list.arr[i] = spawn;
	}

	free(ent_data);

	return spawn_list;
}

